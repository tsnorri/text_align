/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef TEXT_ALIGN_RUN_ALIGNER_HH
#define TEXT_ALIGN_RUN_ALIGNER_HH

#include <boost/beast/core/span.hpp>
#include <libbio/map_on_stack.hh>
#include <libbio/rle_bit_vector.hh>
#include <text_align/alignment_graph_builder.hh>
#include <text_align/smith_waterman/aligner.hh>


namespace text_align { namespace detail {
	
	void copy_long_list_to_vector(PyObject *list, std::vector <long> &dst);
	
	void run_builder_list(
		alignment_graph_builder &builder,
		PyObject *lhso,
		PyObject *rhso,
		libbio::bit_bector &lhs_gaps,
		libbio::bit_vector &rhs_gaps
	);
	
	void run_builder_string(
		alignment_graph_builder &builder,
		PyObject *lhso,
		PyObject *rhso,
		libbio::bit_bector &lhs_gaps,
		libbio::bit_vector &rhs_gaps
	);
	
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
	
	
	template <typename t_aligner_context, typename t_lhs, typename t_rhs>
	void run_aligner_(t_aligner_context &ctx, t_lhs const &lhs, t_rhs const &rhs)
	{
		if (ctx.stopped())
			ctx.restart();
		
		ctx.get_aligner().align(lhss, rhss);
		ctx.run();
	}
	
	
	template <typename t_aligner_context>
	void run_aligner_string(t_aligner_context &ctx, PyObject *lhso, PyObject *rhso)
	{
		// Convert the Python strings to spans.
		libbio::map_on_stack_fn <detail::span_from_buffer>(
			[&ctx](auto const &lhss, auto const &rhss) {
				run_aligner_(ctx, lhss, rhss);
			},
			lhso, rhso
		);
	}
	
	
	template <typename t_aligner_context>
	void run_aligner_list(t_aligner_context &ctx, PyObject *lhso, PyObject *rhso)
	{
		std::vector <long> lhs, rhs;
		copy_long_list_to_vector(lhso, lhs);
		copy_long_list_to_vector(rhso, rhs);
		
		run_aligner_(ctx, lhs, rhs);
	}
}}


namespace text_align
{
	template <typename t_aligner_context>
	void run_aligner(t_aligner_context &ctx, PyObject *lhso, PyObject *rhso)
	{
		if (PyList_Check(lhso) && PyList_Check(rhso))
			detail::run_aligner_list(ctx, lhso, rhso);
		else if (PyUnicode_Check(lhso) && PyUnicode_Check(rhso))
			detail::run_aligner_string(ctx, lhso, rhso);
		else
			throw std::runtime_error("Unexpected Python object type");
	}
	
	
	template <typename t_aligner_context>
	void run_builder(alignment_graph_builder &builder, t_aligner_context const &ctx, PyObject *lhso, PyObject *rhso)
	{
		auto &lhs_gaps(ctx.lhs_gaps());
		auto &rhs_gaps(ctx.rhs_gaps());
		
		if (PyList_Check(lhso) && PyList_Check(rhso))
			detail::run_builder_list(builder, lhso, rhso, lhs_gaps, rhs_gaps);
		else if (PyUnicode_Check(lhso) && PyUnicode_Check(rhso))
			detail::run_builder_string(builder, lhso, rhso, lhs_gaps, rhs_gaps);
		else
			throw std::runtime_error("Unexpected Python object type");
	}
}

#endif
