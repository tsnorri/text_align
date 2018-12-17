/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef TEXT_ALIGN_ASSERT_HH
#define TEXT_ALIGN_ASSERT_HH

#include <sstream>
#include <stdexcept>
#include <text_align/buffer.hh>
#include <text_align/util.hh>

#define text_align_assert(X)		text_align_always_assert(X) // For now, don‘t skip assertions.
#define text_align_always_assert(X)	do { if (!(X)) ::text_align::detail::assertion_failure(__FILE__, __LINE__, #X); } while (false)
#define fail_assertion()			do { ::text_align::detail::fail(__FILE__, __LINE__); } while (false)

#ifndef NDEBUG
#	define do_and_assert_eq(X, Y) do { if ((X) != (Y)) ::text_align::detail::assertion_failure(__FILE__, __LINE__, #X); } while (false)
#else
#	define do_and_assert_eq(X, Y) do { (X); } while (false)
#endif


namespace text_align { namespace detail {
	
	// Copying a standard library class derived from std::exception
	// is not permitted to throw exceptions, so try to avoid it here, too.
	struct assertion_failure_cause
	{
		std::string		reason;
		std::string		file;
		buffer <char>	what;
		int				line{};
		
		assertion_failure_cause() = default;
		
		assertion_failure_cause(char const *file_, int line_):
			file(file_),
			what(copy_format_cstr(boost::format("%s:%d") % file % line_)),
			line(line_)
		{
		}
		
		assertion_failure_cause(char const *file_, int line_, std::string &&reason_):
			reason(std::move(reason_)),
			file(file_),
			what(copy_format_cstr(boost::format("%s:%d: %s") % file % line_ % reason)),
			line(line_)
		{
		}
	};
	
	class assertion_failure_exception final : public std::exception
	{
	protected:
		std::shared_ptr <assertion_failure_cause>	m_cause;
		
	public:
		assertion_failure_exception() = default;
		
		assertion_failure_exception(char const *file, int line, std::string &&reason):
			m_cause(new assertion_failure_cause(file, line, std::move(reason)))
		{
		}
		
		assertion_failure_exception(char const *file, int line):
			m_cause(new assertion_failure_cause(file, line))
		{
		}
		
		virtual char const *what() const noexcept override { return m_cause->what.get(); }
		std::string const &file() const noexcept { return m_cause->file; }
		std::string const &reason() const noexcept { return m_cause->reason; }
		int line() const noexcept { return m_cause->line; }
	};
	
	void assertion_failure(char const *file, long const line, char const *assertion);
	void fail(char const *file, long const line);
}}

#endif
