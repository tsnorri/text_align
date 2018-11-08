/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef TEXT_ALIGN_SMITH_WATERMAN_ALIGNER_HH
#define TEXT_ALIGN_SMITH_WATERMAN_ALIGNER_HH

#include <text_align/algorithm.hh>

#include <atomic>
#include <boost/asio.hpp>
#include <memory>
#include <text_align/matrix.hh>
#include <text_align/smith_waterman/aligner_base.hh>
#include <text_align/smith_waterman/aligner_impl.hh>
#include <text_align/smith_waterman/aligner_sample.hh>


namespace text_align { namespace smith_waterman {
	
	template <typename t_score, typename t_delegate>
	class aligner : public aligner_base
	{
		static_assert(std::is_integral_v <t_score>, "Expected t_score to be a signed integer type.");
		static_assert(std::is_signed_v <t_score>, "Expected t_score to be a signed integer type.");
		
		friend detail::aligner_data <aligner>;
		friend detail::aligner_sample <aligner>;
		
	protected:
		typedef aligner_base::arrow_type arrow_type;
		
		enum {
			SCORE_MIN		= std::numeric_limits <t_score>::min(),
		};
		
		typedef std::atomic_uint8_t						atomic_counter;
		typedef std::atomic_uint64_t					atomic_word;
		typedef t_score									score_type;
		typedef std::vector <score_type>				score_vector;
		typedef matrix <score_type>						score_matrix;
		typedef matrix <atomic_counter>					flag_matrix;	// FIXME: replace with compressed_atomic_matrix <2> or something similar.
		typedef detail::aligner_impl_base <aligner>		impl_base_type;
		typedef boost::asio::thread_pool				context_type;
		
		friend impl_base_type;
		static_assert(atomic_counter::is_always_lock_free);
		static_assert(atomic_word::is_always_lock_free);
		
	protected:
		context_type							*m_ctx{nullptr};
		t_delegate								*m_delegate{nullptr};
		std::unique_ptr <impl_base_type>		m_aligner_impl;
		
		detail::aligner_sample <aligner>		m_lhs; // Vertical vectors.
		detail::aligner_sample <aligner>		m_rhs; // Horizontal vectors.
		detail::aligner_parameters <score_type>	m_parameters;
		detail::aligner_data <aligner>			m_data;
		
		score_type								m_alignment_score{0};
		
	protected:
		void finish(score_type const final_score)
		{
			m_alignment_score = final_score;
			m_aligner_impl.reset();
			m_delegate->finish(*this);
		}
		
	public:
		aligner() = default;
		
		aligner(boost::asio::thread_pool &ctx, t_delegate &delegate):
			m_ctx(&ctx),
			m_delegate(&delegate)
		{
		}
		
		score_type identity_score() const { return m_parameters.identity_score; }
		score_type mismatch_penalty() const { return m_parameters.mismatch_penalty; }
		score_type gap_start_penalty() const { return m_parameters.gap_start_penalty; }
		score_type gap_penalty() const { return m_parameters.gap_penalty; }
		uint32_t segment_length() const { return m_parameters.segment_length; }
		bool prints_debugging_information() const { return m_parameters.print_debugging_information; }
		std::size_t lhs_size() const { return m_parameters.lhs_length; }
		std::size_t rhs_size() const { return m_parameters.rhs_length; }
		
		boost::asio::thread_pool &execution_context() { return *m_ctx; }
		
		void set_identity_score(score_type const score) { m_parameters.identity_score = score; }
		void set_mismatch_penalty(score_type const score) { m_parameters.mismatch_penalty = score; }
		void set_gap_start_penalty(score_type const score) { m_parameters.gap_start_penalty = score; }
		void set_gap_penalty(score_type const score) { m_parameters.gap_penalty = score; }
		void set_segment_length(uint32_t const length) { m_parameters.segment_length = length; }
		void set_prints_debugging_information(bool const should_print) { m_parameters.print_debugging_information = should_print; }
		
		template <typename t_lhs, typename t_rhs>
		void align(t_lhs const &lhs, t_rhs const &rhs);
		
