/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef TEXT_ALIGN_SMITH_WATERMAN_ALIGNER_PARAMETERS_HH
#define TEXT_ALIGN_SMITH_WATERMAN_ALIGNER_PARAMETERS_HH

#include <cstdint>


namespace text_align { namespace smith_waterman { namespace detail {
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
}}}

#endif
