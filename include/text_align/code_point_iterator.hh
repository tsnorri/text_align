/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef TEXT_ALIGN_CODE_POINT_ITERATOR_HH
#define TEXT_ALIGN_CODE_POINT_ITERATOR_HH

#include <boost/iterator/iterator_facade.hpp>
#include <boost/locale/utf.hpp>
#include <boost/range/iterator_range_core.hpp>
#include <range/v3/all.hpp>
#include <type_traits>


namespace text_align {
	enum class string_encoding : std::uint8_t {
		UTF_8,
		UTF_16,
		UTF_32
	};
}


namespace text_align { namespace detail {
	
	template <std::size_t t_size> struct string_encoding_from_size {};
	template <> struct string_encoding_from_size <1> { static constexpr string_encoding value = string_encoding::UTF_8; };
	template <> struct string_encoding_from_size <2> { static constexpr string_encoding value = string_encoding::UTF_16; };
	template <> struct string_encoding_from_size <4> { static constexpr string_encoding value = string_encoding::UTF_32; };
	template <std::size_t t_size> constexpr string_encoding string_encoding_from_size_v = string_encoding_from_size <t_size>::value;
	template <typename t_value> constexpr string_encoding string_encoding_from_value_size_v = string_encoding_from_size_v <sizeof(t_value)>;
	
	
	// Call Boost.Locale’s iterator.
	template <typename t_value, std::uint8_t t_size, typename t_it>
	char32_t read_fwd_boost_locale(t_it &it, t_it const &end)
	{
		auto const cp(boost::locale::utf::utf_traits <t_value, t_size>::decode(it, end));
		if (BOOST_UNLIKELY(boost::locale::utf::illegal == cp))
			throw std::runtime_error("Invalid UTF");
		if (BOOST_UNLIKELY(boost::locale::utf::incomplete == cp))
			throw std::runtime_error("Incomplete UTF");
		return cp;
	}
	
	// Read forward using Boost.Locale.
	template <typename t_value, std::uint8_t t_size, typename t_it>
	char32_t read_fwd(t_it const &it, t_it const &end)
	{
		auto curr(it);
		return read_fwd_boost_locale <t_value, t_size>(curr, end);
	}
	
	// Advance using Boost.Locale.
	template <typename t_value, std::uint8_t t_size, typename t_it>
	void next_fwd(t_it &it, t_it const &end)
	{
		read_fwd_boost_locale <t_value, t_size>(it, end);
	}
	
	
	template <string_encoding t_encoding, bool t_reversed>
	struct code_point_range_traits {};
	
	
	// UTF-8, forward.
	template <>
	struct code_point_range_traits <string_encoding::UTF_8, false>
	{
		template <typename t_value, typename t_it> static char32_t read(t_it const &it, t_it const &end) { return read_fwd <t_value, 1>(it, end); }
		template <typename t_value, typename t_it> static void next(t_it &it, t_it const &end) { next_fwd <t_value, 1>(it, end); }
	};
	
	
	// UTF-8, backward.
	template <>
	struct code_point_range_traits <string_encoding::UTF_8, true>
	{
		template <typename t_it>
		static char32_t read_backward(t_it &it, t_it const &end)
		{
			// Read UTF-8 from right to left.
			// Iterators are assumed to be reverse iterators.
			char32_t retval(*it);
			switch (0xc0 & retval)									// Check the highest two bits.
			{
				case 0x0:											// If the highest bit is zero, return the current byte.
				case 0x40:
					++it;
					return retval;
				
				case 0x80:											// The highest two bits are 10.
				{
					retval &= 0x3f;									// Save the 6 lowest bits (0x3f).
					++it;
					char const b1(*it);
					switch (0xe0 & b1)								// Check the highest three bits.
					{
						case 0xc0:									// The highest three bits are 110.
							++it;
							retval |= (0x1f & b1) << 6;				// Read 5 bits (0x1f).
							return retval;

						case 0x80:									// The highest two bits are 10.
						case 0xa0:
						{
							retval |= (0x3f & b1) << 6;				// Read 6 bits (0x3f).
							++it;
							char const b2(*it);
							switch (0xf0 & b2)						// Check the highest four bits.
							{
								case 0xe0:							// The highest four bits are 1110.
								{
									++it;
									retval |= (0xf & b2) << 12;		// Read 4 bits (0xf0).
									return retval;
								}

								case 0x80:							// The highest two bits are 10.
								case 0x90:
								case 0xa0:
								case 0xb0:
								{
									retval |= (0x3f & b2) << 12;	// Read 6 bits (0x3f), or otherwise 4 (0xf0).
									++it;
									char const b3(*it);
									if (0xf0 == (0xf8 & b3))		// Make sure that the highest five bits are 11110.
									{
										++it;
										retval |= (0x7 & b3) << 18;
										return retval;
									}
									
									goto fail;
								}
									
								default:
									goto fail;
							}
						}
						
						default:
							goto fail;
					}
				}
					
				default:
					goto fail;
			}
			
		fail:
			throw std::runtime_error("Invalid UTF-8");
		}
		
