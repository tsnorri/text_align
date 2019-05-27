# Copyright (c) 2018-2019 Tuukka Norri
# This code is licensed under MIT license (see LICENSE for details).

from enum import Enum


class NodeType(Enum):
	NONE = 0,
	COMMON = 1,
	DISTINCT = 2


def stringify_list(l):
	return "".join([str(x) for x in l])


class NodeBase(object):
	@property
	def type(self):
		return NodeType.NONE
	
	def toJSON(self):
		return {}
	
	def visit(self, visitor, user_info = None):
		raise RuntimeError("Not implemented in base class")


class CommonNode(NodeBase):
	def __init__(self, text = None):
		self.text = text
	
	@property
	def type(self):
		return NodeType.COMMON
	
	@property
	def max_length(self):
		return len(self.text)
	
	def __str__(self):
		return "{text: '%s'}" % stringify_list(self.text)
	
	def toJSON(self):
		return {"type": "common", "text": stringify_list(self.text)}
	
	def visit(self, visitor, user_info = None):
		return visitor.visit_common_alignment_node(self, user_info)


class DistinctNode(NodeBase):
	def __init__(self, lhs = None, rhs = None):
		self.lhs = lhs
		self.rhs = rhs
	
	@property
	def type(self):
		return NodeType.DISTINCT
	
	@property
	def max_length(self):
		lhs_len = len(self.lhs)
		rhs_len = len(self.rhs)
		if lhs_len < rhs_len:
			return rhs_len
		else:
			return lhs_len
	
	def __str__(self):
		return "{lhs: '%s', rhs: '%s'}" % (stringify_list(self.lhs), stringify_list(self.rhs))
	
	def toJSON(self):
		return {"type": "distinct", "lhs": stringify_list(self.lhs), "rhs": stringify_list(self.rhs)}
	
	def visit(self, visitor, user_info = None):
		return visitor.visit_distinct_alignment_node(self, user_info)

