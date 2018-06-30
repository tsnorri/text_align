# Copyright (c) 2018 Tuukka Norri
# This code is licensed under MIT license (see LICENSE for details).

from enum import Enum


class NodeType(Enum):
	NONE = 0,
	COMMON = 1,
	DISTINCT = 2


class NodeBase(object):
	@property
	def type():
		return NodeType.NONE
	
	def toJSON(self):
		return {}


class CommonNode(NodeBase):
	def __init__(self, text = None):
		self.text = text
	
	@property
	def type():
		return NodeType.COMMON
	
	def __str__(self):
		return "{text: '%s'}" % "".join(self.text)
	
	def toJSON(self):
		return {"type": "common", "text": "".join(self.text)}


class DistinctNode(NodeBase):
	def __init__(self, lhs = None, rhs = None):
		self.lhs = lhs
		self.rhs = rhs
	
	@property
	def type():
		return NodeType.DISTINCT
	
	def __str__(self):
		return "{lhs: '%s', rhs: '%s'}" % ("".join(self.lhs), "".join(self.rhs))
	
	def toJSON(self):
		return {"type": "distinct", "lhs": "".join(self.lhs), "rhs": "".join(self.rhs)}
