/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef TEXT_ALIGN_ALIGNER_HH
#define TEXT_ALIGN_ALIGNER_HH

#include <atomic>
#include <boost/asio.hpp>
#include <boost/locale/utf.hpp>
#include <cstdint>
#include <iostream>
#include <text_align/algorithm.hh>
#include <text_align/matrix.hh>
#include <type_traits>


namespace text_align { namespace detail {
	
	template <typename t_owner>
	class smith_waterman_aligner_impl_base
	{
	protected:
		typedef typename t_owner::score_vector					score_vector_type;
		typedef typename t_owner::score_matrix					score_matrix_type;
		typedef typename score_matrix_type::value_type			score_type;
		typedef typename score_matrix_type::const_slice_type	const_slice_type;
		
	protected:
		t_owner		*m_owner{nullptr};
		std::size_t	m_lhs_segments{0};
		std::size_t	m_rhs_segments{0};
		
	public:
		smith_waterman_aligner_impl_base() = default;
		smith_waterman_aligner_impl_base(t_owner &owner, std::size_t const lhs_segments, std::size_t const rhs_segments):
			m_owner(&owner),
			m_lhs_segments(lhs_segments),
			m_rhs_segments(rhs_segments)
		{
		}
		
		score_matrix_type &score_matrix() { return m_owner->m_score; }
		score_matrix_type const &score_matrix() const { return m_owner->m_score; }
		score_vector_type &lhs_gap_scores() { return m_owner->m_lhs_gap_scores; }
		typename t_owner::flag_matrix &flag_matrix() { return m_owner->m_flags; }
		typename t_owner::flag_matrix const &flag_matrix() const { return m_owner->m_flags; }
		void fill_traceback() { this->m_owner->fill_traceback(); }
		void finish() { this->m_owner->finish(); }
	};
	
	
	template <typename t_owner, typename t_lhs, typename t_rhs>
	class smith_waterman_aligner_impl : public smith_waterman_aligner_impl_base <t_owner>
	{
	protected:
		typedef smith_waterman_aligner_impl_base <t_owner> base_class;
		using typename base_class::score_type;
		
	protected:
		t_lhs const	*m_lhs{nullptr};
		t_rhs const	*m_rhs{nullptr};
	
	public:
		smith_waterman_aligner_impl() = default;
		smith_waterman_aligner_impl(
			t_owner &owner,
			t_lhs const &lhs,
			t_rhs const &rhs,
			std::size_t const lhs_segments,
			std::size_t const rhs_segments
		):
			smith_waterman_aligner_impl_base <t_owner>(owner, lhs_segments, rhs_segments),
			m_lhs(&lhs),
			m_rhs(&rhs)
		{
		}
		
		void align_block(std::size_t const lhs_block_idx, std::size_t const rhs_block_idx);
		void print_matrix() const;
		t_lhs const &lhs() const { return *m_lhs; }
		t_rhs const &rhs() const { return *m_rhs; }
	};
	
	
	// Utility function for writing a single code point as UTF-8 to the given buffer.
	inline void to_utf8(char32_t const cp, std::string &dst)
	{
		dst.clear();
		boost::locale::utf::utf_traits <char>::encode(cp, std::back_inserter(dst));
	}
}}


namespace text_align {
	
	template <typename t_score, typename t_delegate>
	class smith_waterman_aligner
	{
		static_assert(std::is_integral_v <t_score>, "Expected t_score to be a signed integer type.");
		static_assert(std::is_signed_v <t_score>, "Expected t_score to be a signed integer type.");
		
	protected:
		enum {
			SCORE_MIN = std::numeric_limits <t_score>::min()
		};
		
		typedef std::atomic_uint8_t													atomic_counter;
		typedef std::vector <t_score>												score_vector;
		typedef matrix <t_score>													score_matrix;
		typedef matrix <atomic_counter>												flag_matrix;
		typedef detail::smith_waterman_aligner_impl_base <smith_waterman_aligner>	impl_base_type;
		
		friend impl_base_type;
		static_assert(atomic_counter::is_always_lock_free);
		
	protected:
		boost::asio::thread_pool			*m_ctx{nullptr};
		t_delegate							*m_delegate{nullptr};
		std::unique_ptr <impl_base_type>	m_aligner_impl;
		flag_matrix							m_flags;
		score_matrix						m_score;
		score_vector						m_lhs_gap_scores;
		std::vector <bool>					m_lhs_gaps;
		std::vector <bool>					m_rhs_gaps;
		t_score								m_identity_score{2};
		t_score								m_mismatch_penalty{-1};
		t_score								m_gap_start_penalty{-10};
		t_score								m_gap_penalty{-1};
		std::size_t							m_lhs_length{0};
		std::size_t							m_rhs_length{0};
		std::uint32_t						m_segment_length{64};
		bool								m_print_debugging_information{false};
		
