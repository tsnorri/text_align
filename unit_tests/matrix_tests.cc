/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#define BOOST_TEST_MODULE "Unit tests"
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <iostream>
#include <text_align/algorithm.hh>
#include <text_align/matrix.hh>
#include <text_align/packed_matrix.hh>
#include <tuple>
#include <type_traits>


typedef std::tuple <
	std::int8_t,
	std::int16_t,
	std::int32_t,
	std::int64_t,
	std::uint8_t,
	std::uint16_t,
	std::uint32_t,
	std::uint64_t,
	float,
	double
> all_test_types;


template <typename t_value>
void create_matrix_12(text_align::matrix <t_value> &dst)
{
	std::size_t const rows(3);
	std::size_t const columns(4);
	
	text_align::matrix <t_value> temp(rows, columns);
	t_value val{};
	for (std::size_t i(0); i < columns; ++i)
	{
		for (std::size_t j(0); j < rows; ++j)
		{
			auto &ref(temp(j, i));
			ref = val++;
		}
	}
	
	using std::swap;
	swap(temp, dst);
}


void create_packed_matrix_8(text_align::packed_matrix <4, std::uint16_t> &dst)
{
	std::size_t const rows(4);
	std::size_t const columns(2);
	
	text_align::packed_matrix <4, std::uint16_t> temp(rows, columns);
	std::uint8_t val{};
	for (auto ref : temp)
		ref.fetch_or((val++) & 0xf);
	
	using std::swap;
	swap(temp, dst);
}


void create_packed_matrix_7x3(text_align::packed_matrix <4, std::uint16_t> &dst)
{
	std::size_t const rows(7);
	std::size_t const columns(3);
	
	text_align::packed_matrix <4, std::uint16_t> temp(rows, columns);
	std::uint8_t val{};
	for (auto ref : temp)
		ref.fetch_or((val++) & 0xf);
	
	using std::swap;
	swap(temp, dst);
}


BOOST_AUTO_TEST_CASE_TEMPLATE(test_creation_empty, T, all_test_types)
{
	text_align::matrix <T> matrix;
	BOOST_TEST(matrix.size() == 0);
	BOOST_TEST(matrix.number_of_rows() == 1);
	BOOST_TEST(matrix.number_of_columns() == 0);
}


BOOST_AUTO_TEST_CASE_TEMPLATE(test_creation, T, all_test_types)
{
	text_align::matrix <T> matrix(3, 4);
	BOOST_TEST(matrix.size() == 12);
	BOOST_TEST(matrix.number_of_rows() == 3);
	BOOST_TEST(matrix.number_of_columns() == 4);
}


BOOST_AUTO_TEST_CASE_TEMPLATE(test_values, T, all_test_types)
{
	text_align::matrix <T> matrix;
	create_matrix_12(matrix);
	
	BOOST_TEST(matrix.size() == 12);
	BOOST_TEST(matrix.number_of_rows() == 3);
	BOOST_TEST(matrix.number_of_columns() == 4);
	
	T val{};
	for (std::size_t i(0), columns(matrix.number_of_columns()); i < columns; ++i)
	{
		for (std::size_t j(0), rows(matrix.number_of_rows()); j < rows; ++j)
			BOOST_TEST(matrix(j, i) == val++);
	}
}


BOOST_AUTO_TEST_CASE_TEMPLATE(test_row_slice, T, all_test_types)
{
	text_align::matrix <T> matrix;
	create_matrix_12(matrix);
	
	BOOST_TEST(matrix.number_of_rows() == 3);
	for (std::size_t i(0), rows(matrix.number_of_rows()); i < rows; ++i)
	{
		auto const slice(matrix.row(i));
		BOOST_TEST(4 == std::distance(slice.begin(), slice.end()));
		
		auto it(slice.begin());
		auto end(slice.end());
		
		for (std::size_t j(0); j < 4; ++j)
		{
			auto const expected(j * 3 + i);
			BOOST_TEST(expected == *it++);
		}
	
		BOOST_TEST(it == end);
	}
}


