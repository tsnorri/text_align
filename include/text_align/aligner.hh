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
#include <text_align/compare.hh>
#include <type_traits>
#include <vector>


//#define my_assert(X) do { if (! (X)) { std::cerr << "Assertion failed on line " << __LINE__ << ": " << #X << std::endl; abort(); }} while (false)


namespace text_align { namespace detail {
	
	inline std::size_t idx(std::size_t const x, std::size_t const y, std::size_t const height) { return x * height + y; }
	
	
	template <typename t_owner>
	class smith_waterman_aligner_impl_base
	{
	protected:
		t_owner		*m_owner{nullptr};
		std::size_t	m_lhs_segments{0};
		std::size_t	m_rhs_segments{0};
		
	protected:
		inline std::size_t fidx(std::size_t const x, std::size_t const y) const { return detail::idx(x, y, m_lhs_segments); }
		
	public:
		smith_waterman_aligner_impl_base() = default;
		smith_waterman_aligner_impl_base(t_owner &owner, std::size_t const lhs_segments, std::size_t const rhs_segments):
			m_owner(&owner),
			m_lhs_segments(lhs_segments),
			m_rhs_segments(rhs_segments)
		{
		}
		
		typename t_owner::score_matrix &score_matrix() { return m_owner->m_score; }
		typename t_owner::score_matrix const &score_matrix() const { return m_owner->m_score; }
		typename t_owner::flag_matrix &flag_matrix() { return m_owner->m_flags; }
		typename t_owner::flag_matrix const &flag_matrix() const { return m_owner->m_flags; }
		void print_matrix();
		void fill_traceback() { this->m_owner->fill_traceback(); }
		void finish() { this->m_owner->finish(); }
	};
	
	
	template <typename t_owner, typename t_lhs, typename t_rhs>
	class smith_waterman_aligner_impl : public smith_waterman_aligner_impl_base <t_owner>
	{
	protected:
		typedef smith_waterman_aligner_impl_base <t_owner> base_class;
		
	protected:
		t_lhs const	*m_lhs{nullptr};
		t_rhs const	*m_rhs{nullptr};
	
	protected:
		inline std::size_t idx(std::size_t const x, std::size_t const y) const { return detail::idx(x, y, (1 + this->m_owner->lhs_size())); }
		
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
		
		typedef std::vector <t_score>												score_matrix;
		typedef std::vector <std::atomic_flag>										flag_matrix;
		typedef detail::smith_waterman_aligner_impl_base <smith_waterman_aligner>	impl_base_type;
		friend impl_base_type;
		
	protected:
		boost::asio::io_context				*m_ctx{nullptr};
		t_delegate							*m_delegate{nullptr};
		score_matrix						m_score;
		flag_matrix							m_flags;
		std::vector <bool>					m_lhs_gaps;
		std::vector <bool>					m_rhs_gaps;
		std::unique_ptr <impl_base_type>	m_aligner_impl;
		t_score								m_identity_score{2};
		t_score								m_mismatch_penalty{-1};
		t_score								m_gap_penalty{-1};
		std::size_t							m_lhs_length{0};
		std::size_t							m_rhs_length{0};
		std::uint32_t						m_segment_length{64};
		bool								m_print_debugging_information{false};
		
	protected:
		inline std::size_t idx(std::size_t const x, std::size_t const y) const { return detail::idx(x, y, 1 + m_lhs_length); }
		
		void fill_traceback();
		void finish() { m_aligner_impl.reset(); m_delegate->finish(*this); }
		
	public:
		smith_waterman_aligner() = default;
		
		smith_waterman_aligner(boost::asio::io_context &ctx, t_delegate &delegate):
			m_ctx(&ctx),
			m_delegate(&delegate)
		{
		}
		
		t_score identity_score() const { return m_identity_score; }
		t_score mismatch_penalty() const { return m_mismatch_penalty; }
		t_score gap_penalty() const { return m_gap_penalty; }
		uint32_t segment_length() const { return m_segment_length; }
		bool prints_debugging_information() const { return m_print_debugging_information; }
		std::size_t lhs_size() const { return m_lhs_length; }
		std::size_t rhs_size() const { return m_rhs_length; }
		
		boost::asio::io_context &io_context() { return *m_ctx; }
		
		void set_identity_score(t_score const score) { m_identity_score = score; }
		void set_mismatch_penalty(t_score const score) { m_mismatch_penalty = score; }
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
		boost::asio::io_context		m_ctx;
		
	public:
		alignment_context():
			m_aligner(m_ctx, *this)
		{
		}
		
		aligner_type &aligner() { return m_aligner; }
		aligner_type const &aligner() const { return m_aligner; }

		boost::asio::io_context &io_context() { return m_ctx; }
		boost::asio::io_context const &io_context() const { return m_ctx; }
		
		void run() { m_ctx.run(); }
		
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
		return m_score[idx(x, y)];
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
		m_score.resize((1 + m_lhs_length) * (1 + m_rhs_length), 0);
		
