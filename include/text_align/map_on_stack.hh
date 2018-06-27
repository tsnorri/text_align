/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef TEXT_ALIGN_MAP_ON_STACK_HH
#define TEXT_ALIGN_MAP_ON_STACK_HH


namespace text_align { namespace detail {
	
	template <int t_count>
	struct map_on_stack
	{
		template <typename t_transform, typename t_fn, typename t_first, typename ... t_args>
		void call(t_transform &transform, t_fn &fn, t_first &&first, t_args && ... args)
		{
			transform(first, [&transform, &fn, &args...](auto &&transformed){
				map_on_stack <t_count - 1> ctx;
				ctx.template call(transform, fn, args..., transformed);
			});
		}
	};
	
	
	template <>
	struct map_on_stack <0>
	{
		template <typename t_transform, typename t_fn, typename ... t_args>
		void call(t_transform &transform, t_fn &fn, t_args && ... args)
		{
			fn(args...);
		}
	};
}}


namespace text_align {
	
	// Instantiate t_transform t, then for each object o in t_args, call t(o).
	// Finally, call t_fn with the resulting objects.
	template <typename t_transform, typename t_fn, typename ... t_args>
	void map_on_stack_fn(t_fn &&fn, t_args && ... args)
	{
		t_transform transform;
		detail::map_on_stack <sizeof...(t_args)> ctx;
		ctx.template call(transform, fn, args...);
	}
	
	
	// Instantiate t_transform t and t_fn, then for each object o in t_args, call t(o).
	// Finally, pass the objects to t_fn's operator().
	template <typename t_transform, typename t_fn, typename ... t_args>
	void map_on_stack(t_args && ... args)
	{
		t_fn fn;
		map_on_stack_fn <t_transform>(fn, args...);
	}
}

#endif
