# Copyright (c) 2018 Tuukka Norri
# This code is licensed under MIT license (see LICENSE for details).

from enum import Enum


class NodeType(Enum):
	NONE = 0,
	COMMON = 1,
	DISTINCT = 2


class NodeBase(object):
	def __init__(self):
		self.type = NodeType.NONE
	
	def toJSON(self):
		return {}


class CommonNode(NodeBase):
	def __init__(self):
		self.type = NodeType.COMMON
		self.text = []
	
	def __str__(self):
		return "{text: '%s'}" % "".join(self.text)
	
	def toJSON(self):
		return {"type": "common", "text": "".join(self.text)}


class DistinctNode(NodeBase):
	def __init__(self):
		self.type = NodeType.DISTINCT
		self.lhs = []
		self.rhs = []
	
	def __str__(self):
		return "{lhs: '%s', rhs: '%s'}" % ("".join(self.lhs), "".join(self.rhs))
	
	def toJSON(self):
		return {"type": "distinct", "lhs": "".join(self.lhs), "rhs": "".join(self.rhs)}


class AlignmentGraphBuilder(object):
	
	def __cinit__(self):
		self.text_segments = None
		self.current_segment = None
	
	def append_to_common_segment(self, cc):
		if self.current_segment.type is not NodeType.COMMON:
			self.text_segments.append(self.current_segment)
			self.current_segment = CommonNode()
		self.current_segment.text.append(cc)
	
	def append_to_distinct_segment(self, lhsc, rhsc):
		if self.current_segment.type is not NodeType.DISTINCT:
			self.text_segments.append(self.current_segment)
			self.current_segment = DistinctNode()
			
		if lhsc is not None:
			self.current_segment.lhs.append(lhsc)
		if rhsc is not None:
			self.current_segment.rhs.append(rhsc)
	
	def build_graph(self, lhs, rhs, lhs_gaps, rhs_gaps):
		self.text_segments = []
		self.current_segment = NodeBase()
		lhs_it = iter(lhs)
		rhs_it = iter(rhs)
		
		# At a given aligned index, if both lhs and rhs have the same character,
		# append it to a graph node that represents a common segment.
		# Otherwise, append the characters to a graph node that represents
		# distinct segments.
		for lhs_has_gap, rhs_has_gap in zip(lhs_gaps, rhs_gaps):
			assert not (lhs_has_gap and rhs_has_gap)
			if not (lhs_has_gap or rhs_has_gap):
				lhsc = next(lhs_it)
				rhsc = next(rhs_it)
				
				if lhsc == rhsc:
					self.append_to_common_segment(lhsc)
				else:
					self.append_to_distinct_segment(lhsc, rhsc)
					
			elif lhs_has_gap:
				rhsc = next(rhs_it)
				self.append_to_distinct_segment(None, rhsc)
			elif rhs_has_gap:
				lhsc = next(lhs_it)
				self.append_to_distinct_segment(lhsc, None)
		
		return self.text_segments[1:]
