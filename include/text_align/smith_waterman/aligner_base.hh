/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef TEXT_ALIGN_SMITH_WATERMAN_ALIGNER_BASE_HH
#define TEXT_ALIGN_SMITH_WATERMAN_ALIGNER_BASE_HH

#include <text_align/algorithm.hh>

#include <text_align/matrix.hh>


namespace text_align { namespace smith_waterman {
	
	struct aligner_base
	{
	public:
		typedef boost::dynamic_bitset <std::uint64_t>	bit_vector;
		
		enum arrow_type : std::uint8_t
		{
			ARROW_DIAGONAL	= 0x0,
			ARROW_LEFT		= 0x1,
			ARROW_UP		= 0x2,
			ARROW_FINISH	= 0x3,
			ARROW_MASK		= 0x3
		};
		
		enum gap_start_position_type : std::uint8_t
		{
			GSP_NONE		= 0x0,
			GSP_RIGHT		= 0x1,
			GSP_DOWN		= 0x2,
			GSP_BOTH		= 0x3,
			GSP_MASK		= 0x3
		};
		
		virtual ~aligner_base() {}
		virtual void set_segment_length(std::uint32_t const length) = 0;
		virtual void set_prints_debugging_information(bool const should_print) = 0;
	};
	
	inline aligner_base::gap_start_position_type operator|(aligner_base::gap_start_position_type lhs, aligner_base::gap_start_position_type rhs)
	{
		return static_cast <aligner_base::gap_start_position_type>(to_underlying(lhs) | to_underlying(rhs));
	}
	
	inline aligner_base::gap_start_position_type &operator|=(aligner_base::gap_start_position_type &lhs, aligner_base::gap_start_position_type rhs)
	{
		return lhs = (lhs | rhs);
	}
}}

#endif
