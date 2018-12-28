/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <text_align/alignment_graph_builder.hh>


namespace text_align { namespace alignment_graph {

	void common_node::to_json(std::ostream &stream) const
	{
		json::write(stream, "type", "common");
		stream << ", ";
		json::write(stream, "text", m_text);
	}
	
	
	void distinct_node::to_json(std::ostream &stream) const
	{
		json::write(stream, "type", "distinct");
		stream << ", ";
		json::write(stream, "lhs", m_lhs);
		stream << ", ";
		json::write(stream, "rhs", m_rhs);
	}
}}


namespace text_align {
	
	void alignment_graph_builder::append_to_common_segment(char32_t const cc)
	{
		typedef alignment_graph::common_node node_type;
		
		if (!m_current_segment || m_current_segment->type() != alignment_graph::node_type::COMMON)
		{
			if (m_current_segment)
				m_text_segments.emplace_back(std::move(m_current_segment));
			
			m_current_segment.reset(new node_type());
		}
		static_cast <node_type &>(*m_current_segment).add_code_point(cc);
	}
	
	void alignment_graph_builder::append_to_distinct_segment(char32_t const lhs, char32_t const rhs)
	{
		typedef alignment_graph::distinct_node node_type;

		libbio_always_assert(! (alignment_graph::GAP_CHARACTER == lhs && alignment_graph::GAP_CHARACTER == rhs));
		
		if (!m_current_segment || m_current_segment->type() != alignment_graph::node_type::DISTINCT)
		{
			if (m_current_segment)
				m_text_segments.emplace_back(std::move(m_current_segment));
			
			m_current_segment.reset(new node_type());
		}
		
		auto &node(static_cast <node_type &>(*m_current_segment));
		
		if (alignment_graph::GAP_CHARACTER != lhs)
			node.add_code_point_lhs(lhs);
		
		if (alignment_graph::GAP_CHARACTER != rhs)
			node.add_code_point_rhs(rhs);
	}
	
	void alignment_graph_builder::finalize_graph()
	{
		if (m_current_segment)
			m_text_segments.emplace_back(std::move(m_current_segment));
	}
}
