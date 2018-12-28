/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef TEXT_ALIGN_SMITH_WATERMAN_ALIGNER_DATA_HH
#define TEXT_ALIGN_SMITH_WATERMAN_ALIGNER_DATA_HH

#include <libbio/packed_matrix.hh>


namespace text_align { namespace smith_waterman { namespace detail {
	
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
	void aligner_data <t_aligner>::init(
		std::size_t const lhs_len,
		std::size_t const segment_len,
		std::size_t const segments_along_y,
		std::size_t const segments_along_x
	)
	{
		// Initialize the flags.
		{
			libbio::matrices::initialize_atomic(flags, segments_along_y, segments_along_x);
			
			// Set the value of each flag.
			std::fill(flags.word_begin(), flags.word_end(), 0);
			
			// Set the first row and column to one so that the blocks to the right and below the initial
			// block may be filled.
			auto column(flags.column(0));
			auto row(flags.row(0));
			std::for_each(column.begin(), column.end(),	[](auto ref){ ref.fetch_or(0x1); });	// ref will be a reference proxy.
			std::for_each(row.begin(), row.end(),		[](auto ref){ ref.fetch_or(0x1); });
		}
		
		libbio::resize_and_zero(score_buffer_1, 1 + lhs_len);	// Vertical.
		libbio::resize_and_zero(score_buffer_2, 1 + lhs_len);	// Vertical.
		libbio::resize_and_zero(gap_scores_lhs, 1 + lhs_len);	// Vertical.
		
		libbio::matrices::initialize_atomic(traceback, segment_len, segment_len);
		std::fill(traceback.word_begin(), traceback.word_end(), 0);
		
		libbio::matrices::initialize_atomic(gap_start_positions, segment_len, segment_len);
		std::fill(gap_start_positions.word_begin(), gap_start_positions.word_end(), 0);
	}
}}}

#endif
