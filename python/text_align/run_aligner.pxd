# Copyright (c) 2018-2019 Tuukka Norri
# This code is licensed under MIT license (see LICENSE for details).

# cython: language_level=3

from .alignment_graph_builder cimport alignment_graph_builder


cdef extern from "run_aligner.hh" namespace "text_align":
	cdef void run_aligner[t_context](t_context &, object, object) except +
	cdef void run_builder[t_character, t_aligner](alignment_graph_builder[t_character] &, const t_aligner &, object, object) except +
	cdef object process_alignment_graph[t_character](alignment_graph_builder[t_character] &) except +
