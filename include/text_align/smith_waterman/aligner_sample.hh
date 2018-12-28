/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef TEXT_ALIGN_SMITH_WATERMAN_ALIGNER_SAMPLE_HH
#define TEXT_ALIGN_SMITH_WATERMAN_ALIGNER_SAMPLE_HH

#include <libbio/packed_matrix.hh>


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
		
		score_matrix				score_samples;				// Sample vectors.
		score_matrix				gap_score_samples;			// Sample vectors for gap start position scores.
		traceback_matrix			traceback_samples;			// Direction sample vectors.
		gap_start_position_matrix	gap_start_position_samples;	// Sample vectors for gap start positions.
		
		void init(
			std::size_t const input_length,
			std::size_t const segments_along_axis,
			arrow_type const arrow,
			gap_start_position_type const gap_start_position,
			score_type const gap_penalty,
			score_type const gap_start_penalty
		);
		
		void copy_first_sample_values(
			aligner_sample const &src,
			std::size_t const segment_length,
			std::size_t const segment_count
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
		// Reserve memory for the score samples and the scores.
		score_samples.resize(1 + input_length, 1 + segments_along_axis);
		gap_score_samples.resize(1 + input_length, 1 + segments_along_axis);
		
		std::fill(score_samples.begin(), score_samples.end(), 0);
		std::fill(gap_score_samples.begin(), gap_score_samples.end(), 0);
		
		// Fill the first vectors with gap scores.
		fill_gap_scores(score_samples.column(0), gap_penalty, gap_start_penalty);
		fill_gap_scores(gap_score_samples.column(0), gap_penalty, gap_start_penalty);
		
		// Initialize the traceback samples.
		{
			libbio::matrices::initialize_atomic(traceback_samples, 1 + input_length, 1 + segments_along_axis);
			std::fill(traceback_samples.word_begin(), traceback_samples.word_end(), 0);
			
			// Fill the first vectors with arrows.
			auto column(traceback_samples.column(0));
			libbio::matrices::fill_column_with_bit_pattern <2>(column, arrow);
			
			// Add ARROW_FINISH to the corner, make sure that it does not change the previous value.
			libbio_assert(arrow_type::ARROW_FINISH == (arrow_type::ARROW_FINISH | (column[0] & arrow_type::ARROW_MASK)));
			column[0].fetch_or(arrow_type::ARROW_FINISH);
		}
		
		// Initialize the gap start position samples.
		{
			libbio::matrices::initialize_atomic(gap_start_position_samples, 1 + input_length, 1 + segments_along_axis);
			std::fill(gap_start_position_samples.word_begin(), gap_start_position_samples.word_end(), 0);
			
			// Fill the first vector.
			auto column(gap_start_position_samples.column(0));
			libbio::matrices::fill_column_with_bit_pattern <2>(column, gap_start_position);
			
			// Add GSGT_BOTH to the corner, make sure that it does not change the previous value.
			libbio_assert(gap_start_position_type::GSP_BOTH == (gap_start_position_type::GSP_BOTH | (column[0] & gap_start_position_type::GSP_MASK)));
			column[0].fetch_or(gap_start_position_type::GSP_BOTH);
		}
	}
	
	
	template <typename t_aligner>
	void aligner_sample <t_aligner>::copy_first_sample_values(
		aligner_sample <t_aligner> const &src,
		std::size_t const segment_length,
		std::size_t const segment_count
	)
	{
		auto const first_score_sample(src.score_samples.column(0));
		auto const first_gap_score_sample(src.gap_score_samples.column(0));
		for (std::size_t i(1); i < segment_count; ++i)
		{
			// Currently this function should only be called when these values have not been set.
			libbio_assert(0 == score_samples(0, i));
			libbio_assert(0 == gap_score_samples(0, i));
			
			score_samples(0, i)		= first_score_sample[i * segment_length];
			gap_score_samples(0, i)	= first_gap_score_sample[i * segment_length];
		}
	}
}}}

#endif
