# Copyright (c) 2018 Tuukka Norri
# This code is licensed under MIT license (see LICENSE for details).

# cython: language_level=3

from . cimport interface as cxx


cdef extern from "cast_bit_vector.hh" namespace "text_align":
	cdef cppclass cast[t_word]:
		const cxx.rle_bit_vector[t_word] &to_rle_bit_vector_fr[t_bv](t_bv &) except +
