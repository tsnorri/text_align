/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef TEXT_ALIGN_SMITH_WATERMAN_ALIGNMENT_CONTEXT_HH
#define TEXT_ALIGN_SMITH_WATERMAN_ALIGNMENT_CONTEXT_HH

#include <text_align/smith_waterman/aligner.hh>


namespace text_align { namespace smith_waterman {
	
	// Hold both an aligner and an IO context.
	template <
		typename t_self,	// CRTP
		typename t_score,
		typename t_word
	>
	class alignment_context_tpl
	{
	public:
		typedef aligner <t_score, t_word, t_self>	aligner_type;
		friend aligner_type;
		
	protected:
		aligner_type				m_aligner;
		boost::asio::io_context		m_ctx;
		
	public:
		alignment_context_tpl():
			m_aligner(m_ctx, static_cast <t_self &>(*this)),
			m_ctx()
		{
		}
		
		alignment_context_tpl(std::size_t const num_threads):
			m_aligner(m_ctx, static_cast <t_self &>(*this)),
			m_ctx(num_threads)
		{
		}
		
		aligner_type &get_aligner() { return m_aligner; }
		aligner_type const &get_aligner() const { return m_aligner; }
		
		boost::asio::io_context &get_execution_context() { return m_ctx; }
		boost::asio::io_context const &get_execution_context() const { return m_ctx; }
		
		void run() { m_ctx.run(); }
		void restart() { m_ctx.restart(); }
		bool stopped() const { return m_ctx.stopped(); }
		
	protected:
		void finish(aligner_base &aligner) { m_ctx.stop(); }
	};
	
	
	template <typename t_score, typename t_word, typename t_bit_vector>
	class alignment_context final : public alignment_context_tpl <
		alignment_context <t_score, t_word, t_bit_vector>,
		t_score,
		t_word
	>
	{
	protected:
		typedef alignment_context <t_score, t_word, t_bit_vector>	self_type;
		typedef alignment_context_tpl <self_type, t_score, t_word>	superclass;
		
	public:
		typedef	t_bit_vector										bit_vector_type;
		typedef	typename superclass::aligner_type					aligner_type;
		friend aligner_type;
		
	protected:
		t_bit_vector	m_lhs_gaps;
		t_bit_vector	m_rhs_gaps;
		
	public:
		using superclass::superclass;
		
		bit_vector_type &lhs_gaps() { return m_lhs_gaps; }
		bit_vector_type &rhs_gaps() { return m_rhs_gaps; }
		bit_vector_type const &lhs_gaps() const { return m_lhs_gaps; }
		bit_vector_type const &rhs_gaps() const { return m_rhs_gaps; }
		
	protected:
		void push_lhs(bool flag, std::size_t count) { m_lhs_gaps.push_back(flag, count); }
		void push_rhs(bool flag, std::size_t count) { m_rhs_gaps.push_back(flag, count); }
		void clear_gaps() { m_lhs_gaps.clear(); m_rhs_gaps.clear(); }
		void reverse_gaps() { m_lhs_gaps.reverse(); m_rhs_gaps.reverse(); }
	};
}}

#endif

