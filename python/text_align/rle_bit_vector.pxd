# Copyright (c) 2018 Tuukka Norri
# This code is licensed under MIT license (see LICENSE for details).

# cython: language_level=3

from libcpp cimport bool
from libcpp.vector cimport vector


cdef extern from "<text_align/rle_bit_vector.hh>" namespace "text_align":
	cdef cppclass rle_bit_vector[t_count]:
		bool starts_with_zero() except +
		vector[t_count] to_run_vector() except +
