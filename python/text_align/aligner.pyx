# Copyright (c) 2018-2019 Tuukka Norri
# This code is licensed under MIT license (see LICENSE for details).

# cython: language_level=3

from cython.operator cimport dereference as deref, preincrement as inc
from libc.stdint cimport uint8_t, int32_t, uint32_t, uint64_t
from libcpp cimport bool
from libcpp.cast cimport dynamic_cast
from libcpp.map cimport map
from libcpp.memory cimport unique_ptr
from libcpp.pair cimport pair
from libcpp.vector cimport vector
from .alignment_graph_builder cimport alignment_graph_builder, common_node, distinct_node, COMMON, DISTINCT
from .alignment_graph_node import CommonNode, DistinctNode
from .cast_bit_vector cimport cast #to_rle_bit_vector
from .int_vector cimport bit_vector
from .python_alignment_context cimport python_alignment_context_base, python_alignment_context, python_scoring_fp_alignment_context
from .rle_bit_vector cimport rle_bit_vector
from .run_aligner cimport run_aligner, run_builder, process_alignment_graph

include "char32_t.pxi"


cdef public void handle_common_node(object dst, object node_content):
	node = CommonNode(node_content)
	dst.append(node)

cdef public void handle_distinct_node(object dst, object lhs, object rhs):
	node = DistinctNode(lhs, rhs)
	dst.append(node)


cdef class AlignmentContext(object):
	
	cdef object lhs
	cdef object rhs
	cdef bool has_bit_vectors

	def __cinit__(self):
		self.lhs = None
		self.rhs = None
		self.has_bit_vectors = False

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
	
	# FIXME: write me.
	@property
	def scaled_alignment_score(self):
		return 0.0


cdef class SmithWatermanAlignmentContextBase(AlignmentContext):

	cdef python_alignment_context_base *get_context(self):
		raise NotImplementedError("Not implemented in base class")
	
	cdef convert_rle_vector(self, const rle_bit_vector[uint32_t] &vec_ref):
		retval = []
		vec = vec_ref.to_run_vector()
		it = vec.const_begin()
		end = vec.const_end()
		while it != end:
			retval.append(deref(it))
			inc(it)
		return vec_ref.starts_with_zero(), retval
	
	def check_bit_vectors(self):
		if not self.has_bit_vectors:
			raise RuntimeError("Bit vectors not initialized")
	
	def make_lhs_runs(self):
		"""Return the alignment as a list of runs."""
		cdef cast[uint32_t] c
		return self.convert_rle_vector(c.to_rle_bit_vector_fr(deref(self.get_context()).lhs_gaps()))
	
	def make_rhs_runs(self):
		"""Return the alignment as a list of runs."""
		cdef cast[uint32_t] c
		return self.convert_rle_vector(c.to_rle_bit_vector_fr(deref(self.get_context()).rhs_gaps()))


