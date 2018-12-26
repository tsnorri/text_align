# Copyright (c) 2018 Tuukka Norri
# This code is licensed under MIT license (see LICENSE for details).

# cython: language_level=3

from libc.stdint cimport int32_t, uint32_t, uint64_t
from libcpp.memory cimport unique_ptr
from libcpp.vector cimport vector
from .alignment_graph_builder cimport node_base, common_node, distinct_node
from .bit_vector_interface cimport bit_vector_interface
from .rle_bit_vector cimport rle_bit_vector
from .smith_waterman_aligner cimport alignment_context_pv

include "char32_t.pxi"


cdef class AlignmentContext(object):
	cdef object lhs
	cdef object rhs
	
	cdef void copy_to_list(self, const vector[char32_t] &vec, object dst)
	cdef object make_common_node(self, const common_node &seg)
	cdef object make_distinct_node(self, const distinct_node &seg)
	cdef object make_alignment_graph_from_segments(self, const vector[unique_ptr[node_base]] &segments)

cdef class SmithWatermanAlignmentContext(AlignmentContext):
	cdef alignment_context_pv[int32_t, uint64_t] *ctx
	cdef convert_rle_vector(self, const rle_bit_vector[uint32_t] &)
