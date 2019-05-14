/*
 * Copyright (c) 2018-2019 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef TEXT_ALIGN_RUN_ALIGNER_HH
#define TEXT_ALIGN_RUN_ALIGNER_HH

#include <boost/beast/core/span.hpp>
#include <libbio/map_on_stack.hh>
#include <libbio/rle_bit_vector.hh>
#include <Python.h>
#include <text_align/alignment_graph_builder.hh>
#include <text_align/smith_waterman/aligner.hh>

// Cython tries to not include this file when the file passed to the C++ compiler is
// the compiled .pyx file, but we need it.
#undef __PYX_HAVE__text_align__aligner
#ifdef __PYX_HAVE_API__text_align__aligner
#	undef __PYX_HAVE_API__text_align__aligner
#	include "aligner.h"
#	define __PYX_HAVE_API__text_align__aligner
#else
#	include "aligner.h"
#endif


namespace text_align { namespace detail {
	
	void copy_long_list_to_vector(PyObject *list, std::vector <long> &dst);
	
	PyObject *copy_long_vector_to_list(std::vector <long> const &src);
	PyObject *copy_char32_vector_to_unicode(std::vector <char32_t> const &src);
	
	
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
	
	
	template <typename t_character>
	void run_builder_string(
		alignment_graph_builder <t_character> &builder,
		PyObject *lhso,
		PyObject *rhso,
		libbio::bit_vector &lhs_gaps,
		libbio::bit_vector &rhs_gaps
	)
	{
		// Convert the Python strings to spans. Throw if the vectors are not convertible to bit_vectors.
		libbio::map_on_stack_fn <detail::span_from_buffer>(
			[&builder, &lhs_gaps, &rhs_gaps](auto const &lhss, auto const &rhss) {
				builder.build_graph(lhss, rhss, lhs_gaps, rhs_gaps);
			},
			lhso, rhso
		);
	}
	
	
	template <typename t_character>
	void run_builder_list(
		alignment_graph_builder <t_character> &builder,
		PyObject *lhso,
		PyObject *rhso,
		libbio::bit_vector &lhs_gaps,
		libbio::bit_vector &rhs_gaps
	)
	{
		std::vector <long> lhs, rhs;
		copy_long_list_to_vector(lhso, lhs);
		copy_long_list_to_vector(rhso, rhs);
		
		builder.build_graph(lhs, rhs, lhs_gaps, rhs_gaps);
	}
	
	
	template <typename t_aligner_context, typename t_lhs, typename t_rhs>
	void run_aligner_(t_aligner_context &ctx, t_lhs const &lhs, t_rhs const &rhs)
	{
		if (ctx.stopped())
			ctx.restart();
		
		ctx.get_aligner().align(lhs, rhs);
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
	
	
	template <typename t_character>
	struct process_node_characters {};
	
	template <>
	struct process_node_characters <std::vector <long>>
	{
		typedef std::vector <long>		vector_type;
		static PyObject *process(vector_type const &vec) { return copy_long_vector_to_list(vec); }
	};
	
	template <>
	struct process_node_characters <std::vector <char32_t>>
	{
		typedef std::vector <char32_t>	vector_type;
		static PyObject *process(vector_type const &vec) { return copy_char32_vector_to_unicode(vec); }
	};
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
	
	
	template <typename t_character, typename t_aligner_context>
	void run_builder(alignment_graph_builder <t_character> &builder, t_aligner_context &ctx, PyObject *lhso, PyObject *rhso)
	{
		// Builder requires bit vectors (as opposed to RLE vectors).
		auto &lhs_gaps(dynamic_cast <libbio::bit_vector &>(ctx.lhs_gaps()));
		auto &rhs_gaps(dynamic_cast <libbio::bit_vector &>(ctx.rhs_gaps()));
		
		if (PyList_Check(lhso) && PyList_Check(rhso))
			detail::run_builder_list(builder, lhso, rhso, lhs_gaps, rhs_gaps);
		else if (PyUnicode_Check(lhso) && PyUnicode_Check(rhso))
			detail::run_builder_string(builder, lhso, rhso, lhs_gaps, rhs_gaps);
		else
			throw std::runtime_error("Unexpected Python object type");
	}
	
	
	template <typename t_character>
	void process_alignment_graph(alignment_graph_builder <t_character> const &builder, PyObject *dst)
	{
		libbio_always_assert(PyList_Check(dst));
		
		struct visitor : public alignment_graph::node_visitor
		{
			PyObject *dst{};
			
			visitor(PyObject *dst_):
				dst(dst_)
			{
				if (!dst)
					throw std::runtime_error("Unable to create a Python list");
			}
			
			virtual void visit_common_node(alignment_graph::node_base &nb) override
			{
				typedef alignment_graph::common_node <t_character> node_type;
				typedef detail::process_node_characters <typename node_type::vector_type> pn_type;
				auto const &node(static_cast <node_type &>(nb));
				auto *characters(pn_type::process(node.characters()));
				
				// From aligner.h
				::handle_common_node(dst, characters);
				
				// FIXME: wrap into a smart pointer?
				Py_DECREF(characters);
			}
			
			virtual void visit_distinct_node(alignment_graph::node_base &nb) override
			{
				typedef alignment_graph::distinct_node <t_character> node_type;
				typedef detail::process_node_characters <typename node_type::vector_type> pn_type;
				auto const &node(static_cast <node_type &>(nb));
				auto *characters_lhs(pn_type::process(node.characters_lhs()));
				auto *characters_rhs(pn_type::process(node.characters_rhs()));
				
				// From aligner.h
				::handle_distinct_node(dst, characters_lhs, characters_rhs);
				
				// FIXME: wrap into a smart pointer?
				Py_DECREF(characters_lhs);
				Py_DECREF(characters_rhs);
			}
		};
		
		auto const &text_segments(builder.text_segments());
		visitor visitor(dst);
		for (auto const &seg : text_segments)
			seg->visit(visitor);
	}
}

#endif
