/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef TEXT_ALIGN_CAST_BIT_VECTOR_HH
#define TEXT_ALIGN_CAST_BIT_VECTOR_HH

#include <text_align/bit_vector_interface.hh>
#include <text_align/rle_bit_vector.hh>


namespace text_align {
	
	// Cython lacks some useful features, such as allowing both explicit and deduced template parameters.
	// Furthermore, the argument is __Pyx_FakeReference <...> instead of bit_vector_interface const &.
	// Fortunately, the class has a pointer-returning operator &.
	template <typename t_word>
	struct cast
	{
		template <typename t_bv>
		inline rle_bit_vector <t_word> const &to_rle_bit_vector_fr(t_bv &bv)
		{
			return dynamic_cast <rle_bit_vector <t_word> const &>(*(bv.operator &()));
		}
	};
}

#endif