	protected:
		void fill_traceback();
		void finish() { m_aligner_impl.reset(); m_delegate->finish(*this); }
		
	public:
		smith_waterman_aligner() = default;
		
		smith_waterman_aligner(boost::asio::thread_pool &ctx, t_delegate &delegate):
			m_ctx(&ctx),
			m_delegate(&delegate)
		{
		}
		
		t_score identity_score() const { return m_identity_score; }
		t_score mismatch_penalty() const { return m_mismatch_penalty; }
		t_score gap_start_penalty() const { return m_gap_start_penalty; }
		t_score gap_penalty() const { return m_gap_penalty; }
		uint32_t segment_length() const { return m_segment_length; }
		bool prints_debugging_information() const { return m_print_debugging_information; }
		std::size_t lhs_size() const { return m_lhs_length; }
		std::size_t rhs_size() const { return m_rhs_length; }
		
		boost::asio::thread_pool &execution_context() { return *m_ctx; }
		
		void set_identity_score(t_score const score) { m_identity_score = score; }
		void set_mismatch_penalty(t_score const score) { m_mismatch_penalty = score; }
		void set_gap_start_penalty(t_score const score) { m_gap_start_penalty = score; }
		void set_gap_penalty(t_score const score) { m_gap_penalty = score; }
		void set_segment_length(uint32_t const length) { m_segment_length = length; }
		void set_prints_debugging_information(bool const should_print) { m_print_debugging_information = should_print; }
		
		template <typename t_lhs, typename t_rhs>
		void align(t_lhs const &lhs, t_rhs const &rhs);
		
		template <typename t_lhs, typename t_rhs>
		void align(
			t_lhs const &lhs,
			t_rhs const &rhs,
			std::size_t const lhs_len,
			std::size_t const rhs_len
		);
		
		t_score alignment_score() const;
		
		std::vector <bool> &lhs_gaps() { return m_lhs_gaps; };
		std::vector <bool> &rhs_gaps() { return m_rhs_gaps; };
		std::vector <bool> const &lhs_gaps() const { return m_lhs_gaps; };
		std::vector <bool> const &rhs_gaps() const { return m_rhs_gaps; };
	};
	
	
	// Hold both an aligner and an IO context.
	template <typename t_score, template <typename, typename> class t_aligner>
	class alignment_context
	{
	protected:
		typedef t_aligner <t_score, alignment_context>	aligner_type;
		
		friend aligner_type;
		
	protected:
		aligner_type				m_aligner;
		boost::asio::thread_pool	m_ctx{};
		
	public:
		alignment_context():
			m_aligner(m_ctx, *this)
		{
		}
		
		aligner_type &aligner() { return m_aligner; }
		aligner_type const &aligner() const { return m_aligner; }

		boost::asio::thread_pool &execution_context() { return m_ctx; }
		boost::asio::thread_pool const &execution_context() const { return m_ctx; }
		
		void run() { m_ctx.join(); }
		
