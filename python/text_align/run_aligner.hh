/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef TEXT_ALIGN_RUN_ALIGNER_HH
#define TEXT_ALIGN_RUN_ALIGNER_HH

#include <boost/beast/core/span.hpp>
#include <text_align/alignment_graph_builder.hh>
#include <text_align/map_on_stack.hh>
#include <text_align/rle_bit_vector.hh>
#include <text_align/smith_waterman/aligner.hh>


namespace text_align { namespace detail {
	
	struct span_from_buffer
	{
		template <typename t_data, typename t_fn>
		void handle_data(t_data *data, Py_ssize_t const length, t_fn &&fn)
		{
			boost::beast::span span(data, length);
			fn(span);
		}
		
		
		// Python Unicode strings select the character type in which the greatest code point
		// fits (without using e.g. surrogate pairs). Determine the character type, create
		// a span of that type and pass it to the callback.
		template <typename t_fn>
		void operator()(PyObject *arg, t_fn &&fn)
		{
			if (0 != PyUnicode_READY(arg))
				throw std::runtime_error("Given Python string is not in canonical representation");
				
			auto const length(PyUnicode_GET_LENGTH(arg));
			if (length < 0)
				throw std::runtime_error("PyUnicode_GET_LENGTH returned a negative value");
			
			// Determine the character type and create a span of that type.
			switch (PyUnicode_KIND(arg))
			{
				case PyUnicode_WCHAR_KIND:
				{
					auto *data(reinterpret_cast <wchar_t const *>(PyUnicode_DATA(arg)));
					handle_data(data, length, fn);
					break;
				}
				
				case PyUnicode_1BYTE_KIND:
				{
					auto *data(PyUnicode_1BYTE_DATA(arg));
					handle_data(data, length, fn);
					break;
				}
				
				case PyUnicode_2BYTE_KIND:
				{
					auto *data(PyUnicode_2BYTE_DATA(arg));
					handle_data(data, length, fn);
					break;
				}
				
				case PyUnicode_4BYTE_KIND:
				{
					auto *data(PyUnicode_4BYTE_DATA(arg));
					handle_data(data, length, fn);
					break;
				}
				
				default:
					throw std::runtime_error("Unexpected Python string representation");
			};
		}
	};
}}


namespace text_align
{
	template <typename t_aligner_context>
	void run_aligner(t_aligner_context &ctx, PyObject *lhso, PyObject *rhso)
	{
		// Convert the Python strings to spans.
		text_align::map_on_stack_fn <detail::span_from_buffer>(
			[&ctx](auto const &lhss, auto const &rhss) {
				if (ctx.stopped())
					ctx.restart();
				
				ctx.aligner().align(lhss, rhss);
				ctx.run();
			},
			lhso, rhso
		);
	}
	
	
	template <typename t_aligner_context>
	void run_builder(alignment_graph_builder &builder, t_aligner_context const &ctx, PyObject *lhso, PyObject *rhso)
	{
		auto &lhs_gaps(ctx.lhs_gaps());
		auto &rhs_gaps(ctx.rhs_gaps());
		// Convert the Python strings to spans. Throw if the vectors are not convertible to bit_vectors.
		text_align::map_on_stack_fn <detail::span_from_buffer>(
			[&builder, &lhs_gaps, &rhs_gaps](auto const &lhss, auto const &rhss) {
				builder.build_graph(
					lhss,
					rhss,
					dynamic_cast <bit_vector const &>(lhs_gaps),
					dynamic_cast <bit_vector const &>(rhs_gaps)
				);
			},
			lhso, rhso
		);
	}
}

#endif
