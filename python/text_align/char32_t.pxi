# Copyright (c) 2018 Tuukka Norri
# This code is licensed under MIT license (see LICENSE for details).

# Cython does not declare char32_t. Fortunately this seems to solve the issue.
# The value needs to be cast to something like uint32_t, though.
cdef extern from *:
	ctypedef void char32_t
