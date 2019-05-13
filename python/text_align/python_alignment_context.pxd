# Copyright (c) 2019 Tuukka Norri
# This code is licensed under MIT license (see LICENSE for details).

# cython: language_level=3

from libc.stdint cimport int32_t, uint32_t, uint64_t
from libc.stddef cimport size_t
from libcpp cimport bool
from libcpp.map cimport map
from libcpp.pair cimport pair
from .bit_vector_interface cimport bit_vector_interface
from .smith_waterman_aligner cimport aligner


cdef extern from "python_alignment_context.hh" namespace "text_align":

	cdef cppclass python_alignment_context_base:
		python_alignment_context_base() except +
		python_alignment_context_base(size_t const) except +
		
		void run() except +
		void restart() except +
		bool stopped() except +
		
		const bit_vector_interface[uint64_t] &lhs_gaps() except +
		const bit_vector_interface[uint64_t] &rhs_gaps() except +
		
		void instantiate_lhs_gaps[t_lhs]() except +
		void instantiate_rhs_gaps[t_rhs]() except +
		
		map[pair[uint32_t, uint32_t], float] &get_scores() except +
		
	cdef cppclass python_alignment_context(python_alignment_context_base):
		python_alignment_context() except +
		python_alignment_context(size_t const) except +
		aligner[int32_t, uint64_t, python_alignment_context] &get_aligner() except +
	
	cdef cppclass python_scoring_fp_alignment_context(python_alignment_context_base):
		python_scoring_alignment_context() except +
		python_scoring_alignment_context(size_t const) except +
		aligner[float, uint64_t, python_scoring_fp_alignment_context] &get_aligner() except +
