#ifndef BOOST_TEXT_GRAPHEME_BREAK_HPP
#define BOOST_TEXT_GRAPHEME_BREAK_HPP

#include <boost/text/algorithm.hpp>
#include <boost/text/config.hpp>

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

    template<typename CPIter, typename Sentinel>
    CPIter next_grapheme_break(CPIter first, Sentinel last) noexcept
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

}}

#endif
