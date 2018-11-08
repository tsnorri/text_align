/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef TEXT_ALIGN_SMITH_WATERMAN_MATRIX_PRINTER_HH
#define TEXT_ALIGN_SMITH_WATERMAN_MATRIX_PRINTER_HH

#include <text_align/smith_waterman/aligner_base.hh>


namespace text_align { namespace smith_waterman { namespace detail {
	
	template <typename t_lhs_it, typename t_rhs_it>
	class matrix_printer
	{
	public:
		typedef aligner_base::bit_vector		bit_vector;
		typedef aligner_base::arrow_type		arrow_type;
		typedef aligner_base::traceback_matrix	traceback_matrix;
			
	protected:
		bit_vector	m_lhs_gaps{};
		bit_vector	m_rhs_gaps{};
		t_lhs_it	m_lhs_it{};
		t_rhs_it	m_rhs_it{};
		std::size_t	m_rows{};
		std::size_t	m_columns{};
		std::size_t	m_path_x{};
		std::size_t	m_path_y{};
		std::size_t	m_gap_idx{};
		
	public:
		matrix_printer() = default;
		
		matrix_printer(
			std::size_t rows,
			std::size_t columns,
			bit_vector const &lhs_gaps,
			bit_vector const &rhs_gaps,
			t_lhs_it const &lhs_it,
			t_rhs_it const &rhs_it
		):
			m_lhs_gaps(lhs_gaps),
			m_rhs_gaps(rhs_gaps),
			m_lhs_it(lhs_it),
			m_rhs_it(rhs_it),
			m_rows(rows),
			m_columns(columns)
		{
			reverse_bitset(m_lhs_gaps);
			reverse_bitset(m_rhs_gaps);
		}
		
		void print_header();
		void print_traceback(traceback_matrix const &tb);
		
		template <typename t_score>
		void print_scores(matrix <t_score> const &scores);
		
	protected:
		bool check_on_path(std::size_t const j, std::size_t const i);
	};
	
	
	template <typename t_lhs_it, typename t_rhs_it>
	void matrix_printer <t_lhs_it, t_rhs_it>::print_header()
	{
		std::cerr << "\t\t";
		auto rhs_it(m_rhs_it); // Copy.
		for (std::size_t i(1); i < m_columns; ++i)
		{
			auto const rhs_c(*rhs_it++);
			std::cerr << '\t' << static_cast <char const>(rhs_c);
		}
		std::cerr << '\n';
		
		std::cerr << '\t';
		for (std::size_t i(0); i < m_columns; ++i)
			std::cerr << '\t' << i;
		std::cerr << '\n';
	}
	
	
	template <typename t_lhs_it, typename t_rhs_it>
	bool matrix_printer <t_lhs_it, t_rhs_it>::check_on_path(std::size_t const j, std::size_t const i)
	{
		bool on_path(false);
		if (j == m_path_x && i == m_path_y)
		{
			on_path = true;
			std::uint8_t path_arrow_bits(0);
			path_arrow_bits |= m_lhs_gaps[m_gap_idx] << 1;
			path_arrow_bits |= m_rhs_gaps[m_gap_idx];
			arrow_type const path_arrow(static_cast <arrow_type>(path_arrow_bits));

			switch (path_arrow)
			{
				case arrow_type::ARROW_DIAGONAL:
					++m_path_x;
					++m_path_y;
					break;
					
				case arrow_type::ARROW_UP:
					++m_path_y;
					break;
					
				case arrow_type::ARROW_LEFT:
					++m_path_x;
					break;
					
				default:
					fail_assertion();
					break;
			}
			
			++m_gap_idx;
		}
		
		return on_path;
	}
	
	
	template <typename t_lhs_it, typename t_rhs_it>
	template <typename t_score>
	void matrix_printer <t_lhs_it, t_rhs_it>::print_scores(matrix <t_score> const &scores)
	{
		//namespace io = boost::iostreams;
		//typedef std::vector <char> buffer_type;
		//typedef io::stream <io::back_insert_device <buffer_type>> buffer_stream;

		m_gap_idx = 0;
		m_path_x = 0;
		m_path_y = 0;
		
		print_header();
		char current_char(' ');
		auto lhs_it(m_lhs_it); // Copy.
		
		//buffer_type buffer;
		//buffer_stream os(buffer);
		
		for (std::size_t j(0); j < m_rows; ++j)
		{
			if (j)
				current_char = *lhs_it++;
			std::cerr << current_char << '\t' << j;
			for (std::size_t i(0); i < m_columns; ++i)
			{
				std::cerr << '\t';
				
				// If we're on the path, find out the next path position.
				bool const on_path(check_on_path(j, i));
				auto const current_score(scores(j, i));
				
				if (on_path)
					std::cerr << "\033[31m" << current_score << "\033[0m";
				else
					std::cerr << current_score;
			}
			std::cerr << '\n';
		}
	}
	
	
	template <typename t_lhs_it, typename t_rhs_it>
	void matrix_printer <t_lhs_it, t_rhs_it>::print_traceback(traceback_matrix const &tb)
	{
		m_gap_idx = 0;
		m_path_x = 0;
		m_path_y = 0;
		
		print_header();
		char current_char(' ');
		auto lhs_it(m_lhs_it); // Copy.
		for (std::size_t j(0); j < m_rows; ++j)
		{
			if (j)
				current_char = *lhs_it++;
			std::cerr << current_char << '\t' << j;
			for (std::size_t i(0); i < m_columns; ++i)
			{
				std::cerr << '\t';
				
				// If we're on the path, find out the next path position.
				bool const on_path(check_on_path(j, i));
				auto const arrow(tb(j, i));
				if (on_path)
				{
					switch (arrow)
					{
						case arrow_type::ARROW_DIAGONAL:
							std::cerr << "↖️";
							break;
							
						case arrow_type::ARROW_LEFT:
							std::cerr << "⬅️";
							break;
							
						case arrow_type::ARROW_UP:
							std::cerr << "⬆️";
							break;
							
						case arrow_type::ARROW_FINISH:
							std::cerr << "✳️";
							break;
							
						default:
							fail_assertion();
							break;
					}
				}
				else
				{
					switch (arrow)
					{
						case arrow_type::ARROW_DIAGONAL:
							std::cerr << ' ';
							break;
							
						case arrow_type::ARROW_LEFT:
							std::cerr << "←";
							break;
							
						case arrow_type::ARROW_UP:
							std::cerr << "↑";
							break;
							
						case arrow_type::ARROW_FINISH:
						default:
							fail_assertion();
							break;
					}
				}
			}
			std::cerr << '\n';
		}
	}
}}}

#endif
