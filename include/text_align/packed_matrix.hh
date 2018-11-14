/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef TEXT_ALIGN_PACKED_MATRIX_HH
#define TEXT_ALIGN_PACKED_MATRIX_HH

#include <text_align/matrix_slice.hh>
#include <text_align/matrix_utils.hh>
#include <text_align/packed_matrix_fwd.hh>
#include <text_align/packed_vector.hh>


namespace text_align { namespace detail {
	
	template <typename t_vector, typename t_reference>
	class packed_matrix_iterator final :
		public packed_vector_iterator_base <t_vector, t_reference>,
		public boost::iterator_facade <
			packed_matrix_iterator <t_vector, t_reference>,
			t_reference,
			boost::random_access_traversal_tag
		>
	{
		friend class boost::iterator_core_access;
		
	protected:
		typedef packed_vector_iterator_base <t_vector, t_reference>	iterator_base;
		
	protected:
		std::size_t m_stride{1};
		
	public:
		packed_matrix_iterator() = default;
		packed_matrix_iterator(t_vector &vector, std::size_t idx, std::size_t stride):
			iterator_base(vector, idx),
			m_stride(stride)
		{
		}
		
		template <
			typename t_other_vector,
			typename t_other_reference,
			std::enable_if <std::is_convertible_v <t_other_vector *, t_vector *>>
		> packed_matrix_iterator(packed_matrix_iterator <t_other_vector, t_other_reference> const &other):
			iterator_base(other.m_vector, other.m_idx),
			m_stride(other.m_stride)
		{
		}
		
	private:
		void advance(std::ptrdiff_t const diff) override { this->m_idx += diff * m_stride; }
		std::ptrdiff_t distance_to(packed_matrix_iterator const &other) const override { assert(this->m_idx <= other.m_idx); return (other.m_idx - this->m_idx) / m_stride; }
		bool equal(packed_matrix_iterator const &other) const override { return iterator_base::equal(other) && m_stride == other.m_stride; }
	};
}}


namespace text_align {
	
	template <std::uint8_t t_bits, typename t_word>
	class packed_matrix
	{
	protected:
		typedef packed_vector <t_bits, t_word>						vector_type;
		
	public:
		typedef typename vector_type::word_type						word_type;
		typedef typename vector_type::reference_proxy				reference_proxy;
		
		typedef word_type											value_type;
		typedef reference_proxy										reference;
		typedef word_type const										const_reference;
		
		typedef typename vector_type::iterator						iterator;
		typedef typename vector_type::const_iterator				const_iterator;
		typedef typename vector_type::word_iterator					word_iterator;
		typedef typename vector_type::const_word_iterator			const_word_iterator;
		typedef detail::packed_matrix_iterator <
			vector_type,
			reference
		>															matrix_iterator;
		typedef detail::packed_matrix_iterator <
			vector_type const,
			const_reference
		>															const_matrix_iterator;
		
		typedef detail::packed_matrix_slice <packed_matrix>			slice_type;
		typedef detail::packed_matrix_slice <packed_matrix const>	const_slice_type;
		
		enum {
			SUBELEMENT_COUNT	= vector_type::SUBELEMENT_COUNT,
			SUBELEMENT_MASK		= vector_type::SUBELEMENT_MASK,
			WORD_BITS			= vector_type::WORD_BITS
		};

	protected:
		vector_type											m_data;
#ifndef NDEBUG
		std::size_t											m_columns{};
#endif
		std::size_t											m_stride{1};
		
	public:
		packed_matrix() = default;
		packed_matrix(std::size_t const rows, std::size_t const columns):
			m_data(columns * rows),
#ifndef NDEBUG
			m_columns(columns),
#endif
			m_stride(rows)
		{
			assert(m_stride);
		}
		
