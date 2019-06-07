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
#include <text_align/code_point_range.hh>
#include <text_align/json_serialize.hh>
#include <text_align/smith_waterman/alignment_context.hh>


namespace {
	
	typedef std::int32_t score_type;
	
	typedef text_align::smith_waterman::alignment_context <
		score_type,
		std::uint64_t,
		libbio::bit_vector
	> alignment_bv_context_type;
		
	typedef text_align::smith_waterman::alignment_context <
		score_type,
		std::uint64_t,
		libbio::rle_bit_vector <std::uint32_t>
	> alignment_rle_context_type;
	
	
	void make_string_view(text const *txt, std::string_view &out_sv)
	{
		out_sv = std::string_view(VARDATA(txt), VARSIZE(txt) - VARHDRSZ);
	}
	
	
	template <typename t_range>
	std::size_t copy_distance(t_range range)
	{
		return ranges::distance(range);
	}
	
	
	template <typename t_aligner>
	void assign_scores(t_aligner &aligner, score_type const match, score_type const mismatch, score_type const gap_start, score_type const gap)
	{
		aligner.set_identity_score(match);
		aligner.set_mismatch_penalty(mismatch);
		aligner.set_gap_start_penalty(gap_start);
		aligner.set_gap_penalty(gap);
		aligner.set_reverses_texts(true);
	}
	
	
	void serialize_to_json(alignment_rle_context_type const &ctx, std::ostringstream &os)
	{
		// Write JSON directly to os.
		os << "{\"left\":";
		text_align::json::to_json(os, ctx.lhs_gaps());
		os << ",\"right\":";
		text_align::json::to_json(os, ctx.rhs_gaps());
		os << "}";
	}
}


extern "C" {

	PG_MODULE_MAGIC;

	PG_FUNCTION_INFO_V1(align_texts);
	Datum align_texts(PG_FUNCTION_ARGS)
	{
		namespace lb = libbio;
		namespace ta = text_align;
		
		if (7 != PG_NARGS())
		{
			ereport(ERROR, (
				errcode(ERRCODE_PROTOCOL_VIOLATION),
				errmsg("expected seven arguments: lhs, rhs, match_score, mismatch_penalty, gap_start_penalty, gap_penalty, should_return_alignment_graph")
			));
		}

		auto const *lhs(PG_GETARG_TEXT_P(0));
		auto const *rhs(PG_GETARG_TEXT_P(1));
		auto const match_score(PG_GETARG_INT32(2));
		auto const mismatch_penalty(PG_GETARG_INT32(3));
		auto const gap_start_penalty(PG_GETARG_INT32(4));
		auto const gap_penalty(PG_GETARG_INT32(5));
		auto const should_return_alignment_graph(PG_GETARG_BOOL(6));

		// Don’t leak anything thrown.
		try
		{
			// Create iterator ranges out of the UTF-8 strings.
			std::string_view lhsv, rhsv;
			make_string_view(lhs, lhsv);
			make_string_view(rhs, rhsv);
			auto const lhsr(ta::make_reversed_code_point_range(lhsv));
			auto const rhsr(ta::make_reversed_code_point_range(rhsv));
			auto const lhs_len(copy_distance(lhsr));
			auto const rhs_len(copy_distance(rhsr));
			
			// Buffer for the return value.
			std::ostringstream os;
			
			if (should_return_alignment_graph)
			{
				// Instantiate the aligner.
				alignment_bv_context_type ctx;
				auto &aligner(ctx.get_aligner());
				assign_scores(aligner, match_score, mismatch_penalty, gap_start_penalty, gap_penalty);
				//aligner.set_prints_debugging_information(print_debugging_information);
			
				// Align the texts.
				aligner.align(lhsr, rhsr, lhs_len, rhs_len);
				ctx.run();
			
				// Build the alignment graph.
				ta::alignment_graph_builder <char32_t> builder;
				builder.build_graph(lhsr, rhsr, ctx.lhs_gaps(), ctx.rhs_gaps());
			
				// Serialize to JSON.
				ta::json::to_json(os, builder.text_segments());
			}
			else
			{
				// Instantiate the aligner.
				alignment_rle_context_type ctx;
				auto &aligner(ctx.get_aligner());
				assign_scores(aligner, match_score, mismatch_penalty, gap_start_penalty, gap_penalty);
				
				// Align the texts.
				aligner.align(lhsr, rhsr, lhs_len, rhs_len);
				ctx.run();
				
				// Serialize to JSON.
				serialize_to_json(ctx, os);
			}
			
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
