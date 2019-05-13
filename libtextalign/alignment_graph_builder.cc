/*
 * Copyright (c) 2018-2019 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <text_align/alignment_graph_builder.hh>


namespace text_align {
	
	bool alignment_graph_builder_base::check_current_segment(
		enum alignment_graph::node_type const nt
	)
	{
		if (!m_current_segment || m_current_segment->type() != nt)
		{
			if (m_current_segment)
				m_text_segments.emplace_back(std::move(m_current_segment));
			
			return true;
		}
		
		return false;
	}
	
	
	void alignment_graph_builder_base::finalize_graph()
	{
		if (m_current_segment)
			m_text_segments.emplace_back(std::move(m_current_segment));
	}
}
