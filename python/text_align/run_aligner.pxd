# Copyright (c) 2018-2019 Tuukka Norri
# This code is licensed under MIT license (see LICENSE for details).

# cython: language_level=3

from . cimport interface as cxx


cdef extern from "run_aligner.hh" namespace "text_align":
	cdef void run_aligner[t_context](t_context &, object, object) except +
	cdef void run_builder[t_character, t_aligner](cxx.alignment_graph_builder[t_character] &, const t_aligner &, object, object) except +
	cdef object process_alignment_graph[t_character](cxx.alignment_graph_builder[t_character] &) except +