BOOST_AUTO_TEST_CASE_TEMPLATE(test_column_slice, T, all_test_types)
{
	text_align::matrix <T> matrix;
	create_matrix_12(matrix);
	
	BOOST_TEST(matrix.number_of_columns() == 4);
	T val{};
	for (std::size_t i(0), columns(matrix.number_of_columns()); i < columns; ++i)
	{
		auto const slice(matrix.column(i));
		BOOST_TEST(3 == std::distance(slice.begin(), slice.end()));
		
		auto it(slice.begin());
		auto end(slice.end());
		
		for (std::size_t j(0); j < 3; ++j)
			BOOST_TEST(*it++ == val++);
		
		BOOST_TEST(it == end);
	}
}


BOOST_AUTO_TEST_CASE_TEMPLATE(test_assign, T, all_test_types)
{
	text_align::matrix <T> matrix;
	create_matrix_12(matrix);
	
	BOOST_TEST(matrix(1, 3) == 10);
	matrix(1, 3) = 9;
	BOOST_TEST(matrix(1, 3) == 9);
}


BOOST_AUTO_TEST_CASE_TEMPLATE(test_assign_slice_row, T, all_test_types)
{
	text_align::matrix <T> matrix;
	create_matrix_12(matrix);
	
	auto slice(matrix.row(1));
	
	BOOST_TEST(slice[3] == 10);
	slice[3] = 9;
	BOOST_TEST(slice[3] == 9);
	BOOST_TEST(matrix(1, 3) == 9);
}


BOOST_AUTO_TEST_CASE_TEMPLATE(test_assign_slice_column, T, all_test_types)
{
	text_align::matrix <T> matrix;
	create_matrix_12(matrix);
	
	auto slice(matrix.column(3));
	
	BOOST_TEST(slice[1] == 10);
	slice[1] = 9;
	BOOST_TEST(slice[1] == 9);
	BOOST_TEST(matrix(1, 3) == 9);
}


BOOST_AUTO_TEST_CASE_TEMPLATE(test_slice_min_row, T, all_test_types)
{
	text_align::matrix <T> matrix;
	create_matrix_12(matrix);
	
	auto const slice(matrix.row(1));
	BOOST_TEST(*std::min_element(slice.begin(), slice.end()) == 1);
}


BOOST_AUTO_TEST_CASE_TEMPLATE(test_slice_min_column, T, all_test_types)
{
	text_align::matrix <T> matrix;
	create_matrix_12(matrix);
	
	auto const slice(matrix.column(3));
	BOOST_TEST(*std::min_element(slice.begin(), slice.end()) == 9);
}


// FIXME: these should be moved to another module but I was not able to configure Boost.Test so that it would work.
BOOST_AUTO_TEST_CASE(test_reverse_bits_1)
{
	std::uint64_t const val(0xf0f0f0f0f0f0f0f0UL);
	std::uint64_t const reversed(text_align::reverse_bits(val));
	BOOST_TEST(0x0f0f0f0f0f0f0f0f == reversed);
}


BOOST_AUTO_TEST_CASE(test_reverse_bits_2)
{
	std::uint64_t const val(0xf0f0f0f0UL);
	std::uint64_t const reversed(text_align::reverse_bits(val));
	BOOST_TEST(0x0f0f0f0f00000000 == reversed);
}


BOOST_AUTO_TEST_CASE(test_reverse_bits_3)
{
	std::uint64_t const val(0xff7f3f1f0f070301UL);
	std::uint64_t const reversed(text_align::reverse_bits(val));
	BOOST_TEST(0x80c0e0f0f8fcfeff == reversed);
}


BOOST_AUTO_TEST_CASE(test_packed_vector_creation)
{
	text_align::packed_vector <4, std::uint16_t> vec(8);
	BOOST_TEST(16 == vec.word_bits());
	BOOST_TEST(4 == vec.element_bits());
	BOOST_TEST(4 == vec.element_count_in_word());
	BOOST_TEST(8 == vec.size());
	BOOST_TEST(2 == vec.word_size());
}


