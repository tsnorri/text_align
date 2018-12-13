/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef TEXT_ALIGN_SMITH_WATERMAN_ALIGNER_SAMPLE_HH
#define TEXT_ALIGN_SMITH_WATERMAN_ALIGNER_SAMPLE_HH

#include <text_align/packed_matrix.hh>
#include <text_align/smith_waterman/aligner_base.hh>
#include <vector>


namespace text_align { namespace smith_waterman { namespace detail {
	
	template <typename t_aligner>
	struct aligner_sample
	{
		typedef typename t_aligner::score_type					score_type;
		typedef typename t_aligner::arrow_type					arrow_type;
		typedef typename t_aligner::gap_start_position_type		gap_start_position_type;
		
		typedef typename t_aligner::score_matrix				score_matrix;
		typedef typename t_aligner::traceback_matrix			traceback_matrix;
		typedef typename t_aligner::gap_start_position_matrix	gap_start_position_matrix;
		typedef aligner_base::bit_vector						bit_vector;
		
		score_matrix				score_samples;				// Sample vectors.
		score_matrix				gap_score_samples;			// Sample vectors for gap start position scores.
		traceback_matrix			traceback_samples;			// Direction sample vectors.
		gap_start_position_matrix	gap_start_position_samples;	// Sample vectors for gap start positions.
		bit_vector					gaps;
		
		void init(
			std::size_t const input_length,
			std::size_t const segments_along_axis,
			arrow_type const arrow,
			gap_start_position_type const gap_start_position,
			score_type const gap_penalty,
			score_type const gap_start_penalty
		);
		
	protected:
		void fill_gap_scores(
			typename score_matrix::slice_type &slice,
			score_type const gap_penalty,
			score_type const gap_start_penalty
		) const;
		void fill_gap_scores(
			typename score_matrix::slice_type &&slice,
			score_type const gap_penalty,
			score_type const gap_start_penalty
		) const;
	};
	

	template <typename t_score>
	struct aligner_parameters
	{
		t_score			identity_score{2};
		t_score			mismatch_penalty{-2};
		t_score			gap_start_penalty{-3};
		t_score			gap_penalty{-1};
		
		std::size_t		lhs_length{0};
		std::size_t		rhs_length{0};
		std::size_t		lhs_segments{0};
		std::size_t		rhs_segments{0};
		std::uint32_t	segment_length{0};
		bool			print_debugging_information{false};
	};
	
	
	template <typename t_aligner>
	struct aligner_data
	{
		typedef typename t_aligner::flag_matrix					flag_matrix;
		typedef typename t_aligner::score_vector				score_vector;
		typedef typename t_aligner::traceback_matrix			traceback_matrix;
		typedef typename t_aligner::gap_start_position_matrix	gap_start_position_matrix;

		flag_matrix					flags;
		score_vector				score_buffer_1;			// Source score buffer.
		score_vector				score_buffer_2;			// Destination score buffer.
		score_vector				gap_scores_lhs;			// Buffer for lhs gap scores.
		
		traceback_matrix			traceback;
		gap_start_position_matrix	gap_start_positions;	// For finding the gap start in case gap was considered for the position.
		
