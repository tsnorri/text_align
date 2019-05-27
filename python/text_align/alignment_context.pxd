# Copyright (c) 2019 Tuukka Norri
# This code is licensed under MIT license (see LICENSE for details).

# cython: language_level=3

from . cimport interface as cxx
from libc.stdint cimport int32_t, uint32_t, uint64_t
from libc.stddef cimport size_t
from libcpp cimport bool
from libcpp.map cimport map
from libcpp.pair cimport pair


cdef extern from "alignment_context.hh" namespace "text_align::python":

	cdef cppclass alignment_context_base:
		alignment_context_base() except +
		alignment_context_base(size_t const) except +
		
		void run() except +
		void restart() except +
		bool stopped() except +
		
		const cxx.bit_vector_interface[uint64_t] &lhs_gaps() except +
		const cxx.bit_vector_interface[uint64_t] &rhs_gaps() except +
		
		void instantiate_lhs_gaps[t_lhs]() except +
		void instantiate_rhs_gaps[t_rhs]() except +
		
		map[pair[long, long], float] &get_scores() except +
		map[pair[long, long], float] *get_scores_ptr() except +
		
	cdef cppclass alignment_context(alignment_context_base):
		alignment_context() except +
		alignment_context(size_t const) except +
		cxx.aligner[int32_t, uint64_t, alignment_context] &get_aligner() except +
	
	cdef cppclass scoring_fp_alignment_context(alignment_context_base):
		scoring_alignment_context() except +
		scoring_alignment_context(size_t const) except +
		cxx.aligner[float, uint64_t, scoring_fp_alignment_context] &get_aligner() except +
