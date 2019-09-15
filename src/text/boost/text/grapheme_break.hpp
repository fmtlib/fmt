#ifndef BOOST_TEXT_GRAPHEME_BREAK_HPP
#define BOOST_TEXT_GRAPHEME_BREAK_HPP

#include <boost/text/algorithm.hpp>
#include <boost/text/lazy_segment_range.hpp>
#include <boost/text/utility.hpp>

#include <boost/assert.hpp>

#include <array>
#include <unordered_map>

#include <stdint.h>


namespace boost { namespace text {

    /** The grapheme properties defined by Unicode. */
    enum class grapheme_property {
        Other,
        CR,
        LF,
        Control,
        Extend,
        Regional_Indicator,
        Prepend,
        SpacingMark,
        L,
        V,
        T,
        LV,
        LVT,
        ExtPict,
        ZWJ
    };

    namespace detail {
        struct grapheme_prop_interval
        {
            uint32_t lo_;
            uint32_t hi_;
            grapheme_property prop_;
        };

        inline bool operator<(
            grapheme_prop_interval lhs, grapheme_prop_interval rhs) noexcept
        {
            return lhs.hi_ <= rhs.lo_;
        }

        BOOST_TEXT_DECL std::array<grapheme_prop_interval, 6> const &
        make_grapheme_prop_intervals();
        BOOST_TEXT_DECL std::unordered_map<uint32_t, grapheme_property>
        make_grapheme_prop_map();
    }

    /** Returns the grapheme property associated with code point `cp`. */
    inline grapheme_property grapheme_prop(uint32_t cp) noexcept
    {
        static auto const map = detail::make_grapheme_prop_map();
        static auto const intervals = detail::make_grapheme_prop_intervals();

        auto const it = map.find(cp);
        if (it == map.end()) {
            auto const it2 = std::lower_bound(
                intervals.begin(),
                intervals.end(),
                detail::grapheme_prop_interval{cp, cp + 1});
            if (it2 == intervals.end() || cp < it2->lo_ || it2->hi_ <= cp)
                return grapheme_property::Other;
            return it2->prop_;
        }
        return it->second;
    }

    namespace detail {
        inline bool skippable(grapheme_property prop) noexcept
        {
            return prop == grapheme_property::Extend;
        }

        enum class grapheme_break_emoji_state_t {
            none,
            first_emoji, // Indicates that prop points to an odd-count
                         // emoji.
            second_emoji // Indicates that prop points to an even-count
                         // emoji.
        };

        template<typename CPIter>
        struct grapheme_break_state
        {
            CPIter it;

            grapheme_property prev_prop;
            grapheme_property prop;

            grapheme_break_emoji_state_t emoji_state;
        };

        template<typename CPIter>
        grapheme_break_state<CPIter> next(grapheme_break_state<CPIter> state)
        {
            ++state.it;
            state.prev_prop = state.prop;
            return state;
        }

        template<typename CPIter>
        grapheme_break_state<CPIter> prev(grapheme_break_state<CPIter> state)
        {
            --state.it;
            state.prop = state.prev_prop;
            return state;
        }

        template<typename CPIter>
        bool gb11_prefix(CPIter first, CPIter prev_it)
        {
            auto final_prop = grapheme_property::Other;
            find_if_backward(first, prev_it, [&final_prop](uint32_t cp) {
                final_prop = grapheme_prop(cp);
                return final_prop != grapheme_property::Extend;
            });
            return final_prop == grapheme_property::ExtPict;
        }

