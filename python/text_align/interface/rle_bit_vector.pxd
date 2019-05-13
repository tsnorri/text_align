# Copyright (c) 2018 Tuukka Norri
# This code is licensed under MIT license (see LICENSE for details).

# cython: language_level=3

from libcpp cimport bool
from libcpp.vector cimport vector


cdef extern from "<libbio/rle_bit_vector.hh>" namespace "libbio":
	cdef cppclass rle_bit_vector[t_count]:
		bool starts_with_zero() except +
		vector[t_count] to_run_vector() except +
