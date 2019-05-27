# Copyright (c) 2018-2019 Tuukka Norri
# This code is licensed under MIT license (see LICENSE for details).

# cython: language_level=3

from cython.operator cimport dereference as deref, preincrement as inc
from libc.float cimport FLT_MAX
from libc.stdint cimport uint8_t, int32_t, uint32_t, uint64_t
from libcpp cimport bool
from libcpp.cast cimport dynamic_cast
from libcpp.map cimport map
from libcpp.memory cimport unique_ptr
from libcpp.pair cimport pair
from libcpp.vector cimport vector
from . cimport interface as cxx
from .alignment_context cimport alignment_context_base, alignment_context, scoring_fp_alignment_context
from .alignment_graph_node import CommonNode, DistinctNode
from .cast_bit_vector cimport cast #to_rle_bit_vector
from .run_aligner cimport run_aligner, run_builder, process_alignment_graph

include "char32_t.pxi"


cdef public void handle_common_node(object dst, object node_content):
	node = CommonNode(node_content)
	dst.append(node)

cdef public void handle_distinct_node(object dst, object lhs, object rhs):
	node = DistinctNode(lhs, rhs)
	dst.append(node)


cdef class Aligner(object):
	
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
	
	@property
	def scaled_alignment_score(self):
		"""Scale the alignment score to [0, 1]."""
		# Interpret the identity score as the maximum of the scoring matrix, if one has been given.
		max_ch_score = self.identity_score
		
		# Suppose the given strings have distinct lengths. In this case, the length difference must
		# consist of non-matching characters. If mismatch and gap penalties are non-positive,
		# the non-matching part cannot increase the score. Therefore the maximum is max_ch_score
		# multiplied by the length of the shorter string.
		min_len = min(len(self.lhs), len(self.rhs))
		max_score = float(max_ch_score * min_len)
		
		return self.alignment_score / max_score


cdef class SmithWatermanAlignerBase(Aligner):
	
	cdef alignment_context_base *get_context(self):
		raise NotImplementedError("Not implemented in base class")
	
	cdef convert_rle_vector(self, const cxx.rle_bit_vector[uint32_t] &vec_ref):
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


cdef class SmithWatermanAligner(SmithWatermanAlignerBase):
	
	cdef unique_ptr[alignment_context] ctx
	
	def __cinit__(self):
		# In-place construction does not seem to be possible in Cython;
		# hence the use of operator new.
		# Apparently super’s implementation is called automatically.
		self.ctx.reset(new alignment_context())
	
	cdef alignment_context_base *get_context(self):
		return self.ctx.get()
	
	def align(self):
		"""Align self.lhs and self.rhs."""
		self.check_bit_vectors()
		run_aligner(deref(self.ctx), self.lhs, self.rhs)
	
	def make_alignment_graph(self):
		"""Return the alignment as a graph."""
		retval = []
		cdef unique_ptr[cxx.alignment_graph_builder[char32_t]] builder
		builder.reset(new cxx.alignment_graph_builder[char32_t]())
		run_builder(deref(builder), deref(self.ctx), self.lhs, self.rhs)
		process_alignment_graph(deref(builder), retval)
		return retval
	
	def setup_bit_vectors(self):
		"""Use bit vectors for gaps."""
		self.has_bit_vectors = True
		deref(self.ctx).instantiate_lhs_gaps[cxx.bit_vector]()
		deref(self.ctx).instantiate_rhs_gaps[cxx.bit_vector]()
	
	def setup_run_vectors(self):
		"""Use run vectors for gaps."""
		self.has_bit_vectors = True
		deref(self.ctx).instantiate_lhs_gaps[cxx.rle_bit_vector[uint32_t]]()
		deref(self.ctx).instantiate_rhs_gaps[cxx.rle_bit_vector[uint32_t]]()
	
	# FIXME: come up with a way to move these to an included file.
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
		return deref(self.ctx).get_aligner().prints_debugging_information()
	
	@prints_debugging_information.setter
	def prints_debugging_information(self, should_print):
		deref(self.ctx).get_aligner().set_prints_debugging_information(should_print)
	
	@property
	def alignment_score(self):
		return deref(self.ctx).get_aligner().alignment_score()


cdef class SmithWatermanScoringFpAligner(SmithWatermanAlignerBase):
	
	cdef unique_ptr[scoring_fp_alignment_context] ctx

	def __cinit__(self):
		self.ctx.reset(new scoring_fp_alignment_context())

	cdef alignment_context_base *get_context(self):
		return self.ctx.get()
	
	def similarity(self, object similarity_map):
		cdef float max_score = -FLT_MAX
		cdef float min_score = FLT_MAX
		# For some reason, Cython wants to wrap the result of &deref(self.ctx).get_scores() into
		# a __Pyx_FakeReference, which did not seem to work. Hence the use of get_scores_ptr().
		cdef map[pair[long, long], float] *dst = deref(self.ctx).get_scores_ptr()
		deref(dst).clear()
		for key, value in similarity_map.items():
			assert key is not None
			assert value is not None
			deref(dst).insert((key, value))
			max_score = max(max_score, value)
			min_score = min(min_score, value)
		self.max_similarity = max_score
		self.min_similarity = min_score
	
	similarity = property(None, similarity)
	
	def align(self):
		"""Align self.lhs and self.rhs."""
		self.check_bit_vectors()
		run_aligner(deref(self.ctx), self.lhs, self.rhs)
	
	def make_alignment_graph(self):
		"""Return the alignment as a graph."""
		retval = []
		cdef unique_ptr[cxx.alignment_graph_builder[long]] builder
		builder.reset(new cxx.alignment_graph_builder[long]())
		run_builder(deref(builder), deref(self.ctx), self.lhs, self.rhs)
		process_alignment_graph(deref(builder), retval)
		return retval
	
	def setup_bit_vectors(self):
		"""Use bit vectors for gaps."""
		self.has_bit_vectors = True
		deref(self.ctx).instantiate_lhs_gaps[cxx.bit_vector]()
		deref(self.ctx).instantiate_rhs_gaps[cxx.bit_vector]()
	
	def setup_run_vectors(self):
		"""Use run vectors for gaps."""
		self.has_bit_vectors = True
		deref(self.ctx).instantiate_lhs_gaps[cxx.rle_bit_vector[uint32_t]]()
		deref(self.ctx).instantiate_rhs_gaps[cxx.rle_bit_vector[uint32_t]]()
	
	@property
	def max_similarity(self):
		return deref(self.ctx).get_aligner().identity_score()
	
	@max_similarity.setter
	def max_similarity(self, score):
		deref(self.ctx).get_aligner().set_identity_score(score)
	
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
		return deref(self.ctx).get_aligner().prints_debugging_information()
	
	@prints_debugging_information.setter
	def prints_debugging_information(self, should_print):
		deref(self.ctx).get_aligner().set_prints_debugging_information(should_print)
	
	@property
	def alignment_score(self):
		return deref(self.ctx).get_aligner().alignment_score()
