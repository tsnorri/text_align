# Copyright (c) 2018 Tuukka Norri
# This code is licensed under MIT license (see LICENSE for details).

from libc.stdint cimport int32_t
from run_aligner cimport run_aligner
from alignment_graph_builder import AlignmentGraphBuilder
from smith_waterman_aligner cimport smith_waterman_alignment_context


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
	def gap_penalty(self):
		return self.ctx.aligner().gap_penalty()
	
	@gap_penalty.setter
	def gap_penalty(self, score):
		self.ctx.aligner().set_gap_penalty(score)
	
	@property
	def segment_length(self):
		return self.ctx.aligner().segment_length()
	
	@segment_length.setter
	def segment_length(self, score):
		self.ctx.aligner().set_segment_length(score)
		
	@property
	def prints_debugging_information(self):
		self.ctx.aligner().prints_debugging_information()
	
	@prints_debugging_information.setter
	def prints_debugging_information(self, should_print):
		self.ctx.aligner().set_prints_debugging_information(should_print)
	
	@property
	def alignment_score(self):
		self.ctx.aligner().alignment_score()


cdef class SmithWatermanAlignmentContext(AlignmentContext):
	
	def __cinit__(self):
		# In-place construction does not seem to be possible in Cython;
		# hence the use of operator new.
		# Apparently super's implementation is called automatically.
		self.ctx = new smith_waterman_alignment_context[int32_t]()
	
	def __dealloc__(self):
		# super's implementation called automatically,
		# http://cython.readthedocs.io/en/latest/src/userguide/special_methods.html#finalization-method-dealloc
		del self.ctx
	
	def align(self):
		"""Align self.lhs and self.rhs."""
		run_aligner(self.ctx[0], self.lhs, self.rhs)
	
	def make_alignment_graph(self):
		"""Return the optimal alignment as a graph."""
		builder = AlignmentGraphBuilder()
		return builder.build_graph(
			self.lhs,
			self.rhs,
			self.ctx[0].aligner().lhs_gaps(),
			self.ctx[0].aligner().rhs_gaps()
		)
