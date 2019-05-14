/*
 * Copyright (c) 2019 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef TEXT_ALIGN_COPY_PYTHON_LIST_HH
#define TEXT_ALIGN_COPY_PYTHON_LIST_HH

#include <algorithm>
#include <range/v3/view/enumerate.hpp>
#include "run_aligner.hh"


namespace text_align { namespace detail {
	
	void copy_long_list_to_vector(PyObject *list, std::vector <long> &dst)
	{
		auto const len(PyList_GET_SIZE(list));
		dst.resize(len, 0);
		
		typedef std::remove_const_t <decltype(len)> counter_type;
		for (counter_type i(0); i < len; ++i)
		{
			auto *item(PyList_GetItem(list, i));
			auto const val(PyLong_AsLong(item));
			dst[i] = val;
		}
	}
	
	
	PyObject *copy_long_vector_to_list(std::vector <long> const &src)
	{
		auto *retval(PyList_New(src.size()));
		
		for (auto const &[idx, val] : ranges::view::enumerate(src))
		{
			PyObject* pyval(PyLong_FromLong(val));
			if (!pyval)
				throw std::runtime_error("Unable to create a Python long");
			
			if (0 != PyList_SetItem(retval, idx, pyval)) // Consumes a reference to pyval.
				throw std::runtime_error("Unable to set a list item");
		}
		
		return retval;
	}
	
	
	PyObject *copy_char32_vector_to_unicode(std::vector <char32_t> const &src)
	{
		static_assert(4 == sizeof(char32_t));
		return PyUnicode_FromKindAndData(PyUnicode_4BYTE_KIND, src.data(), src.size()); // Returns a new reference.
	}
}}

#endif
