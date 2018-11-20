/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef TEXT_ALIGN_MATRIX_SLICE_HH
#define TEXT_ALIGN_MATRIX_SLICE_HH

#include <boost/range.hpp>
#include <boost/range/algorithm/for_each.hpp>
#include <valarray>


namespace text_align { namespace detail {
	
	template <typename t_matrix, typename t_word_iterator, typename t_iterator>
	class packed_word_range
	{
	public:
		typedef t_matrix								matrix_type;
		typedef typename matrix_type::word_type			word_type;
		typedef t_iterator								iterator;
		typedef t_word_iterator							word_iterator;
		typedef boost::iterator_range <iterator>		iterator_range;
		typedef boost::iterator_range <word_iterator>	word_iterator_range;
		
	protected:
		word_iterator_range	m_mid;
		iterator_range		m_left_extent;
		iterator_range		m_right_extent;
		
	public:
		packed_word_range() = default;
		packed_word_range(
			word_iterator mid_begin,
			word_iterator mid_end,
			iterator e1_begin,
			iterator e1_end,
			iterator e2_begin,
			iterator e2_end
		):
			m_mid(mid_begin, mid_end),
			m_left_extent(e1_begin, e1_end),
			m_right_extent(e2_begin, e2_end)
		{
		}
		
		word_iterator_range mid() { return m_mid; }
		iterator_range left_extent() { return m_left_extent; }
		iterator_range right_extent() { return m_right_extent; }
		template <typename t_unary_fn> void apply_aligned(t_unary_fn &&unary_fn, std::memory_order = std::memory_order_seq_cst) const;
		template <typename t_fn> void apply_left(t_fn &&fn);
		template <typename t_fn> void apply_right(t_fn &&fn);
		template <typename t_fn> void apply_mid(t_fn &&fn);
	};
	
	
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
		
	};
	
	
	template <typename t_matrix>
	class packed_matrix_slice final : public matrix_slice <t_matrix>
	{
	public:
		typedef matrix_slice <t_matrix>													superclass;
		typedef typename superclass::matrix_type										matrix_type;
		typedef typename t_matrix::word_type											word_type;
		typedef typename superclass::iterator											iterator;
		typedef typename superclass::iterator_range										iterator_range;
		typedef typename superclass::const_iterator										const_iterator;
		typedef typename superclass::const_iterator_range								const_iterator_range;

		typedef typename matrix_type::word_iterator										word_iterator;
		typedef typename matrix_type::const_word_iterator								const_word_iterator;
		typedef boost::iterator_range <word_iterator>									word_iterator_range;
		typedef boost::iterator_range <const_word_iterator>								const_word_iterator_range;
		
		typedef packed_word_range <matrix_type, word_iterator, iterator>				word_range;
		typedef packed_word_range <matrix_type, const_word_iterator, const_iterator>	const_word_range;

		enum { WORD_BITS = t_matrix::WORD_BITS };
		
	protected:
		template <typename t_word_range, typename t_iterator, typename t_caller>
		t_word_range to_word_range(t_caller &caller) const;

	public:
		using matrix_slice <t_matrix>::matrix_slice;
		
		// Check whether the starting position of the slice is word aligned.
		bool is_word_aligned() const { return 0 == this->m_slice.start() % matrix_type::ELEMENT_COUNT; }
		
