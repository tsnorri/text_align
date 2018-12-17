# Copyright (c) 2018 Tuukka Norri
# This code is licensed under MIT license (see LICENSE for details).

# cython: language_level=3

from .alignment_graph_builder cimport alignment_graph_builder


cdef extern from "run_aligner.hh" namespace "text_align":
	cdef void run_aligner[t_context](t_context &, object, object) except +
	cdef void run_builder[t_aligner](alignment_graph_builder &, const t_aligner &, object, object) except +
