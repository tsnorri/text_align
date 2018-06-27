# Copyright (c) 2018 Tuukka Norri
# This code is licensed under MIT license (see LICENSE for details).


cdef extern from "run_aligner.hh" namespace "text_align":
	cdef void run_aligner[t_context](t_context, object, object)
