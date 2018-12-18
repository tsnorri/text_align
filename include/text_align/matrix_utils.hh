/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef TEXT_ALIGN_MATRIX_UTILS_HH
#define TEXT_ALIGN_MATRIX_UTILS_HH

#include <boost/range/algorithm/fill.hpp>
#include <boost/range/algorithm/for_each.hpp>
#include <text_align/packed_matrix_fwd.hh>


namespace text_align { namespace matrices { namespace detail {
	
	template <unsigned int t_length_diff>
	struct fill_bit_pattern_helper
	{
		template <unsigned int t_pattern_length, typename t_word>
		static constexpr t_word fill_bit_pattern(t_word pattern)
		{
			pattern |= pattern << t_pattern_length;
			
			typedef fill_bit_pattern_helper <CHAR_BIT * sizeof(t_word) - 2 * t_pattern_length> helper;
			return helper::template fill_bit_pattern <2 * t_pattern_length>(pattern);
		}
	};
	
	template <>
	struct fill_bit_pattern_helper <0>
	{
		template <unsigned int t_pattern_length, typename t_word>
		static constexpr t_word fill_bit_pattern(t_word pattern)
		{
			return pattern;
		}
	};
	
	template <unsigned int t_pattern_length, typename t_word>
	constexpr t_word fill_bit_pattern(t_word pattern)
	{
		typedef fill_bit_pattern_helper <CHAR_BIT * sizeof(t_word) - t_pattern_length> helper;
		return helper::template fill_bit_pattern <t_pattern_length>(pattern);
	}
}}}


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
		text_align_assert(limit <= matrix.number_of_columns());
		std::slice const slice(matrix.idx(row, first), limit - first, matrix.stride());
		return typename t_matrix::slice_type(matrix, slice);
	}
	
	
	template <typename t_matrix>
	typename t_matrix::slice_type column(t_matrix &matrix, std::size_t const column, std::size_t const first, std::size_t const limit)
	{
		text_align_assert(limit <= matrix.number_of_rows());
		std::slice const slice(matrix.idx(first, column), limit - first, 1);
		return typename t_matrix::slice_type(matrix, slice);
	}
	
	
	template <typename t_matrix>
	typename t_matrix::const_slice_type const_row(t_matrix const &matrix, std::size_t const row, std::size_t const first, std::size_t const limit)
	{
		text_align_assert(limit <= matrix.number_of_columns());
		std::slice const slice(matrix.idx(row, first), limit - first, matrix.stride());
		return typename t_matrix::const_slice_type(matrix, slice);
	}
	
	
	template <typename t_matrix>
	typename t_matrix::const_slice_type const_column(t_matrix const &matrix, std::size_t const column, std::size_t const first, std::size_t const limit)
	{
		text_align_assert(limit <= matrix.number_of_rows());
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
	
	
	template <typename t_matrix>
	void copy_to_word_aligned(
		::text_align::detail::packed_matrix_slice <t_matrix> const &src,
		::text_align::detail::packed_matrix_slice <t_matrix> &dst
	)
	{
		text_align_assert(src.size() <= dst.size());
		text_align_always_assert(dst.is_word_aligned());
		auto dst_it(dst.word_begin());
		src.to_word_range().apply_aligned([&dst_it](auto word, std::size_t element_count){
			dst_it->store(word);
			++dst_it;
		});
	}
	
	
	template <typename t_matrix>
	void copy_to_word_aligned(
		::text_align::detail::packed_matrix_slice <t_matrix> const &src,
		::text_align::detail::packed_matrix_slice <t_matrix> &&dst	// Proxy, allow moving.
	)
	{
		copy_to_word_aligned(src, dst);
	}
	
	
	template <typename t_src_matrix, typename t_dst_matrix>
	void transpose_column_to_row(
		::text_align::detail::packed_matrix_slice <t_src_matrix> const &src,
		::text_align::detail::packed_matrix_slice <t_dst_matrix> &dst
	)
	{
		text_align_assert(src.size() <= dst.size());
		auto dst_it(dst.begin());
		
		auto const &src_range(src.to_word_range());
		src_range.apply_aligned(
			[
#ifndef TEXT_ALIGN_NDEBUG
			 	&dst,
#endif
				&dst_it
			](typename t_src_matrix::word_type word, std::size_t const element_count){ // FIXME: memory_order_acquire?
				for (std::size_t i(0); i < element_count; ++i)
				{
					text_align_assert(dst_it != dst.end());
					dst_it->fetch_or(word & t_src_matrix::ELEMENT_MASK); // FIXME: memory_order_release?
					word >>= t_src_matrix::ELEMENT_BITS;
					++dst_it;
				}
			}
		);
	}
	
	
	template <typename t_src_matrix, typename t_dst_matrix>
	void transpose_column_to_row(
		::text_align::detail::packed_matrix_slice <t_src_matrix> const &src,
		::text_align::detail::packed_matrix_slice <t_dst_matrix> &&dst	// Proxy, allow moving.
	)
	{
		transpose_column_to_row(src, dst);
	}

	
	template <unsigned int t_pattern_length, typename t_matrix>
	void fill_column_with_bit_pattern(
		::text_align::detail::packed_matrix_slice <t_matrix> &column,
		typename t_matrix::word_type pattern
	)
	{
		pattern = detail::fill_bit_pattern <t_pattern_length>(pattern);
		auto word_range(column.to_word_range());
		word_range.apply_parts(
			[pattern](auto &atomic){ do_and_assert_eq(atomic.fetch_or(pattern), 0); },
			[pattern](auto &atomic, std::size_t const offset, std::size_t const length){
				text_align_assert(length);
				auto p(pattern);
				p >>= (t_matrix::WORD_BITS - length);
				p <<= offset;
				do_and_assert_eq(atomic.fetch_or(p), 0);
			}
		);
	}
	
	
	template <unsigned int t_pattern_length, typename t_matrix>
	void fill_column_with_bit_pattern(
		::text_align::detail::packed_matrix_slice <t_matrix> &&column,	// Proxy, allow moving.
		typename t_matrix::word_type pattern
	)
	{
		fill_column_with_bit_pattern <t_pattern_length>(column, pattern);
	}
}}

#endif
