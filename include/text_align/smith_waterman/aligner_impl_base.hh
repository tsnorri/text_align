/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef TEXT_ALIGN_SMITH_WATERMAN_ALIGNER_IMPL_BASE_HH
#define TEXT_ALIGN_SMITH_WATERMAN_ALIGNER_IMPL_BASE_HH

#include <text_align/smith_waterman/aligner_sample.hh>


namespace text_align { namespace smith_waterman { namespace detail {
	
	template <typename t_owner>
	class aligner_impl_base
	{
	protected:
		typedef typename t_owner::context_type			context_type;
		typedef typename t_owner::score_type			score_type;
		typedef typename t_owner::score_matrix			score_matrix;
		typedef typename t_owner::arrow_type			arrow_type;
		typedef typename t_owner::gap_score_gt_type		gap_score_gt_type;

	protected:
		// Pointers from t_owner.
		// FIXME: make m_owner const?
		t_owner							*m_owner{};
		context_type					*m_ctx{};
		aligner_data <t_owner>			*m_data{};
		aligner_parameters <score_type>	*m_parameters{};
		aligner_sample <t_owner>		*m_lhs{};
		aligner_sample <t_owner>		*m_rhs{};
		
		score_type						m_gap_score_rhs{};
		
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

		void finish() { this->m_owner->finish(m_block_score); }
		score_type block_score() const { return m_block_score; }
	};
	
	
	template <typename t_owner>
	bool aligner_impl_base <t_owner>::find_gap_start_x(
		std::size_t &j,
		std::size_t &i,
		std::size_t &steps
	) const
	{
		assert(i);
		gap_score_gt_type gsgt{};
		do
		{
			if (0 == i)
				return false;
			
			--i;
			++steps;
			gsgt = static_cast <gap_score_gt_type>(m_data->gap_score_gt.value_at(j, i));
		} while (0 == (gap_score_gt_type::GSGT_LEFT & gsgt));
		return true;
	}
	
	
	template <typename t_owner>
	bool aligner_impl_base <t_owner>::find_gap_start_y(
		std::size_t &j,
		std::size_t &i,
		std::size_t &steps
	) const
	{
		assert(j);
		gap_score_gt_type gsgt{};
		do
		{
			if (0 == j)
				return false;
			
			--j;
			++steps;
			gsgt = static_cast <gap_score_gt_type>(m_data->gap_score_gt.value_at(j, i));
		} while (0 == (gap_score_gt_type::GSGT_UP & gsgt));
		return true;
	}
}}}

#endif

