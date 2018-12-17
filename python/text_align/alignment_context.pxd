# Copyright (c) 2018 Tuukka Norri
# This code is licensed under MIT license (see LICENSE for details).

# cython: language_level=3

from libc.stdint cimport int32_t, uint64_t
from libcpp.memory cimport unique_ptr
from libcpp.vector cimport vector
from .alignment_graph_builder cimport node_base, common_node, distinct_node
from .smith_waterman_aligner cimport alignment_context

include "char32_t.pxi"


cdef class AlignmentContext(object):
	cdef object lhs
	cdef object rhs
	
	cdef void copy_to_list(self, const vector[char32_t] &vec, object dst)
	cdef object make_common_node(self, const common_node &seg)
	cdef object make_distinct_node(self, const distinct_node &seg)
	cdef object make_alignment_graph_from_segments(self, const vector[unique_ptr[node_base]] &segments)
	
cdef class SmithWatermanAlignmentContext(AlignmentContext):
	cdef alignment_context[int32_t, uint64_t] *ctx
