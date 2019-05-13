/*
 * Copyright (c) 2018-2019 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef TEXT_ALIGN_ALIGNMENT_GRAPH_BUILDER_HH
#define TEXT_ALIGN_ALIGNMENT_GRAPH_BUILDER_HH

#include <cstdint>
#include <libbio/algorithm.hh>
#include <libbio/assert.hh>
#include <libbio/int_vector.hh>
#include <range/v3/view/zip.hpp>
#include <text_align/json_serialize.hh>
#include <vector>


namespace text_align { namespace alignment_graph {
	
	enum class node_type : std::uint8_t
	{
		NONE = 0,
		COMMON = 1,
		DISTINCT = 2
	};
	
	
	template <typename t_character>
	struct node_traits
	{
	};
	
	template <>
	struct node_traits <char32_t>
	{
		static constexpr char32_t gap_character() { return 0xffffffff; }
	};
	
	template <>
	struct node_traits <long>
	{
		static constexpr long gap_character() { return LONG_MAX; }
	};
	
	
	struct node_base; // Fwd
	
	struct node_visitor
	{
		// The actual parameter types (common_node, distinct_node) have template parameters,
		// hence the values need to be static_casted manually.
		virtual void visit_common_node(node_base &node) = 0;
		virtual void visit_distinct_node(node_base &node) = 0;
	};
	
	
	struct node_base : public json::serializable
	{
		virtual ~node_base() {}
		virtual enum node_type type() const { return node_type::NONE; }
		virtual void to_json(std::ostream &stream) const override {}
		virtual void visit(node_visitor &visitor) {}
	};
	
	
	template <typename t_character>
	class common_node final : public node_base
	{
	public:
		typedef t_character						character_type;
		typedef std::vector <character_type>	vector_type;

	protected:
		vector_type								m_text;
		
	public:
		static constexpr enum node_type node_type() { return node_type::COMMON; }
		virtual enum node_type type() const override { return this->node_type(); }
		void add_character(character_type const c) { m_text.push_back(c); }
		vector_type const &characters() const { return m_text; }
		virtual void to_json(std::ostream &stream) const override;
		virtual void visit(node_visitor &visitor) override { visitor.visit_common_node(*this); }
	};
	
	
	template <typename t_character>
	class distinct_node final : public node_base
	{
	public:
		typedef t_character						character_type;
		typedef std::vector <character_type>	vector_type;
		
	protected:
		vector_type								m_lhs;
		vector_type								m_rhs;
		
	public:
		static constexpr enum node_type node_type() { return node_type::DISTINCT; }
		virtual enum node_type type() const override { return this->node_type(); }
		void add_character_lhs(character_type const c) { m_lhs.push_back(c); }
		void add_character_rhs(character_type const c) { m_rhs.push_back(c); }
		vector_type const &characters_lhs() const { return m_lhs; }
		vector_type const &characters_rhs() const { return m_rhs; }
		virtual void to_json(std::ostream &stream) const override;
		virtual void visit(node_visitor &visitor) override { visitor.visit_distinct_node(*this); }
	};
}}


namespace text_align {
	
	class alignment_graph_builder_base
	{
	public:
		typedef std::unique_ptr <alignment_graph::node_base>	node_ptr;
		typedef std::vector <node_ptr>							node_ptr_vector;
	
	protected:
		node_ptr_vector	m_text_segments;
		node_ptr		m_current_segment;
			
	protected:
		bool check_current_segment(enum alignment_graph::node_type const nt);
		void finalize_graph();
		
	public:
		node_ptr_vector const &text_segments() const { return m_text_segments; }
	};
	
	
	template <typename t_character>
	class alignment_graph_builder final : public alignment_graph_builder_base
	{
	public:
		typedef t_character										character_type;
		typedef alignment_graph::node_traits <character_type>	traits_type;
		
	protected:
		void append_to_common_segment(character_type const c);
		void append_to_distinct_segment(character_type const lhs, character_type const rhs);
		
	public:
		template <typename t_lhs, typename t_rhs>
		void build_graph(
			t_lhs const &lhs,
			t_rhs const &rhs,
			libbio::bit_vector const &lhs_gaps,
			libbio::bit_vector const &rhs_gaps
		);
	};
}


namespace text_align { namespace alignment_graph {

	template <typename t_character>
	void common_node <t_character>::to_json(std::ostream &stream) const
	{
		json::write(stream, "type", "common");
		stream << ", ";
		json::write(stream, "text", m_text);
	}
	
	
	template <typename t_character>
	void distinct_node <t_character>::to_json(std::ostream &stream) const
	{
		json::write(stream, "type", "distinct");
		stream << ", ";
		json::write(stream, "lhs", m_lhs);
		stream << ", ";
		json::write(stream, "rhs", m_rhs);
	}
}}


namespace text_align {

	template <typename t_character>
	void alignment_graph_builder <t_character>::append_to_common_segment(character_type const cc)
	{
		typedef alignment_graph::common_node <t_character> node_type;
		if (check_current_segment(node_type::node_type()))
			m_current_segment.reset(new node_type());
		static_cast <node_type &>(*this->m_current_segment).add_character(cc);
	}
	
	
	template <typename t_character>
	void alignment_graph_builder <t_character>::append_to_distinct_segment(
		character_type const lhs,
		character_type const rhs
	)
	{
		typedef alignment_graph::distinct_node <t_character> node_type;
		if (check_current_segment(node_type::node_type()))
			m_current_segment.reset(new node_type());
		auto &node(static_cast <node_type &>(*this->m_current_segment));
		
		if (traits_type::gap_character() != lhs)
			node.add_character_lhs(lhs);
		
		if (traits_type::gap_character() != rhs)
			node.add_character_rhs(rhs);
	}
	
	
	template <typename t_character>
	template <typename t_lhs, typename t_rhs>
	void alignment_graph_builder <t_character>::build_graph(
		t_lhs const &lhs,
		t_rhs const &rhs,
		libbio::bit_vector const &lhs_gaps,
		libbio::bit_vector const &rhs_gaps
	)
	{
		auto lhs_it(lhs.begin());
		auto rhs_it(rhs.begin());
		auto const lhs_end(lhs.end());
		auto const rhs_end(rhs.end());
		
		// At a given aligned index, if both lhs and rhs have the same character,
		// append it to a graph node that represents a common segment.
		// Otherwise, append the characters to a graph node that represents
		// distinct segments.
		libbio_assert(lhs_gaps.size() == rhs_gaps.size());
		for (auto const &tup : ranges::view::zip(lhs_gaps, rhs_gaps))
		{
			bool const lhs_has_gap(std::get <0>(tup));
			bool const rhs_has_gap(std::get <1>(tup));
			
			libbio_always_assert(! (lhs_has_gap && rhs_has_gap));
			if (! (lhs_has_gap || rhs_has_gap))
			{
				libbio_always_assert(lhs_it != lhs_end);
				libbio_always_assert(rhs_it != rhs_end);
				
				auto const lhsc(*lhs_it++);
				auto const rhsc(*rhs_it++);
				
				if (libbio::is_equal(lhsc, rhsc))
					append_to_common_segment(lhsc);
				else
					append_to_distinct_segment(lhsc, rhsc);
			}
			else if (lhs_has_gap)
			{
				libbio_always_assert(rhs_it != rhs_end);
				auto const rhsc(*rhs_it++);
				append_to_distinct_segment(traits_type::gap_character(), rhsc);
			}
			else if (rhs_has_gap)
			{
				libbio_always_assert(lhs_it != lhs_end);
				auto const lhsc(*lhs_it++);
				append_to_distinct_segment(lhsc, traits_type::gap_character());
			}
		}
		
		finalize_graph();
	}
}

#endif
