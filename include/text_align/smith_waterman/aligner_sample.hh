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
		typedef typename t_aligner::score_matrix		score_matrix;
		typedef typename t_aligner::gap_score_gt_matrix	gap_score_gt_matrix;
		typedef typename t_aligner::direction_matrix	direction_matrix;
		typedef aligner_base::bit_vector				bit_vector;
		
		score_matrix		score_samples;			// Sample vectors.
		score_matrix		gap_score_samples;		// Sample vectors for max{}.
		gap_score_gt_matrix	gap_score_gt;			// Sample vectors for max positions.
		direction_matrix	initial_traceback;		// Direction sample vectors.
		bit_vector			gaps;
	};
	

	template <typename t_score>
	struct aligner_parameters
	{
		t_score			identity_score{2};
		t_score			mismatch_penalty{-1};
		t_score			gap_start_penalty{-10};
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
		typedef typename t_aligner::flag_matrix			flag_matrix;
		typedef typename t_aligner::score_vector		score_vector;
		typedef typename t_aligner::traceback_matrix	traceback_matrix;
		typedef typename t_aligner::gap_score_gt_matrix	gap_score_gt_matrix;

		flag_matrix			flags;
		score_vector		score_buffer_1;			// Source score buffer.
		score_vector		score_buffer_2;			// Destination score buffer.
		score_vector		gap_scores_lhs;			// Buffer for lhs gap scores.
		
		traceback_matrix	traceback;
		gap_score_gt_matrix	gap_score_gt;			// For finding the gap start in case gap was considered for the position.
	};
}}}

#endif
