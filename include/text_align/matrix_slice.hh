/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef TEXT_ALIGN_MATRIX_SLICE_HH
#define TEXT_ALIGN_MATRIX_SLICE_HH

#include <boost/range.hpp>
#include <valarray>


namespace text_align { namespace detail {
	
	template <typename t_matrix>
	class matrix_slice
	{
	public:
		typedef t_matrix										matrix_type;
		typedef typename matrix_type::reference					reference;
		typedef typename matrix_type::const_reference			const_reference;
		typedef typename matrix_type::iterator					iterator;
		typedef typename matrix_type::const_iterator			const_iterator;
		typedef typename matrix_type::matrix_iterator			matrix_iterator;
		typedef typename matrix_type::const_matrix_iterator		const_matrix_iterator;
		typedef boost::iterator_range <iterator>				iterator_range;
		typedef boost::iterator_range <const_iterator>			const_iterator_range;
		typedef boost::iterator_range <matrix_iterator>			matrix_iterator_range;
		typedef boost::iterator_range <const_matrix_iterator>	const_matrix_iterator_range;
		
	protected:
		t_matrix	*m_matrix{nullptr};
		std::slice	m_slice{};
		
	public:
		matrix_slice() = default;
		matrix_slice(t_matrix &matrix, std::slice const &slice):
			m_matrix(&matrix),
			m_slice(slice)
		{
		}
		
		std::size_t size() const { return m_slice.size(); }
		
		reference operator[](std::size_t const idx) { return *(begin() + idx); }
		const_reference operator[](std::size_t const idx) const { return *(begin() + idx); }
		
		matrix_iterator begin() { return matrix_iterator(m_matrix->begin() + m_slice.start(), m_slice.stride()); }
		matrix_iterator end() { return matrix_iterator(m_matrix->begin() + m_slice.start() + m_slice.size() * m_slice.stride(), m_slice.stride()); }
		matrix_iterator_range range() { return boost::make_iterator_range(begin(), end()); }
		const_matrix_iterator begin() const { return cbegin(); }
		const_matrix_iterator end() const { return cend(); }
		const_matrix_iterator_range range() const { return const_range(); }
		const_matrix_iterator cbegin() const { return const_matrix_iterator(m_matrix->cbegin() + m_slice.start(), m_slice.stride()); }
		const_matrix_iterator cend() const { return const_matrix_iterator(m_matrix->cbegin() + m_slice.start() + m_slice.size() * m_slice.stride(), m_slice.stride()); }
		const_matrix_iterator_range const_range() const { return boost::make_iterator_range(cbegin(), cend()); }
		
	};
	
	
	template <typename t_matrix>
	class packed_matrix_slice final : public matrix_slice <t_matrix>
	{
	public:
		typedef matrix_slice <t_matrix>						superclass;
		typedef typename superclass::matrix_type			matrix_type;
		typedef typename superclass::iterator				iterator;
		typedef typename superclass::iterator_range			iterator_range;
		
		typedef typename matrix_type::word_iterator			word_iterator;
		typedef typename matrix_type::const_word_iterator	const_word_iterator;
		typedef boost::iterator_range <word_iterator>		word_iterator_range;
		typedef boost::iterator_range <const_word_iterator>	const_word_iterator_range;
		
		struct word_range
		{
			word_iterator_range	mid;
			iterator_range		left_extent;
			iterator_range		right_extent;
			
			word_range() = default;
			word_range(
				word_iterator mid_begin,
				word_iterator mid_end,
				iterator e1_begin,
				iterator e1_end,
				iterator e2_begin,
				iterator e2_end
			):
				mid(mid_begin, mid_end),
				left_extent(e1_begin, e1_end),
				right_extent(e2_begin, e2_end)
			{
			}
		};
		
	public:
		using matrix_slice <t_matrix>::matrix_slice;
		word_range to_word_range();
	};
	
	
	template <typename t_matrix>
	auto packed_matrix_slice <t_matrix>::to_word_range() -> word_range
	{
		auto const begin(this->begin());
		auto const end(this->end());
		auto const begin_idx(begin.index());
		auto const end_idx(end.index());

		iterator e1_end, e2_begin;
		
		auto const rem_1(begin_idx % t_matrix::SUBELEMENT_COUNT);
		if (0 == rem_1)
			e1_end = begin;
		else
			e1_end = begin + (t_matrix::SUBELEMENT_COUNT - rem_1);
			
		auto const rem_2(end_idx % t_matrix::SUBELEMENT_COUNT);
		if (0 == rem_2)
			e2_begin = end;
		else
			e2_begin = end - rem_2;
		
		return word_range(e1_end, e2_begin, begin, e1_end, e2_begin, end);
	}
}}

#endif
