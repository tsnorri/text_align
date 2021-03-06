/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef TEXT_ALIGN_SMITH_WATERMAN_MATRIX_PRINTER_HH
#define TEXT_ALIGN_SMITH_WATERMAN_MATRIX_PRINTER_HH

#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/locale/utf.hpp>
#include <iterator>
#include <text_align/smith_waterman/aligner_base.hh>


namespace text_align { namespace smith_waterman { namespace detail {
	
	template <typename t_lhs_it, typename t_rhs_it>
	class matrix_printer
	{
		static_assert(!std::is_same_v <void, t_lhs_it>);
		static_assert(!std::is_same_v <void, t_rhs_it>);
		
	public:
		typedef aligner_base::arrow_type	arrow_type;
		
	protected:
		libbio::packed_matrix <1, std::uint64_t>	m_path;
		t_lhs_it									m_lhs_it{};
		t_rhs_it									m_rhs_it{};
		std::size_t									m_padding{};
		std::size_t									m_lhs_offset{};
		std::size_t									m_rhs_offset{};
		std::size_t									m_rows{};
		std::size_t									m_columns{};
		std::size_t									m_j_start{};
		std::size_t									m_i_start{};
		bool										m_prints_values_converted_to_utf8{};
		
	public:
		matrix_printer() = default;
		
		matrix_printer(
			std::size_t j_start,
			std::size_t i_start,
			std::size_t rows,
			std::size_t columns,
			t_lhs_it const &lhs_it,
			t_rhs_it const &rhs_it,
			bool prints_values_converted_to_utf8
		):
			m_path(rows, columns),
			m_lhs_it(lhs_it),
			m_rhs_it(rhs_it),
			m_rows(rows),
			m_columns(columns),
			m_j_start(j_start),
			m_i_start(i_start),
			m_prints_values_converted_to_utf8(prints_values_converted_to_utf8)
		{
		}
		
		void set_lhs_offset(std::size_t off) { m_lhs_offset = off; }
		void set_rhs_offset(std::size_t off) { m_rhs_offset = off; }
		void set_padding(std::size_t padding) { m_padding = padding; }

		void print_header();
		
		template <typename t_matrix>
		void prepare(t_matrix const &tb);
		
		template <typename t_matrix>
		void print_traceback(t_matrix const &tb);
		
		template <typename t_score>
		void print_scores(libbio::matrix <t_score> const &scores);
		
