/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef TEXT_ALIGN_JSON_SERIALIZE_HH
#define TEXT_ALIGN_JSON_SERIALIZE_HH

#include <boost/locale/utf.hpp>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <string_view>
#include <vector>


namespace text_align { namespace json {
	
	struct serializable
	{
		virtual void to_json(std::ostream &stream) const = 0;
	};
	
	
	// FIXME: add enable_ifs, vector could also be any sequential container.
	template <typename t_ptr>
	void to_json(std::ostream &stream, std::vector <t_ptr> const &nodes)
	{
		stream << '[';
		bool first(true);
		for (auto const &node_ptr : nodes)
		{
			if (first)
				stream << '{';
			else
				stream << ", {";
			
			node_ptr->to_json(stream);
			stream << '}';
			first = false;
		}
		stream << ']';
	}
	
	
	template <typename t_input_it>
	void escape_string(std::ostream &ostream, t_input_it it, t_input_it end)
	{
		while (it != end)
		{
			auto const c(*it++);
			switch (c)
			{
				case '"':	ostream << "\\\""; break;
				case '\\':	ostream << "\\\\"; break;
				case '\b':	ostream << "\\b"; break;
				case '\f':	ostream << "\\f"; break;
				case '\n':	ostream << "\\n"; break;
				case '\r':	ostream << "\\r"; break;
				case '\t':	ostream << "\\t"; break;
				default:
				{
					if (0x00 <= c && c <= 0x1f)
						ostream << std::hex << std::setw(4) << std::setfill('0') << +c;
					else
					{
						std::ostream_iterator <char> it(ostream);
						boost::locale::utf::utf_traits <char>::encode(c, it);
					}
				}
			}
		}
	}
	
	
	template <std::size_t t_length>
	void escape_string(std::ostream &stream, char const (&str)[t_length])
	{
		std::string_view sv(str, t_length - 1);
		escape_string(stream, sv.begin(), sv.end());
	}
	
	
	template <
		typename t_range,
		typename = std::enable_if_t <!std::is_pointer <t_range>::value>
	>
	void escape_string(std::ostream &stream, t_range const &range)
	{
		escape_string(stream, range.begin(), range.end());
	}
	
	
	template <typename t_key, typename t_val>
	void write(std::ostream &stream, t_key const &key, t_val const &val)
	{
		stream << '"';
		escape_string(stream, key);
		stream << "\": \"";
		escape_string(stream, val);
		stream << '"';
	}
}}

#endif
