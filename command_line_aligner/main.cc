/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <boost/locale/utf.hpp>
#include <iostream>
#include <string>
#include <text_align/aligner.hh>
#include <vector>

#include "cmdline.h"


namespace ta = text_align;


typedef std::vector <char32_t> utf32_string;


template <typename t_input_it, typename t_output_it>
void utf8to32(t_input_it it, t_input_it end, t_output_it out)
{
	namespace utf = boost::locale::utf;
	
	while (it != end)
	{
		auto const code_point(utf::utf_traits <char>::decode(it, end));
		if (utf::illegal == code_point || utf::incomplete == code_point)
			throw std::runtime_error("Received malformed UTF-8");
		*out = code_point;
		++out;
	}
}


void print_aligned(utf32_string const &str, std::vector <bool> const &gaps)
{
	assert(str.size() <= gaps.size());
	
	std::string buffer;
	
	auto it(str.begin()), end(str.end());
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
	
	utf32_string lhs, rhs;
	std::string_view lhsv(args_info.lhs_arg, strlen(args_info.lhs_arg));
	std::string_view rhsv(args_info.rhs_arg, strlen(args_info.rhs_arg));
	utf8to32(lhsv.cbegin(), lhsv.cend(), std::back_inserter(lhs));
	utf8to32(rhsv.cbegin(), rhsv.cend(), std::back_inserter(rhs));
	
	auto const match_score(args_info.match_score_arg);
	auto const mismatch_penalty(args_info.mismatch_penalty_arg);
	auto const gap_penalty(args_info.gap_penalty_arg);
	bool const print_debugging_information(args_info.print_debugging_information_flag);
	
	cmdline_parser_free(&args_info);
	
	ta::alignment_context <std::int32_t, ta::smith_waterman_aligner> ctx;
	auto &aligner(ctx.aligner());
	
	aligner.set_identity_score(match_score);
	aligner.set_mismatch_penalty(mismatch_penalty);
	aligner.set_gap_penalty(gap_penalty);
	aligner.set_prints_debugging_information(print_debugging_information);
	aligner.align(lhs, rhs);
	ctx.run();
	
	std::cout << "Score: " << aligner.alignment_score() << std::endl;
	print_aligned(lhs, aligner.lhs_gaps());
	print_aligned(rhs, aligner.rhs_gaps());
	
	return 0;
}
