/*
 * Copyright (c) 2019 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef TEXT_ALIGN_PYTHON_ALIGNMENT_CONTEXT_HH
#define TEXT_ALIGN_PYTHON_ALIGNMENT_CONTEXT_HH

#include <text_align/smith_waterman/aligner.hh>


namespace text_align { namespace detail {
	struct no_op {};
}}


namespace text_align {
	
	struct unit_alignment_scorer_base
	{
		typedef std::uint32_t						index_type;
		typedef std::pair <index_type, index_type>	key_type;
		typedef float								value_type;
		typedef std::map <key_type, value_type>		map_type;
		
		virtual map_type &get_scores() { throw std::runtime_error("Not implemented"); }
	};
	
	
	class python_alignment_context_base: public virtual unit_alignment_scorer_base
	{
	public:
		typedef std::uint64_t						word_type;
		typedef	bit_vector_interface <word_type>	bit_vector_type;
		
	protected:
		boost::asio::io_context						m_ctx;
		std::unique_ptr <bit_vector_type>			m_lhs_gaps;
		std::unique_ptr <bit_vector_type>			m_rhs_gaps;
		
	public:
		python_alignment_context_base():
			m_ctx()
		{
		}
		
		python_alignment_context_base(std::size_t const num_threads):
			m_ctx(num_threads)
		{
		}
		
		boost::asio::io_context &get_execution_context() { return m_ctx; }
		boost::asio::io_context const &get_execution_context() const { return m_ctx; }
		
		void run() { m_ctx.run(); }
		void restart() { m_ctx.restart(); }
		bool stopped() const { return m_ctx.stopped(); }
		
		template <typename t_bv> void instantiate_lhs_gaps() { m_lhs_gaps.reset(new bit_vector_wrapper <word_type, t_bv>); }
		template <typename t_bv> void instantiate_rhs_gaps() { m_rhs_gaps.reset(new bit_vector_wrapper <word_type, t_bv>); }
		
		bit_vector_type &lhs_gaps() { return *m_lhs_gaps; }
		bit_vector_type &rhs_gaps() { return *m_rhs_gaps; }
		bit_vector_type const &lhs_gaps() const { return *m_lhs_gaps; }
		bit_vector_type const &rhs_gaps() const { return *m_rhs_gaps; }
		
	public:
		void push_lhs(bool flag, std::size_t count) { m_lhs_gaps->push_back(flag, count); }
		void push_rhs(bool flag, std::size_t count) { m_rhs_gaps->push_back(flag, count); }
		void clear_gaps() { m_lhs_gaps->clear(); m_rhs_gaps->clear(); }
		void reverse_gaps() { m_lhs_gaps->reverse(); m_rhs_gaps->reverse(); }
		
		// Aligner delegate.
		void finish(smith_waterman::aligner_base &aligner) { m_ctx.stop(); }
	};
	
	
	template <template <typename, typename, typename> typename t_aligner, typename t_score, typename t_base>
	class python_alignment_context_tpl final : public python_alignment_context_base, public t_base
	{
	public:
		typedef t_aligner <t_score, std::uint64_t, python_alignment_context_tpl> aligner_type;
		
	protected:
		aligner_type	m_aligner;
		
	public:
		python_alignment_context_tpl():
			python_alignment_context_base(),
			t_base(),
			m_aligner(this->m_ctx, *this)
		{
		}
		
		python_alignment_context_tpl(std::size_t const num_threads):
			python_alignment_context_base(num_threads),
			t_base(),
			m_aligner(this->m_ctx, *this)
		{
		}
		
		aligner_type &get_aligner() { return m_aligner; }
		aligner_type const &get_aligner() const { return m_aligner; }
	};
	
	
	template <typename t_index, typename t_value>
	class alignment_scorer_tpl
	{
	public:
		typedef std::pair <t_index, t_index>	key_type;
		typedef std::map <key_type, t_value>	map_type;
		
	protected:
		map_type	m_scores;
		
	public:
		t_value score_pair(t_index const lhs, t_index const rhs) const
		{
			key_type const key(std::min(lhs, rhs), std::max(lhs, rhs));
			auto const it(m_scores.find(key));
			if (it == m_scores.end())
				throw std::runtime_error("Key not found from similarity tree");
			return it->second;
		}
	};
	
	
	class alignment_scorer :
		public alignment_scorer_tpl <unit_alignment_scorer_base::index_type, unit_alignment_scorer_base::value_type>,
		public virtual unit_alignment_scorer_base
	{
	public:
		typedef unit_alignment_scorer_base::index_type			index_type;
		typedef unit_alignment_scorer_base::value_type			value_type;
		typedef alignment_scorer_tpl <index_type, value_type>	scorer_tpl_type;
		typedef typename scorer_tpl_type::map_type				map_type;
		
	public:
		virtual map_type &get_scores() { return this->m_scores; }
	};
	
	
	typedef python_alignment_context_tpl <
		smith_waterman::aligner,
		std::int32_t,
		detail::no_op
	> python_alignment_context;
	
	typedef python_alignment_context_tpl <
		smith_waterman::aligner,
		float,
		alignment_scorer
	> python_scoring_fp_alignment_context;
}

#endif
