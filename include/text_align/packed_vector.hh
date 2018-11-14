/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef TEXT_ALIGN_PACKED_VECTOR_HH
#define TEXT_ALIGN_PACKED_VECTOR_HH

#include <atomic>
#include <vector>


namespace text_align { namespace detail {
	
	template <typename t_vector>
	class packed_vector_value_reference
	{
	public:
		typedef typename t_vector::word_type	word_type;
		
	protected:
		t_vector			*m_vector{};
		std::size_t			m_idx{};
		
	public:
		packed_vector_value_reference() = default;
		
		packed_vector_value_reference(t_vector &vector, std::size_t idx):
			m_vector(&vector),
			m_idx(idx)
		{
		}
		
	public:
		word_type load(std::memory_order order = std::memory_order_seq_cst) const { return m_vector->load(m_idx, order); }
		word_type fetch_or(word_type val, std::memory_order const order = std::memory_order_seq_cst) { return m_vector->fetch_or(m_idx, val, order); }
		
		operator word_type() const { return load(); }
	};
	
	
	template <typename t_vector, typename t_reference>
	class packed_vector_iterator_base
	{
	public:
		typedef typename t_vector::word_iterator		word_iterator;
		typedef typename t_vector::const_word_iterator	const_word_iterator;
		typedef typename t_vector::reference_proxy		reference_proxy;
		
		enum { SUBELEMENT_COUNT = t_vector::SUBELEMENT_COUNT };

	protected:
		t_vector		*m_vector{};
		std::size_t		m_idx{};
		
	public:
		virtual ~packed_vector_iterator_base() {}

		packed_vector_iterator_base() = default;
		packed_vector_iterator_base(t_vector &vector, std::size_t idx):
			m_vector(&vector),
			m_idx(idx)
		{
		}
		
		template <
			typename t_other_vector,
			typename t_other_reference,
			std::enable_if <std::is_convertible_v <t_other_vector *, t_vector *>>
		> packed_vector_iterator_base(packed_vector_iterator_base <t_other_vector, t_other_reference> const &other):
			m_vector(other.m_vector),
			m_idx(other.m_idx)
		{
		}
		
		std::size_t index() const { return m_idx; }
		
		virtual void advance(std::ptrdiff_t) = 0;
		virtual bool equal(packed_vector_iterator_base const &other) const { return m_vector == other.m_vector && m_idx == other.m_idx; }
		
		void increment() { advance(1); }
		void decrement() { advance(-1); }
		t_reference dereference() const { return m_vector->operator()(m_idx); }
		
		reference_proxy to_reference_proxy() const { return reference_proxy(*m_vector, m_idx); }
		word_iterator to_word_iterator() const { assert(0 == m_idx % SUBELEMENT_COUNT); return m_vector->begin() + m_idx / SUBELEMENT_COUNT; }
		const_word_iterator to_const_word_iterator() const { assert(0 == m_idx % SUBELEMENT_COUNT); return m_vector->begin() + m_idx / SUBELEMENT_COUNT; }
		
		operator reference_proxy() const { return to_reference_proxy(); }
		operator word_iterator() const { return to_word_iterator(); }
		operator const_word_iterator() const { return to_const_word_iterator(); }
	};
	
	
	template <typename t_vector, typename t_reference>
	class packed_vector_iterator final :
		public packed_vector_iterator_base <t_vector, t_reference>,
		public boost::iterator_facade <
			packed_vector_iterator <t_vector, t_reference>,
			t_reference,
			boost::random_access_traversal_tag,
			t_reference
		>
	{
		friend class boost::iterator_core_access;
		
	public:
		using packed_vector_iterator_base <t_vector, t_reference>::packed_vector_iterator_base;
		
	private:
		virtual void advance(std::ptrdiff_t const diff) override { this->m_idx += diff; }
		std::ptrdiff_t distance_to(packed_vector_iterator const &other) const { assert(this->m_idx <= other.m_idx); return other.m_idx - this->m_idx; }
	};
}}


namespace text_align {
	
	template <std::uint8_t t_bits, typename t_word = std::uint64_t>
	class packed_vector
	{
		static_assert(std::is_unsigned_v <t_word>);
		
	public:
		typedef t_word		word_type;
		enum { WORD_BITS = CHAR_BIT * sizeof(word_type) };
		