		inline std::size_t idx(std::size_t const y, std::size_t const x) const
		{
			/* Column major order. */
			assert(y < m_stride);
			assert(x < m_columns);
			assert(x < m_data.size() / m_stride);
			std::size_t const retval(x * m_stride + y);
			assert(retval < m_data.size());
			return retval;
		}
		
		word_type load(std::size_t const y, std::size_t const x, std::memory_order order = std::memory_order_seq_cst) const { return m_data.load(idx(y, x), order); }
		word_type fetch_or(std::size_t const y, std::size_t const x, std::memory_order order = std::memory_order_seq_cst) const { return m_data.fetch_or(idx(y, x), order); }
		
		word_type operator()(std::size_t const y, std::size_t const x, std::memory_order order = std::memory_order_seq_cst) const { return load(y, x, order); };
		reference_proxy operator()(std::size_t const y, std::size_t const x) { return m_data(idx(y, x)); }
		
		std::size_t size() const { return m_data.size(); }
		std::size_t available_size() const { return m_data.available_size(); }
		std::size_t word_size() const { return m_data.word_size(); }
		void set_size(std::size_t new_size) { m_data.set_size(new_size); }
		std::size_t const number_of_columns() const { return m_data.size() / m_stride; }
		std::size_t const number_of_rows() const { return m_stride; }
		std::size_t const stride() const { return m_stride; }
		void set_stride(std::size_t const stride) { m_stride = stride; }
		
		// Iterators to packed values.
		iterator begin() { return m_data.begin(); }
		iterator end() { return m_data.end(); }
		const_iterator begin() const { return m_data.begin(); }
		const_iterator end() const { return m_data.end(); }
		const_iterator cbegin() const { return m_data.cbegin(); }
		const_iterator cend() const { return m_data.cend(); }
		
		// Iterators to whole words.
		word_iterator word_begin() { return m_data.word_begin(); }
		word_iterator word_end() { return m_data.word_end(); }
		const_word_iterator word_begin() const { return m_data.word_begin(); }
		const_word_iterator word_end() const { return m_data.word_end(); }
		const_word_iterator word_cbegin() const { return m_data.word_cbegin(); }
		const_word_iterator word_cend() const { return m_data.word_cend(); }
		
		// Slices.
		slice_type row(std::size_t const row, std::size_t const first = 0)												{ return matrices::row(*this, row, first, this->number_of_columns()); }
		slice_type column(std::size_t const column, std::size_t const first = 0)										{ return matrices::column(*this, column, first, this->number_of_rows()); }
		slice_type row(std::size_t const row, std::size_t const first, std::size_t const limit)							{ return matrices::row(*this, row, first, limit); }
		slice_type column(std::size_t const column, std::size_t const first, std::size_t const limit)					{ return matrices::column(*this, column, first, limit); }
		const_slice_type row(std::size_t const row, std::size_t const first = 0) const									{ return matrices::const_row(*this, row, first, this->number_of_columns()); }
		const_slice_type column(std::size_t const column, std::size_t const first = 0) const							{ return matrices::const_column(*this, column, first, this->number_of_rows()); }
		const_slice_type row(std::size_t const row, std::size_t const first, std::size_t const limit) const				{ return matrices::const_row(*this, row, first, limit); }
		const_slice_type column(std::size_t const column, std::size_t const first, std::size_t const limit) const		{ return matrices::const_column(*this, column, first, limit); }
		const_slice_type const_row(std::size_t const row, std::size_t const first = 0) const							{ return matrices::const_row(*this, row, first, this->number_of_columns()); }
		const_slice_type const_column(std::size_t const column, std::size_t const first = 0) const						{ return matrices::const_column(*this, column, first, this->number_of_rows()); }
		const_slice_type const_row(std::size_t const row, std::size_t const first, std::size_t const limit) const		{ return matrices::const_row(*this, row, first, limit); }
		const_slice_type const_column(std::size_t const column, std::size_t const first, std::size_t const limit) const	{ return matrices::const_column(*this, column, first, limit); }
	};
}

#endif
