/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef TEXT_ALIGN_BUFFER_HH
#define TEXT_ALIGN_BUFFER_HH

#include <boost/format.hpp>
#include <memory>

namespace text_align { namespace detail {
	
	template <typename t_type>
	struct malloc_deleter
	{
		void operator()(t_type *ptr) const { free(ptr); }
	};
}}



namespace text_align {
	
	template <typename t_type>
	class buffer
	{
	protected:
		std::unique_ptr <t_type, detail::malloc_deleter <t_type>>	m_content;
		
	public:
		buffer() = default;
		
		buffer(t_type *value):	// Takes ownership.
			m_content(value)
		{
		}
		
		t_type const *get() const { return m_content.get(); }
	};
}

#endif