	protected:
		void finish(aligner_type &aligner) { m_ctx.stop(); }
	};
	
	
	template <typename t_score>
	using smith_waterman_alignment_context = alignment_context <t_score, smith_waterman_aligner>;
	
	
	// Return the total alignment score.
	template <typename t_score, typename t_delegate>
	t_score smith_waterman_aligner <t_score, t_delegate>::alignment_score() const
	{
		std::size_t y(m_lhs_length);
		std::size_t x(m_rhs_length);
		return m_score(y, x);
	}
	
	
	// Align the given strings.
	template <typename t_score, typename t_delegate>
	template <typename t_lhs, typename t_rhs>
	void smith_waterman_aligner <t_score, t_delegate>::align(t_lhs const &lhs, t_rhs const &rhs)
	{
		align(lhs, rhs, lhs.size(), rhs.size());
	}
	
	
	// Align the given strings.
	template <typename t_score, typename t_delegate>
	template <typename t_lhs, typename t_rhs>
	void smith_waterman_aligner <t_score, t_delegate>::align(
		t_lhs const &lhs,
		t_rhs const &rhs,
		std::size_t const lhs_len,
		std::size_t const rhs_len
	)
	{
		// Clear the state, m_score and m_flags are reset below.
		m_lhs_gaps.clear();
		m_rhs_gaps.clear();
		
		m_lhs_length = lhs_len;
		m_rhs_length = rhs_len;
		
		// Reserve memory for the scoring matrix.
		m_score.resize(1 + m_lhs_length, 1 + m_rhs_length, 0);
		std::fill(m_score.begin(), m_score.end(), 0);
		
		// Reserve memory for the maximum gap scores.
		m_lhs_gap_scores.resize(m_lhs_length);
		std::fill(m_lhs_gap_scores.begin(), m_lhs_gap_scores.end(), 0);
		
		// Initialize the first column and row.
		// Not used before calculating the traceback.)
		for (std::size_t i(0); i < m_lhs_length; ++i)
			m_score(1 + i, 0) = m_gap_start_penalty + (1 + i) * m_gap_penalty;
		for (std::size_t i(0); i < m_rhs_length; ++i)
			m_score(0, 1 + i) = m_gap_start_penalty + (1 + i) * m_gap_penalty;
		
		// Count the segments.
		auto const lhs_segments(std::ceil(1.0 * m_lhs_length / m_segment_length));
		auto const rhs_segments(std::ceil(1.0 * m_rhs_length / m_segment_length));
		
		// Initialize the flags.
		{
			if (m_flags.row_size() < lhs_segments || m_flags.column_size() < rhs_segments)
			{
				using std::swap;
				flag_matrix temp_flags(lhs_segments, rhs_segments);
				temp_flags.set_stride(lhs_segments);
				swap(m_flags, temp_flags);
			}
			
			for (auto &flag : m_flags)
				flag.store(0);
			
			// Set the first row and column to one so that the blocks to the right and below the initial
			// block may be filled.
			for (std::size_t i(0); i < lhs_segments; ++i)
				m_flags(i, 0).store(1);
			
			for (std::size_t i(0); i < rhs_segments; ++i)
				m_flags(0, i).store(1);
		}
		
		// Instantiate the implementation.
		auto impl_ptr(
			new detail::smith_waterman_aligner_impl <smith_waterman_aligner, t_lhs, t_rhs>(
				*this,
				lhs,
				rhs,
				lhs_segments,
				rhs_segments
			)
		);
		m_aligner_impl.reset(impl_ptr);
		
		// Start the alignment tasks.
		boost::asio::post(*m_ctx, [impl_ptr](){
			impl_ptr->align_block(0, 0);
		});
	}
	
	
	// Trace the optimal alignment and fill the gap bit vectors.
	template <typename t_score, typename t_delegate>
	void smith_waterman_aligner <t_score, t_delegate>::fill_traceback()
	{
		// Considering
		//  ⋱   ⋮
		//    A B
		//  … C D
		// Start from the co-ordinates of D that contain the final score, check the values of A, B, C.
		std::size_t y(m_lhs_length);
		std::size_t x(m_rhs_length);
		while (0 < x || 0 < y)
		{
			if (m_print_debugging_information)
				std::cerr << "x: " << x << " y: " << y << std::endl;
			
			auto const s1((0 < x && 0 < y) ? m_score(y - 1, x - 1) : SCORE_MIN);
			auto const s2(0 < y ? m_score(y - 1, x) : SCORE_MIN);
			auto const s3(0 < x ? m_score(y, x - 1) : SCORE_MIN);
		
			if (s1 >= s2 && s1 >= s3)
			{
				m_lhs_gaps.push_back(0);
				m_rhs_gaps.push_back(0);
				--x;
				--y;
			}
			else if (s2 >= s3)
			{
				m_lhs_gaps.push_back(0);
				m_rhs_gaps.push_back(1);
				--y;
			}
			else
			{
				m_lhs_gaps.push_back(1);
				m_rhs_gaps.push_back(0);
				--x;
			}
		}
		
		std::reverse(m_lhs_gaps.begin(), m_lhs_gaps.end());
		std::reverse(m_rhs_gaps.begin(), m_rhs_gaps.end());
	}
}


namespace text_align { namespace detail {
	
