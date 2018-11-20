/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef TEXT_ALIGN_ASSERT_HH
#define TEXT_ALIGN_ASSERT_HH

#include <sstream>
#include <stdexcept>

#define always_assert(X) do { if (!(X)) ::text_align::detail::assertion_failure(__FILE__, __LINE__, #X); } while (false)
#define fail_assertion() do { ::text_align::detail::fail(__FILE__, __LINE__); } while (false)

#ifndef NDEBUG
#	define do_and_assert_eq(X, Y) do { if ((X) != (Y)) ::text_align::detail::assertion_failure(__FILE__, __LINE__, #X); } while (false)
#else
#	define do_and_assert_eq(X, Y) do { (X); } while (false)
#endif


namespace text_align { namespace detail {
	
	void assertion_failure(char const *file, long const line, char const *assertion);
	void fail(char const *file, long const line);
}}

#endif