		word_range to_word_range() { return to_word_range <word_range, iterator>(*this); }
		const_word_range to_word_range() const { return to_const_word_range(); }
		const_word_range to_const_word_range() const { return to_word_range <const_word_range, const_iterator>(*this); }
		word_iterator word_begin() { return this->begin().to_vector_iterator().to_word_iterator(); }
	};
	
	
	template <typename t_matrix>
	template <typename t_word_range, typename t_iterator, typename t_caller>
	auto packed_matrix_slice <t_matrix>::to_word_range(t_caller &caller) const -> t_word_range
	{
		assert(1 == caller.m_slice.stride());
		
		auto const begin(caller.begin());
		auto const end(caller.end());
		auto const begin_idx(begin.index());
		auto const end_idx(end.index());

		t_iterator e1_end, e2_begin;
		
		auto const rem_1(begin_idx % t_matrix::ELEMENT_COUNT);
		if (0 == rem_1)
			e1_end = begin.to_vector_iterator();
		else
			e1_end = (begin + (t_matrix::ELEMENT_COUNT - rem_1)).to_vector_iterator();
			
		auto const rem_2(end_idx % t_matrix::ELEMENT_COUNT);
		if (0 == rem_2)
			e2_begin = end.to_vector_iterator();
		else
			e2_begin = (end - rem_2).to_vector_iterator();
		
		return t_word_range(
			e1_end.to_word_iterator(),
			e2_begin.to_word_iterator(),
			begin.to_vector_iterator(),
			e1_end,
			e2_begin,
			end.to_vector_iterator()
		);
	}
	
	
	template <typename t_matrix, typename t_word_iterator, typename t_iterator>
	template <typename t_fn>
	void packed_word_range <t_matrix, t_word_iterator, t_iterator>::apply_mid(t_fn &&fn)
	{
		// Call fn with the middle words if needed.
		for (auto &atomic : m_mid)
			fn(atomic);
	}
	
	
	template <typename t_matrix, typename t_word_iterator, typename t_iterator>
	template <typename t_fn>
	void packed_word_range <t_matrix, t_word_iterator, t_iterator>::apply_left(t_fn &&fn)
	{
		// Call fn with the leftmost word and bits set if needed.
		if (m_left_extent.empty())
			return;
		
		fn(*(m_mid.begin() - 1), m_left_extent.size() * matrix_type::ELEMENT_BITS);
	}
	
	
	template <typename t_matrix, typename t_word_iterator, typename t_iterator>
	template <typename t_fn>
	void packed_word_range <t_matrix, t_word_iterator, t_iterator>::apply_right(t_fn &&fn)
	{
		// Call fn with the rightmost word and bits set if needed.
		if (m_right_extent.empty())
			return;
		
		fn(*(m_mid.end() + 1), m_right_extent.size() * matrix_type::ELEMENT_BITS);
	}
	
	
	template <typename t_matrix, typename t_word_iterator, typename t_iterator>
	template <typename t_unary_fn>
	void packed_word_range <t_matrix, t_word_iterator, t_iterator>::apply_aligned(t_unary_fn &&unary_fn, std::memory_order order) const
	{
		// Call unary_fn with each word s.t. the bits are word aligned.
		if (m_left_extent.empty())
		{
			// Already aligned, just handle the last word separately if needed.
			boost::for_each(m_mid, [&unary_fn, order](auto const &val){ unary_fn(val.load(order)); });
			if (!m_right_extent.empty())
			{
				auto const bits(m_right_extent.size() * matrix_type::ELEMENT_BITS);
				assert(bits < matrix_type::WORD_BITS);
				
				// end() is not past-the-end b.c. right_extent is not empty.
				word_type const mask((0x1 << bits) - 1);
				auto last_word(m_mid.end()->load(order) & mask);
				unary_fn(last_word);
			}
		}
		else
		{
			// Not aligned, shift and call the function.
			// m_mid.begin() is valid b.c. left extent is not empty.
			auto const left_bits(m_left_extent.size() * matrix_type::ELEMENT_BITS);
			assert(left_bits < matrix_type::WORD_BITS);
			auto word((m_mid.begin() - 1)->load(order));
			word >>= (matrix_type::WORD_BITS - left_bits);
			for (auto const &atomic : m_mid)
			{
				auto const next_word(atomic.load(order));
				word |= next_word << left_bits;
				unary_fn(word);
				word = next_word >> (matrix_type::WORD_BITS - left_bits);
			}
			
			if (m_right_extent.empty())
				unary_fn(word);
			else
			{
				auto const right_bits(m_right_extent.size() * matrix_type::ELEMENT_BITS);
				assert(right_bits < matrix_type::WORD_BITS);

				// m_mid.end() is not past-the-end b.c. right_extent is not empty.
				word_type const mask((0x1 << right_bits) - 1);
				auto last_word(m_mid.end()->load(order) & mask);
				word |= last_word << left_bits;
				unary_fn(word);
				
				if (matrix_type::WORD_BITS - left_bits < right_bits)
					unary_fn(last_word >> (matrix_type::WORD_BITS - left_bits));
			}
		}
	}
}}

#endif