        inline bool table_grapheme_break(
            grapheme_property lhs, grapheme_property rhs) noexcept
        {
            // Note that RI.RI was changed to '1' since that case is handled
            // in the grapheme break FSM.

            // clang-format off
// See chart at https://unicode.org/Public/11.0.0/ucd/auxiliary/GraphemeBreakTest.html .
constexpr std::array<std::array<bool, 15>, 15> grapheme_breaks = {{
//   Other CR LF Ctrl Ext RI Pre SpcMk L  V  T  LV LVT ExtPict ZWJ
    {{1,   1, 1, 1,   0,  1, 1,  0,    1, 1, 1, 1, 1,  1,      0}}, // Other
    {{1,   1, 0, 1,   1,  1, 1,  1,    1, 1, 1, 1, 1,  1,      1}}, // CR
    {{1,   1, 1, 1,   1,  1, 1,  1,    1, 1, 1, 1, 1,  1,      1}}, // LF
    {{1,   1, 1, 1,   1,  1, 1,  1,    1, 1, 1, 1, 1,  1,      1}}, // Control
    {{1,   1, 1, 1,   0,  1, 1,  0,    1, 1, 1, 1, 1,  1,      0}}, // Extend
    {{1,   1, 1, 1,   0,  1, 1,  0,    1, 1, 1, 1, 1,  1,      0}}, // RI
    {{0,   1, 1, 1,   0,  0, 0,  0,    0, 0, 0, 0, 0,  0,      0}}, // Prepend
    {{1,   1, 1, 1,   0,  1, 1,  0,    1, 1, 1, 1, 1,  1,      0}}, // SpacingMark
    {{1,   1, 1, 1,   0,  1, 1,  0,    0, 0, 1, 0, 0,  1,      0}}, // L
    {{1,   1, 1, 1,   0,  1, 1,  0,    1, 0, 0, 1, 1,  1,      0}}, // V
    {{1,   1, 1, 1,   0,  1, 1,  0,    1, 1, 0, 1, 1,  1,      0}}, // T
    {{1,   1, 1, 1,   0,  1, 1,  0,    1, 0, 0, 1, 1,  1,      0}}, // LV
    {{1,   1, 1, 1,   0,  1, 1,  0,    1, 1, 0, 1, 1,  1,      0}}, // LVT
    {{1,   1, 1, 1,   0,  1, 1,  0,    1, 1, 1, 1, 1,  1,      0}}, // ExtPict
    {{1,   1, 1, 1,   0,  1, 1,  0,    1, 1, 1, 1, 1,  1,      0}}, // ZWJ

}};
            // clang-format on
            auto const lhs_int = static_cast<int>(lhs);
            auto const rhs_int = static_cast<int>(rhs);
            return grapheme_breaks[lhs_int][rhs_int];
        }
    }

#ifdef BOOST_TEXT_DOXYGEN

    /** Finds the nearest grapheme break at or before before `it`.  If `it ==
        first`, that is returned.  Otherwise, the first code point of the
        grapheme that `it` is within is returned (even if `it` is already at
        the first code point of a grapheme).

        This function only participates in overload resolution if `CPIter`
        models the CPIter concept. */
    template<typename CPIter, typename Sentinel>
    CPIter prev_grapheme_break(CPIter first, CPIter it, Sentinel last) noexcept;

    /** Finds the next word break after `first`.  This will be the first code
        point after the current word, or `last` if no next word exists.

        This function only participates in overload resolution if `CPIter`
        models the CPIter concept.

        \pre `first` is at the beginning of a word. */
    template<typename CPIter, typename Sentinel>
    CPIter next_grapheme_break(CPIter first, Sentinel last) noexcept;

    /** Finds the nearest grapheme break at or before before `it`.  If `it ==
        range.begin()`, that is returned.  Otherwise, the first code point of
        the grapheme that `it` is within is returned (even if `it` is already
        at the first code point of a grapheme).

        This function only participates in overload resolution if `CPRange`
        models the CPRange concept. */
    template<typename CPRange, typename CPIter>
    detail::undefined prev_grapheme_break(CPRange & range, CPIter it) noexcept;

    /** Finds the next grapheme break after `it`.  This will be the first code
        point after the current grapheme, or `range.end()` if no next grapheme
        exists.

        This function only participates in overload resolution if `CPRange`
        models the CPRange concept.

        \pre `it` is at the beginning of a grapheme. */
    template<typename CPRange, typename CPIter>
    detail::undefined next_grapheme_break(CPRange & range, CPIter it) noexcept;

#else

