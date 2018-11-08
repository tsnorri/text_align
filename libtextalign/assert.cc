/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <text_align/assert.hh>


namespace text_align { namespace detail {
	
	void assertion_failure(char const *file, long const line, char const *assertion)
	{
		std::ostringstream os;
		os << "Assertion failure at " << file << ':' << line << ": " << assertion;
		throw std::runtime_error(os.str());
	}
	
	
	void fail(char const *file, long const line)
	{
		std::ostringstream os;
		os << "Assertion failure at " << file << ':' << line;
		throw std::runtime_error(os.str());
	}
}}
