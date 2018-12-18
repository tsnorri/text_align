# Copyright (c) 2018 Tuukka Norri
# This code is licensed under MIT license (see LICENSE for details).

# cython: language_level=3

from cython.operator cimport dereference as deref, preincrement as inc
from libc.stdint cimport uint8_t, int32_t, uint32_t, uint64_t
from libcpp.vector cimport vector
from .alignment_graph_builder cimport alignment_graph_builder, common_node, distinct_node, COMMON, DISTINCT
from .alignment_graph_node import CommonNode, DistinctNode
from .run_aligner cimport run_aligner, run_builder
from .smith_waterman_aligner cimport alignment_context


cdef class AlignmentContext(object):

	def __cinit__(self):
		self.lhs = None
		self.rhs = None

	@property
	def lhs(self):
		return self.lhs

	@lhs.setter
	def lhs(self, string):
		self.lhs = string

	@property
	def rhs(self):
		return self.rhs

	@rhs.setter
	def rhs(self, string):
		self.rhs = string

	cdef void copy_to_list(self, const vector[char32_t] &vec, object dst):
		it = vec.const_begin()
		end = vec.const_end()
		cdef uint32_t cp = 0
		while it != end:
			cp = <uint32_t> deref(it)
			dst.append(chr(cp))
			inc(it)

	cdef object make_common_node(self, const common_node &seg):
		dst = []
		self.copy_to_list(seg.code_points(), dst)
		return CommonNode(dst)

	cdef object make_distinct_node(self, const distinct_node &seg):
		lhs = []
		rhs = []
		self.copy_to_list(seg.code_points_lhs(), lhs)
		self.copy_to_list(seg.code_points_rhs(), rhs)
		return DistinctNode(lhs, rhs)

	cdef object make_alignment_graph_from_segments(self, const vector[unique_ptr[node_base]] &segments):
		retval = []

		# Using a list comprehension with const_iterators is difficult.
		it = segments.const_begin()
		end = segments.const_end()
		while it != end:
			seg = deref(it).get()
			seg_type = <uint8_t> deref(seg).type()
			if <uint8_t> COMMON == seg_type:
				retval.append(self.make_common_node(deref(<const common_node *> seg)))
			elif <uint8_t> DISTINCT == seg_type:
				retval.append(self.make_distinct_node(deref(<const distinct_node *> seg)))
			inc(it)
		return retval


cdef class SmithWatermanAlignmentContext(AlignmentContext):

	def __cinit__(self):
		# In-place construction does not seem to be possible in Cython;
		# hence the use of operator new.
		# Apparently super's implementation is called automatically.
		self.ctx = new alignment_context[int32_t, uint64_t]()

	def __dealloc__(self):
		# super's implementation called automatically,
		# http://cython.readthedocs.io/en/latest/src/userguide/special_methods.html#finalization-method-dealloc
		del self.ctx

	def align(self):
		"""Align self.lhs and self.rhs."""
		run_aligner(deref(self.ctx), self.lhs, self.rhs)

	def make_alignment_graph(self):
		"""Return the alignment as a graph."""
		cdef alignment_graph_builder builder
		run_builder(builder, deref(self.ctx).aligner(), self.lhs, self.rhs)
		return self.make_alignment_graph_from_segments(builder.text_segments())

	@property
	def identity_score(self):
		return self.ctx.aligner().identity_score()

	@identity_score.setter
	def identity_score(self, score):
		self.ctx.aligner().set_identity_score(score)

	@property
	def mismatch_penalty(self):
		return self.ctx.aligner().mismatch_penalty()

	@mismatch_penalty.setter
	def mismatch_penalty(self, score):
		self.ctx.aligner().set_mismatch_penalty(score)

	@property
	def gap_start_penalty(self):
		return self.ctx.aligner().gap_start_penalty()

	@gap_start_penalty.setter
	def gap_start_penalty(self, score):
		self.ctx.aligner().set_gap_start_penalty(score)

	@property
	def gap_penalty(self):
		return self.ctx.aligner().gap_penalty()

	@gap_penalty.setter
	def gap_penalty(self, score):
		self.ctx.aligner().set_gap_penalty(score)

	@property
	def segment_length(self):
		return self.ctx.aligner().segment_length()

	@segment_length.setter
	def segment_length(self, length):
		self.ctx.aligner().set_segment_length(length)

	@property
	def prints_debugging_information(self):
		self.ctx.aligner().prints_debugging_information()

	@prints_debugging_information.setter
	def prints_debugging_information(self, should_print):
		self.ctx.aligner().set_prints_debugging_information(should_print)

	@property
	def alignment_score(self):
		self.ctx.aligner().alignment_score()