BOOST_AUTO_TEST_CASE(test_packed_vector_values)
{
	text_align::packed_vector <4, std::uint16_t> vec(8);
	BOOST_TEST(8 == vec.size());
	BOOST_TEST(2 == vec.word_size());
	
	std::size_t idx(0);
	for (auto proxy : vec)
		proxy.fetch_or(idx++);
	BOOST_TEST(8 == idx);
	
	idx = 0;
	for (auto proxy : vec)
	{
		BOOST_TEST(idx == proxy.load());
		++idx;
	}
	
	idx = 0;
	while (idx < 8)
	{
		BOOST_TEST(idx == vec.load(idx));
		++idx;
	}
}


BOOST_AUTO_TEST_CASE(test_packed_vector_words)
{
	text_align::packed_vector <4, std::uint16_t> vec(8);
	BOOST_TEST(8 == vec.size());
	BOOST_TEST(2 == vec.word_size());
	
	std::size_t idx(0);
	for (auto proxy : vec)
		BOOST_TEST(0 == proxy.fetch_or(idx++));
	BOOST_TEST(8 == idx);

	BOOST_TEST(0x3210 == vec.word_begin()->load());
	BOOST_TEST(0x7654 == (vec.word_begin() + 1)->load());
}


BOOST_AUTO_TEST_CASE(test_packed_vector_fetch_or)
{
	text_align::packed_vector <4, std::uint16_t> vec(8);
	BOOST_TEST(8 == vec.size());
	BOOST_TEST(2 == vec.word_size());
	
	BOOST_TEST(0x0 == vec(1).fetch_or(0x2));
	BOOST_TEST(0x2 == vec(1).fetch_or(0x1));
	BOOST_TEST(0x3 == vec.load(1));
}


BOOST_AUTO_TEST_CASE(test_packed_matrix_creation)
{
	text_align::packed_matrix <4, std::uint16_t> matrix;
	create_packed_matrix_8(matrix);
	
	BOOST_TEST(16 == matrix.word_bits());
	BOOST_TEST(4 == matrix.element_bits());
	BOOST_TEST(4 == matrix.element_count_in_word());
	BOOST_TEST(8 == matrix.size());
	BOOST_TEST(2 == matrix.word_size());
}


BOOST_AUTO_TEST_CASE(test_packed_matrix_values)
{
	text_align::packed_matrix <4, std::uint16_t> matrix;
	create_packed_matrix_8(matrix);
	
	BOOST_TEST(matrix.size() == 8);
	BOOST_TEST(matrix.number_of_rows() == 4);
	BOOST_TEST(matrix.number_of_columns() == 2);
	
	{
		std::uint8_t val{};
		for (std::size_t i(0), columns(matrix.number_of_columns()); i < columns; ++i)
		{
			for (std::size_t j(0), rows(matrix.number_of_rows()); j < rows; ++j)
				BOOST_TEST(matrix(j, i) == ((val++) & 0xf));
		}
	}
	
	{
		std::uint8_t val{};
		for (auto const k : matrix)
			BOOST_TEST(((val++) & 0xf) == k);
	}
}


BOOST_AUTO_TEST_CASE(test_packed_matrix_slices)
{
	text_align::packed_matrix <4, std::uint16_t> matrix;
	create_packed_matrix_8(matrix);
	
	BOOST_TEST(matrix.size() == 8);
	BOOST_TEST(matrix.number_of_rows() == 4);
	BOOST_TEST(matrix.number_of_columns() == 2);
	
	{
		auto column(matrix.column(0));
		BOOST_TEST(4 == column.size());
		BOOST_TEST(0 == column[0]);
		BOOST_TEST(1 == column[1]);
		BOOST_TEST(2 == column[2]);
		BOOST_TEST(3 == column[3]);
	}
	
	{
		auto column(matrix.column(1));
		BOOST_TEST(4 == column.size());
		BOOST_TEST(4 == column[0]);
		BOOST_TEST(5 == column[1]);
		BOOST_TEST(6 == column[2]);
		BOOST_TEST(7 == column[3]);
	}

	{
		auto row(matrix.row(0));
		BOOST_TEST(2 == row.size());
		BOOST_TEST(0 == row[0]);
		BOOST_TEST(4 == row[1]);
	}

	{
		auto row(matrix.row(1));
		BOOST_TEST(2 == row.size());
		BOOST_TEST(1 == row[0]);
		BOOST_TEST(5 == row[1]);
	}

	{
		auto row(matrix.row(2));
		BOOST_TEST(2 == row.size());
		BOOST_TEST(2 == row[0]);
		BOOST_TEST(6 == row[1]);
	}

	{
		auto row(matrix.row(3));
		BOOST_TEST(2 == row.size());
		BOOST_TEST(3 == row[0]);
		BOOST_TEST(7 == row[1]);
	}
}


