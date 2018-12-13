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
#include <text_align/smith_waterman/aligner_data.hh>
#include <text_align/smith_waterman/aligner_impl.hh>
#include <text_align/smith_waterman/aligner_parameters.hh>
#include <text_align/smith_waterman/aligner_sample.hh>


namespace text_align { namespace smith_waterman {
	
	template <typename t_score, typename t_word, typename t_delegate>
	class aligner : public aligner_base
	{
		static_assert(std::is_integral_v <t_score>, "Expected t_score to be a signed integer type.");
		static_assert(std::is_signed_v <t_score>, "Expected t_score to be a signed integer type.");
		
		friend detail::aligner_data <aligner>;
		friend detail::aligner_sample <aligner>;
		
	public:
		typedef aligner_base::bit_vector				bit_vector;

	protected:
		typedef aligner_base::arrow_type arrow_type;
		
		enum {
			SCORE_MIN		= std::numeric_limits <t_score>::min()
		};

		typedef t_word									word_type;
		typedef t_score									score_type;
		typedef std::vector <score_type>				score_vector;
		typedef matrix <score_type>						score_matrix;
		typedef packed_matrix <2, word_type>			traceback_matrix;
		typedef packed_matrix <2, word_type>			gap_start_position_matrix;
		typedef packed_matrix <1, word_type>			flag_matrix;

		typedef detail::aligner_impl_base <aligner>		impl_base_type;
		typedef boost::asio::thread_pool				context_type;
		
		friend impl_base_type;
		
	protected:
		context_type									*m_ctx{nullptr};
		t_delegate										*m_delegate{nullptr};
		std::unique_ptr <impl_base_type>				m_aligner_impl;
		
		detail::aligner_sample <aligner>				m_lhs; // Vertical vectors.
		detail::aligner_sample <aligner>				m_rhs; // Horizontal vectors.
		detail::aligner_parameters <score_type>			m_parameters;
		detail::aligner_data <aligner>					m_data;
		
		score_type										m_alignment_score{0};
		
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
		std::uint32_t segment_length() const { return m_parameters.segment_length; }
		bool prints_debugging_information() const { return m_parameters.print_debugging_information; }
		std::size_t lhs_size() const { return m_parameters.lhs_length; }
		std::size_t rhs_size() const { return m_parameters.rhs_length; }
		
		boost::asio::thread_pool &execution_context() { return *m_ctx; }
		
		void set_identity_score(score_type const score) { m_parameters.identity_score = score; }
		void set_mismatch_penalty(score_type const score) { m_parameters.mismatch_penalty = score; }
		void set_gap_start_penalty(score_type const score) { m_parameters.gap_start_penalty = score; }
		void set_gap_penalty(score_type const score) { m_parameters.gap_penalty = score; }
		void set_segment_length(std::uint32_t const length) { m_parameters.segment_length = length; }
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
	template <typename t_score, typename t_word, typename t_delegate>
	template <typename t_lhs, typename t_rhs>
	void aligner <t_score, t_word, t_delegate>::align(t_lhs const &lhs, t_rhs const &rhs)
	{
		align(lhs, rhs, lhs.size(), rhs.size());
	}
	
	
	// Align the given strings.
	template <typename t_score, typename t_word, typename t_delegate>
	template <typename t_lhs, typename t_rhs>
	void aligner <t_score, t_word, t_delegate>::align(
		t_lhs const &lhs,
		t_rhs const &rhs,
		std::size_t const lhs_len,
		std::size_t const rhs_len
	)
	{
		m_parameters.lhs_length = lhs_len;
		m_parameters.rhs_length = rhs_len;
		
		// Set the segment length.
		if (0 == m_parameters.segment_length)
		{
			auto const max_len(std::max(lhs_len, rhs_len));
			auto const sqrt(std::sqrt(max_len));
			auto const length(std::ceil(sqrt / traceback_matrix::ELEMENT_COUNT) * traceback_matrix::ELEMENT_COUNT);
			m_parameters.segment_length = length;
		}
		
		// Count the segments.
		auto const segments_along_y(std::ceil(1.0 * (1 + m_parameters.lhs_length) / m_parameters.segment_length)); // Segment count along Y axis.
		auto const segments_along_x(std::ceil(1.0 * (1 + m_parameters.rhs_length) / m_parameters.segment_length)); // Segment count along X axis.
		
		m_parameters.lhs_segments = segments_along_y;
		m_parameters.rhs_segments = segments_along_x;
		
		m_lhs.init(
			lhs_len,
			segments_along_x,
			arrow_type::ARROW_UP,
			gap_start_position_type::GSP_RIGHT,
			m_parameters.gap_penalty,
			m_parameters.gap_start_penalty
		);
		m_rhs.init(
			rhs_len,
			segments_along_y,
			arrow_type::ARROW_LEFT,
			gap_start_position_type::GSP_DOWN,
			m_parameters.gap_penalty,
			m_parameters.gap_start_penalty
		);
		m_data.init(lhs_len, m_parameters.segment_length, segments_along_y, segments_along_x);
		
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
