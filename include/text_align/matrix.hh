/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef TEXT_ALIGN_MATRIX_HH
#define TEXT_ALIGN_MATRIX_HH

#include <boost/iterator/iterator_facade.hpp>
#include <boost/range.hpp>
#include <iterator>
#include <text_align/assert.hh>
#include <type_traits>
#include <valarray>
#include <vector>


namespace text_align { namespace detail {
	
	template <typename t_iterator>
	class matrix_iterator : public boost::iterator_facade <
		matrix_iterator <t_iterator>,
		std::remove_reference_t <typename std::iterator_traits <t_iterator>::reference>,
		boost::random_access_traversal_tag
	>
	{
		friend class boost::iterator_core_access;
		
	private:
		typedef typename std::iterator_traits <t_iterator>::reference		iterator_reference_type;
		
	public:
		typedef typename std::remove_reference_t <iterator_reference_type> 	value_type;
		
	private:
		t_iterator			m_it{};
		std::ptrdiff_t		m_stride{1}; // Need sign for division.

	public:
		matrix_iterator() = default;

		matrix_iterator(t_iterator it, std::size_t stride):
			m_it(it),
			m_stride(stride)
		{
		}
		
		template <
			typename t_other_iterator,
			std::enable_if_t <std::is_convertible_v <t_other_iterator *, t_iterator *>>
		> matrix_iterator(matrix_iterator <t_other_iterator> const &other):
			m_it(other.m_it),
			m_stride(other.m_stride)
		{
		}
		
	private:
		value_type &dereference() const { return *m_it; }
		bool equal(matrix_iterator const &other) const { return m_it == other.m_it && m_stride == other.m_stride; }
		void increment() { advance(1); }
		void advance(std::ptrdiff_t const diff) { std::advance(m_it, m_stride * diff); }
		
		std::ptrdiff_t distance_to(matrix_iterator const &other) const {
			auto const dist(std::distance(m_it, other.m_it));
			auto const retval(dist / m_stride);
			return retval;
		}
	};
	
	
	template <typename t_iterator>
	std::ostream &operator<<(std::ostream &stream, matrix_iterator <t_iterator> const &it) { stream << "(matrix iterator)"; return stream; }
	
	
	template <typename t_matrix>
	class matrix_slice
	{
	public:
		typedef t_matrix										matrix_type;
		typedef typename matrix_type::value_type				value_type;
		typedef typename matrix_type::iterator					matrix_iterator;
		typedef typename matrix_type::const_iterator			const_matrix_iterator;
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
		
		value_type &operator[](std::size_t const idx) { return *(begin() + idx); }
		value_type const &operator[](std::size_t const idx) const { return *(begin() + idx); }
		
		matrix_iterator begin() { return matrix_iterator(m_matrix->begin() + m_slice.start(), m_slice.stride()); }
		matrix_iterator end() { return matrix_iterator(m_matrix->begin() + m_slice.start() + m_slice.size() * m_slice.stride(), m_slice.stride()); }
		matrix_iterator range() { return boost::make_iterator_range(begin(), end()); }
		const_matrix_iterator begin() const { return cbegin(); }
		const_matrix_iterator end() const { return cend(); }
		const_matrix_iterator_range range() const { return const_range(); }
		const_matrix_iterator cbegin() const { return const_matrix_iterator(m_matrix->cbegin() + m_slice.start(), m_slice.stride()); }
		const_matrix_iterator cend() const { return const_matrix_iterator(m_matrix->cbegin() + m_slice.start() + m_slice.size() * m_slice.stride(), m_slice.stride()); }
		const_matrix_iterator_range const_range() const { return boost::make_iterator_range(cbegin(), cend()); }
	};
}}


namespace text_align {
	
	// Wrap an std::vector so that parts of it that correspond to matrix rows and columns may be used with
	// std and Boost Range algorithms. Said parts may be obtained with row() and column().
	template <typename t_value>
	class matrix
	{
		template <typename t_matrix>
		friend class detail::matrix_slice;
		
	public:
		typedef t_value											value_type;
		typedef std::vector <value_type>						vector_type;
		typedef typename vector_type::iterator					vector_iterator;
		typedef typename vector_type::const_iterator			const_vector_iterator;
		typedef detail::matrix_iterator <vector_iterator>		iterator;
		typedef detail::matrix_iterator <const_vector_iterator>	const_iterator;
		typedef detail::matrix_slice <matrix>					slice_type;
		typedef detail::matrix_slice <matrix const>				const_slice_type;
		
	protected:
		std::vector <value_type>	m_values;
		std::size_t					m_stride{1};
		
	protected:
		inline std::size_t idx(std::size_t y, std::size_t x) const
		{
			/* Column major order. */
			assert(y < m_stride);
			std::size_t const retval(x * m_stride + y);
			assert(retval < m_values.size());
			return retval;
			
		}
		
	public:
		matrix() = default;
		matrix(std::size_t const rows, std::size_t const columns, value_type const val):
			m_values(columns * rows, val),
			m_stride(rows)
		{
			assert(m_stride);
		}
		
