# Copyright (c) 2018 Tuukka Norri
# This code is licensed under MIT license (see LICENSE for details).

# cython: language_level=3


cdef extern from "<text_align/int_vector.hh>" namespace "text_align":
	cdef cppclass int_vector[t_bits, t_word]:
		pass

	cdef cppclass bit_vector:
		pass
