/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef TEXT_ALIGN_MATRIX_SLICE_HH
#define TEXT_ALIGN_MATRIX_SLICE_HH

#include <boost/range.hpp>
#include <boost/range/algorithm/for_each.hpp>
#include <experimental/iterator>
#include <range/v3/algorithm/copy.hpp>
#include <range/v3/utility/iterator.hpp>
#include <range/v3/view/transform.hpp>
#include <text_align/packed_word_range.hh>
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
		
		matrix_type &matrix() { return *m_matrix; }
		matrix_type const &matrix() const { return *m_matrix; }
		
		std::size_t size() const { return m_slice.size(); }
		
		reference operator[](std::size_t const idx) { assert(idx < size()); return *(begin() + idx); } // FIXME: throw.
		const_reference operator[](std::size_t const idx) const { assert(idx < size()); return *(begin() + idx); } // FIXME: throw.
		
		matrix_iterator begin() { return matrix_iterator(*m_matrix, m_slice.start(), m_slice.stride()); }
		matrix_iterator end() { return matrix_iterator(*m_matrix, m_slice.start() + m_slice.size() * m_slice.stride(), m_slice.stride()); }
		matrix_iterator_range range() { return boost::make_iterator_range(begin(), end()); }
		const_matrix_iterator begin() const { return cbegin(); }
		const_matrix_iterator end() const { return cend(); }
		const_matrix_iterator_range range() const { return const_range(); }
		const_matrix_iterator cbegin() const { return const_matrix_iterator(*m_matrix, m_slice.start(), m_slice.stride()); }
		const_matrix_iterator cend() const { return const_matrix_iterator(*m_matrix, m_slice.start() + m_slice.size() * m_slice.stride(), m_slice.stride()); }
		const_matrix_iterator_range const_range() const { return boost::make_iterator_range(cbegin(), cend()); }
		
		void output_to_stderr() { std::cerr << *this << std::endl; } // For debugging.
	};
	
	
	template <typename t_matrix>
	std::ostream &operator<<(std::ostream &os, matrix_slice <t_matrix> const &slice)
	{
		ranges::copy(slice | ranges::view::transform([](auto val){ return +val; }), ranges::make_ostream_joiner(os, "\t"));
		return os;
	}
	
	
	template <typename t_matrix>
	class packed_matrix_slice final : public matrix_slice <t_matrix>
	{
	public:
		typedef matrix_slice <t_matrix>							superclass;
		typedef typename superclass::matrix_type				matrix_type;
		typedef typename matrix_type::vector_type				vector_type;
		typedef typename matrix_type::word_type					word_type;
		typedef typename superclass::iterator					iterator;
		typedef typename superclass::iterator_range				iterator_range;
		typedef typename superclass::const_iterator				const_iterator;
		typedef typename superclass::const_iterator_range		const_iterator_range;

		typedef typename matrix_type::word_iterator				word_iterator;
		typedef typename matrix_type::const_word_iterator		const_word_iterator;
		typedef boost::iterator_range <word_iterator>			word_iterator_range;
		typedef boost::iterator_range <const_word_iterator>		const_word_iterator_range;
		
		typedef packed_word_range <vector_type>					word_range;
		typedef packed_word_range <vector_type const>			const_word_range;
		
	protected:
		template <typename t_word_range, typename t_caller>
		t_word_range to_word_range(t_caller &caller) const;

	public:
		using matrix_slice <t_matrix>::matrix_slice;
		
		// Check whether the starting position of the slice is word aligned.
		bool is_word_aligned() const { return 0 == this->m_slice.start() % matrix_type::ELEMENT_COUNT; }
		
		word_range to_word_range() { return to_word_range <word_range>(*this); }
		const_word_range to_word_range() const { return to_const_word_range(); }
		const_word_range to_const_word_range() const { return to_word_range <const_word_range>(*this); }
		word_iterator word_begin() { return this->begin().to_vector_iterator().to_word_iterator(); }
	};
	
	
	template <typename t_matrix>
	template <typename t_word_range, typename t_caller>
	auto packed_matrix_slice <t_matrix>::to_word_range(t_caller &caller) const -> t_word_range
	{
		assert(1 == caller.m_slice.stride());
		return t_word_range(caller.begin().to_vector_iterator(), caller.end().to_vector_iterator());
	}
}}

#endif
