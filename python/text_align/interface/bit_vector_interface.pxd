# Copyright (c) 2018 Tuukka Norri
# This code is licensed under MIT license (see LICENSE for details).

# cython: language_level=3

from libc.stddef cimport size_t


cdef extern from "<text_align/bit_vector_interface.hh>" namespace "text_align":

	cdef cppclass bit_vector_interface[t_word]:
		void clear() except +
		void push_back(t_word, size_t) except +
		void reverse() except +
