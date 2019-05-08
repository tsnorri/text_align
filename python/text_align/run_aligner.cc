/*
 * Copyright (c) 2019 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef TEXT_ALIGN_COPY_PYTHON_LIST_HH
#define TEXT_ALIGN_COPY_PYTHON_LIST_HH

#include "run_aligner.hh"


namespace text_align { namespace detail {
	void copy_long_list_to_vector(PyObject *list, std::vector <long> &dst)
	{
		auto const len(PyList_GET_SIZE(list));
		dst.resize(len, 0);
		
		typedef std::remove_const_t <decltype(len)> counter_type;
		for (counter_type i(0); i < len; ++i)
		{
			auto *item(PyList_GetItem(list, i));
			auto const val(PyLong_AsLong(item));
			dst[i] = val;
		}
	}
	
	
	void run_builder_list(
		alignment_graph_builder &builder,
		PyObject *lhso,
		PyObject *rhso,
		libbio::bit_bector &lhs_gaps,
		libbio::bit_vector &rhs_gaps
	)
	{
		std::vector <long> lhs, rhs;
		copy_long_list_to_vector(lhso, lhs);
		copy_long_list_to_vector(rhso, rhs);
		
		builder.build_graph(lhs, rhs, lhs_gaps, rhs_gaps);
	}
	
	
	void run_builder_string(
		alignment_graph_builder &builder,
		PyObject *lhso,
		PyObject *rhso,
		libbio::bit_bector &lhs_gaps,
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
}}

#endif