		matrix(std::size_t const rows, std::size_t const columns):
			m_values(columns * rows),
			m_stride(rows)
		{
			assert(m_stride);
		}
		
		std::size_t const size() const { return m_values.size(); }
		std::size_t const column_size() const { return m_values.size() / m_stride; }
		std::size_t const row_size() const { return m_stride; }
		void resize(std::size_t const rows, std::size_t const cols) { resize(rows * cols); set_stride(rows); }
		void resize(std::size_t const rows, std::size_t const cols, value_type const val) { resize(rows * cols, val); set_stride(rows); }
		void resize(std::size_t const size) { m_values.resize(size); }
		void resize(std::size_t const size, value_type const val) { m_values.resize(size, val); }
		void set_stride(std::size_t const stride) { always_assert(0 == m_values.size() % m_stride); m_stride = stride; }
		value_type &operator()(std::size_t const y, std::size_t const x) { return m_values[idx(y, x)]; }
		value_type const &operator()(std::size_t const y, std::size_t const x) const { return m_values[idx(y, x)]; }
		slice_type row(std::size_t const row, std::size_t const first = 0);
		slice_type column(std::size_t const column, std::size_t const first = 0);
		slice_type row(std::size_t const row, std::size_t const first, std::size_t const limit);
		slice_type column(std::size_t const column, std::size_t const first, std::size_t const limit);
		const_slice_type row(std::size_t const row, std::size_t const first = 0) const { return const_row(row, first); };
		const_slice_type column(std::size_t const column, std::size_t const first = 0) const { return const_column(column, first); }
		const_slice_type row(std::size_t const row, std::size_t const first, std::size_t const limit) const { return const_row(row, first, limit); }
		const_slice_type column(std::size_t const column, std::size_t const first, std::size_t const limit) const { return const_column(column, first, limit); }
		const_slice_type const_row(std::size_t const row, std::size_t const first = 0) const;
		const_slice_type const_column(std::size_t const column, std::size_t const first = 0) const;
		const_slice_type const_row(std::size_t const row, std::size_t const first, std::size_t const limit) const;
		const_slice_type const_column(std::size_t const column, std::size_t const first, std::size_t const limit) const;
		vector_iterator begin() { return m_values.begin(); }
		vector_iterator end() { return m_values.end(); }
		const_vector_iterator begin() const { return m_values.cbegin(); }
		const_vector_iterator end() const { return m_values.cend(); }
		const_vector_iterator cbegin() const { return m_values.cbegin(); }
		const_vector_iterator cend() const { return m_values.cend(); }
		
		template <typename t_fn>
		void apply(t_fn &&fn);
		
		void swap(matrix &rhs);
	};
	
	template <typename t_value>
	void swap(matrix <t_value> &lhs, matrix <t_value> &rhs)
	{
		lhs.swap(rhs);
	}
	
	template <typename t_value>
	void matrix <t_value>::swap(matrix <t_value> &rhs)
	{
		using std::swap;
		swap(m_values, rhs.m_values);
		swap(m_stride, rhs.m_stride);
	}
	
	template <typename t_value>
	auto matrix <t_value>::row(std::size_t const row, std::size_t const first) -> slice_type
	{
		return this->row(row, first, this->column_size());
	}
	
	template <typename t_value>
	auto matrix <t_value>::column(std::size_t const column, std::size_t const first) -> slice_type
	{
		return this->column(column, first, this->row_size());
	}
	
	template <typename t_value>
	auto matrix <t_value>::const_row(std::size_t const row, std::size_t const first) const -> const_slice_type
	{
		return this->row(row, first, this->column_size());
	}
	
	template <typename t_value>
	auto matrix <t_value>::const_column(std::size_t const column, std::size_t const first) const -> const_slice_type
	{
		return this->column(column, first, this->row_size());
	}
	
	template <typename t_value>
	auto matrix <t_value>::row(std::size_t const row, std::size_t const first, std::size_t const limit) -> slice_type
	{
		std::slice const slice(idx(row, first), limit - first, m_stride);
		return slice_type(*this, slice);
	}
	
	template <typename t_value>
	auto matrix <t_value>::column(std::size_t const column, std::size_t const first, std::size_t const limit) -> slice_type
	{
		std::slice const slice(idx(first, column), limit - first, 1);
		return slice_type(*this, slice);
	}
	
	template <typename t_value>
	auto matrix <t_value>::const_row(std::size_t const row, std::size_t const first, std::size_t const limit) const -> const_slice_type
	{
		std::slice const slice(idx(row, first), limit - first, m_stride);
		return const_slice_type(*this, slice);
	}
	
	template <typename t_value>
	auto matrix <t_value>::const_column(std::size_t const column, std::size_t const first, std::size_t const limit) const -> const_slice_type
	{
		std::slice const slice(idx(first, column), limit - first, 1);
		return const_slice_type(*this, slice);
	}
	
	template <typename t_value>
	template <typename t_fn>
	void matrix <t_value>::apply(t_fn &&value_type)
	{
		for (auto &val : m_values)
			val = t_fn(val);
	}
}

#endif
