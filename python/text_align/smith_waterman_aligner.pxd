# Copyright (c) 2018 Tuukka Norri
# This code is licensed under MIT license (see LICENSE for details).

# cython: language_level=3

from libc.stdint cimport uint32_t, uint64_t
from libc.stddef cimport size_t
from libcpp cimport bool
from libcpp.vector cimport vector
from .bit_vector_interface cimport bit_vector_interface
from .int_vector cimport bit_vector


cdef extern from "<text_align/smith_waterman/aligner.hh>" namespace "text_align::smith_waterman":

	cdef cppclass aligner[t_score, t_word, t_delegate]:
		
		t_score identity_score()
		t_score mismatch_penalty()
		t_score gap_penalty()
		t_score gap_start_penalty()
		uint32_t segment_length()
		bool prints_debugging_information()
		
		void set_identity_score(t_score const)
		void set_mismatch_penalty(t_score const)
		void set_gap_start_penalty(t_score const)
		void set_gap_penalty(t_score const)
		void set_segment_length(uint32_t const)
		void set_prints_debugging_information(bool const)
		
		void align[t_string](const t_string &, const t_string &)
		
		t_score alignment_score() const


cdef extern from "<text_align/smith_waterman/alignment_context_pv.hh>" namespace "text_align::smith_waterman":

	cdef cppclass alignment_context_pv[t_score, t_word]:
		alignment_context() except +
		void run() except +
		void restart() except +
		bool stopped() except +
		
		aligner[t_score, t_word, alignment_context_pv[t_score, t_word]] &get_aligner()

		const bit_vector_interface[t_word] &lhs_gaps() except +
		const bit_vector_interface[t_word] &rhs_gaps() except +

		void instantiate_lhs_gaps[t_lhs]() except +
		void instantiate_rhs_gaps[t_rhs]() except +