	protected:
		bool check_on_path(std::size_t const j, std::size_t const i);
	};
	
	
	template <typename t_lhs_it, typename t_rhs_it>
	void matrix_printer <t_lhs_it, t_rhs_it>::print_header()
	{
		auto rhs_it(m_rhs_it); // Copy.
		std::advance(rhs_it, m_rhs_offset);
		std::ostream_iterator <char> out_it(std::cerr);
		std::fill_n(out_it, 1 + m_padding, '\t');
		for (std::size_t i(m_padding); i < m_columns; ++i)
		{
			auto const rhs_c(*rhs_it);
			++rhs_it;
			std::cerr << '\t';
			if (m_prints_values_converted_to_utf8)
			{
				if (10 == rhs_c) // newline
					std::cerr << u8"↩︎";
				else
					boost::locale::utf::utf_traits <char, 1>::encode(rhs_c, out_it);
			}
			else
			{
				std::cerr << rhs_c;
			}
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
		return m_path(j, i);
	}
	
	
	template <typename t_lhs_it, typename t_rhs_it>
	template <typename t_matrix>
	void matrix_printer <t_lhs_it, t_rhs_it>::prepare(t_matrix const &tb)
	{
		std::size_t j(m_j_start);
		std::size_t i(m_i_start);
		
		while (true)
		{
			m_path(j, i).fetch_or(0x1);
			auto const dir(static_cast <arrow_type>(tb(j, i)));
			switch (dir)
			{
				case arrow_type::ARROW_DIAGONAL:
					if (! (j && i))
						goto exit_loop;
					--j;
					--i;
					break;
					
				case arrow_type::ARROW_LEFT:
					if (!i)
						goto exit_loop;
					--i;
					break;
					
				case arrow_type::ARROW_UP:
					if (!j)
						goto exit_loop;
					--j;
					break;
					
				case arrow_type::ARROW_FINISH:
					goto exit_loop;
					
				default:
					libbio_fail("Unexpected traceback value");
			}
		}
	exit_loop:
		;
	}
	
	
	template <typename t_lhs_it, typename t_rhs_it>
	template <typename t_score>
	void matrix_printer <t_lhs_it, t_rhs_it>::print_scores(libbio::matrix <t_score> const &scores)
	{
		namespace io = boost::iostreams;
		typedef std::vector <char> buffer_type;
		typedef io::stream <io::back_insert_device <buffer_type>> buffer_stream;

		print_header();
		auto lhs_it(m_lhs_it); // Copy.
		std::advance(lhs_it, m_lhs_offset);

		buffer_type buffer;
		buffer_stream os(buffer);
		std::ostream_iterator <char> out_it(std::cerr);
		
		for (std::size_t j(0); j < m_rows; ++j)
		{
			if (j < m_padding)
				std::cerr << u8" \t" << j;
			else
			{
				auto const lhs_c(*lhs_it);
				++lhs_it;
				if (m_prints_values_converted_to_utf8)
				{
					if (10 == lhs_c) // newline
						std::cerr << u8"↩︎";
					else
						boost::locale::utf::utf_traits <char, 1>::encode(lhs_c, out_it);
				}
				else
				{
					std::cerr << lhs_c;
				}
				std::cerr << '\t' << j;
			}

			for (std::size_t i(0); i < m_columns; ++i)
			{
				std::cerr << '\t';
				
				// If we're on the path, find out the next path position.
				bool const on_path(check_on_path(j, i));
				auto const current_score(scores(j, i));
				
				if (on_path)
				{
					// FIXME: colorize output when the target is a suitable terminal instead of using the keycap characters?
					//std::cerr << "\033[31m" << current_score << "\033[0m";
					buffer.clear();
					os << current_score << std::flush;
					for (auto const c : buffer)
					{
						// FIXME: the underline is probably not needed.
						//std::cerr << c << u8"\u0332";
						
						switch (c)
						{
							case '0': std::cerr << u8"0️⃣"; break;
							case '1': std::cerr << u8"1️⃣"; break;
							case '2': std::cerr << u8"2️⃣"; break;
							case '3': std::cerr << u8"3️⃣"; break;
							case '4': std::cerr << u8"4️⃣"; break;
							case '5': std::cerr << u8"5️⃣"; break;
							case '6': std::cerr << u8"6️⃣"; break;
							case '7': std::cerr << u8"7️⃣"; break;
							case '8': std::cerr << u8"8️⃣"; break;
							case '9': std::cerr << u8"9️⃣"; break;
							default: std::cerr << c; break;
						}
					}
				}
				else
				{
					std::cerr << current_score;
				}
			}
			std::cerr << '\n';
		}
	}
	
	
	template <typename t_lhs_it, typename t_rhs_it>
	template <typename t_matrix>
	void matrix_printer <t_lhs_it, t_rhs_it>::print_traceback(t_matrix const &tb)
	{
		print_header();
		auto lhs_it(m_lhs_it); // Copy.
		std::advance(lhs_it, m_lhs_offset);
		std::ostream_iterator <char> out_it(std::cerr);
		for (std::size_t j(0); j < m_rows; ++j)
		{
			if (j < m_padding)
				std::cerr << u8" \t" << j;
			else
			{
				auto const lhs_c(*lhs_it);
				++lhs_it;
				if (m_prints_values_converted_to_utf8)
				{
					if (10 == lhs_c) // newline
						std::cerr << u8"↩︎";
					else
						boost::locale::utf::utf_traits <char, 1>::encode(lhs_c, out_it);
				}
				else
				{
					std::cerr << lhs_c;
				}
				std::cerr << '\t' << j;
			}

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
							std::cerr << u8"↖️";
							break;
							
						case arrow_type::ARROW_LEFT:
							std::cerr << u8"⬅️";
							break;
							
						case arrow_type::ARROW_UP:
							std::cerr << u8"⬆️";
							break;
							
						case arrow_type::ARROW_FINISH:
							std::cerr << u8"✳️";
							break;
							
						default:
							libbio_fail("Unexpected traceback value");
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
							std::cerr << u8"←";
							break;
							
						case arrow_type::ARROW_UP:
							std::cerr << u8"↑";
							break;
							
						case arrow_type::ARROW_FINISH:
							std::cerr << u8"*"; // Should not happen.
							break;
							
						default:
							libbio_fail("Unexpected traceback value");
							break;
					}
				}
			}
			std::cerr << '\n';
		}
	}
}}}

#endif
