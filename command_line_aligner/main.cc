/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <text_align/algorithm.hh>

#include <boost/dynamic_bitset.hpp>
#include <boost/locale/utf.hpp>
#include <boost/range.hpp>
#include <iostream>
#include <string>
#include <text_align/code_point_iterator.hh>
#include <text_align/map_on_stack.hh>
#include <text_align/mmap_handle.hh>
#include <text_align/smith_waterman/aligner.hh>
#include <text_align/smith_waterman/alignment_context.hh>
#include <vector>

#include "cmdline.h"


namespace ta = text_align;


typedef ta::smith_waterman::alignment_context <std::int32_t, std::uint32_t> alignment_context_type;


template <typename t_range>
std::size_t copy_distance(t_range range)
{
	return boost::distance(range);
}


template <typename t_range, typename t_block>
void print_aligned(t_range const &range, boost::dynamic_bitset <t_block> const &gaps)
{
	//assert(str.size() <= gaps.size());
	
	std::string buffer;
	
	auto it(range.begin()), end(range.end());
	auto ostream_it(std::ostream_iterator <char>(std::cout, ""));
	// std::ostream_iterator's operator++ is a no-op, hence we don't
	// save the output value from utf_traits nor use it in else.
	// boost::dynamic_bitset does not have iterators, so use operator[].
	for (std::size_t i(0), count(gaps.size()); i < count; ++i)
	{
		if (0 == gaps[i])
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


struct string_view_from_input
{
	template <typename t_tuple, typename t_fn>
	void operator()(t_tuple const &tuple, t_fn &&fn)
	{
		auto const *arg(std::get <0>(tuple));
		auto const *path_arg(std::get <1>(tuple));
		
		if (arg)
		{
			std::string_view view(arg, strlen(arg));
			fn(view);
		}
		else
		{
			text_align::mmap_handle handle;
			handle.open(path_arg);
			std::string_view view(handle.data(), handle.size());
			fn(view);
		}
	}
};


void process_input(alignment_context_type &ctx, gengetopt_args_info const &args_info)
{
	auto const lhs_input(std::make_tuple(args_info.lhs_arg, args_info.lhs_file_arg));
	auto const rhs_input(std::make_tuple(args_info.rhs_arg, args_info.rhs_file_arg));
	ta::map_on_stack_fn <string_view_from_input>(
		[&ctx, &args_info](std::string_view const &lhsv, std::string_view const &rhsv) {
			auto &aligner(ctx.aligner());
			
			auto const match_score(args_info.match_score_arg);
			auto const mismatch_penalty(args_info.mismatch_penalty_arg);
			auto const gap_start_penalty(args_info.gap_start_penalty_arg);
			auto const gap_penalty(args_info.gap_penalty_arg);
			auto const block_size(args_info.block_size_arg);
			bool const print_debugging_information(args_info.print_debugging_information_flag);
			
			aligner.set_segment_length(block_size);
			aligner.set_identity_score(match_score);
			aligner.set_mismatch_penalty(mismatch_penalty);
			aligner.set_gap_start_penalty(gap_start_penalty);
			aligner.set_gap_penalty(gap_penalty);
			aligner.set_prints_debugging_information(print_debugging_information);
			
			if (args_info.align_bytes_flag)
			{
				aligner.align(lhsv, rhsv, lhsv.size(), rhsv.size());
				ctx.run();
				std::cout << "Score: " << aligner.alignment_score() << std::endl;
			}
			else
			{
				auto const lhsr(ta::make_code_point_iterator_range(lhsv.cbegin(), lhsv.cend()));
				auto const rhsr(ta::make_code_point_iterator_range(rhsv.cbegin(), rhsv.cend()));
				auto const lhs_len(copy_distance(lhsr));
				auto const rhs_len(copy_distance(rhsr));
				assert(lhsr.begin() != lhsr.end());
				assert(rhsr.begin() != rhsr.end());
				
				aligner.align(lhsr, rhsr, lhs_len, rhs_len);
				ctx.run();
				std::cout << "Score: " << aligner.alignment_score() << std::endl;
				print_aligned(lhsr, aligner.lhs_gaps());
				print_aligned(rhsr, aligner.rhs_gaps());
			}
		},
		lhs_input, rhs_input
	);
}


int main(int argc, char **argv)
{
	//std::string lhs("ACTAAGGCTCTCTACCCCTCTCAGAGA");
	//std::string rhs("ACTAAGGCTCCTAACCCCCTTTTCTCAGA");
	
	gengetopt_args_info args_info;
	if (0 != cmdline_parser(argc, argv, &args_info))
		exit(EXIT_FAILURE);
	
	std::ios_base::sync_with_stdio(false);	// Don't use C style IO after calling cmdline_parser.
	std::cin.tie(nullptr);					// We don't require any input from the user.
	
	if (args_info.print_invocation_flag)
	{
		std::cerr << "Invocation:";
		for (int i(0); i < argc; ++i)
			std::cerr << ' ' << argv[i];
		std::cerr << '\n';
	}
	
	if (args_info.block_size_arg < 0)
	{
		std::cerr << "Block size needs to be non-negative." << std::endl;
		exit(EXIT_FAILURE);
	}
	
	if (args_info.single_threaded_flag)
	{
		alignment_context_type ctx(1);
		process_input(ctx, args_info);
	}
	else
	{
		alignment_context_type ctx;
		process_input(ctx, args_info);
	}
	
	cmdline_parser_free(&args_info);
	
	return 0;
}
