/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <boost/locale/utf.hpp>
#include <boost/range.hpp>
#include <iostream>
#include <string>
#include <text_align/aligner.hh>
#include <text_align/code_point_iterator.hh>
#include <vector>

#include "cmdline.h"


namespace ta = text_align;


template <typename t_range>
std::size_t copy_distance(t_range range)
{
	return boost::distance(range);
}


template <typename t_range>
void print_aligned(t_range const &range, std::vector <bool> const &gaps)
{
	//assert(str.size() <= gaps.size());
	
	std::string buffer;
	
	auto it(range.begin()), end(range.end());
	auto ostream_it(std::ostream_iterator <char>(std::cout, ""));
	// std::ostream_iterator's operator++ is a no-op, hence we don't
	// save the output value from utf_traits nor use it in else.
	for (auto const flag : gaps)
	{
		if (0 == flag)
		{
			assert(it != end);
			boost::locale::utf::utf_traits <char>::encode(*it++, ostream_it);
		}
		else
		{
			std::cout << '-';
		}
	}
	assert(it == end);
	std::cout << std::endl;
}


int main(int argc, char **argv)
{
	//std::string lhs("ACTAAGGCTCTCTACCCCTCTCAGAGA");
	//std::string rhs("ACTAAGGCTCCTAACCCCCTTTTCTCAGA");
	
	gengetopt_args_info args_info;
	if (0 != cmdline_parser(argc, argv, &args_info))
		exit(EXIT_FAILURE);
	
	std::string_view lhsv(args_info.lhs_arg, strlen(args_info.lhs_arg));
	std::string_view rhsv(args_info.rhs_arg, strlen(args_info.rhs_arg));
	auto const lhsr(ta::make_code_point_iterator_range(lhsv.cbegin(), lhsv.cend()));
	auto const rhsr(ta::make_code_point_iterator_range(rhsv.cbegin(), rhsv.cend()));
	auto const lhs_len(copy_distance(lhsr));
	auto const rhs_len(copy_distance(rhsr));
	assert(lhsr.begin() != lhsr.end());
	assert(rhsr.begin() != rhsr.end());
	
	auto const match_score(args_info.match_score_arg);
	auto const mismatch_penalty(args_info.mismatch_penalty_arg);
	auto const gap_penalty(args_info.gap_penalty_arg);
	bool const print_debugging_information(args_info.print_debugging_information_flag);
	
	ta::alignment_context <std::int32_t, ta::smith_waterman_aligner> ctx;
	auto &aligner(ctx.aligner());
	
	aligner.set_identity_score(match_score);
	aligner.set_mismatch_penalty(mismatch_penalty);
	aligner.set_gap_penalty(gap_penalty);
	aligner.set_prints_debugging_information(print_debugging_information);
	aligner.align(lhsr, rhsr, lhs_len, rhs_len);
	ctx.run();
	
	std::cout << "Score: " << aligner.alignment_score() << std::endl;
	print_aligned(lhsr, aligner.lhs_gaps());
	print_aligned(rhsr, aligner.rhs_gaps());
	
	cmdline_parser_free(&args_info);

	return 0;
}