		static_assert(t_bits <= WORD_BITS);
		static_assert(0 == t_bits || 0 == WORD_BITS % t_bits);
		
	protected:
		typedef std::vector <std::atomic <word_type>>	value_vector;
		
	public:
		typedef typename value_vector::reference		word_reference;
		typedef typename value_vector::iterator			word_iterator;
		typedef typename value_vector::const_iterator	const_word_iterator;
		
		typedef detail::packed_vector_value_reference <
			packed_vector
		>												reference_proxy;
		typedef detail::packed_vector_iterator <
			packed_vector,
			reference_proxy
		>												iterator;
		typedef detail::packed_vector_iterator <
			packed_vector,
			word_type
		>												const_iterator;
		
		enum {
			SUBELEMENT_COUNT	= WORD_BITS / t_bits,
			SUBELEMENT_MASK		= (std::numeric_limits <word_type>::max() >> (WORD_BITS - t_bits))
		};
	
	protected:
		value_vector	m_values;
		std::size_t		m_size{};
		
	public:
		packed_vector() = default;
		explicit packed_vector(std::size_t const size):
			m_values(std::ceil(1.0 * size / SUBELEMENT_COUNT)),
			m_size(size)
		{
		}
		
		word_type load(std::size_t const idx, std::memory_order = std::memory_order_seq_cst) const;
		word_type fetch_or(std::size_t const idx, word_type val, std::memory_order const = std::memory_order_seq_cst);
		
		word_type operator()(std::size_t const idx, std::memory_order order = std::memory_order_seq_cst) const { return load(idx, order); }
		reference_proxy operator()(std::size_t const idx) { return reference_proxy(*this, idx); }
		word_type word_at(std::size_t const idx) const { return m_values[idx]; }
		word_reference word_at(std::size_t const idx) { return m_values[idx]; }
		
		std::size_t size() const { return m_size; }											// Size in elements.
		std::size_t available_size() const { return m_values.size() * SUBELEMENT_COUNT; }
		std::size_t word_size() const { return m_values.size(); }
		void set_size(std::size_t new_size) { assert(new_size <= available_size()); m_size = new_size; }
		
		// Iterators to packed values.
		iterator begin() { return iterator(*this, 0); }
		iterator end() { return iterator(*this, m_size); }
		const_iterator begin() const { return const_iterator(*this, 0); }
		const_iterator end() const { return const_iterator(*this, m_size); }
		const_iterator cbegin() const { return const_iterator(*this, 0); }
		const_iterator cend() const { return const_iterator(*this, m_size); }
		
		// Iterators to whole words.
		word_iterator word_begin() { return m_values.begin(); }
		word_iterator word_end() { return m_values.end(); }
		const_word_iterator word_begin() const { return m_values.begin(); }
		const_word_iterator word_end() const { return m_values.end(); }
		const_word_iterator word_cbegin() const { return m_values.cbegin(); }
		const_word_iterator word_cend() const { return m_values.cend(); }
	};
	
	
	template <std::uint8_t t_bits, typename t_word>
	auto packed_vector <t_bits, t_word>::load(
		std::size_t const idx,
		std::memory_order const order
	) const -> word_type
	{
		assert(idx < m_size);
		auto const word_idx(idx / SUBELEMENT_COUNT);
		auto const el_idx(idx % SUBELEMENT_COUNT);
		word_type const retval(m_values[word_idx].load(order));
		return ((retval >> (el_idx * t_bits)) & SUBELEMENT_MASK);
	}
	
	
	template <std::uint8_t t_bits, typename t_word>
	auto packed_vector <t_bits, t_word>::fetch_or(
		std::size_t const idx,
		word_type val,
		std::memory_order const order
	) -> word_type
	{
		assert(idx < m_size);
		auto const word_idx(idx / SUBELEMENT_COUNT);
		auto const el_idx(idx % SUBELEMENT_COUNT);
		auto const shift_amt(t_bits * el_idx);
		
		val &= SUBELEMENT_MASK;
		val <<= shift_amt;
		
		word_type const retval(m_values[word_idx].fetch_or(val, order));
		return ((retval >> shift_amt) & SUBELEMENT_MASK);
	}
}

#endif