    template<typename CPIter, typename Sentinel>
    auto prev_grapheme_break(CPIter first, CPIter it, Sentinel last) noexcept
        -> detail::cp_iter_ret_t<CPIter, CPIter>
    {
        if (it == first)
            return it;

        if (it == last && --it == first)
            return it;

        detail::grapheme_break_state<CPIter> state;
        state.it = it;
        state.prop = grapheme_prop(*state.it);
        state.prev_prop = grapheme_prop(*std::prev(state.it));
        state.emoji_state = detail::grapheme_break_emoji_state_t::none;

        for (; state.it != first; state = prev(state)) {
            state.prev_prop = grapheme_prop(*std::prev(state.it));

            // When we see an RI, back up to the first RI so we can see what
            // emoji state we're supposed to be in here.
            if (state.emoji_state ==
                    detail::grapheme_break_emoji_state_t::none &&
                state.prop == grapheme_property::Regional_Indicator) {
                int ris_before = 0;
                find_if_not_backward(
                    first, state.it, [&ris_before](uint32_t cp) {
                        bool const ri = grapheme_prop(cp) ==
                                        grapheme_property::Regional_Indicator;
                        if (ri)
                            ++ris_before;
                        return ri;
                    });
                state.emoji_state =
                    (ris_before % 2 == 0)
                        ? detail::grapheme_break_emoji_state_t::first_emoji
                        : detail::grapheme_break_emoji_state_t::second_emoji;
            }

            // GB11
            if (state.prev_prop == grapheme_property::ZWJ &&
                state.prop == grapheme_property::ExtPict &&
                detail::gb11_prefix(first, std::prev(state.it))) {
                continue;
            }

            if (state.emoji_state ==
                detail::grapheme_break_emoji_state_t::first_emoji) {
                if (state.prev_prop == grapheme_property::Regional_Indicator) {
                    state.emoji_state =
                        detail::grapheme_break_emoji_state_t::second_emoji;
                    return state.it;
                } else {
                    state.emoji_state =
                        detail::grapheme_break_emoji_state_t::none;
                }
            } else if (
                state.emoji_state ==
                    detail::grapheme_break_emoji_state_t::second_emoji &&
                state.prev_prop == grapheme_property::Regional_Indicator) {
                state.emoji_state =
                    detail::grapheme_break_emoji_state_t::first_emoji;
                continue;
            }

            if (detail::table_grapheme_break(state.prev_prop, state.prop))
                return state.it;
        }

        return first;
    }

    template<typename CPIter, typename Sentinel>
    auto next_grapheme_break(CPIter first, Sentinel last) noexcept
        -> detail::cp_iter_ret_t<CPIter, CPIter>
    {
        if (first == last)
            return first;

        detail::grapheme_break_state<CPIter> state;
        state.it = first;

        if (++state.it == last)
            return state.it;

        state.prev_prop = grapheme_prop(*std::prev(state.it));
        state.prop = grapheme_prop(*state.it);

        state.emoji_state =
            state.prev_prop == grapheme_property::Regional_Indicator
                ? detail::grapheme_break_emoji_state_t::first_emoji
                : detail::grapheme_break_emoji_state_t::none;

        for (; state.it != last; state = next(state)) {
            state.prop = grapheme_prop(*state.it);

            // GB11
            if (state.prev_prop == grapheme_property::ZWJ &&
                state.prop == grapheme_property::ExtPict &&
                detail::gb11_prefix(first, std::prev(state.it))) {
                continue;
            }

            if (state.emoji_state ==
                detail::grapheme_break_emoji_state_t::first_emoji) {
                if (state.prop == grapheme_property::Regional_Indicator) {
                    state.emoji_state =
                        detail::grapheme_break_emoji_state_t::none;
                    continue;
                } else {
                    state.emoji_state =
                        detail::grapheme_break_emoji_state_t::none;
                }
            } else if (state.prop == grapheme_property::Regional_Indicator) {
                state.emoji_state =
                    detail::grapheme_break_emoji_state_t::first_emoji;
            }

            if (detail::table_grapheme_break(state.prev_prop, state.prop))
                return state.it;
        }

        return state.it;
    }

    template<typename CPRange, typename CPIter>
    auto prev_grapheme_break(CPRange & range, CPIter it) noexcept
        -> detail::cp_rng_alg_ret_t<detail::iterator_t<CPRange>, CPRange>
    {
        return prev_grapheme_break(std::begin(range), it, std::end(range));
    }

    template<typename CPRange, typename CPIter>
    auto next_grapheme_break(CPRange & range, CPIter it) noexcept
        -> detail::cp_rng_alg_ret_t<detail::iterator_t<CPRange>, CPRange>
    {
        return next_grapheme_break(it, std::end(range));
    }

#endif

    namespace detail {
        template<typename CPIter, typename Sentinel>
        struct next_grapheme_callable
        {
            CPIter operator()(CPIter it, Sentinel last) const noexcept
            {
                return next_grapheme_break(it, last);
            }
        };

