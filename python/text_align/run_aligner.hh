/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef TEXT_ALIGN_RUN_ALIGNER_HH
#define TEXT_ALIGN_RUN_ALIGNER_HH

#include <boost/beast/core/span.hpp>
#include <text_align/aligner.hh>
#include <text_align/map_on_stack.hh>


namespace text_align { namespace detail {
	
	struct span_from_buffer
	{
		template <typename t_data, typename t_fn>
		void handle_data(t_data *data, Py_ssize_t const length, t_fn &&fn)
		{
			boost::beast::span span(data, length);
			fn(span);
		}
		
		
		// Python Unicode strings select the character type in which the greatest code point
		// fits (without using e.g. surrogate pairs). Determine the character type, create
		// a span of that type and pass it to the callback.
		template <typename t_fn>
		void operator()(PyObject *arg, t_fn &&fn)
		{
			if (0 != PyUnicode_READY(arg))
				throw std::runtime_error("Given Python string is not in canonical representation");
				
			auto const length(PyUnicode_GET_LENGTH(arg));
			if (length < 0)
				throw std::runtime_error("PyUnicode_GET_LENGTH returned a negative value");
			
			// Determine the character type and create a span of that type.
			switch (PyUnicode_KIND(arg))
			{
				case PyUnicode_WCHAR_KIND:
				{
					auto *data(reinterpret_cast <wchar_t const *>(PyUnicode_DATA(arg)));
					handle_data(data, length, fn);
					break;
				}
				
				case PyUnicode_1BYTE_KIND:
				{
					auto *data(PyUnicode_1BYTE_DATA(arg));
					handle_data(data, length, fn);
					break;
				}
				
				case PyUnicode_2BYTE_KIND:
				{
					auto *data(PyUnicode_2BYTE_DATA(arg));
					handle_data(data, length, fn);
					break;
				}
				
				case PyUnicode_4BYTE_KIND:
				{
					auto *data(PyUnicode_4BYTE_DATA(arg));
					handle_data(data, length, fn);
					break;
				}
				
				default:
					throw std::runtime_error("Unexpected Python string representation");
			};
		}
	};
}}


namespace text_align
{
	template <typename t_aligner_context>
	void run_aligner(t_aligner_context &ctx, PyObject *lhso, PyObject *rhso)
	{
		// Convert the Python strings to spans.
		text_align::map_on_stack_fn <detail::span_from_buffer>(
			[&ctx](auto const &lhss, auto const &rhss) {
				ctx.aligner().align(lhss, rhss);
				ctx.run();
			},
			lhso, rhso
		);
	}
}

#endif
