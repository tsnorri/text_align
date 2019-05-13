# Copyright (c) 2018-2019 Tuukka Norri
# This code is licensed under MIT license (see LICENSE for details).

# cython: language_level=3

from libcpp cimport bool
from libcpp.memory cimport unique_ptr
from libcpp.vector cimport vector
from .int_vector cimport bit_vector

include "char32_t.pxi"


cdef extern from "<text_align/alignment_graph_builder.hh>" namespace "text_align::alignment_graph::node_type":
	cdef enum node_type "text_align::alignment_graph::node_type":
		NONE,
		COMMON,
		DISTINCT


cdef extern from "<text_align/alignment_graph_builder.hh>" namespace "text_align::alignment_graph":
	cdef cppclass node_base:
		node_type type()

	cdef cppclass common_node[t_character](node_base):
		const vector[t_character] &text()
	
	cdef cppclass distinct_node[t_character](node_base):
		const vector[t_character] &text_lhs()
		const vector[t_character] &text_rhs()


cdef extern from "<text_align/alignment_graph_builder.hh>" namespace "text_align":

	cdef cppclass alignment_graph_builder[t_character]:
		alignment_graph_builder() except+
		
		const vector[unique_ptr[node_base]] &text_segments()
		
		void build_graph[t_lhs, t_rhs](const t_lhs &, const t_rhs &, const bit_vector &, const bit_vector &)
