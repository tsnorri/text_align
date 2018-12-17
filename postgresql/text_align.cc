/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

// For some reason extern "C" is missing from here.
extern "C" {
#	include <arpa/inet.h>
#	include <postgres.h>
#	include <fmgr.h>
#	include <utils/builtins.h>
}

#include <sstream>
#include <text_align/alignment_graph_builder.hh>
#include <text_align/code_point_iterator.hh>
#include <text_align/json_serialize.hh>
#include <text_align/smith_waterman/alignment_context.hh>


namespace {
	
	void make_string_view(text const *txt, std::string_view &out_sv)
	{
		out_sv = std::string_view(VARDATA(txt), VARSIZE(txt) - VARHDRSZ);
	}
	
	
	template <typename t_range>
	std::size_t copy_distance(t_range range)
	{
		return boost::distance(range);
	}
}


extern "C" {

	PG_MODULE_MAGIC;

	PG_FUNCTION_INFO_V1(align_texts);
	Datum align_texts(PG_FUNCTION_ARGS)
	{
		namespace ta = text_align;
		
		if (6 != PG_NARGS())
		{
			ereport(ERROR, (
				errcode(ERRCODE_PROTOCOL_VIOLATION),
				errmsg("expected six arguments: lhs, rhs, match_score, mismatch_penalty, gap_start_penalty, gap_penalty")
			));
		}

		auto const *lhs(PG_GETARG_TEXT_P(0));
		auto const *rhs(PG_GETARG_TEXT_P(1));
		auto const match_score(PG_GETARG_INT32(2));
		auto const mismatch_penalty(PG_GETARG_INT32(3));
		auto const gap_start_penalty(PG_GETARG_INT32(4));
		auto const gap_penalty(PG_GETARG_INT32(5));

		// Don't leak anything thrown.
		try
		{
			// Create iterator ranges out of the UTF-8 strings.
			std::string_view lhsv, rhsv;
			make_string_view(lhs, lhsv);
			make_string_view(rhs, rhsv);
			auto const lhsr(ta::make_code_point_iterator_range(lhsv.cbegin(), lhsv.cend()));
			auto const rhsr(ta::make_code_point_iterator_range(rhsv.cbegin(), rhsv.cend()));
			auto const lhs_len(copy_distance(lhsr));
			auto const rhs_len(copy_distance(rhsr));
			
			// Instantiate the aligner.
			ta::smith_waterman::alignment_context <std::int32_t, std::uint64_t> ctx;
			auto &aligner(ctx.aligner());
			aligner.set_identity_score(match_score);
			aligner.set_mismatch_penalty(mismatch_penalty);
			aligner.set_gap_start_penalty(gap_start_penalty);
			aligner.set_gap_penalty(gap_penalty);
			//aligner.set_prints_debugging_information(print_debugging_information);
			
			// Align the texts.
			aligner.align(lhsr, rhsr, lhs_len, rhs_len);
			ctx.run();
			
			// Build the alignment graph.
			ta::alignment_graph_builder builder;
			builder.build_graph(lhsr, rhsr, aligner.lhs_gaps(), aligner.rhs_gaps());
			
			// Serialize to JSON.
			std::ostringstream os;
			ta::json::to_json(os, builder.text_segments());
			std::string const &json_buffer(os.str());
			
			PG_RETURN_TEXT_P(cstring_to_text_with_len(json_buffer.data(), json_buffer.size()));
		}
		catch (std::exception const &exc)
		{
			ereport(ERROR, (
				errmsg("caught an exception: %s", exc.what())
			));
		}
		catch (...)
		{
			ereport(ERROR, (
				errmsg("caught an unknown exception")
			));
		}
		
		PG_RETURN_NULL();
	}
}