cdef class SmithWatermanAlignmentContext(SmithWatermanAlignmentContextBase):
	
	cdef unique_ptr[python_alignment_context] ctx
	
	def __cinit__(self):
		# In-place construction does not seem to be possible in Cython;
		# hence the use of operator new.
		# Apparently super’s implementation is called automatically.
		self.ctx.reset(new python_alignment_context())

	cdef python_alignment_context_base *get_context(self):
		return self.ctx.get()
		
	def align(self):
		"""Align self.lhs and self.rhs."""
		self.check_bit_vectors()
		run_aligner(deref(self.ctx), self.lhs, self.rhs)
	
	def make_alignment_graph(self):
		"""Return the alignment as a graph."""
		cdef unique_ptr[alignment_graph_builder[char32_t]] builder
		run_builder(deref(builder), deref(self.ctx), self.lhs, self.rhs)
		return process_alignment_graph(deref(builder))
	
	def setup_bit_vectors(self):
		"""Use bit vectors for gaps."""
		self.has_bit_vectors = True
		deref(self.ctx).instantiate_lhs_gaps[bit_vector]()
		deref(self.ctx).instantiate_rhs_gaps[bit_vector]()
	
	def setup_run_vectors(self):
		"""Use run vectors for gaps."""
		self.has_bit_vectors = True
		deref(self.ctx).instantiate_lhs_gaps[rle_bit_vector[uint32_t]]()
		deref(self.ctx).instantiate_rhs_gaps[rle_bit_vector[uint32_t]]()
	
	@property
	def identity_score(self):
		return deref(self.ctx).get_aligner().identity_score()
	
	@identity_score.setter
	def identity_score(self, score):
		deref(self.ctx).get_aligner().set_identity_score(score)
	
	@property
	def mismatch_penalty(self):
		return deref(self.ctx).get_aligner().mismatch_penalty()
	
	@mismatch_penalty.setter
	def mismatch_penalty(self, score):
		deref(self.ctx).get_aligner().set_mismatch_penalty(score)
	
	# FIXME: come up with a way to move these to an included file.
	@property
	def gap_start_penalty(self):
		return deref(self.ctx).get_aligner().gap_start_penalty()
	
	@gap_start_penalty.setter
	def gap_start_penalty(self, score):
		deref(self.ctx).get_aligner().set_gap_start_penalty(score)
	
	@property
	def gap_penalty(self):
		return deref(self.ctx).get_aligner().gap_penalty()
	
	@gap_penalty.setter
	def gap_penalty(self, score):
		deref(self.ctx).get_aligner().set_gap_penalty(score)
	
	@property
	def segment_length(self):
		return deref(self.ctx).get_aligner().segment_length()
	
	@segment_length.setter
	def segment_length(self, length):
		deref(self.ctx).get_aligner().set_segment_length(length)
	
	@property
	def prints_debugging_information(self):
		deref(self.ctx).get_aligner().prints_debugging_information()
	
	@prints_debugging_information.setter
	def prints_debugging_information(self, should_print):
		deref(self.ctx).get_aligner().set_prints_debugging_information(should_print)
	
	@property
	def alignment_score(self):
		deref(self.ctx).get_aligner().alignment_score()


cdef class SmithWatermanScoringFpAlignmentContext(SmithWatermanAlignmentContextBase):
	
	cdef unique_ptr[python_scoring_fp_alignment_context] ctx

	def __cinit__(self):
		self.ctx.reset(new python_scoring_fp_alignment_context())

	cdef python_alignment_context_base *get_context(self):
		return self.ctx.get()
	
	def set_similarity(self, object similarity_map):
		cdef map[pair[uint32_t, uint32_t], float] *dst = &deref(self.ctx).get_scores()
		deref(dst).clear()
		for key, value in similarity_map.items():
			deref(dst).insert((key, value))
	
	def align(self):
		"""Align self.lhs and self.rhs."""
		self.check_bit_vectors()
		run_aligner(deref(self.ctx), self.lhs, self.rhs)
	
	def make_alignment_graph(self):
		"""Return the alignment as a graph."""
		cdef unique_ptr[alignment_graph_builder[long]] builder
		run_builder(deref(builder), deref(self.ctx), self.lhs, self.rhs)
		return process_alignment_graph(deref(builder))
	
	def setup_bit_vectors(self):
		"""Use bit vectors for gaps."""
		deref(self.ctx).instantiate_lhs_gaps[bit_vector]()
		deref(self.ctx).instantiate_rhs_gaps[bit_vector]()
	
	def setup_run_vectors(self):
		"""Use run vectors for gaps."""
		deref(self.ctx).instantiate_lhs_gaps[rle_bit_vector[uint32_t]]()
		deref(self.ctx).instantiate_rhs_gaps[rle_bit_vector[uint32_t]]()
	
	# FIXME: come up with a way to move these to an included file.
	@property
	def gap_start_penalty(self):
		return deref(self.ctx).get_aligner().gap_start_penalty()
	
	@gap_start_penalty.setter
	def gap_start_penalty(self, score):
		deref(self.ctx).get_aligner().set_gap_start_penalty(score)
	
	@property
	def gap_penalty(self):
		return deref(self.ctx).get_aligner().gap_penalty()
	
	@gap_penalty.setter
	def gap_penalty(self, score):
		deref(self.ctx).get_aligner().set_gap_penalty(score)
	
	@property
	def segment_length(self):
		return deref(self.ctx).get_aligner().segment_length()
	
	@segment_length.setter
	def segment_length(self, length):
		deref(self.ctx).get_aligner().set_segment_length(length)
	
	@property
	def prints_debugging_information(self):
		deref(self.ctx).get_aligner().prints_debugging_information()
	
	@prints_debugging_information.setter
	def prints_debugging_information(self, should_print):
		deref(self.ctx).get_aligner().set_prints_debugging_information(should_print)
	
	@property
	def alignment_score(self):
		deref(self.ctx).get_aligner().alignment_score()