		void init(
			std::size_t const lhs_len,
			std::size_t const segment_len,
			std::size_t const segments_along_y,
			std::size_t const segments_along_x
		);
	};
	
	
	template <typename t_aligner>
	void aligner_sample <t_aligner>::fill_gap_scores(
		typename score_matrix::slice_type &slice,
		score_type const gap_penalty,
		score_type const gap_start_penalty
	) const
	{
		std::size_t idx(0);
		for (auto &val : slice)
		{
			val = idx * gap_penalty + gap_start_penalty;
			++idx;
		}
		slice[0] = 0;
	}
	
	
	template <typename t_aligner>
	void aligner_sample <t_aligner>::fill_gap_scores(
		typename score_matrix::slice_type &&slice,
		score_type const gap_penalty,
		score_type const gap_start_penalty
	) const
	{
		fill_gap_scores(slice, gap_penalty, gap_start_penalty);
	}
	
	
	template <typename t_aligner>
	void aligner_sample <t_aligner>::init(
		std::size_t const input_length,
		std::size_t const segments_along_axis,
		arrow_type const arrow,
		gap_start_position_type const gap_start_position,
		score_type const gap_penalty,
		score_type const gap_start_penalty
	)
	{
		// Clear the state.
		gaps.clear();
		
		// Reserve memory for the score samples and the scores.
		score_samples.resize(1 + input_length, 1 + segments_along_axis);
		gap_score_samples.resize(1 + input_length, 1 + segments_along_axis);
		
		std::fill(score_samples.begin(), score_samples.end(), 0);
		std::fill(gap_score_samples.begin(), gap_score_samples.end(), 0);
		
		// Fill the first vectors with gap scores.
		fill_gap_scores(score_samples.column(0), gap_penalty, gap_start_penalty);
		fill_gap_scores(gap_score_samples.column(0), gap_penalty, gap_start_penalty);
		
		// Initialize the direction samples.
		{
			matrices::initialize_atomic(traceback_samples, 1 + input_length, 1 + segments_along_axis);
			std::fill(traceback_samples.word_begin(), traceback_samples.word_end(), 0);
			
			// Fill the first vectors with arrows.
			auto column(traceback_samples.column(0));
			matrices::fill_column_with_bit_pattern <2>(column, arrow);
			
			// Add ARROW_FINISH to the corner, make sure that it does not change the previous value.
			assert(arrow_type::ARROW_FINISH == (arrow_type::ARROW_FINISH | (column[0] & arrow_type::ARROW_MASK)));
			column[0].fetch_or(arrow_type::ARROW_FINISH);
		}
		
		// Initialize the gap start position samples.
		{
			matrices::initialize_atomic(gap_start_position_samples, 1 + input_length, 1 + segments_along_axis);
			std::fill(gap_start_position_samples.word_begin(), gap_start_position_samples.word_end(), 0);
			
			// Fill the first vector.
			auto column(gap_start_position_samples.column(0));
			matrices::fill_column_with_bit_pattern <2>(column, gap_start_position);
			
			// Add GSGT_BOTH to the corner, make sure that it does not change the previous value.
			assert(gap_start_position_type::GSP_BOTH == (gap_start_position_type::GSP_BOTH | (column[0] & gap_start_position_type::GSP_MASK)));
			column[0].fetch_or(gap_start_position_type::GSP_BOTH);
		}
	}
	
	
	template <typename t_aligner>
	void aligner_data <t_aligner>::init(
		std::size_t const lhs_len,
		std::size_t const segment_len,
		std::size_t const segments_along_y,
		std::size_t const segments_along_x
	)
	{
		// Initialize the flags.
		{
			matrices::initialize_atomic(flags, segments_along_y, segments_along_x);
			
			// Set the value of each flag.
			std::fill(flags.word_begin(), flags.word_end(), 0);
			
			// Set the first row and column to one so that the blocks to the right and below the initial
			// block may be filled.
			auto column(flags.column(0));
			auto row(flags.row(0));
			std::for_each(column.begin(), column.end(),	[](auto ref){ ref.fetch_or(0x1); });	// ref will be a reference proxy.
			std::for_each(row.begin(), row.end(),		[](auto ref){ ref.fetch_or(0x1); });
		}
		
		resize_and_zero(score_buffer_1, 1 + lhs_len);	// Vertical.
		resize_and_zero(score_buffer_2, 1 + lhs_len);	// Vertical.
		resize_and_zero(gap_scores_lhs, 1 + lhs_len);	// Vertical.
		
		matrices::initialize_atomic(traceback, segment_len, segment_len);
		std::fill(traceback.word_begin(), traceback.word_end(), 0);
		
		matrices::initialize_atomic(gap_start_positions, segment_len, segment_len);
		std::fill(gap_start_positions.word_begin(), gap_start_positions.word_end(), 0);
	}
}}}

#endif
