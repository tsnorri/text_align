/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#define BOOST_TEST_MODULE matrix tests
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <iostream>
#include <text_align/matrix.hh>
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



BOOST_AUTO_TEST_CASE_TEMPLATE(test_creation_empty, T, all_test_types)
{
	text_align::matrix <T> matrix;
	BOOST_TEST(matrix.size() == 0);
	BOOST_TEST(matrix.row_size() == 1);
	BOOST_TEST(matrix.column_size() == 0);
}


BOOST_AUTO_TEST_CASE_TEMPLATE(test_creation, T, all_test_types)
{
	text_align::matrix <T> matrix(3, 4);
	BOOST_TEST(matrix.size() == 12);
	BOOST_TEST(matrix.row_size() == 3);
	BOOST_TEST(matrix.column_size() == 4);
}


BOOST_AUTO_TEST_CASE_TEMPLATE(test_values, T, all_test_types)
{
	text_align::matrix <T> matrix;
	create_matrix_12(matrix);
	
	BOOST_TEST(matrix.size() == 12);
	BOOST_TEST(matrix.row_size() == 3);
	BOOST_TEST(matrix.column_size() == 4);
	
	T val{};
	for (std::size_t i(0), columns(matrix.column_size()); i < columns; ++i)
	{
		for (std::size_t j(0), rows(matrix.row_size()); j < rows; ++j)
			BOOST_TEST(matrix(j, i) == val++);
	}
}


BOOST_AUTO_TEST_CASE_TEMPLATE(test_row_slice, T, all_test_types)
{
	text_align::matrix <T> matrix;
	create_matrix_12(matrix);
	
	BOOST_TEST(matrix.row_size() == 3);
	for (std::size_t i(0), rows(matrix.row_size()); i < rows; ++i)
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
	
	BOOST_TEST(matrix.column_size() == 4);
	T val{};
	for (std::size_t i(0), columns(matrix.column_size()); i < columns; ++i)
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

