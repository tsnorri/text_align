/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef TEXT_ALIGN_ALIGNMENT_GRAPH_BUILDER_HH
#define TEXT_ALIGN_ALIGNMENT_GRAPH_BUILDER_HH

#include <boost/range/combine.hpp>
#include <cstdint>
#include <text_align/assert.hh>
#include <text_align/json_serialize.hh>
#include <vector>


namespace text_align { namespace alignment_graph {
	
	enum class node_type : std::uint8_t
	{
		NONE = 0,
		COMMON = 1,
		DISTINCT = 2
	};
	
	enum
	{
		GAP_CHARACTER = 0xffffffff
	};
	

	class node_base : public json::serializable
	{
	public:
		typedef std::vector <char32_t> code_point_vector;
		
	public:
		virtual ~node_base() {}
		virtual enum node_type type() const { return node_type::NONE; }
		virtual void to_json(std::ostream &stream) const override {}
	};
	
	
	class common_node : public node_base
	{
		node_base::code_point_vector m_text;
		
	public:
		virtual enum node_type type() const override { return node_type::COMMON; }
		void add_code_point(char32_t const c) { m_text.push_back(c); }
		node_base::code_point_vector const &code_points() const { return m_text; }
		
		virtual void to_json(std::ostream &stream) const override;
	};
	
	
	class distinct_node : public node_base
	{
	protected:
		node_base::code_point_vector	m_lhs;
		node_base::code_point_vector	m_rhs;
		
	public:
		virtual enum node_type type() const override { return node_type::DISTINCT; }
		void add_code_point_lhs(char32_t const c) { m_lhs.push_back(c); }
		void add_code_point_rhs(char32_t const c) { m_rhs.push_back(c); }
		node_base::code_point_vector const &code_points_lhs() const { return m_lhs; }
		node_base::code_point_vector const &code_points_rhs() const { return m_rhs; }
		virtual void to_json(std::ostream &stream) const override;
	};
}}
	
	
namespace text_align {

	class alignment_graph_builder
	{
	public:
		typedef std::unique_ptr <alignment_graph::node_base>	node_ptr;
		typedef std::vector <node_ptr>							node_ptr_vector;
		typedef std::vector <bool>								gap_vector;
		
	protected:
		node_ptr_vector	m_text_segments;
		node_ptr		m_current_segment;
		
	protected:
		void append_to_common_segment(char32_t const cc);
		void append_to_distinct_segment(char32_t const lhs, char32_t const rhs);
		void finalize_graph();
		
	public:
		alignment_graph_builder() = default;
		
		node_ptr_vector const &text_segments() const { return m_text_segments; }
		
		template <typename t_lhs, typename t_rhs>
		void build_graph(t_lhs const &lhs, t_rhs const &rhs, gap_vector const &lhs_gaps, gap_vector const &rhs_gaps);
	};
}


namespace text_align {
	
	template <typename t_lhs, typename t_rhs>
	void alignment_graph_builder::build_graph(t_lhs const &lhs, t_rhs const &rhs, gap_vector const &lhs_gaps, gap_vector const &rhs_gaps)
	{
		auto lhs_it(lhs.begin());
		auto rhs_it(rhs.begin());
		auto const lhs_end(lhs.end());
		auto const rhs_end(rhs.end());
		
		// At a given aligned index, if both lhs and rhs have the same character,
		// append it to a graph node that represents a common segment.
		// Otherwise, append the characters to a graph node that represents
		// distinct segments.
		for (auto const &zipped : boost::combine(lhs_gaps, rhs_gaps))
		{
			bool lhs_has_gap(false), rhs_has_gap(false);
			boost::tie(lhs_has_gap, rhs_has_gap) = zipped;
			
			always_assert(! (lhs_has_gap && rhs_has_gap));
			if (! (lhs_has_gap || rhs_has_gap))
			{
				always_assert(lhs_it != lhs_end);
				always_assert(rhs_it != rhs_end);
				
				auto const lhsc(*lhs_it++);
				auto const rhsc(*rhs_it++);
				
				if (is_equal(lhsc, rhsc))
					append_to_common_segment(lhsc);
				else
					append_to_distinct_segment(lhsc, rhsc);
			}
			else if (lhs_has_gap)
			{
				always_assert(rhs_it != rhs_end);
				auto const rhsc(*rhs_it++);
				append_to_distinct_segment(alignment_graph::GAP_CHARACTER, rhsc);
			}
			else if (rhs_has_gap)
			{
				always_assert(lhs_it != lhs_end);
				auto const lhsc(*lhs_it++);
				append_to_distinct_segment(lhsc, alignment_graph::GAP_CHARACTER);
			}
		}
		
		finalize_graph();
	}
}

#endif
