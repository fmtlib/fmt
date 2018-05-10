// Formatting library for C++ - the core API
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.
//
// Copyright (c) 2018 - present, Remotion (Igor Schulz)
// All Rights Reserved
// {fmt} support for ranges, containers and types tuple interface.

#ifndef FMT_RANGES_H_
#define FMT_RANGES_H_

#include "format.h"
#include <type_traits>

// output only up to N items from the range.
#ifndef FMT_RANGE_OUTPUT_LENGTH_LIMIT
#define FMT_RANGE_OUTPUT_LENGTH_LIMIT 256
#endif

namespace fmt {

template <typename Char>
struct formatting_base {
	template <typename ParseContext>
	FMT_CONSTEXPR auto parse(ParseContext &ctx) -> decltype(ctx.begin()) { return ctx.begin(); }
};

template <typename Char, typename Enable = void>
struct formatting_range : formatting_base<Char>
{
	Char prefix = '{';
	Char delimiter = ',';
	Char postfix = '}';
	bool add_spaces = true;
};

template <typename Char, typename Enable = void>
struct formatting_tuple : formatting_base<Char>
{
	Char prefix = '[';
	Char delimiter = ',';
	Char postfix = ']';
	bool add_spaces = true;
};

template<typename RangeT, typename OutputIterator>
void copy(const RangeT &range, OutputIterator out) {
	for (const auto& it : range) {
		*out++ = it;
	}
}

template<typename OutputIterator>
void copy(const char *str, OutputIterator out) {
	const char* p_curr = str;
	while (*p_curr) {
		*out++ = *p_curr++;
	}
}

template<typename OutputIterator>
void copy(const char ch, OutputIterator out) {
	*out++ = ch;
}

} // namespace fmt


namespace fmt {
namespace meta {

/// Return true value if T has std::string interface, like std::string_view.
template<typename T>
class is_like_std_string {
	template<typename U> static auto check(U* p) -> decltype(
		p->find('a')
		, p->length()
		, p->data()
		, int());
	template<typename  > static void check(...);
public:
	static const bool value = !std::is_void< decltype(check<T>(nullptr)) >::value;
};

template<typename T>
constexpr bool is_like_std_string_v = is_like_std_string<T>::value;

template<typename... Ts>
struct conditional_helper {};

template<typename T, typename _ = void>
struct is_range_ : std::false_type {};

template<typename T>
struct is_range_<T,
	std::conditional_t< false,
	conditional_helper<
	decltype(std::declval<T>().begin()),
	decltype(std::declval<T>().end())
	>, void>
> : std::true_type {};

template<typename T>
constexpr bool is_range_v = is_range_<T>::value && !is_like_std_string<T>::value;


/// tuple_size and tuple_element check.
template<typename T>
class is_tuple_like_ {
	template<typename U> static auto check(U* p) -> decltype(
		std::tuple_size< U >::value,
		std::declval<typename std::tuple_element<0, U>::type>(),
		int());
	template<typename  > static void check(...);
public:
	static constexpr bool value = !std::is_void< decltype(check<T>(nullptr)) >::value;
};

template<typename T>
constexpr bool is_tuple_like_v = is_tuple_like_<T>::value && !is_range_<T>::value;


//=--------------------------------------------------------------------------------------------------------------------
template<size_t... Is, class Tuple, class F>
void for_each(std::index_sequence<Is...>, Tuple&& tup, F&& f) noexcept {
	using std::get;
	// using free function get<I>(T) now.
	const int _[] = { 0,
		((void)f(get<Is>(tup)),
		0)... };
	(void)_; // blocks warnings
}
//=--------------------------------------------------------------------------------------------------------------------
template<class T>
constexpr std::make_index_sequence<std::tuple_size<T>::value>
get_indexes(T const&) { return {}; }

//=--------------------------------------------------------------------------------------------------------------------
template<class Tuple, class F>
void for_each(Tuple&& tup, F&& f) {
	const auto indexes = get_indexes(tup);
	for_each(indexes, std::forward<Tuple>(tup), std::forward<F>(f));
}

} // namespace meta
} // namespace fmt

namespace fmt {

// =====================================================================================================================
template<typename TupleT, typename Char>
struct formatter< TupleT, Char
	, std::enable_if_t<fmt::meta::is_tuple_like_v<TupleT>> >
{
	fmt::formatting_tuple<Char> formating;

	template <typename ParseContext>
	FMT_CONSTEXPR auto parse(ParseContext &ctx) -> decltype(ctx.begin()) {
		return formating.parse(ctx);
	}

	template <typename FormatContext = format_context>
	auto format(const TupleT &values, FormatContext &ctx) -> decltype(ctx.out()) {
		auto out = ctx.out();
		std::ptrdiff_t i = 0;
		fmt::copy(formating.prefix, out);
		fmt::meta::for_each(values, [&](const auto &v) {
			if (i++ > 0) { fmt::copy(formating.delimiter, out); }
			if (formating.add_spaces) { format_to(out, " {}", v); }
			else { format_to(out, "{}", v); }
		});
		if (formating.add_spaces) { *out++ = ' '; }
		fmt::copy(formating.postfix, out);

		return ctx.out();
	}
};

} // namespace fmt



namespace fmt {


template<typename RangeT, typename Char>
struct formatter <RangeT, Char, std::enable_if_t<fmt::meta::is_range_v<RangeT>> >
{
	static constexpr std::ptrdiff_t range_length_limit = FMT_RANGE_OUTPUT_LENGTH_LIMIT; // output only up to N items from the range.

	fmt::formatting_range<Char> formating;

	template <typename ParseContext>
	FMT_CONSTEXPR auto parse(ParseContext &ctx) -> decltype(ctx.begin()) {
		return formating.parse(ctx);
	}

	template <typename FormatContext>
	auto format(const RangeT &values, FormatContext &ctx) -> decltype(ctx.out()) {
		auto out = ctx.out();
		fmt::copy(formating.prefix, out);
		std::ptrdiff_t i = 0;
		for (const auto& it : values) {
			if (i > 0) { fmt::copy(formating.delimiter, out); }
			if (formating.add_spaces) { format_to(out, " {}", it); }
			else { format_to(out, "{}", it); }
			if (++i > range_length_limit) {
				format_to(out, " ... <other elements>");
				break;
			}
		}
		if (formating.add_spaces) { *out++ = ' '; }
		fmt::copy(formating.postfix, out);
		return ctx.out();
	}
};

} // namespace fmt


#endif // FMT_RANGES_H_