		template <typename, typename t_it> static char32_t read(t_it const &it, t_it const &end) { auto curr(it); return read_backward(curr, end); }
		template <typename, typename t_it> static void next(t_it &it, t_it const &end) { read_backward(it, end); }
	};
	
	
	// UTF-16, forward.
	template <>
	struct code_point_range_traits <string_encoding::UTF_16, false>
	{
		template <typename t_value, typename t_it> static char32_t read(t_it const &it, t_it const &end) { return read_fwd <t_value, 2>(it, end); }
		template <typename t_value, typename t_it> static void next(t_it &it, t_it const &end) { next_fwd <t_value, 2>(it, end); }
	};
	
	
	// UTF-16, backward.
	template <>
	struct code_point_range_traits <string_encoding::UTF_16, true>
	{
		template <typename t_it>
		static char32_t read_backward(t_it &it, t_it const &end)
		{
			// Read UTF-16 from right to left.
			// Iterators are assumed to be reverse iterators.
			char32_t retval(*it);
			if (0x0 <= retval && retval <= 0xd7ff)
				return retval;
			else if (0xe000 <= retval && retval <= 0xffff)
				return retval;
			else if (! (0xdc00 <= retval && retval <= 0xdfff))
				throw std::runtime_error("Invalid UTF-16");
			
			retval -= 0xdc00;
			
			++it;
			char32_t bb(*it);
			if (! (0xd800 <= bb && bb <= 0xdbff))
				throw std::runtime_error("Invalid UTF-16");
			retval |= (bb - 0xd800) << 10;
			return retval;
		}
		
		template <typename, typename t_it> static char32_t read(t_it const &it, t_it const &end) { auto curr(it); return read_backward(curr, end); }
		template <typename, typename t_it> static void next(t_it &it, t_it const &end) { read_backward(it, end); }
	};
	
	
	// UTF-32.
	template <bool t_reversed>
	struct code_point_range_traits <string_encoding::UTF_32, t_reversed>
	{
		template <typename, typename t_it> static char32_t read(t_it const &it, t_it const &) { return *it; }
		template <typename, typename t_it> static void next(t_it &it, t_it const &) { ++it; }
	};
}}


namespace text_align {
	
	template <typename t_range, string_encoding t_encoding, bool t_reversed>
	class code_point_range : public ranges::view_facade <code_point_range <t_range, t_encoding, t_reversed>>
	{
		friend ranges::range_access;
		
	protected:
		typedef decltype(std::declval <t_range const>().begin())	const_iterator_type;
		typedef typename ranges::range_value_type_t <t_range>		range_value_type;
		
	protected:
		const_iterator_type	m_it{};
		const_iterator_type	m_end{};

	public:
		code_point_range() = default;
		explicit code_point_range(t_range const &range):
			m_it(ranges::begin(range)),
			m_end(ranges::end(range))
		{
		}
		
		char32_t read() const { return detail::code_point_range_traits <t_encoding, t_reversed>::template read <range_value_type>(m_it, m_end); }
		void next() { return detail::code_point_range_traits <t_encoding, t_reversed>::template next <range_value_type>(m_it, m_end); }
		bool equal(ranges::default_sentinel) const { return m_it == m_end; }
	};
}


namespace text_align { namespace detail {
	
	// Helper for specifying the template parameters of code_point_range.
	template <typename t_range, bool t_reverse>
	using code_point_range_t = code_point_range <
		t_range,
		detail::string_encoding_from_value_size_v <
			typename ranges::range_value_type_t <t_range>
		>,
		t_reverse
	>;
}}


namespace text_align {
	
	template <typename t_range>
	auto make_code_point_range(t_range const &rng) -> detail::code_point_range_t <t_range, false>
	{
		return detail::code_point_range_t <t_range, false>(rng);
	}
	
	
	// Range must be reversed.
	template <typename t_range>
	auto make_reversed_code_point_range(t_range const &rng) -> detail::code_point_range_t <t_range, true>
	{
		return detail::code_point_range_t <t_range, true>(rng);
	}
}

#endif
