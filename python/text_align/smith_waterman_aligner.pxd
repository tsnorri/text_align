# Copyright (c) 2018 Tuukka Norri
# This code is licensed under MIT license (see LICENSE for details).

from libc.stdint cimport uint32_t
from libcpp cimport bool
from libcpp.vector cimport vector


cdef extern from "<text_align/aligner.hh>" namespace "text_align":

	cdef cppclass smith_waterman_aligner[t_score, t_delegate]:
		
		t_score identity_score()
		t_score mismatch_penalty()
		t_score gap_penalty()
		uint32_t segment_length()
		bool prints_debugging_information()
		
		void set_identity_score(t_score const)
		void set_mismatch_penalty(t_score const)
		void set_gap_penalty(t_score const)
		void set_segment_length(uint32_t const)
		void set_prints_debugging_information(bool const)
		
		void align[t_string](t_string const, t_string const)
		
		t_score alignment_score() const
		
		vector[bool] &lhs_gaps()
		vector[bool] &rhs_gaps()
	
	
	cdef cppclass alignment_context[t_score, t_aligner]:
		alignment_context() except+
		
		t_aligner &aligner()
		
		void run()
	
	
	# FIXME: Cython does not seem to support template template arguments.
	# A wrapper like this around a using directive seems fine, though.
	cdef cppclass smith_waterman_alignment_context[t_score]:
		smith_waterman_alignment_context() except+
		void run()
		
		smith_waterman_aligner[t_score, smith_waterman_alignment_context[t_score]] &aligner()
