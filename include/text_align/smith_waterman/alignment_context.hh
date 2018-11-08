/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef TEXT_ALIGN_SMITH_WATERMAN_ALIGNMENT_CONTEXT_HH
#define TEXT_ALIGN_SMITH_WATERMAN_ALIGNMENT_CONTEXT_HH

#include <text_align/smith_waterman/aligner.hh>


namespace text_align { namespace smith_waterman {
	
	// Hold both an aligner and an IO context.
	template <typename t_score, template <typename, typename> class t_aligner>
	class alignment_context_tpl
	{
	protected:
		typedef t_aligner <t_score, alignment_context_tpl>	aligner_type;
		
		friend aligner_type;
		
	protected:
		aligner_type				m_aligner;
		boost::asio::thread_pool	m_ctx{};
		
	public:
		alignment_context_tpl():
			m_aligner(m_ctx, *this)
		{
		}
		
		aligner_type &aligner() { return m_aligner; }
		aligner_type const &aligner() const { return m_aligner; }

		boost::asio::thread_pool &execution_context() { return m_ctx; }
		boost::asio::thread_pool const &execution_context() const { return m_ctx; }
		
		void run() { m_ctx.join(); }
		
	protected:
		void finish(aligner_type &aligner) { m_ctx.stop(); }
	};
	
	
	template <typename t_score>
	using alignment_context = alignment_context_tpl <t_score, aligner>;
}}

#endif

