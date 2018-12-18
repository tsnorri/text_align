/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef TEXT_ALIGN_UTIL_HH
#define TEXT_ALIGN_UTIL_HH

#include <boost/format.hpp>


#define text_align_stringify(X)		#X

namespace text_align { namespace detail {
	
	template <typename t_type>
	inline constexpr bool is_integral_ref_v = std::is_integral_v <std::remove_reference_t <t_type>>;
	
	
	template <typename t_lhs, typename t_rhs, bool t_lhs_signed = std::is_signed_v <t_lhs>, bool t_rhs_signed = std::is_signed_v <t_rhs>>
	struct lte_integral // Both signed or unsigned b.c. of the specializations below.
	{
		static_assert(is_integral_ref_v <t_lhs>);
		static_assert(is_integral_ref_v <t_rhs>);
		
		static inline bool check(t_lhs &&lhs, t_rhs &&rhs) { return lhs <= rhs; }
	};
	
	template <typename t_lhs, typename t_rhs>
	struct lte_integral <t_lhs, t_rhs, true, false> // lhs is signed, rhs unsigned.
	{
		static_assert(is_integral_ref_v <t_lhs>);
		static_assert(is_integral_ref_v <t_rhs>);
		
		static inline bool check(t_lhs &&lhs, t_rhs &&rhs) { return lhs < 0 || static_cast <t_rhs>(lhs) <= rhs; }
	};
	
	template <typename t_lhs, typename t_rhs>
	struct lte_integral <t_lhs, t_rhs, false, true> // lhs is unsigned, rhs signed.
	{
		static_assert(is_integral_ref_v <t_lhs>);
		static_assert(is_integral_ref_v <t_rhs>);
		
		static inline bool check(t_lhs &&lhs, t_rhs &&rhs) { return 0 < rhs && lhs <= static_cast <t_lhs>(rhs); }
	};
	
	
	template <typename t_lhs, typename t_rhs>
	struct lte
	{
		static inline bool check(t_lhs &&lhs, t_rhs &&rhs)
		{
			if constexpr (is_integral_ref_v <t_lhs> && is_integral_ref_v <t_rhs>)
				return lte_integral <t_lhs, t_rhs>::check(std::forward <t_lhs>(lhs), std::forward <t_rhs>(rhs));
			else
				return lhs <= rhs;
		}
	};
}}


namespace text_align {

	inline char *copy_format_cstr(boost::format const fmt) { return strdup(boost::str(fmt).c_str()); }
	
	template <typename t_lhs, typename t_rhs>
	inline bool check_lte(t_lhs &&lhs, t_rhs &&rhs) { return detail::lte <t_lhs, t_rhs>::check(std::forward <t_lhs>(lhs), std::forward <t_rhs>(rhs)); }
}

#endif
