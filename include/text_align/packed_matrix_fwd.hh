/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef TEXT_ALIGN_PACKED_MATRIX_FWD_HH
#define TEXT_ALIGN_PACKED_MATRIX_FWD_HH

#include <cstdint>


namespace text_align {
	
	template <std::uint8_t t_bits, typename t_word = std::uint64_t>
	class packed_matrix;
}

#endif