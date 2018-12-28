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
		typedef typename superclass::score_type					score_type;
		typedef typename superclass::gap_start_position_type	gap_start_position_type;
		typedef typename superclass::arrow_type					arrow_type;
		typedef typename superclass::score_vector				score_vector;
		typedef typename superclass::score_matrix				score_matrix;
		typedef typename superclass::score_result_type			score_result_type;

		enum find_gap_type : std::uint8_t
		{
			UNSET	= 0x0,
			LEFT	= 0x1,
			UP		= 0x2
		};
		
	protected:
		std::vector <typename t_lhs::const_iterator> m_lhs_iterators;
		std::vector <typename t_rhs::const_iterator> m_rhs_iterators;
		t_lhs const	*m_lhs_text{nullptr};
		t_rhs const	*m_rhs_text{nullptr};
	
	public:
		aligner_impl() = default;
		aligner_impl(
			t_owner &owner,
			t_lhs const &lhs,
			t_rhs const &rhs,
			std::size_t const lhs_blocks,
			std::size_t const rhs_blocks
		):
			aligner_impl_base <t_owner>(owner),
			m_lhs_iterators(lhs_blocks),
			m_rhs_iterators(rhs_blocks),
			m_lhs_text(&lhs),
			m_rhs_text(&rhs)
		{
			m_lhs_iterators[0] = m_lhs_text->begin();
			m_rhs_iterators[0] = m_rhs_text->begin();
		}
		
		void align_block(std::size_t const lhs_block_idx, std::size_t const rhs_block_idx);
		
	protected:
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
			score_type gap_score_lhs,
			score_type gap_score_rhs,
			score_result_type &result					// Out
		) const;
		
		template <bool t_initial, typename t_iterator, typename t_rhs_c>
		inline void calculate_score_and_update_gap_scores(
			std::size_t const row_idx,					// Previous row index.
			std::size_t const column_idx,				// Previous column index.
			t_iterator const lhs_it,
			t_rhs_c const rhs_c,
			score_vector const *src_buffer_ptr,
			score_result_type &result,					// Out
			score_type &gap_score_rhs					// Out
		);
			
		inline void update_lhs_samples(std::size_t const row_idx, std::size_t const block_idx, score_result_type const &result);
		inline void update_rhs_samples(std::size_t const column_idx, std::size_t const block_idx, score_result_type const &result);
		
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
		
		libbio_assert_lte(lhs_idx, lhs_limit);
		auto const count(lhs_limit - lhs_idx);
		libbio_assert_lte(count, column.size());
		libbio_assert_lte(count + lhs_idx, std::distance(src.begin(), src.end())); // count <= std::distance(src.begin() + lhs_idx, src.end())
		
		auto it(src.begin());
		std::copy(it + lhs_idx, it + lhs_limit, column.begin());
	}
	
	
	template <typename t_owner, typename t_lhs, typename t_rhs>
	template <typename t_lhs_c, typename t_rhs_c>
	void aligner_impl <t_owner, t_lhs, t_rhs>::calculate_score(
		score_type const prev_diag_score,
		t_lhs_c const lhs_c,
		t_rhs_c const rhs_c,
		score_type gap_score_lhs,	// Copy.
		score_type gap_score_rhs,	// Copy.
		score_result_type &result
	) const
	{
		auto const identity_score(this->m_parameters->identity_score);
		auto const mismatch_penalty(this->m_parameters->mismatch_penalty);
		auto const gap_penalty(this->m_parameters->gap_penalty);
		auto const gap_start_penalty(this->m_parameters->gap_start_penalty);
		
		// Update the existing gap score s.t. the gap would continue here.
		gap_score_lhs += gap_penalty;
		gap_score_rhs += gap_penalty;
		
		auto const s1(prev_diag_score + (libbio::is_equal(lhs_c, rhs_c) ? identity_score : mismatch_penalty));
		auto const s2(gap_score_lhs);	// Lhs string
		auto const s3(gap_score_rhs);	// Rhs string
		
		// Take gap_start_penalty into account when choosing the maximum element and saving the score.
		// When caching the value, don't add gap_start_penalty.
		auto const scores(libbio::make_array <score_type>(s1, gap_start_penalty + s2, gap_start_penalty + s3));
		result.max_idx = libbio::argmax_element(scores.begin(), scores.end());
		libbio_assert(result.max_idx < scores.size());
		result.score = scores[result.max_idx];
		
		// Check if the target cell should be used as a gap start position.
		result.did_start_gap = gap_start_position_type::GSP_NONE;
		
		if (s1 < s2)
			result.gap_score_lhs = s2 + gap_penalty;
		else
		{
			result.gap_score_lhs = s1 + gap_penalty;
			result.did_start_gap |= gap_start_position_type::GSP_RIGHT;
		}
		
		if (s1 < s3)
			result.gap_score_rhs = s3 + gap_penalty;
		else
		{
			result.gap_score_rhs = s1 + gap_penalty;
			result.did_start_gap |= gap_start_position_type::GSP_DOWN;
		}
	}
		
		
	template <typename t_owner, typename t_lhs, typename t_rhs>
	template <bool t_initial, typename t_iterator, typename t_rhs_c>
	void aligner_impl <t_owner, t_lhs, t_rhs>::calculate_score_and_update_gap_scores(
		std::size_t const row_idx,					// Previous row index.
		std::size_t const column_idx,				// Previous column index.
		t_iterator const lhs_it,
		t_rhs_c const rhs_c,
		score_vector const *src_buffer_ptr,
		score_result_type &result,					// Out
		score_type &gap_score_rhs					// Out
	)
	{
		libbio_assert(lhs_it != m_lhs_text->end());
		auto const lhs_c(*lhs_it);
		auto const prev_diag_score((*src_buffer_ptr)[row_idx]);
		calculate_score(prev_diag_score, lhs_c, rhs_c, this->m_data->gap_scores_lhs[1 + row_idx], gap_score_rhs, result);
		this->did_calculate_score(1 + row_idx, 1 + column_idx, result, t_initial);
		
		// Store the values.
		this->m_data->gap_scores_lhs[1 + row_idx]	= result.gap_score_lhs;
		gap_score_rhs								= result.gap_score_rhs;
	}
	
	
	template <typename t_owner, typename t_lhs, typename t_rhs>
	void aligner_impl <t_owner, t_lhs, t_rhs>::update_lhs_samples(std::size_t const row_idx, std::size_t const block_idx, score_result_type const &result)
	{
		this->m_lhs->score_samples(row_idx, block_idx)		= result.score;													// Vertical
		this->m_lhs->gap_score_samples(row_idx, block_idx)	= result.gap_score_lhs;											// Vertical
		libbio_do_and_assert_eq(this->m_lhs->traceback_samples(row_idx, block_idx).fetch_or(result.max_idx), 0);					// Vertical, values same as arrow_type::*
		libbio_do_and_assert_eq(this->m_lhs->gap_start_position_samples(row_idx, block_idx).fetch_or(result.did_start_gap), 0);	// Vertical
	}
	
	
	template <typename t_owner, typename t_lhs, typename t_rhs>
	void aligner_impl <t_owner, t_lhs, t_rhs>::update_rhs_samples(std::size_t const column_idx, std::size_t const block_idx, score_result_type const &result)
	{
		this->m_rhs->score_samples(column_idx, block_idx)		= result.score;												// Horizontal
		this->m_rhs->gap_score_samples(column_idx, block_idx)	= result.gap_score_rhs;										// Horizontal
		libbio_do_and_assert_eq(this->m_rhs->traceback_samples(column_idx, block_idx).fetch_or(result.max_idx), 0);				// Horizontal, values same as arrow_type::*
		libbio_do_and_assert_eq(this->m_rhs->gap_start_position_samples(column_idx, block_idx).fetch_or(result.did_start_gap), 0);	// Horizontal
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
		auto const lhs_limits(libbio::make_array <std::size_t>(1 + this->m_parameters->lhs_length, lhs_idx + segment_length));
		auto const rhs_limits(libbio::make_array <std::size_t>(1 + this->m_parameters->rhs_length, rhs_idx + segment_length));
		bool const should_calculate_final_row(libbio::argmin_element(lhs_limits.begin(), lhs_limits.end()));
		bool const should_calculate_final_column(libbio::argmin_element(rhs_limits.begin(), rhs_limits.end()));
		auto const lhs_limit(lhs_limits[should_calculate_final_row]);
		auto const rhs_limit(rhs_limits[should_calculate_final_column]);
		libbio_assert(lhs_limit - lhs_idx <= segment_length);
		libbio_assert(rhs_limit - rhs_idx <= segment_length);
		
		// Score buffers.
		auto *src_buffer_ptr(&this->m_data->score_buffer_1);
		auto *dst_buffer_ptr(&this->m_data->score_buffer_2);
		
		// Fill the first column up to what is needed.
		{
			auto const column(this->m_lhs->score_samples.column(rhs_block_idx));
			auto const it(column.begin());
			std::copy(
				it + lhs_idx,
				it + lhs_limit,
				src_buffer_ptr->begin() + lhs_idx
			);
		}
		
		// Fill the part of lhs gap scores. The first row is not needed here but the final row is.
		{
			auto const column(this->m_lhs->gap_score_samples.column(rhs_block_idx));
			auto const it(column.begin());
			std::copy(
				it + lhs_idx + 1,
				it + lhs_limit + should_calculate_final_row,
				this->m_data->gap_scores_lhs.begin() + lhs_idx + 1
			);
		}
		
		// Find the correct text position.
		auto lhs_it(m_lhs_iterators[lhs_block_idx]);
		auto rhs_it(m_rhs_iterators[rhs_block_idx]);
		
		// Fill output_score_buffer if needed.
		if (output_score_buffer)
			copy_to_score_buffer(*src_buffer_ptr, 0, lhs_idx, lhs_limit, *output_score_buffer);
		
		// Fill the block. Initialize the result with the final value of the first column in case
		// it is the only column to fill.
		auto const &topmost_row(this->m_rhs->score_samples.column(lhs_block_idx));			// Horizontal.
		auto const &gap_scores_rhs(this->m_rhs->gap_score_samples.column(lhs_block_idx));	// Horizontal.
		score_result_type result((*src_buffer_ptr)[lhs_limit - 1]);
		auto lhs_it_2(lhs_it); // Copy, needed to store the iterator after the loop.
		for (std::size_t i(rhs_idx); i < rhs_limit - 1; ++i) // Column
		{
			libbio_assert(rhs_it != m_rhs_text->end());
			
			auto const rhs_c(*rhs_it);
			lhs_it_2 = lhs_it;
			score_type gap_score_rhs(gap_scores_rhs[1 + i]);
			
			// Fill the first row, needed for the first value of prev_diag_score on the next iteration.
			(*dst_buffer_ptr)[lhs_idx] = topmost_row[1 + i];
			
			for (std::size_t j(lhs_idx); j < lhs_limit - 1; ++j) // Row
			{
				calculate_score_and_update_gap_scores <t_initial>(j, i, lhs_it_2, rhs_c, src_buffer_ptr, result, gap_score_rhs);
				(*dst_buffer_ptr)[1 + j] = result.score;
				
				// Store the traceback value if needed.
				if constexpr (!t_initial)
				{
					auto const y(1 + j - lhs_idx);
					auto const x(1 + i - rhs_idx);
					libbio_do_and_assert_eq(this->m_data->traceback(y, x).fetch_or(result.max_idx), 0);
					libbio_do_and_assert_eq(this->m_data->gap_start_positions(y, x).fetch_or(result.did_start_gap), 0);
				}

				++lhs_it_2;
			}
			
			// Fill the next sample row if needed.
			if (t_initial && should_calculate_final_row)
			{
				calculate_score_and_update_gap_scores <t_initial>(lhs_limit - 1, i, lhs_it_2, rhs_c, src_buffer_ptr, result, gap_score_rhs);
				update_rhs_samples(1 + i, 1 + lhs_block_idx, result);
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
		
		// Store the left iterator if needed.
		if (t_initial && 0 == rhs_block_idx && should_calculate_final_row)
		{
			++lhs_it_2;
			auto const it_idx(1 + lhs_block_idx);
			libbio_assert(it_idx < m_lhs_iterators.size());
			m_lhs_iterators[it_idx] = lhs_it_2;
		}
		
		// Fill the next sample column and the corner if needed.
		if (t_initial && should_calculate_final_column)
		{
			libbio_assert(rhs_it != m_rhs_text->end());
			auto const column_idx(rhs_limit - 1);
			
			auto const rhs_c(*rhs_it);
			auto lhs_it_2(lhs_it);
			score_type gap_score_rhs(gap_scores_rhs[1 + column_idx]);
			
			for (std::size_t j(lhs_idx); j < lhs_limit - 1; ++j) // Row
			{
				calculate_score_and_update_gap_scores <t_initial>(j, column_idx, lhs_it_2, rhs_c, src_buffer_ptr, result, gap_score_rhs);
				update_lhs_samples(1 + j, 1 + rhs_block_idx, result);
				++lhs_it_2;
			}
			
			// Consider the corner.
			if (should_calculate_final_row)
			{
				auto const row_idx(lhs_limit - 1);
				calculate_score_and_update_gap_scores <t_initial>(row_idx, column_idx, lhs_it_2, rhs_c, src_buffer_ptr, result, gap_score_rhs);
				update_lhs_samples(1 + row_idx, 1 + rhs_block_idx, result);
				update_rhs_samples(1 + column_idx, 1 + lhs_block_idx, result);
			}
			
			// Store the right iterator if needed.
			if (0 == lhs_block_idx)
			{
				++rhs_it;
				auto const it_idx(1 + rhs_block_idx);
				libbio_assert(it_idx < m_rhs_iterators.size());
				m_rhs_iterators[it_idx] = rhs_it;
			}
		}
		
		if constexpr (t_initial)
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
		auto &gap_start_positions(this->m_data->gap_start_positions);
		
		std::size_t lhs_block_idx(this->m_parameters->lhs_segments - 1);
		std::size_t rhs_block_idx(this->m_parameters->rhs_segments - 1);
		libbio::matrix <score_type> score_buffer;
		libbio::matrix <score_type> *score_buffer_ptr(nullptr);
		
		// Scoring matrix indices.
		// lhs_idx and rhs_idx point to the upper left corner of the final block, that is, the bottom-right one.
		auto const lhs_idx(seg_len * lhs_block_idx);
		auto const rhs_idx(seg_len * rhs_block_idx);
		libbio_assert(lhs_idx <= lhs_len);
		libbio_assert(rhs_idx <= rhs_len);
		std::size_t j_limit(libbio::min_ct(seg_len, 1 + lhs_len - lhs_idx)); // (Last) row
		std::size_t i_limit(libbio::min_ct(seg_len, 1 + rhs_len - rhs_idx)); // (Last) column
		std::size_t next_i_limit(i_limit);
		std::size_t next_j_limit(j_limit);
		auto j(j_limit - 1);
		auto i(i_limit - 1);
		auto prev_j(j);
		auto prev_i(i);
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
			std::fill(gap_start_positions.word_begin(), gap_start_positions.word_end(), 0);
			
			// Fill the first rows and columns of the matrices used in traceback.
			auto const lhs_first(seg_len * lhs_block_idx);
			auto const rhs_first(seg_len * rhs_block_idx);
			{
				auto const lhs_limit(libbio::min_ct(1 + lhs_len, seg_len * (1 + lhs_block_idx)));
				auto const rhs_limit(libbio::min_ct(1 + rhs_len, seg_len * (1 + rhs_block_idx)));
			
				libbio::matrices::copy_to_word_aligned(
					this->m_lhs->traceback_samples.column(rhs_block_idx, lhs_first, lhs_limit),
					traceback.column(0)
				);
				libbio::matrices::copy_to_word_aligned(
					this->m_lhs->gap_start_position_samples.column(rhs_block_idx, lhs_first, lhs_limit),
					gap_start_positions.column(0)
				);
				
				libbio::matrices::transpose_column_to_row(this->m_rhs->traceback_samples.column(lhs_block_idx, rhs_first, rhs_limit), traceback.row(0));
				libbio::matrices::transpose_column_to_row(this->m_rhs->gap_start_position_samples.column(lhs_block_idx, rhs_first, rhs_limit), this->m_data->gap_start_positions.row(0));
			}
			
			// The first row and column are now filled, run the filling algorithm.
			std::fill(score_buffer.begin(), score_buffer.end(), 0);
			fill_block <false>(lhs_block_idx, rhs_block_idx, score_buffer_ptr);

			// If this is the last block, check that the corner is marked.
			libbio_assert((! (0 == lhs_block_idx && 0 == rhs_block_idx)) || traceback(0, 0) == arrow_type::ARROW_FINISH);
			
			// Continue finding the gap starting position if needed.
			{
				std::size_t steps(0);
				switch (mode)
				{
					case find_gap_type::LEFT:
					{
						bool const res(this->find_gap_start_x(j, i, steps));
						this->push_lhs(1, steps);
						this->push_rhs(0, steps);
						if (!res)
						{
							libbio_assert(rhs_block_idx);
							--rhs_block_idx;
							i = seg_len - 1;
							goto continue_loop;
						}
						break;
					}
				
					case find_gap_type::UP:
					{
						bool const res(this->find_gap_start_y(j, i, steps));
						this->push_lhs(0, steps);
						this->push_rhs(1, steps);
						if (!res)
						{
							libbio_assert(lhs_block_idx);
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
						this->push_lhs(0, 1);
						this->push_rhs(0, 1);
						
						// If either co-ordinate is zero, move to the adjacent block.
						if (! (i && j))
						{
							if (0 == i)
							{
								libbio_assert(rhs_block_idx);
								--rhs_block_idx;
								i = seg_len - 1;
								next_i_limit = seg_len;
							}
							else
							{
								--i;
							}
							
							if (0 == j)
							{
								libbio_assert(lhs_block_idx);
								--lhs_block_idx;
								j = seg_len - 1;
								next_j_limit = seg_len;
							}
							else
							{
								--j;
							}
							
							goto continue_loop;
						}
					
						libbio_assert(i);
						libbio_assert(j);
						--i;
						--j;
						
						break;
						
					case arrow_type::ARROW_LEFT:
					{
						// Move left as long as possible and to the adjacent block if needed.
						bool const res(this->find_gap_start_x(j, i, steps));
						this->push_lhs(1, steps);
						this->push_rhs(0, steps);
						if (!res)
						{
							libbio_assert(rhs_block_idx);
							--rhs_block_idx;
							i = seg_len - 1;
							next_i_limit = seg_len;
							mode = find_gap_type::LEFT;
							goto continue_loop;
						}
						break;
					}
					
					case arrow_type::ARROW_UP:
					{
						// Move up as long as possible and to the adjacent block if needed.
						bool const res(this->find_gap_start_y(j, i, steps));
						this->push_lhs(0, steps);
						this->push_rhs(1, steps);
						if (!res)
						{
							libbio_assert(lhs_block_idx);
							--lhs_block_idx;
							j = seg_len - 1;
							next_j_limit = seg_len;
							mode = find_gap_type::UP;
							goto continue_loop;
						}
						break;
						
					}
					
					case arrow_type::ARROW_FINISH:
						goto exit_loop;
						
					default:
						libbio_fail("Unexpected traceback value");
				}
			}
			
		continue_loop:
			if (print_debugging_information)
			{
				matrix_printer printer(
					prev_j,
					prev_i,
					j_limit,
					i_limit,
					m_lhs_text->begin(),
					m_rhs_text->begin()
				);
				
				printer.set_lhs_offset(lhs_first - 1);
				printer.set_rhs_offset(rhs_first - 1);
				printer.prepare(traceback);
				printer.print_scores(score_buffer);
				std::cerr << '\n';
				printer.print_traceback(traceback);
				std::cerr << '\n';
			}
			
			j_limit = next_j_limit;
			i_limit = next_i_limit;
			prev_j = j;
			prev_i = i;
		}
		
	exit_loop:
		if (print_debugging_information)
		{
			matrix_printer printer(
				prev_j,
				prev_i,
				j_limit,
				i_limit,
				m_lhs_text->begin(),
				m_rhs_text->begin()
			);

			printer.set_padding(1);
			printer.prepare(traceback);
			printer.print_scores(score_buffer);
			std::cerr << '\n';
			printer.print_traceback(traceback);
			std::cerr << '\n';
		}

		// Reverse the paths.
		this->reverse_gaps();
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

