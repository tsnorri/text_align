/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef TEXT_ALIGN_SMITH_WATERMAN_ALIGNMENT_CONTEXT_PV_HH
#define TEXT_ALIGN_SMITH_WATERMAN_ALIGNMENT_CONTEXT_PV_HH

#include <memory>
#include <text_align/bit_vector_interface.hh>
#include <text_align/smith_waterman/alignment_context.hh>


namespace text_align { namespace smith_waterman {
	
	template <typename t_score, typename t_word>
	class alignment_context_pv final : public alignment_context_tpl <
		alignment_context_pv <t_score, t_word>,
		t_score,
		t_word
	>
	{
	protected:
		typedef alignment_context_pv <t_score, t_word>				self_type;
		typedef alignment_context_tpl <self_type, t_score, t_word>	superclass;

	public:
		typedef bit_vector_interface <t_word>						bit_vector_type;
		typedef	typename superclass::aligner_type					aligner_type;
		friend aligner_type;
		
	protected:
		std::unique_ptr <bit_vector_type>	m_lhs_gaps;
		std::unique_ptr <bit_vector_type>	m_rhs_gaps;
		
	public:
		using superclass::superclass;
		
		template <typename t_bv> void instantiate_lhs_gaps() { m_lhs_gaps.reset(new bit_vector_wrapper <t_word, t_bv>); }
		template <typename t_bv> void instantiate_rhs_gaps() { m_rhs_gaps.reset(new bit_vector_wrapper <t_word, t_bv>); }
		
		bit_vector_type &lhs_gaps() { return *m_lhs_gaps; }
		bit_vector_type &rhs_gaps() { return *m_rhs_gaps; }
		bit_vector_type const &lhs_gaps() const { return *m_lhs_gaps; }
		bit_vector_type const &rhs_gaps() const { return *m_rhs_gaps; }
		
	protected:
		void push_lhs(bool flag, std::size_t count) { m_lhs_gaps->push_back(flag, count); }
		void push_rhs(bool flag, std::size_t count) { m_rhs_gaps->push_back(flag, count); }
		void clear_gaps() { m_lhs_gaps->clear(); m_rhs_gaps->clear(); }
		void reverse_gaps() { m_lhs_gaps->reverse(); m_rhs_gaps->reverse(); }
	};
}}

#endif
