/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef TEXT_ALIGN_COMPARE_HH
#define TEXT_ALIGN_COMPARE_HH

#include <type_traits>


namespace text_align { namespace detail {
	
	template <
		typename t_lhs,
		typename t_rhs,
		bool t_lhs_is_signed = std::is_signed_v <t_lhs>,
		bool t_rhs_is_signed = std::is_signed_v <t_rhs>
	>
	struct is_equal {};
	
	template <typename t_lhs, typename t_rhs>
	struct is_equal <t_lhs, t_rhs, true, true>
	{
		static constexpr bool test(t_lhs lhs, t_rhs rhs) { return lhs == rhs; }
	};
	
	template <typename t_lhs, typename t_rhs>
	struct is_equal <t_lhs, t_rhs, false, false>
	{
		static constexpr bool test(t_lhs lhs, t_rhs rhs) { return lhs == rhs; }
	};
	
	template <typename t_lhs, typename t_rhs>
	struct is_equal <t_lhs, t_rhs, true, false>
	{
		static constexpr bool test(t_lhs lhs, t_rhs rhs)
		{
			if (lhs < 0)
				return false;
			
			typedef std::common_type_t <t_lhs, t_rhs> ct;
			ct lhss(lhs);
			ct rhss(rhs);
			
			return lhss == rhss;
		}
	};
	
	template <typename t_lhs, typename t_rhs>
	struct is_equal <t_lhs, t_rhs, false, true>
	{
		static constexpr bool test(t_lhs lhs, t_rhs rhs)
		{
			return is_equal <t_rhs, t_lhs>::test(rhs, lhs);
		}
	};
}}


namespace text_align {
	
	// Safely compare signed and unsigned.
	template <typename t_lhs, typename t_rhs>
	constexpr bool is_equal(t_lhs lhs, t_rhs rhs)
	{
		static_assert(std::is_integral_v <t_lhs>, "Expected t_lhs to be integral");
		static_assert(std::is_integral_v <t_rhs>, "Expected t_rhs to be integral");
		return detail::is_equal <t_lhs, t_rhs>::test(lhs, rhs);
	}
}

#endif
