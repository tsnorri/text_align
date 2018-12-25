/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef TEXT_ALIGN_BIT_VECTOR_INTERFACE_HH
#define TEXT_ALIGN_BIT_VECTOR_INTERFACE_HH

#include <cstddef>


namespace text_align {
	
	// Virtualize member functions of a bit vector that uses a given word type.
	template <typename t_word>
	struct bit_vector_interface
	{
		virtual ~bit_vector_interface() {}
		virtual void clear() = 0;
		virtual void push_back(t_word mask, std::size_t count) = 0;
		virtual void reverse() = 0;
	};
	
	
	// t_bit_vector needs to be public in order for dynamic_cast <t_bit_vector> to work.
	template <typename t_word, typename t_bit_vector>
	struct bit_vector_wrapper final : public bit_vector_interface <t_word>, public t_bit_vector
	{
		void clear() override { t_bit_vector::clear(); }
		void reverse() override { t_bit_vector::reverse(); }
		void push_back(t_word mask, std::size_t count) override { t_bit_vector::push_back(mask, count); }
	};
}

#endif
