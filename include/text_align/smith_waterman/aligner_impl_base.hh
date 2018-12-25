/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef TEXT_ALIGN_SMITH_WATERMAN_ALIGNER_IMPL_BASE_HH
#define TEXT_ALIGN_SMITH_WATERMAN_ALIGNER_IMPL_BASE_HH

#include <text_align/smith_waterman/aligner_parameters.hh>
#include <text_align/smith_waterman/aligner_sample.hh>


namespace text_align { namespace smith_waterman { namespace detail {
	
	template <typename t_score>
	struct score_result
	{
		t_score score{};
		t_score gap_score_lhs{};
		t_score gap_score_rhs{};
		std::uint8_t max_idx{};
		aligner_base::gap_start_position_type did_start_gap{};
		
		score_result() = default;
		score_result(t_score score_): score(score_) {}
	};
	
	
	template <typename t_owner>
	class aligner_impl_base
	{
	protected:
		typedef typename t_owner::context_type				context_type;
		typedef typename t_owner::score_type				score_type;
		typedef typename t_owner::arrow_type				arrow_type;
		typedef typename t_owner::gap_start_position_type	gap_start_position_type;
		typedef typename t_owner::score_vector				score_vector;
		typedef typename t_owner::score_matrix				score_matrix;
		
	public:
		typedef score_result <score_type>					score_result;
		
	protected:
		// Pointers from t_owner.
		t_owner							*m_owner{};	// Not const b.c. finish() requires mutability.
		context_type					*m_ctx{};
		aligner_data <t_owner>			*m_data{};
		aligner_parameters <score_type>	*m_parameters{};
		aligner_sample <t_owner>		*m_lhs{};
		aligner_sample <t_owner>		*m_rhs{};
		
		std::atomic <score_type>		m_block_score{};
		
	public:
		aligner_impl_base() = default;
		aligner_impl_base(t_owner &owner):
			m_owner(&owner),
			m_ctx(owner.m_ctx),
			m_data(&owner.m_data),
			m_parameters(&owner.m_parameters),
			m_lhs(&owner.m_lhs),
			m_rhs(&owner.m_rhs)
		{
		}
		
		inline bool find_gap_start_x(
			std::size_t &j,
			std::size_t &i,
			std::size_t &steps
		) const;
	
		inline bool find_gap_start_y(
			std::size_t &j,
			std::size_t &i,
			std::size_t &steps
		) const;

		score_type block_score() const { return m_block_score; }

	protected:
		inline void did_calculate_score(std::size_t const j, std::size_t const i, score_result const &result, bool const initial);
		inline void push_lhs(bool const flag, std::size_t const count) { this->m_owner->push_lhs(flag, count); }
		inline void push_rhs(bool const flag, std::size_t const count) { this->m_owner->push_rhs(flag, count); }
		inline void reverse_gaps() { this->m_owner->reverse_gaps(); }
		inline void finish() { this->m_owner->finish(m_block_score); }
	};
	
	
	template <typename t_owner>
	void aligner_impl_base <t_owner>::did_calculate_score(std::size_t const j, std::size_t const i, score_result const &result, bool const initial)
	{
		this->m_owner->did_calculate_score(j, i, result, initial);
	}
	
	
	template <typename t_owner>
	bool aligner_impl_base <t_owner>::find_gap_start_x(
		std::size_t &j,
		std::size_t &i,
		std::size_t &steps
	) const
	{
		gap_start_position_type gsp{};
		while (true)
		{
			++steps;
			if (0 == i)
				return false;
			
			--i;
			gsp = static_cast <gap_start_position_type>(m_data->gap_start_positions.load(j, i));
			if (0 != (gap_start_position_type::GSP_RIGHT & gsp))
				return true;
		}
	}
	
	
	template <typename t_owner>
	bool aligner_impl_base <t_owner>::find_gap_start_y(
		std::size_t &j,
		std::size_t &i,
		std::size_t &steps
	) const
	{
		gap_start_position_type gsp{};
		while (true)
		{
			++steps;
			if (0 == j)
				return false;
			
			--j;
			gsp = static_cast <gap_start_position_type>(m_data->gap_start_positions.load(j, i));
			if (0 != (gap_start_position_type::GSP_DOWN & gsp))
				return true;
		}
	}
}}}

#endif

