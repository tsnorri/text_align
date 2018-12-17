/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef TEXT_ALIGN_CODE_POINT_ITERATOR_HH
#define TEXT_ALIGN_CODE_POINT_ITERATOR_HH

#include <boost/iterator/iterator_facade.hpp>
#include <boost/locale/utf.hpp>
#include <boost/range/iterator_range_core.hpp>
#include <type_traits>


namespace text_align {
	template <typename t_iterator>
	class code_point_iterator : public boost::iterator_facade <
		code_point_iterator <t_iterator>,
		char32_t,
		boost::forward_traversal_tag,
		char32_t
	>
	{
		friend class boost::iterator_core_access;

	private:
		typedef typename std::iterator_traits <t_iterator>::value_type underlying_value_type;
		enum { UNDERLYING_VALUE_SIZE = sizeof(underlying_value_type) };
		
		static_assert(
			1 == UNDERLYING_VALUE_SIZE ||
			2 == UNDERLYING_VALUE_SIZE ||
			4 == UNDERLYING_VALUE_SIZE
		);
		
		struct enabler {};
		
	private:
		t_iterator			m_it{};
		mutable t_iterator	m_next{};
		t_iterator			m_end{};
		mutable char32_t	m_cp;

	public:
		code_point_iterator() = default;

		code_point_iterator(t_iterator it, t_iterator end):
			m_it(it),
			m_next(it),
			m_end(end)
		{
		}
	
	private:
		void do_advance() const
		{
			text_align_assert(m_it == m_next);
			text_align_assert(m_it != m_end);
			
			m_cp = boost::locale::utf::utf_traits <
				underlying_value_type
			>::decode(m_next, m_end);
			
			if (BOOST_UNLIKELY(boost::locale::utf::illegal == m_cp))
				throw std::runtime_error("Invalid UTF");
			if (BOOST_UNLIKELY(boost::locale::utf::incomplete == m_cp))
				throw std::runtime_error("Incomplete UTF");
		}

		bool equal(code_point_iterator const &other) const { return m_it == other.m_it; }

		void increment()
		{
			if (m_it == m_next)
				do_advance();

			m_it = m_next;
		}

		char32_t dereference() const
		{
			if (m_it == m_next)
				do_advance();

			return m_cp;
		}
	};


	template <typename t_iterator>
	boost::iterator_range <code_point_iterator <t_iterator>>
	make_code_point_iterator_range(t_iterator begin, t_iterator end)
	{
		return boost::make_iterator_range(
			code_point_iterator <t_iterator>(begin, end),
			code_point_iterator <t_iterator>(end, end)
		);
	}
}

#endif