BOOST_AUTO_TEST_CASE(test_packed_matrix_slices_start_limit)
{
	text_align::packed_matrix <4, std::uint16_t> matrix;
	create_packed_matrix_8(matrix);
	
	BOOST_TEST(matrix.size() == 8);
	BOOST_TEST(matrix.number_of_rows() == 4);
	BOOST_TEST(matrix.number_of_columns() == 2);
	
	{
		auto column(matrix.column(1, 1, 3));
		BOOST_TEST(2 == column.size());
		BOOST_TEST(5 == column[0]);
		BOOST_TEST(6 == column[1]);
	}
	
	{
		auto row(matrix.row(2, 1, 2));
		BOOST_TEST(1 == row.size());
		BOOST_TEST(6 == row[0]);
	}
}


BOOST_AUTO_TEST_CASE(test_packed_matrix_slices_unaligned)
{
	text_align::packed_matrix <4, std::uint16_t> matrix;
	create_packed_matrix_7x3(matrix);

	BOOST_TEST(matrix.size() == 21);
	BOOST_TEST(matrix.number_of_rows() == 7);
	BOOST_TEST(matrix.number_of_columns() == 3);
	
	{
		auto column(matrix.column(1));
		BOOST_TEST(7 == column.size());
		BOOST_TEST(7 == column[0]);
		BOOST_TEST(8 == column[1]);
		BOOST_TEST(11 == column[4]);
		BOOST_TEST(12 == column[5]);
	}
	
	{
		auto row(matrix.row(3));
		BOOST_TEST(3 == row.size());
		BOOST_TEST(3 == row[0]);
		BOOST_TEST(10 == row[1]);
		BOOST_TEST(1 == row[2]);
	}
}


BOOST_AUTO_TEST_CASE(test_packed_matrix_slices_start_limit_unaligned)
{
	text_align::packed_matrix <4, std::uint16_t> matrix;
	create_packed_matrix_7x3(matrix);
	
	BOOST_TEST(matrix.size() == 21);
	BOOST_TEST(matrix.number_of_rows() == 7);
	BOOST_TEST(matrix.number_of_columns() == 3);
	
	{
		auto column(matrix.column(1, 3, 6));
		BOOST_TEST(3 == column.size());
		BOOST_TEST(10 == column[0]);
		BOOST_TEST(11 == column[1]);
		BOOST_TEST(12 == column[2]);
	}
	
	{
		auto row(matrix.row(5, 1, 3));
		BOOST_TEST(2 == row.size());
		BOOST_TEST(12 == row[0]);
		BOOST_TEST(3 == row[1]);
	}
}


BOOST_AUTO_TEST_CASE(test_packed_matrix_transpose)
{
	text_align::packed_matrix <4, std::uint16_t> dst(2, 8);
	text_align::packed_matrix <4, std::uint16_t> src;
	create_packed_matrix_7x3(src);
	
	for (std::size_t col(0); col < 3; ++col)
	{
		auto const &src_col(src.column(col));
		auto dst_row(dst.row(0));
		
		text_align::matrices::transpose_column_to_row(src_col, dst_row);
		std::size_t i(0);
		while (i < src_col.size())
		{
			BOOST_TEST(src_col[i] == dst_row[i]);
			++i;
		}
		while (i < dst_row.size())
		{
			BOOST_TEST(0 == dst_row[i]);
			++i;
		}
		
		// Clear.
		std::fill(dst.word_begin(), dst.word_end(), 0);
		for (auto const &word : dst.word_range())
			BOOST_TEST(0 == word.load());
	}
}
