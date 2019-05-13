# Copyright (c) 2018-2019 Tuukka Norri
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
