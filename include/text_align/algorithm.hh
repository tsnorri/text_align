/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef TEXT_ALIGN_ALGORITHM_HH
#define TEXT_ALIGN_ALGORITHM_HH

#define BOOST_DYNAMIC_BITSET_DONT_USE_FRIENDS 1	// XXX Hack: the bitset does not provide access to the blocks.
#include <boost/dynamic_bitset.hpp>

#include <iostream>
#include <text_align/assert.hh>
#include <type_traits>


namespace text_align { namespace detail {
	
	template <
		typename t_lhs,
		typename t_rhs,
		bool t_lhs_is_signed = std::is_signed_v <t_lhs>,
		bool t_rhs_is_signed = std::is_signed_v <t_rhs>
	>
	struct is_equal {};
	
	// Both lhs and rhs are signed.
	template <typename t_lhs, typename t_rhs>
	struct is_equal <t_lhs, t_rhs, true, true>
	{
		static constexpr bool test(t_lhs lhs, t_rhs rhs) { return lhs == rhs; }
	};
	
	// Both lhs and rhs are unsigned.
	template <typename t_lhs, typename t_rhs>
	struct is_equal <t_lhs, t_rhs, false, false>
	{
		static constexpr bool test(t_lhs lhs, t_rhs rhs) { return lhs == rhs; }
	};
	
	// lhs is signed, rhs is unsigned.
	template <typename t_lhs, typename t_rhs>
	struct is_equal <t_lhs, t_rhs, true, false>
	{
		static constexpr bool test(t_lhs lhs, t_rhs rhs)
		{
			if (lhs < 0)
				return false;
			
			// Copy the values to the common type and compare.
			typedef std::common_type_t <t_lhs, t_rhs> ct;
			ct lhss(lhs);
			ct rhss(rhs);
			
			return lhss == rhss;
		}
	};
	
	// lhs is unsigned, rhs is signed.
	template <typename t_lhs, typename t_rhs>
	struct is_equal <t_lhs, t_rhs, false, true>
	{
		static constexpr bool test(t_lhs lhs, t_rhs rhs)
		{
			return is_equal <t_rhs, t_lhs>::test(rhs, lhs);
		}
	};
}}


namespace text_align {
	
