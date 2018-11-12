/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef TEXT_ALIGN_SMITH_WATERMAN_ALIGNER_IMPL_HH
#define TEXT_ALIGN_SMITH_WATERMAN_ALIGNER_IMPL_HH

#include <text_align/smith_waterman/aligner_impl_base.hh>
#include <text_align/smith_waterman/matrix_printer.hh>


namespace text_align { namespace smith_waterman { namespace detail {
	
	template <typename t_owner, typename t_lhs, typename t_rhs>
	class aligner_impl : public aligner_impl_base <t_owner>
	{
	protected:
		typedef aligner_impl_base <t_owner>	superclass;
		typedef typename superclass::score_type				score_type;
		typedef typename superclass::gap_score_gt_type		gap_score_gt_type;
		typedef typename superclass::score_matrix			score_matrix;
		typedef typename superclass::arrow_type				arrow_type;

		enum find_gap_type : std::uint8_t
		{
			UNSET	= 0x0,
			LEFT	= 0x1,
			UP		= 0x2
		};
		
	protected:
		t_lhs const	*m_lhs_text{nullptr};
		t_rhs const	*m_rhs_text{nullptr};
	
	public:
		aligner_impl() = default;
		aligner_impl(
			t_owner &owner,
			t_lhs const &lhs,
			t_rhs const &rhs
		):
			aligner_impl_base <t_owner>(owner),
			m_lhs_text(&lhs),
			m_rhs_text(&rhs)
		{
		}
		
		void align_block(std::size_t const lhs_block_idx, std::size_t const rhs_block_idx);
		
	protected:
		struct score_result
		{
			score_type score{};
			score_type gap_score{};
			std::uint8_t max_idx{};
			gap_score_gt_type is_gap_score_gt{};
		};
		
		template <typename t_lhs_c, typename t_rhs_c>
		inline void calculate_score(
			score_type const prev_diag_score,
			t_lhs_c const lhs_c,
			t_rhs_c const rhs_c,
			std::size_t const j,
			std::size_t const i,
			score_result &result
		);
		
		template <bool t_initial>
		void fill_block(
			std::size_t const lhs_block_idx,
			std::size_t const rhs_block_idx,
			score_matrix *score_buffer = nullptr
		);
			