	// Fill one block in the dynamic programming matrix.
	template <typename t_owner, typename t_lhs, typename t_rhs>
	void smith_waterman_aligner_impl <t_owner, t_lhs, t_rhs>::align_block(
		std::size_t const lhs_block_idx,
		std::size_t const rhs_block_idx
	)
	{
		// Variables from the owner object.
		auto const seg_len(this->m_owner->segment_length());
		auto const identity_score(this->m_owner->identity_score());
		auto const mismatch_penalty(this->m_owner->mismatch_penalty());
		auto const gap_start_penalty(this->m_owner->gap_start_penalty());
		auto const gap_penalty(this->m_owner->gap_penalty());
		auto const lhs_len(this->m_owner->lhs_size());
		auto const rhs_len(this->m_owner->rhs_size());
		auto &ctx(this->m_owner->execution_context());
		auto &score_mat(this->score_matrix());
		auto &lhs_gap_scores(this->lhs_gap_scores());
		auto &flags(this->flag_matrix());
		
		// Scoring matrix index limits.
		auto const lhs_idx(seg_len * lhs_block_idx);
		auto const rhs_idx(seg_len * rhs_block_idx);
		auto const lhs_limit(std::min(lhs_len, lhs_idx + seg_len));
		auto const rhs_limit(std::min(rhs_len, rhs_idx + seg_len));
	
		// Fill the scoring matrix.
		auto lhs_it(m_lhs->begin());
		auto rhs_it(m_rhs->begin());
		// FIXME: after processing the range, store the iterators for use in the other blocks.
		std::advance(lhs_it, lhs_idx);
		std::advance(rhs_it, rhs_idx);
		for (std::size_t i(rhs_idx); i < rhs_limit; ++i) // Column
		{
			assert(rhs_it != m_rhs->end());
			
			auto const rhs_c(*rhs_it);
			auto lhs_it_2(lhs_it);
			score_type rhs_gap_score(0);
			for (std::size_t j(lhs_idx); j < lhs_limit; ++j) // Row
			{
				assert(lhs_it_2 != m_lhs->end());
				
				// Update the existing max. score s.t. the gap would continue here.
				lhs_gap_scores[1 + j] += gap_penalty;
				rhs_gap_score += gap_penalty;
				
				auto const lhs_c(*lhs_it_2);
				auto const s1(score_mat(j, i) + (text_align::is_equal(lhs_c, rhs_c) ? identity_score : mismatch_penalty));
				auto const s2(lhs_gap_scores[1 + j]);
				auto const s3(rhs_gap_score);
				
				// Take gap_start_penalty into account when choosing the maximum element and saving the score,
				// when caching the value, don't add gap_start_penalty.
				std::initializer_list <score_type> const scores{s1, gap_start_penalty + s2, gap_start_penalty + s3};
				std::initializer_list <score_type> const gap_scores{s1, s2, s3};
				auto const max_idx(argmax_element(scores.begin(), scores.end()));
				assert(max_idx < scores.size());
				assert(max_idx < gap_scores.size());
				auto const score(*(max_idx + scores.begin()));
				auto const gap_score(*(max_idx + gap_scores.begin()));
				
				score_mat(1 + j, 1 + i) = score;
				lhs_gap_scores[1 + j] = std::max({gap_score, lhs_gap_scores[1 + j]});
				rhs_gap_score = std::max({gap_score, rhs_gap_score});
				
				++lhs_it_2;
			}
			
			++rhs_it;
		}
		
		// Considering the folliwing blocks:
		//  A B
		//  C D
		//  E F
		// When C finishes, increment flags for D and E. D may be started if B has finished before.
		// A need not be considered in this case b.c. in order to start C, A has to have finished before.
	
		if (1 + lhs_block_idx == this->m_lhs_segments && 1 + rhs_block_idx == this->m_rhs_segments)
		{
			if (this->m_owner->prints_debugging_information())
				this->print_matrix();
			
			this->fill_traceback();
			this->finish();
		}
		else
		{
			if (1 + lhs_block_idx < this->m_lhs_segments)
			{
				auto const prev_val(flags(1 + lhs_block_idx, rhs_block_idx)++);
				if (1 == prev_val)
				{
					boost::asio::post(ctx, [this, lhs_block_idx, rhs_block_idx](){
						align_block(1 + lhs_block_idx, rhs_block_idx);
					});
				}
			}
	
			if (1 + rhs_block_idx < this->m_rhs_segments)
			{
				auto const prev_val(flags(lhs_block_idx, 1 + rhs_block_idx)++);
				if (1 == prev_val)
				{
					boost::asio::post(ctx, [this, lhs_block_idx, rhs_block_idx](){
						align_block(lhs_block_idx, 1 + rhs_block_idx);
					});
				}
			}
		}
	}
	
	
	template <typename t_owner, typename t_lhs, typename t_rhs>
	void smith_waterman_aligner_impl <t_owner, t_lhs, t_rhs>::print_matrix() const
	{
		auto &score_mat(this->score_matrix());
		std::string buffer;
		
		std::cout << '\t';
		auto const rhs_len(this->m_owner->rhs_size());
		for (std::size_t j(0); j <= rhs_len; ++j)
			std::cout << '\t' << '(' << j << ')';
		std::cout << '\n';
		
		std::cout << '\t';
		auto rhs_it(m_rhs->begin());
		for (std::size_t j(0); j <= rhs_len; ++j)
		{
			std::cout << '\t';
			if (j)
			{
				detail::to_utf8(*rhs_it++, buffer);
				std::cout << buffer;
			}
		}
		std::cout << '\n';
		
		auto lhs_it(m_lhs->begin());
		auto const lhs_len(this->m_owner->lhs_size());
		for (std::size_t i(0); i <= lhs_len; ++i)
		{
			std::cout << '(' << i << ")\t";
			if (i)
			{
				detail::to_utf8(*lhs_it++, buffer);
				std::cout << buffer;
			}
		
			for (std::size_t j(0); j <= rhs_len; ++j)
				std::cout << '\t' << score_mat(i, j);
			std::cout << '\n';
		}
	}
}}

#endif
