/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#define BOOST_TEST_MODULE "Unit tests"
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <iostream>
#include <libbio/int_vector.hh>
#include <text_align/alignment_graph_builder.hh>
#include <text_align/code_point_iterator.hh>
#include <text_align/smith_waterman/aligner.hh>
#include <text_align/smith_waterman/alignment_context.hh>

#include <tuple>
#include <type_traits>


namespace ta = text_align;


// For aligner tests.
typedef std::int32_t score_type;

template <typename t_block>
using alignment_context_type = text_align::smith_waterman::alignment_context <score_type, t_block, libbio::bit_vector>;


template <typename t_range>
std::size_t copy_distance(t_range range)
{
	return ranges::distance(range);
}

template <typename t_block>
void run_aligner(
	alignment_context_type <t_block> &ctx,
	std::string const &lhs,
	std::string const &rhs,
	typename alignment_context_type <t_block>::bit_vector_type const &expected_lhs,
	typename alignment_context_type <t_block>::bit_vector_type const &expected_rhs,
	score_type const expected_score,
	std::size_t const block_size,
	score_type const match_score,
	score_type const mismatch_penalty,
	score_type const gap_start_penalty,
	score_type const gap_penalty
)
{
	auto &aligner(ctx.get_aligner());

	aligner.set_segment_length(block_size);
	aligner.set_identity_score(match_score);
	aligner.set_mismatch_penalty(mismatch_penalty);
	aligner.set_gap_start_penalty(gap_start_penalty);
	aligner.set_gap_penalty(gap_penalty);
	aligner.set_reverses_texts(true);

	auto const lhsr(ta::make_reversed_code_point_range(ranges::view::reverse(lhs)));
	auto const rhsr(ta::make_reversed_code_point_range(ranges::view::reverse(rhs)));
	auto const lhs_len(copy_distance(lhsr));
	auto const rhs_len(copy_distance(rhsr));
	assert(lhsr.begin() != lhsr.end());
	assert(rhsr.begin() != rhsr.end());
	
	aligner.align(lhsr, rhsr, lhs_len, rhs_len);
	ctx.run();
	
	BOOST_TEST(aligner.alignment_score() == expected_score);
	BOOST_TEST(ctx.lhs_gaps() == expected_lhs);
	BOOST_TEST(ctx.rhs_gaps() == expected_rhs);
}


// Aligner tests
BOOST_AUTO_TEST_CASE(test_aligner_0)
{
	typedef alignment_context_type <std::uint16_t> alignment_context;
	typedef typename alignment_context::bit_vector_type bit_vector;
	
	bit_vector const lhs(4, 0x0);
	bit_vector rhs(4, 0x0);
	alignment_context ctx;
	run_aligner(ctx, "asdf", "asdf", lhs, rhs, 8, 8, 2, -2, -2, -1);
}


BOOST_AUTO_TEST_CASE(test_aligner_1)
{
	typedef alignment_context_type <std::uint16_t> alignment_context;
	typedef typename alignment_context::bit_vector_type bit_vector;
	
	bit_vector const lhs(5, 0x0);
	bit_vector rhs(5, 0x0);
	*rhs.word_begin() = 0x4;
	alignment_context ctx;
	run_aligner(ctx, "xaasd", "xasd", lhs, rhs, 4, 8, 2, -2, -2, -1);
}


BOOST_AUTO_TEST_CASE(test_aligner_2_32)
{
	typedef alignment_context_type <std::uint32_t> alignment_context;
	typedef typename alignment_context::bit_vector_type bit_vector;
	
	bit_vector const lhs(10, 0x0);
	bit_vector rhs(10, 0x0);
	*rhs.word_begin() = 0x84;
	alignment_context ctx;
	run_aligner(ctx, "xaasdxaasd", "xasdxasd", lhs, rhs, 8, 16, 2, -2, -2, -1);
}


BOOST_AUTO_TEST_CASE(test_aligner_2_16)
{
	typedef alignment_context_type <std::uint16_t> alignment_context;
	typedef typename alignment_context::bit_vector_type bit_vector;
	
	bit_vector const lhs(10, 0x0);
	bit_vector rhs(10, 0x0);
	*rhs.word_begin() = 0x84;
	alignment_context ctx;
	run_aligner(ctx, "xaasdxaasd", "xasdxasd", lhs, rhs, 8, 8, 2, -2, -2, -1);
}


BOOST_AUTO_TEST_CASE(test_aligner_2_8)
{
	typedef alignment_context_type <std::uint16_t> alignment_context;
	typedef typename alignment_context::bit_vector_type bit_vector;
	
	bit_vector const lhs(10, 0x0);
	bit_vector rhs(10, 0x0);
	*rhs.word_begin() = 0x84;
	alignment_context ctx;
	run_aligner(ctx, "xaasdxaasd", "xasdxasd", lhs, rhs, 8, 4, 2, -2, -2, -1);
}


BOOST_AUTO_TEST_CASE(test_aligner_2_8_graph)
{
	typedef alignment_context_type <std::uint16_t> alignment_context;
	typedef typename alignment_context::bit_vector_type bit_vector;
	
	bit_vector const lhs(10, 0x0);
	bit_vector rhs(10, 0x0);
	*rhs.word_begin() = 0x84;
	alignment_context ctx;
	
	auto const lhss("xaasdxaasd");
	auto const rhss("xasdxasd");
	run_aligner(ctx, lhss, rhss, lhs, rhs, 8, 4, 2, -2, -2, -1);
	
	ta::alignment_graph_builder <char32_t> builder;
	builder.build_graph(std::string(lhss), std::string(rhss), ctx.lhs_gaps(), ctx.rhs_gaps());
}