		void fill_traceback();
	};
	
	
	template <typename t_owner, typename t_lhs, typename t_rhs>
	template <typename t_lhs_c, typename t_rhs_c>
	void aligner_impl <t_owner, t_lhs, t_rhs>::calculate_score(
		score_type const prev_diag_score,
		t_lhs_c const lhs_c,
		t_rhs_c const rhs_c,
		std::size_t const j,
		std::size_t const i,
		score_result &result
	)
	{
		auto const identity_score(this->m_parameters->identity_score);
		auto const mismatch_penalty(this->m_parameters->mismatch_penalty);
		auto const gap_start_penalty(this->m_parameters->gap_start_penalty);
		auto const gap_penalty(this->m_parameters->gap_penalty);
		
		// Update the existing gap score s.t. the gap would continue here.
		this->m_data->gap_scores_lhs[1 + j]	+= gap_penalty;
		this->m_gap_score_rhs				+= gap_penalty;
	
		auto const s1(prev_diag_score + (text_align::is_equal(lhs_c, rhs_c) ? identity_score : mismatch_penalty));
		auto const s2((this->m_data->gap_scores_lhs)[1 + j]);	// Lhs string
		auto const s3(this->m_gap_score_rhs);					// Rhs string
	
		// Take gap_start_penalty into account when choosing the maximum element and saving the score.
		// When caching the value, don't add gap_start_penalty.
		std::initializer_list <score_type> const scores{s1, gap_start_penalty + s2, gap_start_penalty + s3};
		result.max_idx = argmax_element(scores.begin(), scores.end());
	
		// Score
		assert(result.max_idx < scores.size());
		result.score = *(result.max_idx + scores.begin());
		
		// Gap score
		std::initializer_list <score_type> const gap_scores{s1, s2, s3};
		auto const max_gs_idx(argmax_element(gap_scores.begin(), gap_scores.end()));
		
		std::initializer_list <gap_score_gt_type> const is_gap_score_gt_values{
			gap_score_gt_type::GSGT_BOTH,
			gap_score_gt_type::GSGT_UP,
			gap_score_gt_type::GSGT_LEFT
		};
	
		result.gap_score = *(result.max_idx + gap_scores.begin());
		
		if (s2 == s3)
			result.is_gap_score_gt = gap_score_gt_type::GSGT_BOTH;
		else
			result.is_gap_score_gt = *(max_gs_idx + is_gap_score_gt_values.begin());
	}
	
	
	template <typename t_owner, typename t_lhs, typename t_rhs>
	template <bool t_initial>
	void aligner_impl <t_owner, t_lhs, t_rhs>::fill_block(
		std::size_t const lhs_block_idx,
		std::size_t const rhs_block_idx,
		score_matrix *score_buffer
	)
	{
		auto const segment_length(this->m_owner->segment_length());
		
		// Scoring matrix index limits.
		auto const lhs_idx(segment_length * lhs_block_idx);
		auto const rhs_idx(segment_length * rhs_block_idx);
		std::initializer_list <std::size_t> const lhs_limits{this->m_parameters->lhs_length, lhs_idx + segment_length - 1};
		std::initializer_list <std::size_t> const rhs_limits{this->m_parameters->rhs_length, rhs_idx + segment_length - 1};
		bool const calculate_final_row(argmin_element(lhs_limits.begin(), lhs_limits.end()));
		bool const calculate_final_column(argmin_element(rhs_limits.begin(), rhs_limits.end()));
		auto const lhs_limit(*(calculate_final_row + lhs_limits.begin()));
		auto const rhs_limit(*(calculate_final_column + rhs_limits.begin()));
		
		// Score buffers.
		auto *src_buffer_ptr(&this->m_data->score_buffer_1);
		auto *dst_buffer_ptr(&this->m_data->score_buffer_2);
		
		// Fill the first column.
		{
			auto const column(this->m_lhs->score_samples.column(rhs_block_idx));
			auto const it(column.begin());
			std::copy(it + lhs_idx, it + lhs_limit, src_buffer_ptr->begin() + lhs_idx);
		}
		
		// Fill the part of lhs max scores.
		{
			auto const column(this->m_lhs->gap_score_samples.column(rhs_block_idx).begin());
			auto const it(column);
			std::copy(it + lhs_idx, it + lhs_limit, this->m_data->gap_scores_lhs.begin() + lhs_idx);
		}
		
		// Find the correct text position.
		auto lhs_it(m_lhs_text->begin());
		auto rhs_it(m_rhs_text->begin());
		// FIXME: after processing the range, store the iterators for use in the other blocks.
		std::advance(lhs_it, lhs_idx);
		std::advance(rhs_it, rhs_idx);
		
		// Fill the output buffer if needed.
		if (score_buffer)
		{
			auto column(score_buffer->column(0));
			std::copy(src_buffer_ptr->begin(), src_buffer_ptr->end(), column.begin());
		}
		
		// Fill the block.
		score_result result;
		for (std::size_t i(rhs_idx); i < rhs_limit; ++i) // Column
		{
			assert(rhs_it != m_rhs_text->end());
			
			auto const rhs_c(*rhs_it);
			auto lhs_it_2(lhs_it);
			this->m_gap_score_rhs = 0;
			
			for (std::size_t j(lhs_idx); j < lhs_limit; ++j) // Row
			{
				assert(lhs_it_2 != m_lhs_text->end());
				auto const lhs_c(*lhs_it_2);
				auto const prev_diag_score((*src_buffer_ptr)[j]);
				calculate_score(prev_diag_score, lhs_c, rhs_c, j, i, result);
				
				// Store the values.
				(*dst_buffer_ptr)[1 + j]			= result.score;
				this->m_data->gap_scores_lhs[1 + j]	= result.gap_score;
				this->m_gap_score_rhs				= result.gap_score;
				
				// Store the traceback value if needed.
				if (!t_initial)
				{
					auto const y(1 + j - lhs_idx);
					auto const x(1 + i - rhs_idx);
					this->m_data->traceback(y, x)		= result.max_idx;
					this->m_data->gap_score_gt(y, x)	= result.is_gap_score_gt;
				}
				
				++lhs_it_2;
			}
			
			// Fill the next sample row.
			if (t_initial && calculate_final_row)
			{
				assert(lhs_it_2 != m_lhs_text->end());
				auto const lhs_c(*lhs_it_2);
				auto const prev_diag_score((*src_buffer_ptr)[lhs_limit]);
				calculate_score(prev_diag_score, lhs_c, rhs_c, lhs_limit, i, result);
				
				this->m_rhs->score_samples(1 + i, 1 + lhs_block_idx)		= result.score;				// Horizontal
				this->m_rhs->gap_score_samples(1 + i, 1 + lhs_block_idx)	= this->m_gap_score_rhs;	// Horizontal
				this->m_rhs->gap_score_gt(1 + i, 1 + lhs_block_idx)			= result.is_gap_score_gt;	// Horizontal
				this->m_rhs->initial_traceback(1 + i, 1 + lhs_block_idx)	= result.max_idx;			// Horizontal, values same as arrow_type::*
			}
			
			// Copy to score_buffer if needed.
			if (score_buffer)
			{
				auto column(score_buffer->column(i - rhs_idx + 1));
				auto it(dst_buffer_ptr->begin());
				std::copy(it + lhs_idx, it + lhs_limit, column.begin());
				column[lhs_limit] = result.score;
			}
			
			// Swap src and dst.
			{
				using std::swap;
				swap(src_buffer_ptr, dst_buffer_ptr);
			}
			
			++rhs_it;
		}
		
		// Handle the final column.
		if (t_initial && calculate_final_column)
		{
			assert(rhs_it != m_rhs_text->end());
			
			auto const rhs_c(*rhs_it);
			auto lhs_it_2(lhs_it);
			for (std::size_t j(lhs_idx); j < lhs_limit; ++j) // Row
			{
				assert(lhs_it_2 != m_lhs_text->end());
				auto const lhs_c(*lhs_it_2);
				auto const prev_diag_score((*src_buffer_ptr)[j]);
				calculate_score(prev_diag_score, lhs_c, rhs_c, j, rhs_limit, result);
				
				this->m_lhs->score_samples(1 + j, 1 + rhs_block_idx)		= result.score;							// Horizontal
				this->m_lhs->gap_score_samples(1 + j, 1 + rhs_block_idx)	= this->m_data->gap_scores_lhs[1 + j];	// Horizontal
				this->m_lhs->gap_score_gt(1 + j, 1 + rhs_block_idx)			= result.is_gap_score_gt;
				this->m_lhs->initial_traceback(1 + j, 1 + rhs_block_idx)	= result.max_idx;						// Same as arrow_type::*
			}
		}
		
		if (t_initial)
		{
			// Update the score.
			this->m_block_score = result.score;
		}
	}
	
	
	template <typename t_owner, typename t_lhs, typename t_rhs>
	void aligner_impl <t_owner, t_lhs, t_rhs>::fill_traceback()
	{
		arrow_type dir{};
		
		// Variables from the owner object.
		auto const seg_len(this->m_owner->segment_length());
		auto const lhs_len(this->m_owner->lhs_size());
		auto const rhs_len(this->m_owner->rhs_size());
		auto const print_debugging_information(this->m_owner->prints_debugging_information());
		
		auto &traceback(this->m_data->traceback);
		
		assert(64 <= seg_len);
		
		std::size_t lhs_block_idx(this->m_parameters->lhs_segments - 1);
		std::size_t rhs_block_idx(this->m_parameters->rhs_segments - 1);
		matrix <score_type> score_buffer;
		matrix <score_type> *score_buffer_ptr(nullptr);
		
		// Scoring matrix indices.
		// lhs_idx and rhs_idx point to the upper left corner of the final block.
		auto const lhs_idx(seg_len * lhs_block_idx);
		auto const rhs_idx(seg_len * rhs_block_idx);
		assert(lhs_idx < lhs_len);
		assert(rhs_idx < rhs_len);
		auto const lhs_limit(min(seg_len, lhs_len - lhs_idx));
		auto const rhs_limit(min(seg_len, rhs_len - rhs_idx));
		std::size_t j(lhs_limit); // (Last) row
		std::size_t i(rhs_limit); // (Last) column
		std::uint8_t mode(0);
		
		if (print_debugging_information)
		{
			score_buffer.resize(traceback.row_size() * 32, traceback.column_size() * 32);
			score_buffer_ptr = &score_buffer;
		}
		
		while (true)
		{
			// Initialize the block to calculate the traceback.
			std::fill(traceback.begin(), traceback.end(), 0);
			std::fill(this->m_data->gap_score_gt.begin(), this->m_data->gap_score_gt.end(), 0);
			
			// Initialize the score buffer if needed.
			std::fill(score_buffer.begin(), score_buffer.end(), 0);

			// Fill the first rows and columns of the matrices used in traceback.
			{
				auto const first_lhs(lhs_block_idx * seg_len);
				auto const first_rhs(rhs_block_idx * seg_len);
				auto const limit_lhs((1 + lhs_block_idx) * seg_len);
				auto const limit_rhs((1 + rhs_block_idx) * seg_len);
				
				{
					auto dst(traceback.column(0));
					auto src(this->m_lhs->initial_traceback.column(rhs_block_idx, first_lhs / 32, limit_lhs / 32));
					assert(src.size() == dst.size());
					std::transform(src.begin(), src.end(), dst.begin(), [](auto &atomic){ return atomic.load(); });
				}
				
				{
					auto dst(this->m_data->gap_score_gt.column(0));
					auto src(this->m_lhs->gap_score_gt.column(rhs_block_idx, first_lhs / 32, limit_lhs / 32));
					assert(src.size() == dst.size());
					std::transform(src.begin(), src.end(), dst.begin(), [](auto &atomic){ return atomic.load(); });
				}
				
				transpose_column_to_row(traceback.row(0), this->m_rhs->initial_traceback.column(lhs_block_idx, first_rhs / 32, limit_rhs / 32));
				transpose_column_to_row(this->m_data->gap_score_gt.row(0), this->m_rhs->gap_score_gt.column(lhs_block_idx, first_rhs / 32, limit_rhs / 32));
			}
			
			// The first row and column are now filled, run the filling algorithm.
			fill_block <false>(lhs_block_idx, rhs_block_idx, score_buffer_ptr);
			
			// If this is the last block, check that the corner is marked.
			assert((! (0 == lhs_block_idx && 0 == rhs_block_idx)) || traceback(0, 0) == arrow_type::ARROW_FINISH);
			
			// Continue finding the gap starting position if needed.
			{
				std::size_t steps(0);
				switch (mode)
				{
					case find_gap_type::LEFT:
					{
						bool const res(this->find_gap_start_x(j, i, steps));
						append_bits(this->m_lhs->gaps, steps, 1);
						append_bits(this->m_rhs->gaps, steps, 0);
						if (!res)
						{
							assert(rhs_block_idx);
							--rhs_block_idx;
							i = seg_len - 1;
							goto continue_loop;
						}
						break;
					}
				
					case find_gap_type::UP:
					{
						bool const res(this->find_gap_start_y(j, i, steps));
						append_bits(this->m_lhs->gaps, steps, 0);
						append_bits(this->m_rhs->gaps, steps, 1);
						if (!res)
						{
							assert(lhs_block_idx);
							--lhs_block_idx;
							j = seg_len - 1;
							goto continue_loop;
						}
						break;
					}
					
					default:
						break;
				}
			}
			
			// Follow the traceback.
			mode = find_gap_type::UNSET;
			while (true)
			{
				dir = static_cast <arrow_type>(traceback(j, i).operator std::uint8_t());
				std::size_t steps(0);
				
				switch (dir)
				{
					case arrow_type::ARROW_DIAGONAL:
						// No gaps.
						this->m_lhs->gaps.push_back(0);
						this->m_rhs->gaps.push_back(0);
						
						// If either co-ordinate is zero, move to the adjacent block.
						if (! (i && j))
						{
							if (0 == i)
							{
								assert(rhs_block_idx);
								--rhs_block_idx;
								i = seg_len - 1;
							}
							else
							{
								--i;
							}
							
							if (0 == j)
							{
								assert(lhs_block_idx);
								--lhs_block_idx;
								j = seg_len - 1;
							}
							else
							{
								--j;
							}
							
							goto continue_loop;
						}
					
						assert(i);
						assert(j);
						--i;
						--j;
						
						break;
						
					case arrow_type::ARROW_LEFT:
					{
						// Move left as long as possible and to the adjacent block if needed.
						bool const res(this->find_gap_start_x(j, i, steps));
						append_bits(this->m_lhs->gaps, steps, 1);
						append_bits(this->m_rhs->gaps, steps, 0);
						if (!res)
						{
							assert(rhs_block_idx);
							--rhs_block_idx;
							i = seg_len - 1;
							mode = find_gap_type::LEFT;
							goto continue_loop;
						}
						break;
					}
					
					case arrow_type::ARROW_UP:
					{
						// Move up as long as possible and to the adjacent block if needed.
						bool const res(this->find_gap_start_y(j, i, steps));
						append_bits(this->m_lhs->gaps, steps, 0);
						append_bits(this->m_rhs->gaps, steps, 1);
						if (!res)
						{
							assert(lhs_block_idx);
							--lhs_block_idx;
							j = seg_len - 1;
							mode = find_gap_type::UP;
							goto continue_loop;
						}
						break;
						
					}
					
					case arrow_type::ARROW_FINISH:
						goto exit_loop;
						
					default:
						fail_assertion();
				}
			}
			
		continue_loop:
			if (print_debugging_information)
			{
				// FIXME: handle more than 63 characters.
				matrix_printer printer(
					1 + lhs_limit,
					1 + rhs_limit,
					this->m_lhs->gaps,
					this->m_rhs->gaps,
					m_lhs_text->begin(),
					m_rhs_text->begin()
				);
				
				printer.print_scores(score_buffer);
				std::cerr << '\n';
				printer.print_traceback(traceback);
				std::cerr << '\n';
			}
		}
		
	exit_loop:
		if (print_debugging_information)
		{
			// FIXME: handle more than 63 characters.
			matrix_printer printer(
				1 + lhs_limit,
				1 + rhs_limit,
				this->m_lhs->gaps,
				this->m_rhs->gaps,
				m_lhs_text->begin(),
				m_rhs_text->begin()
			);

			printer.print_scores(score_buffer);
			std::cerr << '\n';
			printer.print_traceback(traceback);
			std::cerr << '\n';
		}

		// Reverse the paths.
		reverse_bitset(this->m_lhs->gaps);
		reverse_bitset(this->m_rhs->gaps);
	}
	
	
	// Fill one block in the dynamic programming matrix.
	template <typename t_owner, typename t_lhs, typename t_rhs>
	void aligner_impl <t_owner, t_lhs, t_rhs>::align_block(
		std::size_t const lhs_block_idx,
		std::size_t const rhs_block_idx
	)
	{
		fill_block <true>(lhs_block_idx, rhs_block_idx);
		
		// Considering the folliwing blocks:
		//  A B
		//  C D
		//  E F
		// When C finishes, increment flags for D and E. D may be started if B has finished before.
		// A need not be considered in this case b.c. in order to start C, A has to have finished before.
	
		auto const lhs_segments(this->m_parameters->lhs_segments);
		auto const rhs_segments(this->m_parameters->rhs_segments);
		if (1 + lhs_block_idx == lhs_segments && 1 + rhs_block_idx == rhs_segments)
		{
			this->fill_traceback();
			this->finish();
		}
		else
		{
			if (1 + lhs_block_idx < lhs_segments)
			{
				auto const prev_val((this->m_data->flags)(1 + lhs_block_idx, rhs_block_idx)++);
				if (1 == prev_val)
				{
					boost::asio::post(*this->m_ctx, [this, lhs_block_idx, rhs_block_idx](){
						align_block(1 + lhs_block_idx, rhs_block_idx);
					});
				}
			}
			
			if (1 + rhs_block_idx < rhs_segments)
			{
				auto const prev_val((this->m_data->flags)(lhs_block_idx, 1 + rhs_block_idx)++);
				if (1 == prev_val)
				{
					boost::asio::post(*this->m_ctx, [this, lhs_block_idx, rhs_block_idx](){
						align_block(lhs_block_idx, 1 + rhs_block_idx);
					});
				}
			}
		}
	}
}}}

#endif

