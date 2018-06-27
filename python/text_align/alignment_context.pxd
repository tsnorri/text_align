# Copyright (c) 2018 Tuukka Norri
# This code is licensed under MIT license (see LICENSE for details).

from libc.stdint cimport int32_t
from smith_waterman_aligner cimport smith_waterman_alignment_context


cdef class AlignmentContext(object):
	cdef object lhs
	cdef object rhs
	
cdef class SmithWatermanAlignmentContext(AlignmentContext):
	cdef smith_waterman_alignment_context[int32_t] *ctx
