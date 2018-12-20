/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#define BOOST_TEST_MODULE "Unit tests"
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <iostream>
#include <text_align/algorithm.hh>
#include <text_align/code_point_iterator.hh>
#include <text_align/int_vector.hh>
#include <text_align/matrix.hh>
#include <text_align/matrix_utils.hh>
#include <text_align/packed_matrix.hh>
#include <text_align/smith_waterman/aligner.hh>
#include <text_align/smith_waterman/alignment_context.hh>

#include <tuple>
#include <type_traits>


namespace ta = text_align;


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

// For aligner tests.
typedef std::int32_t score_type;

template <typename t_block>
using alignment_context_type = text_align::smith_waterman::alignment_context <score_type, t_block>;


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


template <typename t_range>
std::size_t copy_distance(t_range range)
{
	return boost::distance(range);
}

template <typename t_block>
void run_aligner(
	alignment_context_type <t_block> &ctx,
	std::string const &lhs,
	std::string const &rhs,
	typename alignment_context_type <t_block>::aligner_type::bit_vector const &expected_lhs,
	typename alignment_context_type <t_block>::aligner_type::bit_vector const &expected_rhs,
	score_type const expected_score,
	std::size_t const block_size,
	score_type const match_score,
	score_type const mismatch_penalty,
	score_type const gap_start_penalty,
	score_type const gap_penalty
)
{
	auto &aligner(ctx.aligner());

	aligner.set_segment_length(block_size);
	aligner.set_identity_score(match_score);
	aligner.set_mismatch_penalty(mismatch_penalty);
	aligner.set_gap_start_penalty(gap_start_penalty);
	aligner.set_gap_penalty(gap_penalty);
	
	auto const lhsr(ta::make_code_point_iterator_range(lhs.cbegin(), lhs.cend()));
	auto const rhsr(ta::make_code_point_iterator_range(rhs.cbegin(), rhs.cend()));
	auto const lhs_len(copy_distance(lhsr));
	auto const rhs_len(copy_distance(rhsr));
	assert(lhsr.begin() != lhsr.end());
	assert(rhsr.begin() != rhsr.end());
	
	aligner.align(lhsr, rhsr, lhs_len, rhs_len);
	ctx.run();
	
	BOOST_TEST(aligner.alignment_score() == expected_score);
	BOOST_TEST(aligner.lhs_gaps() == expected_lhs);
	BOOST_TEST(aligner.rhs_gaps() == expected_rhs);
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
	std::uint64_t const reversed(text_align::reverse_bits <1>(val));
	BOOST_TEST(0x0f0f0f0f0f0f0f0f == reversed);
}


BOOST_AUTO_TEST_CASE(test_reverse_bits_2)
{
	std::uint64_t const val(0xf0f0f0f0UL);
	std::uint64_t const reversed(text_align::reverse_bits <1>(val));
	BOOST_TEST(0x0f0f0f0f00000000 == reversed);
}