	template <unsigned int t_bits, typename t_word>
	inline constexpr t_word reverse_bits(t_word const rr)
	{
		enum { WORD_BITS = sizeof(t_word) * CHAR_BIT };
		static_assert(1 == t_bits || 2 == t_bits || 4 == t_bits || 8 == t_bits || 16 == t_bits || 32 == t_bits || 64 == t_bits);
		static_assert(t_bits <= WORD_BITS);
		
		std::uint64_t r(rr);
		
		// Swap n-tuples of bits in order and return early if t_word size has been reached.
		if constexpr (t_bits <= 1)
			r = ((r & 0x5555555555555555UL) <<  1) | ((r & 0xaaaaaaaaaaaaaaaaUL) >> 1);		// Swap adjacent bits.
		
		if constexpr (t_bits <= 2)
			r = ((r & 0x3333333333333333UL) <<  2) | ((r & 0xccccccccccccccccUL) >> 2);		// Swap adjacent pairs.
		
		if constexpr (t_bits <= 4)
			r = ((r & 0x0f0f0f0f0f0f0f0fUL) <<  4) | ((r & 0xf0f0f0f0f0f0f0f0UL) >> 4);		// Swap adjacent 4's.
		
		if constexpr (8 == WORD_BITS) return r;
		
		if constexpr (t_bits <= 8)
			r = ((r & 0x00ff00ff00ff00ffUL) <<  8) | ((r & 0xff00ff00ff00ff00UL) >> 8);		// Swap adjacent 8's.
			
		if constexpr (16 == WORD_BITS) return r;
		
		if constexpr (t_bits <= 16)
			r = ((r & 0x0000ffff0000ffffUL) << 16) | ((r & 0xffff0000ffff0000UL) >> 16);	// Swap adjacent 16's.
		
		if constexpr (32 == WORD_BITS) return r;
		
		if constexpr (t_bits <= 32)
			r = ((r & 0x00000000ffffffffUL) << 32) | ((r & 0xffffffff00000000UL) >> 32);	// Swap adjacent 32's.
		
		return r;
	}
	
	
	template <typename t_word>
	inline void reverse_bitset(boost::dynamic_bitset <t_word> &bs)
	{
		// Reverse the order of the blocks.
		std::reverse(bs.m_bits.begin(), bs.m_bits.end());

		// Reverse the contents of each block.
		for (auto &word : bs.m_bits)
		{
			std::uint64_t const reversed(reverse_bits <1>(word));
			word = reversed;
		}

		// Shift by the amount of tailing zeros in the original last block.
		// If there is only one block, shift just that block since dynamic_bitset
		// would like to reset itself.
		auto const size(bs.size());
		auto const shift(64 - size % 64);
		if (1 == bs.m_bits.size())
			bs.m_bits.front() >>= shift;
		else
			bs >>= shift;
	}
	
	
	template <typename t_enum>
	constexpr std::underlying_type_t <t_enum> to_underlying(t_enum const en)
	{
		return static_cast <std::underlying_type_t <t_enum>>(en);
	}
	
	
	// Safely compare signed and unsigned.
	template <typename t_lhs, typename t_rhs>
	constexpr bool is_equal(t_lhs lhs, t_rhs rhs)
	{
		static_assert(std::is_integral_v <t_lhs>, "Expected t_lhs to be integral");
		static_assert(std::is_integral_v <t_rhs>, "Expected t_rhs to be integral");
		return detail::is_equal <t_lhs, t_rhs>::test(lhs, rhs);
	}
	
	
	template <typename t_it>
	constexpr std::size_t argmax_element(t_it begin, t_it end)
	{
		return std::distance(begin, std::max_element(begin, end));
	}
	
	
	template <typename t_it>
	constexpr std::size_t argmin_element(t_it begin, t_it end)
	{
		return std::distance(begin, std::min_element(begin, end));
	}
	
	
	// Return the indices of the maximum elements as a bit mask.
	template <typename t_it>
	constexpr std::uint64_t argmax_elements(t_it it, t_it end)
	{
		typedef typename std::iterator_traits <t_it>::value_type value_type;
		auto max_val(std::numeric_limits <value_type>::min());

		std::uint64_t mask{0x1};
		std::uint64_t retval{};
		
		text_align_assert(std::distance(it, end) <= 64);
		while (it != end)
		{
			auto val(*it);
			if (max_val == val)
				retval |= mask;
			else if (max_val < val)
			{
				retval = mask;
				max_val = val;
			}
			
			mask <<= 1;
		}
		
		return retval;
	}
	
	
	// Simpler variant of std::experimental::make_array.
	template <typename t_value, typename ... t_args>
	constexpr auto make_array(t_args && ... args) -> std::array <t_value, sizeof...(t_args)>
	{
		return {std::forward <t_args>(args)...};
	}
	
	
	template <typename t_coll>
	void resize_and_fill_each(t_coll &vec_collection, std::size_t const size)
	{
		for (auto &vec : vec_collection)
		{
			vec.resize(size);
			std::fill(vec.begin(), vec.end(), 0);
		}
	}
	
	
	template <typename t_vec>
	void resize_and_zero(t_vec &vec, std::size_t const size)
	{
		vec.resize(size);
		std::fill(vec.begin(), vec.end(), 0);
	}
	
	
	template <typename t_lhs, typename t_rhs>
	constexpr auto min(t_lhs const &lhs, t_rhs const &rhs) -> std::common_type_t <t_lhs, t_rhs>
	{
		return std::min <std::common_type_t <t_lhs, t_rhs>>(lhs, rhs);
	}
	
	
	template <typename t_block>
	void append_bits(boost::dynamic_bitset <t_block> &dst, std::size_t const bit_count, bool const val)
	{
		auto const block_bits(sizeof(t_block) * CHAR_BIT);
		
		// Fill with full blocks.
		{
			t_block block(0);
			if (val)
				--block;
			
			for (std::size_t i(0), count(bit_count / block_bits); i < count; ++i)
				dst.append(block);
		}
		
		// Add the remaining block.
		auto const remaining_count(bit_count % block_bits);
		text_align_assert(remaining_count < block_bits);
		if (remaining_count)
		{
			t_block block(0);
			if (val)
			{
				block = 1;
				block <<= remaining_count;
				--block;
			}
			
			dst.append(block);
			
			// Remove the extra bits.
			auto const extra_count(block_bits - remaining_count);
			auto const actual_size(dst.size() - extra_count);
			dst.resize(actual_size);
		}
	}
}

#endif