		template <typename t_lhs, typename t_rhs>
		void align(
			t_lhs const &lhs,
			t_rhs const &rhs,
			std::size_t const lhs_len,
			std::size_t const rhs_len
		);
		
		score_type alignment_score() const { return m_alignment_score; };
		
		bit_vector &lhs_gaps() { return m_lhs.gaps; };
		bit_vector &rhs_gaps() { return m_rhs.gaps; };
		bit_vector const &lhs_gaps() const { return m_lhs.gaps; };
		bit_vector const &rhs_gaps() const { return m_rhs.gaps; };
	};
	
	
	// Align the given strings.
	template <typename t_score, typename t_delegate>
	template <typename t_lhs, typename t_rhs>
	void aligner <t_score, t_delegate>::align(t_lhs const &lhs, t_rhs const &rhs)
	{
		align(lhs, rhs, lhs.size(), rhs.size());
	}
	
	
	// Align the given strings.
	template <typename t_score, typename t_delegate>
	template <typename t_lhs, typename t_rhs>
	void aligner <t_score, t_delegate>::align(
		t_lhs const &lhs,
		t_rhs const &rhs,
		std::size_t const lhs_len,
		std::size_t const rhs_len
	)
	{
		// Clear the state, m_score and m_flags are reset below.
		m_lhs.gaps.clear();
		m_rhs.gaps.clear();
		
		m_parameters.lhs_length = lhs_len;
		m_parameters.rhs_length = rhs_len;
		
		// Set the segment length.
		if (0 == m_parameters.segment_length)
		{
			auto const max_len(std::max(lhs_len, rhs_len));
			auto const sqrt(std::sqrt(max_len));
			auto const frac(sqrt / 64.0);
			auto const ceil(std::ceil(frac));
			auto const segment_length(64.0 * ceil);
			m_parameters.segment_length = segment_length;
		}
		
		// Count the segments.
		auto const segments_along_y(std::ceil(1.0 * m_parameters.lhs_length / m_parameters.segment_length)); // Segment count along Y axis.
		auto const segments_along_x(std::ceil(1.0 * m_parameters.rhs_length / m_parameters.segment_length)); // Segment count along X axis.
		
		// Initialize the flags.
		{
			initialize_atomic_matrix(m_data.flags, segments_along_y, segments_along_x);
			
			// Set the value of each flag.
			std::fill(m_data.flags.begin(), m_data.flags.end(), 0);
			
			// Set the first row and column to one so that the blocks to the right and below the initial
			// block may be filled.
			auto column(m_data.flags.column(0));
			auto row(m_data.flags.row(0));
			std::fill(column.begin(), column.end(), 1);
			std::fill(row.begin(), row.end(), 1);
		}
		
		// Reserve memory for the score samples and the scores.
		m_lhs.score_samples.resize(1 + lhs_len, 1 + segments_along_x);		// Vertical.
		m_rhs.score_samples.resize(1 + rhs_len, 1 + segments_along_y);		// Horizontal.
		m_lhs.gap_score_samples.resize(1 + lhs_len, 1 + segments_along_x);	// Vertical.
		m_rhs.gap_score_samples.resize(1 + rhs_len, 1 + segments_along_y);	// Horizontal.
		
		std::fill(m_lhs.score_samples.begin(), m_lhs.score_samples.end(), 0);
		std::fill(m_rhs.score_samples.begin(), m_rhs.score_samples.end(), 0);
		std::fill(m_lhs.gap_score_samples.begin(), m_lhs.gap_score_samples.end(), 0);
		std::fill(m_rhs.gap_score_samples.begin(), m_rhs.gap_score_samples.end(), 0);
		
		resize_and_zero(m_data.score_buffer_1, 1 + lhs_len);				// Vertical.
		resize_and_zero(m_data.score_buffer_2, 1 + lhs_len);				// Vertical.
		resize_and_zero(m_data.gap_scores_lhs, 1 + lhs_len);				// Vertical.
		
		{
			auto const words_along_x(std::ceil(rhs_len / 32.0));
			auto const words_along_y(std::ceil(lhs_len / 32.0));
			
			initialize_atomic_matrix(m_lhs.gap_score_gt, 1 + words_along_y, 1 + segments_along_x);	// Vertical
			initialize_atomic_matrix(m_rhs.gap_score_gt, 1 + words_along_x, 1 + segments_along_y);	// Horizontal
			std::fill(m_lhs.gap_score_gt.begin(), m_lhs.gap_score_gt.end(), 0);
			std::fill(m_rhs.gap_score_gt.begin(), m_rhs.gap_score_gt.end(), 0);
			
			// Fill the first vectors with arrows.
			auto column(m_lhs.gap_score_gt.column(0));
			auto row(m_rhs.gap_score_gt.column(0));
			fill_column_with_2_bit_pattern(column, gap_score_gt_type::GSGT_LEFT);
			fill_column_with_2_bit_pattern(row, gap_score_gt_type::GSGT_UP);
			// Add GSGT_BOTH to the corner, make sure that it does not change the previous value.
			assert(gap_score_gt_type::GSGT_BOTH == (gap_score_gt_type::GSGT_BOTH | (column[0] & gap_score_gt_type::GSGT_MASK)));
			assert(gap_score_gt_type::GSGT_BOTH == (gap_score_gt_type::GSGT_BOTH | (row[0] & gap_score_gt_type::GSGT_MASK)));
			column[0] |= gap_score_gt_type::GSGT_BOTH;
			row[0] |= gap_score_gt_type::GSGT_BOTH;
		}
		
		// Reserve memory for the tracebacks.
		{
			// Use two bits for each traceback arrow.
			auto const words_along_x(std::ceil(rhs_len / 32.0));
			auto const words_along_y(std::ceil(lhs_len / 32.0));
			
			initialize_atomic_matrix(m_lhs.initial_traceback, 1 + words_along_y, 1 + segments_along_x);	// Vertical.
			initialize_atomic_matrix(m_rhs.initial_traceback, 1 + words_along_x, 1 + segments_along_y);	// Horizontal.
			std::fill(m_lhs.initial_traceback.begin(), m_lhs.initial_traceback.end(), 0);
			std::fill(m_rhs.initial_traceback.begin(), m_rhs.initial_traceback.end(), 0);
			
			// Fill the first vectors with arrows.
			auto column(m_lhs.initial_traceback.column(0));
			auto row(m_rhs.initial_traceback.column(0));
			fill_column_with_2_bit_pattern(column, arrow_type::ARROW_UP);
			fill_column_with_2_bit_pattern(row, arrow_type::ARROW_LEFT);
			// Add ARROW_FINISH to the corner, make sure that it does not change the previous value.
			assert(arrow_type::ARROW_FINISH == (arrow_type::ARROW_FINISH | (column[0] & arrow_type::ARROW_MASK)));
			assert(arrow_type::ARROW_FINISH == (arrow_type::ARROW_FINISH | (row[0] & arrow_type::ARROW_MASK)));
			column[0] |= arrow_type::ARROW_FINISH;
			row[0] |= arrow_type::ARROW_FINISH;
			
			// Again, two bits for each arrow.
			auto const segment_words_along_y(segments_along_y * std::ceil(m_parameters.segment_length / 32.0));
			initialize_atomic_matrix(m_data.traceback, segment_words_along_y, m_parameters.segment_length);
			std::fill(m_data.traceback.begin(), m_data.traceback.end(), 0);
		}
		
		{
			auto const segment_words_along_y(segments_along_y * std::ceil(m_parameters.segment_length / 32.0));
			initialize_atomic_matrix(m_data.gap_score_gt, segment_words_along_y, m_parameters.segment_length);
			std::fill(m_data.gap_score_gt.begin(), m_data.gap_score_gt.end(), 0);
		}
		
		m_parameters.lhs_segments = segments_along_y;
		m_parameters.rhs_segments = segments_along_x;
		
		// Instantiate the implementation.
		auto impl_ptr(
			new detail::aligner_impl <aligner, t_lhs, t_rhs>(
				*this,
				lhs,
				rhs
			)
		);
		m_aligner_impl.reset(impl_ptr); // noexcept.
		
		// Start the alignment tasks.
		boost::asio::post(*m_ctx, [impl_ptr](){
			impl_ptr->align_block(0, 0);
		});
	}
}}

#endif