BOOST_AUTO_TEST_CASE(test_reverse_bits_3)
{
	std::uint64_t const val(0xff7f3f1f0f070301UL);
	std::uint64_t const reversed(text_align::reverse_bits <1>(val));
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


BOOST_AUTO_TEST_CASE(test_packed_matrix_transpose_aligned_extent)
{
	// Source smaller than word size but word-aligned.
	text_align::packed_matrix <2, std::uint16_t> src(4, 4);
	text_align::packed_matrix <4, std::uint16_t> dst(4, 4);
	
	src(0, 0).fetch_or(0x3);
	src(1, 0).fetch_or(0x0);
	src(2, 0).fetch_or(0x1);
	src(3, 0).fetch_or(0x2);
	
	for (std::size_t i(0); i < dst.number_of_rows(); ++i)
	{
		auto const &row(dst.row(i));
		for (auto const val : row)
			BOOST_TEST(0 == val);
	}
	
	text_align::matrices::transpose_column_to_row(src.column(0), dst.row(0));
	
	for (std::size_t i(1); i < dst.number_of_rows(); ++i)
	{
		auto const &row(dst.row(i));
		for (auto const val : row)
			BOOST_TEST(0 == val);
	}

	BOOST_TEST(0x3 == dst(0, 0));
	BOOST_TEST(0x0 == dst(0, 1));
	BOOST_TEST(0x1 == dst(0, 2));
	BOOST_TEST(0x2 == dst(0, 3));
}


BOOST_AUTO_TEST_CASE(test_packed_matrix_fill)
{
	text_align::packed_matrix <2, std::uint8_t> mat(5, 2);
	for (std::size_t i(0); i < mat.number_of_columns(); ++i)
	{
		for (auto const val : mat.column(i))
			BOOST_TEST(0 == val);
	}
	
	text_align::matrices::fill_column_with_bit_pattern <2>(mat.column(0), 0x1);
	
	for (auto const val : mat.column(0))
		BOOST_TEST(0x1 == val);
	for (auto const val : mat.column(1))
		BOOST_TEST(0x0 == val);
}


BOOST_AUTO_TEST_CASE(test_packed_matrix_fill_2)
{
	text_align::packed_matrix <2, std::uint8_t> mat(5, 3);
	for (std::size_t i(0); i < mat.number_of_columns(); ++i)
	{
		for (auto const val : mat.column(i))
			BOOST_TEST(0 == val);
	}
	
	text_align::matrices::fill_column_with_bit_pattern <2>(mat.column(1), 0x1);

	for (auto const val : mat.column(0))
		BOOST_TEST(0x0 == val);
	for (auto const val : mat.column(1))
		BOOST_TEST(0x1 == val);
	for (auto const val : mat.column(2))
		BOOST_TEST(0x0 == val);
}


BOOST_AUTO_TEST_CASE(test_packed_matrix_copy_mid_bits)
{
	text_align::packed_matrix <2, std::uint32_t> src(16, 1);
	text_align::packed_matrix <2, std::uint32_t> dst(16, 1);
	
	src(1, 0).fetch_or(0x1);
	src(2, 0).fetch_or(0x2);
	src(3, 0).fetch_or(0x3);
	
	auto const col(src.column(0, 1, 4));
	text_align::matrices::copy_to_word_aligned(col, dst.column(0));
	
	BOOST_TEST(0x1 == dst(0, 0));
	BOOST_TEST(0x2 == dst(1, 0));
	BOOST_TEST(0x3 == dst(2, 0));
	for (std::size_t i(3); i < dst.number_of_rows(); ++i)
		BOOST_TEST(0x0 == dst(i, 0));
}


BOOST_AUTO_TEST_CASE(test_packed_matrix_copy_mid_bits_skip_extra)
{
	text_align::packed_matrix <2, std::uint32_t> src(16, 1);
	text_align::packed_matrix <2, std::uint32_t> dst(16, 1);
	
	src(1, 0).fetch_or(0x1);
	src(2, 0).fetch_or(0x2);
	src(3, 0).fetch_or(0x3);
	// Extra values that should not be copied.
	src(4, 0).fetch_or(0x1);
	src(5, 0).fetch_or(0x1);
	src(6, 0).fetch_or(0x1);
	
	auto const col(src.column(0, 1, 4));
	text_align::matrices::copy_to_word_aligned(col, dst.column(0));
	
	BOOST_TEST(0x1 == dst(0, 0));
	BOOST_TEST(0x2 == dst(1, 0));
	BOOST_TEST(0x3 == dst(2, 0));
	for (std::size_t i(3); i < dst.number_of_rows(); ++i)
		BOOST_TEST(0x0 == dst(i, 0));
}


// Int vector tests
BOOST_AUTO_TEST_CASE(test_int_vector_push)
{
	text_align::int_vector <8> vec;
	for (std::size_t i(0); i < 10; ++i)
		vec.push_back(i);

	BOOST_TEST(10 == vec.size());
	for (std::size_t i(0); i < 10; ++i)
		BOOST_TEST(i == vec[i]);
	
	std::size_t i(0);
	for (auto const val : vec)
		BOOST_TEST(i++ == val);
}


BOOST_AUTO_TEST_CASE(test_int_vector_push_multiple)
{
	text_align::int_vector <8> vec;
	vec.push_back(0);
	
	vec.push_back(88, 18);
	
	BOOST_TEST(19 == vec.size());
	BOOST_TEST(0 == vec[0]);
	for (std::size_t i(1); i < 19; ++i)
		BOOST_TEST(88 == vec[i]);
}


// Aligner tests
BOOST_AUTO_TEST_CASE(test_aligner_0)
{
	typedef alignment_context_type <std::uint16_t> alignment_context;
	typedef typename alignment_context::aligner_type::bit_vector bit_vector;
	
	bit_vector const lhs(4, 0x0);
	bit_vector const rhs(4, 0x0);
	alignment_context ctx;
	run_aligner(ctx, "asdf", "asdf", lhs, rhs, 8, 8, 2, -2, -2, -1);
}


BOOST_AUTO_TEST_CASE(test_aligner_1)
{
	typedef alignment_context_type <std::uint16_t> alignment_context;
	typedef typename alignment_context::aligner_type::bit_vector bit_vector;
	
	bit_vector const lhs(5, 0x0);
	bit_vector const rhs(5, 0x2);
	alignment_context ctx;
	run_aligner(ctx, "xaasd", "xasd", lhs, rhs, 4, 8, 2, -2, -2, -1);
}


BOOST_AUTO_TEST_CASE(test_aligner_2_32)
{
	typedef alignment_context_type <std::uint32_t> alignment_context;
	typedef typename alignment_context::aligner_type::bit_vector bit_vector;
	
	bit_vector const lhs(10, 0x0);
	bit_vector const rhs(10, 0x42);
	alignment_context ctx;
	run_aligner(ctx, "xaasdxaasd", "xasdxasd", lhs, rhs, 8, 16, 2, -2, -2, -1);
}


BOOST_AUTO_TEST_CASE(test_aligner_2_16)
{
	typedef alignment_context_type <std::uint16_t> alignment_context;
	typedef typename alignment_context::aligner_type::bit_vector bit_vector;
	
	bit_vector const lhs(10, 0x0);
	bit_vector const rhs(10, 0x42);
	alignment_context ctx;
	run_aligner(ctx, "xaasdxaasd", "xasdxasd", lhs, rhs, 8, 8, 2, -2, -2, -1);
}


BOOST_AUTO_TEST_CASE(test_aligner_2_8)
{
	typedef alignment_context_type <std::uint16_t> alignment_context;
	typedef typename alignment_context::aligner_type::bit_vector bit_vector;
	
	bit_vector const lhs(10, 0x0);
	bit_vector const rhs(10, 0x42);
	alignment_context ctx;
	run_aligner(ctx, "xaasdxaasd", "xasdxasd", lhs, rhs, 8, 4, 2, -2, -2, -1);
}
