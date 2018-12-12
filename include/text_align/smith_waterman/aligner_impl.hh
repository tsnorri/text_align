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
		typedef typename superclass::arrow_type				arrow_type;
		typedef typename superclass::score_vector			score_vector;
		typedef typename superclass::score_matrix			score_matrix;

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
			score_type gap_score_lhs{};
			score_type gap_score_rhs{};
			std::uint8_t max_idx{};
			gap_score_gt_type is_gap_score_gt{};
			
			score_result() = default;
			score_result(score_type score_): score(score_) {}
		};
			
		inline void copy_to_score_buffer(
			score_vector const &src,
			std::size_t const column_idx,
			std::size_t const lhs_idx,
			std::size_t const lhs_limit,
			score_matrix &score_buffer
		) const;
		
		template <typename t_lhs_c, typename t_rhs_c>
		inline void calculate_score(
			score_type const prev_diag_score,
			t_lhs_c const lhs_c,
			t_rhs_c const rhs_c,
			std::size_t const j,
			std::size_t const i,
			score_type gap_score_rhs,
			score_result &result
		) const;
		
		template <bool t_initial>
		void fill_block(
			std::size_t const lhs_block_idx,
			std::size_t const rhs_block_idx,
			score_matrix *output_score_buffer = nullptr
		);
			
		void fill_traceback();
	};
		
		
	template <typename t_owner, typename t_lhs, typename t_rhs>
	void aligner_impl <t_owner, t_lhs, t_rhs>::copy_to_score_buffer(
		score_vector const &src,
		std::size_t const column_idx,
		std::size_t const lhs_idx,
		std::size_t const lhs_limit,
		score_matrix &score_buffer
	) const
	{
		auto column(score_buffer.column(column_idx));
		auto const count(lhs_limit - lhs_idx);
		
		assert(count <= column.size());
		assert(count <= std::distance(src.begin() + lhs_idx, src.end()));
		
		auto it(src.begin());
		std::copy(it + lhs_idx, it + lhs_limit, column.begin());
	}
	
	
	template <typename t_owner, typename t_lhs, typename t_rhs>
	template <typename t_lhs_c, typename t_rhs_c>
	void aligner_impl <t_owner, t_lhs, t_rhs>::calculate_score(
		score_type const prev_diag_score,
		t_lhs_c const lhs_c,
		t_rhs_c const rhs_c,
		std::size_t const j,
		std::size_t const i,
		score_type gap_score_rhs,	// Copy.
		score_result &result
	) const
	{
		// Calculate the score for (j + 1, i + 1).
		auto const identity_score(this->m_parameters->identity_score);
		auto const mismatch_penalty(this->m_parameters->mismatch_penalty);
		auto const gap_start_penalty(this->m_parameters->gap_start_penalty);
		auto const gap_penalty(this->m_parameters->gap_penalty);
		
		// Update the existing gap score s.t. the gap would continue here.
		auto const gap_score_lhs(this->m_data->gap_scores_lhs[1 + j] + gap_penalty);
		gap_score_rhs += gap_penalty;
		
		auto const s1(prev_diag_score + (text_align::is_equal(lhs_c, rhs_c) ? identity_score : mismatch_penalty));
		auto const s2(gap_score_lhs);	// Lhs string
		auto const s3(gap_score_rhs);	// Rhs string
		
		result.gap_score_lhs = s2;
		result.gap_score_rhs = s3;
		
		// Take gap_start_penalty into account when choosing the maximum element and saving the score.
		// When caching the value, don't add gap_start_penalty.
		auto const scores(make_array <score_type>(s1, gap_start_penalty + s2, gap_start_penalty + s3));
		result.max_idx = argmax_element(scores.begin(), scores.end());
		
		// Score
		assert(result.max_idx < scores.size());
		result.score = scores[result.max_idx];
		
		// Gap score
		auto const gap_scores(make_array <score_type>(s1, s2, s3));
		auto const max_gs_idx(argmax_element(gap_scores.begin(), gap_scores.end()));
		
		auto const is_gap_score_gt_values(make_array <gap_score_gt_type>(
			gap_score_gt_type::GSGT_BOTH,
			gap_score_gt_type::GSGT_UP,
			gap_score_gt_type::GSGT_LEFT
		));
		
		if (s2 == s3)
			result.is_gap_score_gt = gap_score_gt_type::GSGT_BOTH;
		else
			result.is_gap_score_gt = is_gap_score_gt_values[max_gs_idx];
	}
	
	
	template <typename t_owner, typename t_lhs, typename t_rhs>
	template <bool t_initial>
	void aligner_impl <t_owner, t_lhs, t_rhs>::fill_block(
		std::size_t const lhs_block_idx,
		std::size_t const rhs_block_idx,
		score_matrix *output_score_buffer
	)
	{
		// Fill the first column (represented by the relevant range of src_buffer_ptr) with pre-calculated values.
		// Then, for each subsequent column, take dst_buffer_ptr, fill the value on the first row with a precalculated one
		// and calculate the scores for each row. After the last row of each column, consider filling the next row of
		// values to be used as pre-calculated ones for the next block downwards. Finally, swap the buffers and repeat.
		// When the column limit is reached, consider again filling the next column to the right, including the top-left cell
		// in the next block diagonally to bottom-right from the current one.
		// If output_score_buffer was given, after filling a column copy its contents there.
		
		auto const segment_length(this->m_owner->segment_length());
		
		// Scoring matrix limits. Note that later, one is subtracted b.c.
		// the last row and column (in the adjacent blocks) are handled separately.
		auto const lhs_idx(segment_length * lhs_block_idx);
		auto const rhs_idx(segment_length * rhs_block_idx);
		auto const lhs_limits(make_array <std::size_t>(1 + this->m_parameters->lhs_length, lhs_idx + segment_length));
		auto const rhs_limits(make_array <std::size_t>(1 + this->m_parameters->rhs_length, rhs_idx + segment_length));
		bool const should_calculate_final_row(argmin_element(lhs_limits.begin(), lhs_limits.end()));
		bool const should_calculate_final_column(argmin_element(rhs_limits.begin(), rhs_limits.end()));
		auto const lhs_limit(lhs_limits[should_calculate_final_row]);
		auto const rhs_limit(rhs_limits[should_calculate_final_column]);
		assert(lhs_limit - lhs_idx <= segment_length);
		assert(rhs_limit - rhs_idx <= segment_length);

		// Score buffers.
		auto *src_buffer_ptr(&this->m_data->score_buffer_1);
		auto *dst_buffer_ptr(&this->m_data->score_buffer_2);
		
		// Fill the first column up to what is needed.
		{
			auto const column(this->m_lhs->score_samples.column(rhs_block_idx));
			auto const it(column.begin());
			std::copy(it + lhs_idx, it + lhs_limit, src_buffer_ptr->begin() + lhs_idx);
		}
		
		// Fill the part of lhs max scores.
		{
			auto const column(this->m_lhs->gap_score_samples.column(rhs_block_idx));
			auto const it(column.begin());
			std::copy(it + lhs_idx, it + lhs_limit, this->m_data->gap_scores_lhs.begin() + lhs_idx);
		}
		
		// For filling the topmost value of the dst buffer (using the sampled row).
		auto const &topmost_row(this->m_rhs->score_samples.column(rhs_block_idx)); // Horizontal.
		
		// Find the correct text position.
		auto lhs_it(m_lhs_text->begin());
		auto rhs_it(m_rhs_text->begin());
		// FIXME: after processing the range, store the iterators for use in the other blocks.
		std::advance(lhs_it, lhs_idx);
		std::advance(rhs_it, rhs_idx);
		
		// Fill output_score_buffer if needed.
		if (output_score_buffer)
			copy_to_score_buffer(*src_buffer_ptr, 0, lhs_idx, lhs_limit, *output_score_buffer);
		
		// Fill the block. Initialize the result with the final value of the first column in case
		// it is the only column to fill.
		score_result result((*src_buffer_ptr)[lhs_limit - 1]);
		auto const &gap_scores_rhs(this->m_rhs->gap_score_samples.column(lhs_block_idx));
		for (std::size_t i(rhs_idx); i < rhs_limit - 1; ++i) // Column
		{
			assert(rhs_it != m_rhs_text->end());
			
			auto const rhs_c(*rhs_it);
			auto lhs_it_2(lhs_it);
			score_type gap_score_rhs(gap_scores_rhs[1 + i]);

			// Fill the first row, needed for the first value of prev_diag_score on the next iteration.
			(*dst_buffer_ptr)[lhs_idx] = topmost_row[1 + i];
			
			for (std::size_t j(lhs_idx); j < lhs_limit - 1; ++j) // Row
			{
				assert(lhs_it_2 != m_lhs_text->end());
				auto const lhs_c(*lhs_it_2);
				auto const prev_diag_score((*src_buffer_ptr)[j]);
				calculate_score(prev_diag_score, lhs_c, rhs_c, j, i, gap_score_rhs, result);
				
				// Store the values.
				(*dst_buffer_ptr)[1 + j]			= result.score;
				this->m_data->gap_scores_lhs[1 + j]	= result.gap_score_lhs;
				gap_score_rhs						= result.gap_score_rhs;
				
				// Store the traceback value if needed.
				if (!t_initial)
				{
					auto const y(1 + j - lhs_idx);
					auto const x(1 + i - rhs_idx);
					do_and_assert_eq(this->m_data->traceback(y, x).fetch_or(result.max_idx), 0);
					do_and_assert_eq(this->m_data->gap_score_gt(y, x).fetch_or(result.is_gap_score_gt), 0);
				}
				
				++lhs_it_2;
			}
			
			// Fill the next sample row.
			if (t_initial && should_calculate_final_row)
			{
				assert(lhs_it_2 != m_lhs_text->end());
				
				auto const lhs_c(*lhs_it_2);
				auto const prev_diag_score((*src_buffer_ptr)[lhs_limit - 1]);
				calculate_score(prev_diag_score, lhs_c, rhs_c, lhs_limit - 1, i, gap_score_rhs, result);
				
				this->m_rhs->score_samples(1 + i, 1 + lhs_block_idx)		= result.score;									// Horizontal
				this->m_rhs->gap_score_samples(1 + i, 1 + lhs_block_idx)	= result.gap_score_rhs;							// Horizontal
				do_and_assert_eq(this->m_rhs->gap_score_gt(1 + i, 1 + lhs_block_idx).fetch_or(result.is_gap_score_gt), 0);	// Horizontal
				do_and_assert_eq(this->m_rhs->initial_traceback(1 + i, 1 + lhs_block_idx).fetch_or(result.max_idx), 0);		// Horizontal, values same as arrow_type::*
			}
			
			// Copy to output_score_buffer if needed. For these columns, the value of the final row is relevant (hence lhs_limit, not -1).
			if (output_score_buffer)
				copy_to_score_buffer(*dst_buffer_ptr, i - rhs_idx + 1, lhs_idx, lhs_limit, *output_score_buffer);
			
			// Swap src and dst.
			{
				using std::swap;
				swap(src_buffer_ptr, dst_buffer_ptr);
			}
			
			++rhs_it;
		}
		
		// Fill the next sample column and the corner if needed.
		if (t_initial && should_calculate_final_column)
		{
			assert(rhs_it != m_rhs_text->end());
			
			auto const rhs_c(*rhs_it);
			auto lhs_it_2(lhs_it);
			score_type gap_score_rhs(gap_scores_rhs[rhs_limit - 1]);
			auto const limit(lhs_limit + should_calculate_final_row - 1); // Consider the bottom-right corner.

			for (std::size_t j(lhs_idx); j < limit; ++j) // Row
			{
				assert(lhs_it_2 != m_lhs_text->end());
				
				auto const lhs_c(*lhs_it_2);
				auto const prev_diag_score((*src_buffer_ptr)[j]);
				calculate_score(prev_diag_score, lhs_c, rhs_c, j, rhs_limit - 1, gap_score_rhs, result);
				
				this->m_lhs->score_samples(1 + j, 1 + rhs_block_idx)		= result.score;									// Vertical
				this->m_lhs->gap_score_samples(1 + j, 1 + rhs_block_idx)	= result.gap_score_lhs;							// Vertical
				do_and_assert_eq(this->m_lhs->gap_score_gt(1 + j, 1 + rhs_block_idx).fetch_or(result.is_gap_score_gt), 0);	// Vertical
				do_and_assert_eq(this->m_lhs->initial_traceback(1 + j, 1 + rhs_block_idx).fetch_or(result.max_idx), 0);		// Vertical, values same as arrow_type::*
				
				++lhs_it_2;
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
		auto &gap_score_gt(this->m_data->gap_score_gt);
		
		std::size_t lhs_block_idx(this->m_parameters->lhs_segments - 1);
		std::size_t rhs_block_idx(this->m_parameters->rhs_segments - 1);
		matrix <score_type> score_buffer;
		matrix <score_type> *score_buffer_ptr(nullptr);
		
		// Scoring matrix indices.
		// lhs_idx and rhs_idx point to the upper left corner of the final block, that is, the bottom-right one.
		auto const lhs_idx(seg_len * lhs_block_idx);
		auto const rhs_idx(seg_len * rhs_block_idx);
		assert(lhs_idx <= lhs_len);
		assert(rhs_idx <= rhs_len);
		std::size_t j_limit(min(seg_len, 1 + lhs_len - lhs_idx)); // (Last) row
		std::size_t i_limit(min(seg_len, 1 + rhs_len - rhs_idx)); // (Last) column
		auto j(j_limit - 1);
		auto i(i_limit - 1);
		std::uint8_t mode(0);
		
		if (print_debugging_information)
		{
			score_buffer.resize(traceback.number_of_rows(), traceback.number_of_columns());
			score_buffer_ptr = &score_buffer;
		}
		
		while (true)
		{
			// Initialize the block to calculate the traceback.
			std::fill(traceback.word_begin(), traceback.word_end(), 0);
			std::fill(gap_score_gt.word_begin(), gap_score_gt.word_end(), 0);
			
			// Fill the first rows and columns of the matrices used in traceback.
			auto const lhs_first(seg_len * lhs_block_idx);	// FIXME: minus one?
			auto const rhs_first(seg_len * rhs_block_idx);	// FIXME: minus one?
			{
				auto const lhs_limit(min(1 + lhs_len, seg_len * (1 + lhs_block_idx)));
				auto const rhs_limit(min(1 + rhs_len, seg_len * (1 + rhs_block_idx)));
			
				matrices::copy_to_word_aligned(
					this->m_lhs->initial_traceback.column(rhs_block_idx, lhs_first, lhs_limit),
					traceback.column(0)
				);
				matrices::copy_to_word_aligned(
					this->m_lhs->gap_score_gt.column(rhs_block_idx, lhs_first, lhs_limit),
					gap_score_gt.column(0)
				);
				
				matrices::transpose_column_to_row(this->m_rhs->initial_traceback.column(lhs_block_idx, rhs_first, rhs_limit), traceback.row(0));
				matrices::transpose_column_to_row(this->m_rhs->gap_score_gt.column(lhs_block_idx, rhs_first, rhs_limit), this->m_data->gap_score_gt.row(0));
			}
			
			// The first row and column are now filled, run the filling algorithm.
			std::fill(score_buffer.begin(), score_buffer.end(), 0);
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
				dir = static_cast <arrow_type>(traceback(j, i).load());
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
				matrix_printer printer(
					j_limit,
					i_limit,
					this->m_lhs->gaps,
					this->m_rhs->gaps,
					m_lhs_text->begin(),
					m_rhs_text->begin()
				);
				
				printer.set_lhs_offset(lhs_first - 1);
				printer.set_rhs_offset(rhs_first - 1);
				printer.print_scores(score_buffer);
				std::cerr << '\n';
				printer.print_traceback(traceback);
				std::cerr << '\n';
			}
			
			i_limit = seg_len;
			j_limit = seg_len;
		}
		
	exit_loop:
		if (print_debugging_information)
		{
			matrix_printer printer(
				j_limit,
				i_limit,
				this->m_lhs->gaps,
				this->m_rhs->gaps,
				m_lhs_text->begin(),
				m_rhs_text->begin()
			);

			printer.set_padding(1);
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
				auto const prev_val((this->m_data->flags)(1 + lhs_block_idx, rhs_block_idx).fetch_or(0x1));
				if (0x1 == prev_val)
				{
					boost::asio::post(*this->m_ctx, [this, lhs_block_idx, rhs_block_idx](){
						align_block(1 + lhs_block_idx, rhs_block_idx);
					});
				}
			}
			
			if (1 + rhs_block_idx < rhs_segments)
			{
				auto const prev_val((this->m_data->flags)(lhs_block_idx, 1 + rhs_block_idx).fetch_or(0x1));
				if (0x1 == prev_val)
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