        template<typename CPIter>
        struct prev_grapheme_callable
        {
            CPIter operator()(CPIter first, CPIter it, CPIter last) const
                noexcept
            {
                return prev_grapheme_break(first, it, last);
            }
        };
    }

#if 0
    /** Returns the bounds of the grapheme that `it` lies within. */
    template<typename CPIter, typename Sentinel>
    cp_range<CPIter> grapheme(CPIter first, CPIter it, Sentinel last) noexcept
    {
        first = prev_grapheme_break(first, it, last);
        return cp_range<CPIter>{first, next_grapheme_break(first, last)};
    }
#endif

#ifdef BOOST_TEXT_DOXYGEN

#if 0
    /** Returns the bounds of the grapheme that `it` lies within,
        as a cp_range. */
    template<typename CPRange, typename CPIter>
    detail::undefined grapheme(CPRange & range, CPIter it) noexcept;
#endif

    /** Returns a lazy range of the code point ranges delimiting graphemes in
        `[first, last)`. */
    template<typename CPIter, typename Sentinel>
    detail::undefined graphemes(CPIter first, Sentinel last) noexcept;

    /** Returns a lazy range of the code point ranges delimiting graphemes in
        `range`. */
    template<typename CPRange>
    detail::undefined graphemes(CPRange & range) noexcept;

    /** Returns a lazy range of the code point ranges delimiting graphemes in
        `[first, last)`, in reverse. */
    template<typename CPIter>
    detail::undefined reversed_graphemes(CPIter first, CPIter last) noexcept;

    /** Returns a lazy range of the code point ranges delimiting graphemes in
        `range`, in reverse. */
    template<typename CPRange>
    detail::undefined reversed_graphemes(CPRange & range) noexcept;

#else

#if 0
    template<typename CPRange, typename CPIter>
    auto grapheme(CPRange & range, CPIter it) noexcept
        -> cp_range<detail::iterator_t<CPRange>>
    {
        auto first =
            prev_grapheme_break(std::begin(range), it, std::end(range));
        return cp_range<CPIter>{first, next_grapheme_break(first, range.end())};
    }
#endif

    template<typename CPIter, typename Sentinel>
    lazy_segment_range<
        CPIter,
        Sentinel,
        detail::next_grapheme_callable<CPIter, Sentinel>>
    graphemes(CPIter first, Sentinel last) noexcept
    {
        detail::next_grapheme_callable<CPIter, Sentinel> next;
        return {std::move(next), {first, last}, {last}};
    }

    template<typename CPRange>
    auto graphemes(CPRange & range) noexcept -> lazy_segment_range<
        detail::iterator_t<CPRange>,
        detail::sentinel_t<CPRange>,
        detail::next_grapheme_callable<
            detail::iterator_t<CPRange>,
            detail::sentinel_t<CPRange>>>
    {
        detail::next_grapheme_callable<
            detail::iterator_t<CPRange>,
            detail::sentinel_t<CPRange>>
            next;
        return {std::move(next),
                {std::begin(range), std::end(range)},
                {std::end(range)}};
    }

    template<typename CPIter>
    lazy_segment_range<
        CPIter,
        CPIter,
        detail::prev_grapheme_callable<CPIter>,
        cp_range<CPIter>,
        detail::const_reverse_lazy_segment_iterator,
        true>
    reversed_graphemes(CPIter first, CPIter last) noexcept
    {
        detail::prev_grapheme_callable<CPIter> prev;
        return {std::move(prev), {first, last, last}, {first, first, last}};
    }

    template<typename CPRange>
    auto reversed_graphemes(CPRange & range) noexcept -> lazy_segment_range<
        detail::iterator_t<CPRange>,
        detail::sentinel_t<CPRange>,
        detail::prev_grapheme_callable<detail::iterator_t<CPRange>>,
        cp_range<detail::iterator_t<CPRange>>,
        detail::const_reverse_lazy_segment_iterator,
        true>
    {
        detail::prev_grapheme_callable<detail::iterator_t<CPRange>> prev;
        return {std::move(prev),
                {std::begin(range), std::end(range), std::end(range)},
                {std::begin(range), std::begin(range), std::end(range)}};
    }

#endif

}}

#endif
