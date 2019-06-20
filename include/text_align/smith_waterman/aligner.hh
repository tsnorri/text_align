/*
 * Copyright (c) 2018-2019 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef TEXT_ALIGN_SMITH_WATERMAN_ALIGNER_HH
#define TEXT_ALIGN_SMITH_WATERMAN_ALIGNER_HH

#include <libbio/algorithm.hh>

#include <atomic>
#include <boost/asio.hpp>
#include <libbio/int_vector.hh>
#include <libbio/matrix.hh>
#include <memory>
#include <range/v3/all.hpp>
#include <text_align/smith_waterman/aligner_base.hh>
#include <text_align/smith_waterman/aligner_data.hh>
#include <text_align/smith_waterman/aligner_impl.hh>
#include <text_align/smith_waterman/aligner_parameters.hh>
#include <text_align/smith_waterman/aligner_sample.hh>

// FIXME: move to a compatibility header.
#include <experimental/type_traits>
namespace std { using ::std::experimental::is_detected_v; }


namespace text_align { namespace smith_waterman {
	
	template <typename t_score, typename t_word, typename t_delegate>
	class aligner final : public aligner_base
	{
		static_assert(std::is_signed_v <t_score>, "Expected t_score to be a signed type.");
		
		friend detail::aligner_data <aligner>;
		friend detail::aligner_sample <aligner>;
		
	public:
		typedef t_delegate								delegate_type;
		typedef boost::asio::io_context					context_type;

	protected:
		typedef aligner_base::arrow_type arrow_type;
		
		static constexpr t_score const SCORE_MIN = std::numeric_limits <t_score>::min();

		typedef t_word										word_type;
		typedef t_score										score_type;
		typedef std::vector <score_type>					score_vector;
		typedef libbio::matrix <score_type>					score_matrix;
		typedef libbio::packed_matrix <2, word_type>		traceback_matrix;
		typedef libbio::packed_matrix <2, word_type>		gap_start_position_matrix;
		typedef libbio::packed_matrix <1, word_type>		flag_matrix;

		typedef detail::aligner_impl_base <aligner>			impl_base_type;
		friend impl_base_type;
		
		typedef typename impl_base_type::score_result_type	score_result_type;
		
	protected:
		context_type										*m_ctx{nullptr};
		t_delegate											*m_delegate{nullptr};
		std::unique_ptr <impl_base_type>					m_aligner_impl;
		
		detail::aligner_sample <aligner>					m_lhs; // Vertical vectors.
		detail::aligner_sample <aligner>					m_rhs; // Horizontal vectors.
		detail::aligner_parameters <score_type>				m_parameters;
		detail::aligner_data <aligner>						m_data;
		
		score_type											m_alignment_score{0};
		bool												m_reverses_texts{};
		
	protected:
		// Delegate member functions.
		template <typename t_class>
		using did_calculate_score_t = std::integral_constant <
			void (t_class::*)(aligner_base &, std::size_t, std::size_t, score_result_type const &, bool),
			&t_class::did_calculate_score
		>;
		
		inline void did_calculate_score(std::size_t const row, std::size_t const column, score_result_type const &result, bool const initial);
		inline void push_lhs(bool const flag, std::size_t const count) { this->m_delegate->push_lhs(flag, count); }
		inline void push_rhs(bool const flag, std::size_t const count) { this->m_delegate->push_rhs(flag, count); }
		inline void reverse_gaps() { if (!m_reverses_texts) this->m_delegate->reverse_gaps(); }
		inline void finish(score_type const final_score);
		
		template <typename t_lhs, typename t_rhs>
		void do_align(
			t_lhs const &lhs,
			t_rhs const &rhs,
			std::size_t const segments_along_y,
			std::size_t const segments_along_x
		);
		
	public:
		aligner() = default;
		
		aligner(context_type &ctx, t_delegate &delegate):
			m_ctx(&ctx),
			m_delegate(&delegate)
		{
		}
		
		delegate_type &delegate() const { return *m_delegate; }
		
		score_type identity_score() const { return m_parameters.identity_score; }
		score_type mismatch_penalty() const { return m_parameters.mismatch_penalty; }
		score_type gap_start_penalty() const { return m_parameters.gap_start_penalty; }
		score_type gap_penalty() const { return m_parameters.gap_penalty; }
		std::uint32_t segment_length() const { return m_parameters.segment_length; }
		bool prints_debugging_information() const { return m_parameters.print_debugging_information; }
		bool prints_values_converted_to_utf8() const { return m_parameters.prints_values_converted_to_utf8; }
		std::size_t lhs_size() const { return m_parameters.lhs_length; }
		std::size_t rhs_size() const { return m_parameters.rhs_length; }
		bool reverses_texts() const { return m_reverses_texts; }
		
		context_type &execution_context() { return *m_ctx; }
		
		void set_identity_score(score_type const score) { m_parameters.identity_score = score; }
		void set_mismatch_penalty(score_type const score) { m_parameters.mismatch_penalty = score; }
		void set_gap_start_penalty(score_type const score) { m_parameters.gap_start_penalty = score; }
		void set_gap_penalty(score_type const score) { m_parameters.gap_penalty = score; }
		virtual void set_segment_length(std::uint32_t const length) { m_parameters.segment_length = length; }
		virtual void set_prints_debugging_information(bool const should_print) { m_parameters.print_debugging_information = should_print; }
		void set_prints_values_converted_to_utf8(bool const should_print) { m_parameters.prints_values_converted_to_utf8 = should_print; }
		void set_reverses_texts(bool const flag) { m_reverses_texts = flag; }
		
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
	};
	
	
	template <typename t_score, typename t_word, typename t_delegate>
	void aligner <t_score, t_word, t_delegate>::did_calculate_score(
		std::size_t const row,
		std::size_t const column,
		score_result_type const &result,
		bool const initial
	)
	{
		// Call the delegate if it has a suitable member function.
		if constexpr (std::is_detected_v <did_calculate_score_t, t_delegate>)
			m_delegate->did_calculate_score(*this, row, column, result, initial);
	}
	
	
	template <typename t_score, typename t_word, typename t_delegate>
	void aligner <t_score, t_word, t_delegate>::finish(score_type const final_score)
	{
		m_alignment_score = final_score;
		m_aligner_impl.reset();
		m_delegate->finish(*this);
	}
	
	
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
		m_delegate->clear_gaps();
		
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
		
		m_lhs.copy_first_sample_values(m_rhs, m_parameters.segment_length, segments_along_x);
		m_rhs.copy_first_sample_values(m_lhs, m_parameters.segment_length, segments_along_y);
		
		// Instantiate the implementation.
		do_align(lhs, rhs, segments_along_y, segments_along_x);
	}
	
	
	template <typename t_score, typename t_word, typename t_delegate>
	template <typename t_lhs, typename t_rhs>
	void aligner <t_score, t_word, t_delegate>::do_align(
		t_lhs const &lhs,
		t_rhs const &rhs,
		std::size_t const segments_along_y,
		std::size_t const segments_along_x
	)
	{
		// g++ 8 cannot deduce the argument types; give them explicitly.
		typedef std::remove_reference_t <decltype(*this)> owner_type;
		auto *impl_ptr(
			new detail::aligner_impl <owner_type, t_lhs, t_rhs>(*this, lhs, rhs, segments_along_y, segments_along_x)
		);
		m_aligner_impl.reset(impl_ptr); // noexcept.
		
		// Start the alignment tasks.
		boost::asio::post(*m_ctx, [impl_ptr](){
			impl_ptr->align_block(0, 0);
		});
	}
}}

#endif