		// Initialize the first column and row.
		for (std::size_t i(0); i < m_lhs_length; ++i)
			m_score[idx(0, 1 + i)] = (1 + i) * m_gap_penalty;
		for (std::size_t i(0); i < m_rhs_length; ++i)
			m_score[idx(1 + i, 0)] = (1 + i) * m_gap_penalty;
		
		// Count the segments.
		auto const lhs_segments(std::ceil(1.0 * m_lhs_length / m_segment_length));
		auto const rhs_segments(std::ceil(1.0 * m_rhs_length / m_segment_length));
		
		// Initialize the flags.
		{
			auto const flag_count(lhs_segments * rhs_segments);
			if (m_flags.size() < flag_count)
			{
				using std::swap;
				flag_matrix temp_flags(flag_count);
				swap(m_flags, temp_flags);
			}
			
			for (auto &flag : m_flags)
				flag.clear();
			
			// Set the first row and column to one so that the blocks to the right and below the initial
			// block may be filled.
			for (std::size_t i(0); i < lhs_segments; ++i)
				m_flags[detail::idx(0, i, lhs_segments)].test_and_set();
			
			for (std::size_t i(0); i < rhs_segments; ++i)
				m_flags[detail::idx(i, 0, lhs_segments)].test_and_set();
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
			
			auto const s1((0 < x && 0 < y) ? m_score[idx(x - 1, y - 1)] : SCORE_MIN);
			auto const s2(0 < y ? m_score[idx(x, y - 1)] : SCORE_MIN);
			auto const s3(0 < x ? m_score[idx(x - 1, y)] : SCORE_MIN);
		
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
		auto const gap_penalty(this->m_owner->gap_penalty());
		auto const lhs_len(this->m_owner->lhs_size());
		auto const rhs_len(this->m_owner->rhs_size());
		auto &ctx(this->m_owner->io_context());
		auto &score_mat(this->score_matrix());
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
		for (std::size_t i(lhs_idx); i < lhs_limit; ++i)
		{
			assert(lhs_it != m_lhs->end());
			
			auto const lhs_c(*lhs_it);
			auto rhs_it_2(rhs_it);
			for (std::size_t j(rhs_idx); j < rhs_limit; ++j)
			{
				assert(rhs_it_2 != m_rhs->end());
				
				auto const idx1(idx(j, i));
				auto const idx2(idx(1 + j, i));
				auto const idx3(idx(j, 1 + i));
				auto const idx4(idx(1 + j, 1 + i));
				assert(idx1 < score_mat.size());
				assert(idx2 < score_mat.size());
				assert(idx3 < score_mat.size());
				assert(idx4 < score_mat.size());
				
				auto const rhs_c(*rhs_it_2);
				auto const s1(score_mat[idx1] + (text_align::is_equal(lhs_c, rhs_c) ? identity_score : mismatch_penalty));
				auto const s2(score_mat[idx2] + gap_penalty);
				auto const s3(score_mat[idx3] + gap_penalty);
			
				auto const score(std::max({s1, s2, s3}));
				score_mat[idx4] = score;
				
				++rhs_it_2;
			}
			
			++lhs_it;
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
				auto const prev_val(flags[this->fidx(rhs_block_idx, 1 + lhs_block_idx)].test_and_set());
				if (prev_val)
				{
					boost::asio::post(ctx, [this, lhs_block_idx, rhs_block_idx](){
						align_block(1 + lhs_block_idx, rhs_block_idx);
					});
				}
			}
	
			if (1 + rhs_block_idx < this->m_rhs_segments)
			{
				auto const prev_val(flags[this->fidx(1 + rhs_block_idx, lhs_block_idx)].test_and_set());
				if (prev_val)
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
		
		std::cerr << '\t';
		auto const rhs_len(this->m_owner->rhs_size());
		for (std::size_t j(0); j <= rhs_len; ++j)
			std::cerr << '\t' << '(' << j << ')';
		std::cerr << '\n';
		
		std::cerr << '\t';
		auto rhs_it(m_rhs->begin());
		for (std::size_t j(0); j <= rhs_len; ++j)
		{
			std::cerr << '\t';
			if (j)
			{
				detail::to_utf8(*rhs_it++, buffer);
				std::cerr << buffer;
			}
		}
		std::cerr << '\n';
		
		auto lhs_it(m_lhs->begin());
		auto const lhs_len(this->m_owner->lhs_size());
		for (std::size_t i(0); i <= lhs_len; ++i)
		{
			std::cerr << '(' << i << ")\t";
			if (i)
			{
				detail::to_utf8(*lhs_it++, buffer);
				std::cerr << buffer;
			}
		
			for (std::size_t j(0); j <= rhs_len; ++j)
				std::cerr << '\t' << score_mat[idx(j, i)];
			std::cerr << '\n';
		}
	}
}}

#endif
