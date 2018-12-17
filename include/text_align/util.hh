/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef TEXT_ALIGN_UTIL_HH
#define TEXT_ALIGN_UTIL_HH

#include <boost/format.hpp>


namespace text_align {

	inline char *copy_format_cstr(boost::format const fmt) { return strdup(boost::str(fmt).c_str()); }
}

#endif
