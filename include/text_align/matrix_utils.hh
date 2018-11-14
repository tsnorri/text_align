/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef TEXT_ALIGN_MATRIX_UTILS_HH
#define TEXT_ALIGN_MATRIX_UTILS_HH

#include <boost/range/algorithm/fill.hpp>
#include <boost/range/algorithm/for_each.hpp>
#include <text_align/packed_matrix_fwd.hh>


namespace text_align { namespace matrices {
	
#if 0
	template <typename t_matrix> typename t_matrix::slice_type row(t_matrix &matrix, std::size_t const row, std::size_t const first, std::size_t const limit);
	template <typename t_matrix> typename t_matrix::slice_type column(t_matrix &matrix, std::size_t const column, std::size_t const first, std::size_t const limit);
	template <typename t_matrix> typename t_matrix::const_slice_type const_row(t_matrix const &matrix, std::size_t const row, std::size_t const first, std::size_t const limit);
	template <typename t_matrix> typename t_matrix::const_slice_type const_column(t_matrix const &matrix, std::size_t const column, std::size_t const first, std::size_t const limit);
#endif
	
	
	template <typename t_matrix>
	typename t_matrix::slice_type row(t_matrix &matrix, std::size_t const row, std::size_t const first, std::size_t const limit)
	{
		std::slice const slice(matrix.idx(row, first), limit - first, matrix.stride());
		return typename t_matrix::slice_type(matrix, slice);
	}
	
	
	template <typename t_matrix>
	typename t_matrix::slice_type column(t_matrix &matrix, std::size_t const column, std::size_t const first, std::size_t const limit)
	{
		std::slice const slice(matrix.idx(first, column), limit - first, 1);
		return typename t_matrix::slice_type(matrix, slice);
	}
	
	
	template <typename t_matrix>
	typename t_matrix::const_slice_type const_row(t_matrix const &matrix, std::size_t const row, std::size_t const first, std::size_t const limit)
	{
		std::slice const slice(matrix.idx(row, first), limit - first, matrix.stride());
		return typename t_matrix::const_slice_type(matrix, slice);
	}
	
	
	template <typename t_matrix>
	typename t_matrix::const_slice_type const_column(t_matrix const &matrix, std::size_t const column, std::size_t const first, std::size_t const limit)
	{
		std::slice const slice(matrix.idx(first, column), limit - first, 1);
		return typename t_matrix::const_slice_type(matrix, slice);
	}
	
	
	template <typename t_matrix>
	void initialize_atomic(t_matrix &matrix, std::size_t const rows, std::size_t const cols)
	{
		using std::swap;
		
		auto const number_of_rows(matrix.number_of_rows());
		auto const number_of_columns(matrix.number_of_columns());
		if (number_of_rows < rows || number_of_columns < cols)
		{
			if (matrix.available_size() < rows * cols)
			{
				t_matrix temp(rows, cols);
				swap(matrix, temp);
			}
			else
			{
				matrix.set_size(rows * cols);
				matrix.set_stride(rows);
			}
		}
	}
	
	
	template <std::uint8_t t_src_bits, typename t_src_word, std::uint8_t t_dst_bits, typename t_dst_word>
	void transpose_column_to_row(
		detail::matrix_slice <packed_matrix <t_src_bits, t_src_word>> const &src,
		detail::matrix_slice <packed_matrix <t_dst_bits, t_dst_word>> &&dst
	)
	{
		transpose_column_to_row(src, dst);
	}
	
	
	template <std::uint8_t t_src_bits, typename t_src_word, std::uint8_t t_dst_bits, typename t_dst_word>
	void transpose_column_to_row(
		detail::matrix_slice <packed_matrix <t_dst_bits, t_dst_word>> const &src,
		detail::matrix_slice <packed_matrix <t_src_bits, t_src_word>> &dst
	)
	{
		typedef typename decltype(src)::matrix_type src_matrix_type;
		
		static_assert(t_src_bits <= t_dst_bits);
		t_src_word const mask(src_matrix_type::SUBELEMENT_MASK);
		
		auto const length(std::min(src.size(), dst.size()));
		auto src_it(src.cbegin());
		t_src_word val(0);
		for (std::size_t i(0); i < length; ++i)
		{
			if (0 == i % (src_matrix_type::WORD_BITS / t_src_bits))
				val = *src_it++;
			
			dst[i].fetch_or(val & mask);
			val >>= t_src_bits;
		}
	}
	
	
	template <std::uint8_t t_bits, typename t_slice>
	void fill_column_with_bit_pattern(t_slice &column, typename t_slice::matrix_type::word_type pattern)
	{
		// Fill one word at a time.
		static_assert(2 == t_bits || 4 == t_bits || 8 == t_bits || 16 == t_bits || 32 == t_bits || 64 == t_bits);
		switch (t_bits)
		{
			case 2:
				pattern |= pattern << 2;
				
			case 4:
				pattern |= pattern << 4;
				
			case 8:
				if (8 == t_slice::WORD_BITS)
					goto do_fill;
				pattern |= pattern << 8;
			
			case 16:
				if (16 == t_slice::WORD_BITS)
					goto do_fill;
				pattern |= pattern << 16;
				
			case 32:
				if (32 == t_slice::WORD_BITS)
					goto do_fill;
				pattern |= pattern << 32;
				break;
		}
		
	do_fill:
		auto word_range(column.word_range());
		boost::range::fill(word_range.mid, pattern);
		boost::range::for_each(word_range.left_extent,	[pattern](auto &ref){ ref.fetch_or(pattern); });
		boost::range::for_each(word_range.right_extent,	[pattern](auto &ref){ ref.fetch_or(pattern); });
	}
}}

#endif
