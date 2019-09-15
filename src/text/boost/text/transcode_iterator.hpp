#ifndef BOOST_TEXT_TRANSCODE_ITERATOR_HPP
#define BOOST_TEXT_TRANSCODE_ITERATOR_HPP

#include <boost/assert.hpp>
#include <boost/text/config.hpp>

#include <boost/throw_exception.hpp>

#include <array>
#include <iterator>
#include <type_traits>
#include <stdexcept>


namespace boost { namespace text {

    namespace detail {
        constexpr bool
        in(unsigned char lo, unsigned char c, unsigned char hi) noexcept
        {
            return lo <= c && c <= hi;
        }

        struct throw_on_encoding_error
        {};

        inline BOOST_TEXT_CXX14_CONSTEXPR int
        read_into_buf(uint32_t cp, char buf[5])
        {
            if (cp < 0x80) {
                buf[0] = static_cast<char>(cp);
                buf[1] = '\0';
                return 0;
            } else if (cp < 0x800) {
                buf[0] = static_cast<char>(0xC0 + (cp >> 6));
                buf[1] = static_cast<char>(0x80 + (cp & 0x3f));
                buf[2] = '\0';
                return 1;
            } else if (cp < 0x10000) {
                buf[0] = static_cast<char>(0xe0 + (cp >> 12));
                buf[1] = static_cast<char>(0x80 + ((cp >> 6) & 0x3f));
                buf[2] = static_cast<char>(0x80 + (cp & 0x3f));
                buf[3] = '\0';
                return 2;
            } else {
                buf[0] = static_cast<char>(0xf0 + (cp >> 18));
                buf[1] = static_cast<char>(0x80 + ((cp >> 12) & 0x3f));
                buf[2] = static_cast<char>(0x80 + ((cp >> 6) & 0x3f));
                buf[3] = static_cast<char>(0x80 + (cp & 0x3f));
                buf[4] = '\0';
                return 3;
            }
        }

        template<typename T, typename U>
        struct enable_utf16_cp
            : std::enable_if<std::is_integral<T>::value && sizeof(T) == 2u, U>
        {
        };
        template<typename T, typename U = T>
        using enable_utf16_cp_t = typename enable_utf16_cp<T, U>::type;
    }

    /** The replacement character used to mark invalid portions of a Unicode
        sequence when converting between two encodings.

        \see Unicode 3.2/C10 */
    constexpr uint32_t replacement_character() noexcept { return 0xfffd; }

    /** Returns true if c is a Unicode surrogate, or false otherwise.

        This function is constexpr in C++14 and later. */
    inline BOOST_TEXT_CXX14_CONSTEXPR bool surrogate(uint32_t c) noexcept
    {
        uint32_t const high_surrogate_min = 0xd800;
        uint32_t const low_surrogate_max = 0xdfff;
        return high_surrogate_min <= c && c <= low_surrogate_max;
    }

    /** Returns true if c is a Unicode high surrogate, or false otherwise.

        This function is constexpr in C++14 and later. */
    inline BOOST_TEXT_CXX14_CONSTEXPR bool high_surrogate(uint32_t c) noexcept
    {
        uint32_t const high_surrogate_min = 0xd800;
        uint32_t const high_surrogate_max = 0xdbff;
        return high_surrogate_min <= c && c <= high_surrogate_max;
    }

    /** Returns true if c is a Unicode low surrogate, or false otherwise.

        This function is constexpr in C++14 and later. */
    inline BOOST_TEXT_CXX14_CONSTEXPR bool low_surrogate(uint32_t c) noexcept
    {
        uint32_t const low_surrogate_min = 0xdc00;
        uint32_t const low_surrogate_max = 0xdfff;
        return low_surrogate_min <= c && c <= low_surrogate_max;
    }

    /** Returns true if c is a Unicode reserved noncharacter, or false
        otherwise.

        This function is constexpr in C++14 and later.

        \see Unicode 3.4/D14 */
    inline BOOST_TEXT_CXX14_CONSTEXPR bool
    reserved_noncharacter(uint32_t c) noexcept
    {
        bool const byte01_reserved = (c & 0xffff) >= 0xfffe;
        bool const byte2_at_most_0x10 = ((c & 0xff0000u) >> 16) <= 0x10;
        return (byte01_reserved && byte2_at_most_0x10) ||
               (0xfdd0 <= c && c <= 0xfdef);
    }

    /** Returns true if c is a valid Unicode code point, or false otherwise.

        This function is constexpr in C++14 and later.

        \see Unicode 3.9/D90 */
    inline BOOST_TEXT_CXX14_CONSTEXPR bool valid_code_point(uint32_t c) noexcept
    {
        return c <= 0x10ffff && !surrogate(c) && !reserved_noncharacter(c);
    }

    /** Given the first (and possibly only) code unit of a UTF-8 code point,
        returns the number of bytes occupied by that code point (in the range
        [1, 4]).  Returns a value < 0 if first is not a valid initial UTF-8
        code unit.

        This function is constexpr in C++14 and later. */
    inline BOOST_TEXT_CXX14_CONSTEXPR int
    code_point_bytes(unsigned char first) noexcept
    {
        using detail::in;
        if (first <= 0x7f)
            return 1;
        if (in(0xc2, first, 0xdf))
            return 2;
        if (in(0xe0, first, 0xef))
            return 3;
        if (in(0xf0, first, 0xf4))
            return 4;
        return -1;
    }

    /** Given the first (and possibly only) code unit of a UTF-16 code point,
        returns the number of code units occupied by that code point (in the
        range [1, 2]).  Returns a value < 0 if first is not a valid initial
        UTF-16 code unit.

        This function is constexpr in C++14 and later. */
    template<typename T>
    BOOST_TEXT_CXX14_CONSTEXPR detail::enable_utf16_cp_t<T, int>
    code_point_units(T first) noexcept
    {
        if (boost::text::low_surrogate(first))
            return -1;
        if (boost::text::high_surrogate(first))
            return 2;
        return 1;
    }

    /** Returns true if c is a UTF-8 continuation code unit, and false
        otherwise.  If optional parameters lo and hi are given, the code unit
        must also lie in the range [lo, hi]. */
    constexpr bool continuation(
        unsigned char c,
        unsigned char lo = 0x80,
        unsigned char hi = 0xbf) noexcept
    {
        return detail::in(lo, c, hi);
    }

    namespace detail {
        // optional is not constexpr friendly.
        template<typename Iter>
        struct optional_iter
        {
            constexpr optional_iter() : it_(), valid_(false) {}
            constexpr optional_iter(Iter it) : it_(it), valid_(true) {}

            BOOST_TEXT_CXX14_CONSTEXPR operator bool() const noexcept
            {
                return valid_;
            }
            BOOST_TEXT_CXX14_CONSTEXPR Iter operator*() const noexcept
            {
                BOOST_ASSERT(valid_);
                return it_;
            }
            Iter & operator*() noexcept
            {
                BOOST_ASSERT(valid_);
                return it_;
            }

            friend BOOST_TEXT_CXX14_CONSTEXPR bool
            operator==(optional_iter lhs, optional_iter rhs) noexcept
            {
                return lhs.valid_ == rhs.valid_ &&
                       (!lhs.valid_ || lhs.it_ == rhs.it_);
            }
            friend BOOST_TEXT_CXX14_CONSTEXPR bool
            operator!=(optional_iter lhs, optional_iter rhs) noexcept
            {
                return !(lhs == rhs);
            }

        private:
            Iter it_;
            bool valid_;
        };

        // Follow Table 3-7 in Unicode 3.9/D92
        template<typename Iter>
        BOOST_TEXT_CXX14_CONSTEXPR optional_iter<Iter>
        end_of_invalid_utf8(Iter it) noexcept
        {
            BOOST_ASSERT(!boost::text::continuation(*it));

            using detail::in;

            if (in(0, *it, 0x7f))
                return optional_iter<Iter>{};

            if (in(0xc2, *it, 0xdf)) {
                auto next = it;
                if (!boost::text::continuation(*++next))
                    return next;
                return optional_iter<Iter>{};
            }

            if (in(0xe0, *it, 0xe0)) {
                auto next = it;
                if (!boost::text::continuation(*++next, 0xa0, 0xbf))
                    return next;
                if (!boost::text::continuation(*++next))
                    return next;
                return optional_iter<Iter>{};
            }
            if (in(0xe1, *it, 0xec)) {
                auto next = it;
                if (!boost::text::continuation(*++next))
                    return next;
                if (!boost::text::continuation(*++next))
                    return next;
                return optional_iter<Iter>{};
            }
            if (in(0xed, *it, 0xed)) {
                auto next = it;
                if (!boost::text::continuation(*++next, 0x80, 0x9f))
                    return next;
                if (!boost::text::continuation(*++next))
                    return next;
                return optional_iter<Iter>{};
            }
            if (in(0xee, *it, 0xef)) {
                auto next = it;
                if (!boost::text::continuation(*++next))
                    return next;
                if (!boost::text::continuation(*++next))
                    return next;
                return optional_iter<Iter>{};
            }

            if (in(0xf0, *it, 0xf0)) {
                auto next = it;
                if (!boost::text::continuation(*++next, 0x90, 0xbf))
                    return next;
                if (!boost::text::continuation(*++next))
                    return next;
                if (!boost::text::continuation(*++next))
                    return next;
                return optional_iter<Iter>{};
            }
            if (in(0xf1, *it, 0xf3)) {
                auto next = it;
                if (!boost::text::continuation(*++next))
                    return next;
                if (!boost::text::continuation(*++next))
                    return next;
                if (!boost::text::continuation(*++next))
                    return next;
                return optional_iter<Iter>{};
            }
            if (in(0xf4, *it, 0xf4)) {
                auto next = it;
                if (!boost::text::continuation(*++next, 0x80, 0x8f))
                    return next;
                if (!boost::text::continuation(*++next))
                    return next;
                if (!boost::text::continuation(*++next))
                    return next;
                return optional_iter<Iter>{};
            }

            return it;
        }

        template<typename Iter>
        BOOST_TEXT_CXX14_CONSTEXPR Iter decrement(Iter it) noexcept
        {
            Iter retval = it;

            int backup = 0;
            while (backup < 4 && boost::text::continuation(*--retval)) {
                ++backup;
            }
            backup = it - retval;

            if (boost::text::continuation(*retval))
                return it - 1;

            optional_iter<Iter> first_invalid = end_of_invalid_utf8(retval);
            if (first_invalid == retval)
                ++*first_invalid;
            while (first_invalid && (*first_invalid - retval) < backup) {
                backup -= *first_invalid - retval;
                retval = *first_invalid;
                first_invalid = end_of_invalid_utf8(retval);
                if (first_invalid == retval)
                    ++*first_invalid;
            }

            if (1 < backup) {
                int const cp_bytes = boost::text::code_point_bytes(*retval);
                if (cp_bytes < backup)
                    retval = it - 1;
            }

            return retval;
        }

        template<typename Iter>
        BOOST_TEXT_CXX14_CONSTEXPR Iter decrement(Iter first, Iter it) noexcept
        {
            Iter retval = it;

            int backup = 0;
            while (backup < 4 && it != first &&
                   boost::text::continuation(*--retval)) {
                ++backup;
            }
            backup = std::distance(retval, it);

            if (boost::text::continuation(*retval)) {
                if (it != first)
                    --it;
                return it;
            }

            optional_iter<Iter> first_invalid = end_of_invalid_utf8(retval);
            if (first_invalid == retval)
                ++*first_invalid;
            while (first_invalid &&
                   std::distance(retval, *first_invalid) < backup) {
                backup -= std::distance(retval, *first_invalid);
                retval = *first_invalid;
                first_invalid = end_of_invalid_utf8(retval);
                if (first_invalid == retval)
                    ++*first_invalid;
            }

            if (1 < backup) {
                int const cp_bytes = boost::text::code_point_bytes(*retval);
                if (cp_bytes < backup) {
                    if (it != first)
                        --it;
                    retval = it;
                }
            }

            return retval;
        }

        enum char_class : uint8_t {
            ill = 0,
            asc = 1,
            cr1 = 2,
            cr2 = 3,
            cr3 = 4,
            l2a = 5,
            l3a = 6,
            l3b = 7,
            l3c = 8,
            l4a = 9,
            l4b = 10,
            l4c = 11,
        };

        enum table_state : uint8_t {
            bgn = 0,
            end = bgn,
            err = 12,
            cs1 = 24,
            cs2 = 36,
            cs3 = 48,
            p3a = 60,
            p3b = 72,
            p4a = 84,
            p4b = 96,
        };

        struct first_cu
        {
            unsigned char initial_octet;
            table_state next;
        };

        template<typename InputIter, typename Sentinel>
        uint32_t advance(InputIter & first, Sentinel last)
        {
            uint32_t retval = 0;

            constexpr static first_cu first_cus[256] = {
                {0x00, bgn}, {0x01, bgn}, {0x02, bgn}, {0x03, bgn}, {0x04, bgn},
                {0x05, bgn}, {0x06, bgn}, {0x07, bgn}, {0x08, bgn}, {0x09, bgn},
                {0x0a, bgn}, {0x0b, bgn}, {0x0c, bgn}, {0x0d, bgn}, {0x0e, bgn},
                {0x0f, bgn}, {0x10, bgn}, {0x11, bgn}, {0x12, bgn}, {0x13, bgn},
                {0x14, bgn}, {0x15, bgn}, {0x16, bgn}, {0x17, bgn}, {0x18, bgn},
                {0x19, bgn}, {0x1a, bgn}, {0x1b, bgn}, {0x1c, bgn}, {0x1d, bgn},
                {0x1e, bgn}, {0x1f, bgn}, {0x20, bgn}, {0x21, bgn}, {0x22, bgn},
                {0x23, bgn}, {0x24, bgn}, {0x25, bgn}, {0x26, bgn}, {0x27, bgn},
                {0x28, bgn}, {0x29, bgn}, {0x2a, bgn}, {0x2b, bgn}, {0x2c, bgn},
                {0x2d, bgn}, {0x2e, bgn}, {0x2f, bgn}, {0x30, bgn}, {0x31, bgn},
                {0x32, bgn}, {0x33, bgn}, {0x34, bgn}, {0x35, bgn}, {0x36, bgn},
                {0x37, bgn}, {0x38, bgn}, {0x39, bgn}, {0x3a, bgn}, {0x3b, bgn},
                {0x3c, bgn}, {0x3d, bgn}, {0x3e, bgn}, {0x3f, bgn}, {0x40, bgn},
                {0x41, bgn}, {0x42, bgn}, {0x43, bgn}, {0x44, bgn}, {0x45, bgn},
                {0x46, bgn}, {0x47, bgn}, {0x48, bgn}, {0x49, bgn}, {0x4a, bgn},
                {0x4b, bgn}, {0x4c, bgn}, {0x4d, bgn}, {0x4e, bgn}, {0x4f, bgn},
                {0x50, bgn}, {0x51, bgn}, {0x52, bgn}, {0x53, bgn}, {0x54, bgn},
                {0x55, bgn}, {0x56, bgn}, {0x57, bgn}, {0x58, bgn}, {0x59, bgn},
                {0x5a, bgn}, {0x5b, bgn}, {0x5c, bgn}, {0x5d, bgn}, {0x5e, bgn},
                {0x5f, bgn}, {0x60, bgn}, {0x61, bgn}, {0x62, bgn}, {0x63, bgn},
                {0x64, bgn}, {0x65, bgn}, {0x66, bgn}, {0x67, bgn}, {0x68, bgn},
                {0x69, bgn}, {0x6a, bgn}, {0x6b, bgn}, {0x6c, bgn}, {0x6d, bgn},
                {0x6e, bgn}, {0x6f, bgn}, {0x70, bgn}, {0x71, bgn}, {0x72, bgn},
                {0x73, bgn}, {0x74, bgn}, {0x75, bgn}, {0x76, bgn}, {0x77, bgn},
                {0x78, bgn}, {0x79, bgn}, {0x7a, bgn}, {0x7b, bgn}, {0x7c, bgn},
                {0x7d, bgn}, {0x7e, bgn}, {0x7f, bgn}, {0x00, err}, {0x01, err},
                {0x02, err}, {0x03, err}, {0x04, err}, {0x05, err}, {0x06, err},
                {0x07, err}, {0x08, err}, {0x09, err}, {0x0a, err}, {0x0b, err},
                {0x0c, err}, {0x0d, err}, {0x0e, err}, {0x0f, err}, {0x10, err},
                {0x11, err}, {0x12, err}, {0x13, err}, {0x14, err}, {0x15, err},
                {0x16, err}, {0x17, err}, {0x18, err}, {0x19, err}, {0x1a, err},
                {0x1b, err}, {0x1c, err}, {0x1d, err}, {0x1e, err}, {0x1f, err},
                {0x20, err}, {0x21, err}, {0x22, err}, {0x23, err}, {0x24, err},
                {0x25, err}, {0x26, err}, {0x27, err}, {0x28, err}, {0x29, err},
                {0x2a, err}, {0x2b, err}, {0x2c, err}, {0x2d, err}, {0x2e, err},
                {0x2f, err}, {0x30, err}, {0x31, err}, {0x32, err}, {0x33, err},
                {0x34, err}, {0x35, err}, {0x36, err}, {0x37, err}, {0x38, err},
                {0x39, err}, {0x3a, err}, {0x3b, err}, {0x3c, err}, {0x3d, err},
                {0x3e, err}, {0x3f, err}, {0xc0, err}, {0xc1, err}, {0x02, cs1},
                {0x03, cs1}, {0x04, cs1}, {0x05, cs1}, {0x06, cs1}, {0x07, cs1},
                {0x08, cs1}, {0x09, cs1}, {0x0a, cs1}, {0x0b, cs1}, {0x0c, cs1},
                {0x0d, cs1}, {0x0e, cs1}, {0x0f, cs1}, {0x10, cs1}, {0x11, cs1},
                {0x12, cs1}, {0x13, cs1}, {0x14, cs1}, {0x15, cs1}, {0x16, cs1},
                {0x17, cs1}, {0x18, cs1}, {0x19, cs1}, {0x1a, cs1}, {0x1b, cs1},
                {0x1c, cs1}, {0x1d, cs1}, {0x1e, cs1}, {0x1f, cs1}, {0x00, p3a},
                {0x01, cs2}, {0x02, cs2}, {0x03, cs2}, {0x04, cs2}, {0x05, cs2},
                {0x06, cs2}, {0x07, cs2}, {0x08, cs2}, {0x09, cs2}, {0x0a, cs2},
                {0x0b, cs2}, {0x0c, cs2}, {0x0d, p3b}, {0x0e, cs2}, {0x0f, cs2},
                {0x00, p4a}, {0x01, cs3}, {0x02, cs3}, {0x03, cs3}, {0x04, p4b},
                {0xf5, err}, {0xf6, err}, {0xf7, err}, {0xf8, err}, {0xf9, err},
                {0xfa, err}, {0xfb, err}, {0xfc, err}, {0xfd, err}, {0xfe, err},
                {0xff, err},
            };

            constexpr static char_class octet_classes[256] = {
                asc, asc, asc, asc, asc, asc, asc, asc, asc, asc, asc, asc, asc,
                asc, asc, asc, asc, asc, asc, asc, asc, asc, asc, asc, asc, asc,
                asc, asc, asc, asc, asc, asc, asc, asc, asc, asc, asc, asc, asc,
                asc, asc, asc, asc, asc, asc, asc, asc, asc, asc, asc, asc, asc,
                asc, asc, asc, asc, asc, asc, asc, asc, asc, asc, asc, asc, asc,
                asc, asc, asc, asc, asc, asc, asc, asc, asc, asc, asc, asc, asc,
                asc, asc, asc, asc, asc, asc, asc, asc, asc, asc, asc, asc, asc,
                asc, asc, asc, asc, asc, asc, asc, asc, asc, asc, asc, asc, asc,
                asc, asc, asc, asc, asc, asc, asc, asc, asc, asc, asc, asc, asc,
                asc, asc, asc, asc, asc, asc, asc, asc, asc, asc, asc, cr1, cr1,
                cr1, cr1, cr1, cr1, cr1, cr1, cr1, cr1, cr1, cr1, cr1, cr1, cr1,
                cr1, cr2, cr2, cr2, cr2, cr2, cr2, cr2, cr2, cr2, cr2, cr2, cr2,
                cr2, cr2, cr2, cr2, cr3, cr3, cr3, cr3, cr3, cr3, cr3, cr3, cr3,
                cr3, cr3, cr3, cr3, cr3, cr3, cr3, cr3, cr3, cr3, cr3, cr3, cr3,
                cr3, cr3, cr3, cr3, cr3, cr3, cr3, cr3, cr3, cr3, ill, ill, l2a,
                l2a, l2a, l2a, l2a, l2a, l2a, l2a, l2a, l2a, l2a, l2a, l2a, l2a,
                l2a, l2a, l2a, l2a, l2a, l2a, l2a, l2a, l2a, l2a, l2a, l2a, l2a,
                l2a, l2a, l2a, l3a, l3b, l3b, l3b, l3b, l3b, l3b, l3b, l3b, l3b,
                l3b, l3b, l3b, l3c, l3b, l3b, l4a, l4b, l4b, l4b, l4c, ill, ill,
                ill, ill, ill, ill, ill, ill, ill, ill, ill,
            };

            constexpr static table_state transitions[108] = {
                err, end, err, err, err, cs1, p3a, cs2, p3b, p4a, cs3, p4b,
                err, err, err, err, err, err, err, err, err, err, err, err,
                err, err, end, end, end, err, err, err, err, err, err, err,
                err, err, cs1, cs1, cs1, err, err, err, err, err, err, err,
                err, err, cs2, cs2, cs2, err, err, err, err, err, err, err,
                err, err, err, err, cs1, err, err, err, err, err, err, err,
                err, err, cs1, cs1, err, err, err, err, err, err, err, err,
                err, err, err, cs2, cs2, err, err, err, err, err, err, err,
                err, err, cs2, err, err, err, err, err, err, err, err, err,
            };

            first_cu const info = first_cus[(unsigned char)*first];
            ++first;

            retval = info.initial_octet;
            int state = info.next;

            while (state != bgn) {
                if (first != last) {
                    unsigned char const cu = *first;
                    retval = (retval << 6) | (cu & 0x3f);
                    char_class const class_ = octet_classes[cu];
                    state = transitions[state + class_];
                    if (state == err)
                        return replacement_character();
                    ++first;
                } else {
                    return replacement_character();
                }
            }

            return retval;
        }
    }

    /** Returns the first code unit in [first, last) that is not properly
        UTF-8 encoded, or last if no such code unit is found.

        This function is constexpr in C++14 and later. */
    inline BOOST_TEXT_CXX14_CONSTEXPR char const *
    find_invalid_encoding(char const * first, char const * last) noexcept
    {
        while (first != last) {
            int const cp_bytes = boost::text::code_point_bytes(*first);
            if (cp_bytes == -1 || last - first < cp_bytes)
                return first;

            if (detail::end_of_invalid_utf8(first))
                return first;

            first += cp_bytes;
        }

        return last;
    }

    /** Returns the first code unit in [first, last) that is not properly
        UTF-16 encoded, or last if no such code unit is found.

        This function is constexpr in C++14 and later. */
    template<typename T>
    BOOST_TEXT_CXX14_CONSTEXPR detail::enable_utf16_cp_t<T, T const *>
    find_invalid_encoding(T const * first, T const * last) noexcept
    {
        while (first != last) {
            int const cp_units = boost::text::code_point_units(*first);
            if (cp_units == -1 || last - first < cp_units)
                return first;

            if (cp_units == 2 && !boost::text::low_surrogate(*(first + 1)))
                return first;

            first += cp_units;
        }

        return last;
    }

    /** Returns true if [first, last) is properly UTF-8 encoded, or false
        otherwise.

        This function is constexpr in C++14 and later. */
    inline BOOST_TEXT_CXX14_CONSTEXPR bool
    encoded(char const * first, char const * last) noexcept
    {
        return boost::text::find_invalid_encoding(first, last) == last;
    }

    /** Returns true if [first, last) is properly UTF-16 encoded, or false
        otherwise.

        This function is constexpr in C++14 and later. */
    template<typename T>
    BOOST_TEXT_CXX14_CONSTEXPR detail::enable_utf16_cp_t<T, bool>
    encoded(T const * first, T const * last) noexcept
    {
        return boost::text::find_invalid_encoding(first, last) == last;
    }

    /** Returns true if [first, last) is empty or the initial code units in
        [first, last) form a valid Unicode code point, or false otherwise.

        This function is constexpr in C++14 and later. */
    inline BOOST_TEXT_CXX14_CONSTEXPR bool
    starts_encoded(char const * first, char const * last) noexcept
    {
        if (first == last)
            return true;

        int const cp_bytes = boost::text::code_point_bytes(*first);
        if (cp_bytes == -1 || last - first < cp_bytes)
            return false;

        return !detail::end_of_invalid_utf8(first);
    }

    /** Returns true if [first, last) is empty or the initial code units in
        [first, last) form a valid Unicode code point, or false otherwise.

        This function is constexpr in C++14 and later. */
    template<typename T>
    BOOST_TEXT_CXX14_CONSTEXPR detail::enable_utf16_cp_t<T, bool>
    starts_encoded(T const * first, T const * last) noexcept
    {
        if (first == last)
            return true;

        int const cp_units = boost::text::code_point_units(*first);
        if (cp_units == -1 || last - first < cp_units)
            return false;

        return cp_units == 1 || boost::text::low_surrogate(*(first + 1));
    }

    /** Returns true if [first, last) is empty or the final code units in
        [first, last) form a valid Unicode code point, or false otherwise.

        This function is constexpr in C++14 and later. */
    inline BOOST_TEXT_CXX14_CONSTEXPR bool
    ends_encoded(char const * first, char const * last) noexcept
    {
        if (first == last)
            return true;

        auto it = last;
        while (first != --it && boost::text::continuation(*it))
            ;

        return starts_encoded(it, last);
    }

    /** Returns true if [first, last) is empty or the final code units in
        [first, last) form a valid Unicode code point, or false otherwise.

        This function is constexpr in C++14 and later. */
    template<typename T>
    BOOST_TEXT_CXX14_CONSTEXPR detail::enable_utf16_cp_t<T, bool>
    ends_encoded(T const * first, T const * last) noexcept
    {
        if (first == last)
            return true;

        auto it = last;
        if (boost::text::low_surrogate(*--it))
            --it;

        return starts_encoded(it, last);
    }

    /** Returns true if [first, last) is empty or the initial code units in
        [first, last) form a valid Unicode code point, or false otherwise. */
    template<typename Iter>
    bool starts_encoded(Iter first, Iter last) noexcept
    {
        if (first == last)
            return true;

        int const cp_bytes = boost::text::code_point_bytes(*first);
        if (cp_bytes == -1 || last - first < cp_bytes)
            return false;

        char buf[5] = {0};
        for (int i = 0; i < (int)sizeof(buf); ++i) {
            buf[i] = *first;
            if (++first == last)
                break;
        }

        return !detail::end_of_invalid_utf8(buf);
    }

    /** Returns true if [first, last) is empty or the initial code units in
        [first, last) form a valid Unicode code point, or false otherwise. */
    template<typename Iter>
    detail::enable_utf16_cp_t<typename Iter::value, bool>
    starts_encoded(Iter first, Iter last) noexcept
    {
        if (first == last)
            return true;

        int const cp_units = boost::text::code_point_units(*first);
        if (cp_units == -1 || last - first < cp_units)
            return false;

        return cp_units == 1 || boost::text::low_surrogate(*(first + 1));
    }

    /** Returns true if [first, last) is empty or the final code units in
        [first, last) form a valid Unicode code point, or false otherwise. */
    template<typename Iter>
    bool ends_encoded(Iter first, Iter last) noexcept
    {
        if (first == last)
            return true;

        auto it = last;
        while (first != --it && boost::text::continuation(*it))
            ;

        return boost::text::starts_encoded(it, last);
    }

    /** Returns true if [first, last) is empty or the final code units in
        [first, last) form a valid Unicode code point, or false otherwise. */
    template<typename Iter>
    detail::enable_utf16_cp_t<typename Iter::value, bool>
    ends_encoded(Iter first, Iter last) noexcept
    {
        if (first == last)
            return true;

        auto it = last;
        if (boost::text::low_surrogate(*--it))
            --it;

        return boost::text::starts_encoded(it, last);
    }

    /** An error handler type that can be used with the converting iterators;
        provides the Unicode replacement character on errors. */
    struct use_replacement_character
    {
        constexpr uint32_t operator()(char const *) const noexcept
        {
            return replacement_character();
        }
    };

    /** An error handler type that can be used with the converting iterators;
        throws std::logic_error on errors. */
    struct throw_logic_error
    {
        uint32_t operator()(char const * msg)
        {
            boost::throw_exception(std::logic_error(msg));
            return 0;
        }
    };


    /** A sentinel type that compares equal to a `char const`,
       `utf_8_to_32_iterator`, or `utf_8_to_16_iterator` when the iterator
       points to a `0`. */
    struct null_sentinel
    {};


    inline BOOST_TEXT_CXX14_CONSTEXPR bool
    operator==(char const * p, null_sentinel)
    {
        return !*p;
    }
    inline BOOST_TEXT_CXX14_CONSTEXPR bool
    operator!=(char const * p, null_sentinel)
    {
        return !!*p;
    }

    inline BOOST_TEXT_CXX14_CONSTEXPR bool
    operator==(null_sentinel, char const * p)
    {
        return !*p;
    }
    inline BOOST_TEXT_CXX14_CONSTEXPR bool
    operator!=(null_sentinel, char const * p)
    {
        return !!*p;
    }


    /** A UTF-8 to UTF-16 converting iterator.  Set the ErrorHandler template
        parameter to control error handling.  The default ErrorHandler is
        use_replacement_character, which simply produces a replacement
        character.

        Iter must be a bidirectional iterator with a 1-byte char
        value_type. */
    template<
        typename Iter,
        typename Sentinel = Iter,
        typename ErrorHandler = use_replacement_character>
    struct utf_8_to_16_iterator;


    /** A UTF-32 to UTF-8 converting iterator.  Set the ErrorHandler template
        parameter to control error handling.  The default ErrorHandler is
        use_replacement_character, which simply produces a replacement
        character.

        Iter must be a bidirectional iterator with a 4-byte integral
        value_type. */
    template<
        typename Iter,
        typename Sentinel = Iter,
        typename ErrorHandler = use_replacement_character>
    struct utf_32_to_8_iterator
    {
        using value_type = char;
        using difference_type = int;
        using pointer = char *;
        using reference = char;
        using iterator_category = std::bidirectional_iterator_tag;

        static bool const throw_on_error =
            !noexcept(std::declval<ErrorHandler>()(0));

        static_assert(
            std::is_same<
                typename std::iterator_traits<Iter>::iterator_category,
                std::bidirectional_iterator_tag>::value ||
                std::is_same<
                    typename std::iterator_traits<Iter>::iterator_category,
                    std::random_access_iterator_tag>::value,
            "utf_32_to_8_iterator requires its Iter parameter to be at least "
            "bidirectional.");
        static_assert(
            sizeof(typename std::iterator_traits<Iter>::value_type) == 4,
            "utf_32_to_8_iterator requires its Iter parameter to produce a "
            "4-byte value_type.");

        constexpr utf_32_to_8_iterator() noexcept :
            first_(),
            it_(),
            last_(),
            index_(4),
            buf_()
        {}
        explicit BOOST_TEXT_CXX14_CONSTEXPR
        utf_32_to_8_iterator(Iter first, Iter it, Sentinel last) noexcept :
            first_(first),
            it_(it),
            last_(last),
            index_(0),
            buf_()
        {
            if (it_ != last_)
                read_into_buf();
        }
        template<typename Iter2, typename Sentinel2>
        constexpr utf_32_to_8_iterator(
            utf_32_to_8_iterator<Iter2, Sentinel2, ErrorHandler> const &
                other) noexcept :
            first_(other.first_),
            it_(other.it_),
            last_(other.last_),
            index_(other.index_),
            buf_(other.buf_)
        {}

        /** This function is constexpr in C++14 and later. */
        BOOST_TEXT_CXX14_CONSTEXPR reference operator*() const
            noexcept(!throw_on_error)
        {
            return buf_[index_];
        }

        /** This function is constexpr in C++14 and later. */
        BOOST_TEXT_CXX14_CONSTEXPR Iter base() const noexcept { return it_; }

        /** This function is constexpr in C++14 and later. */
        BOOST_TEXT_CXX14_CONSTEXPR utf_32_to_8_iterator &
        operator++() noexcept(!throw_on_error)
        {
            ++index_;
            if (at_buf_end()) {
                ++it_;
                index_ = 0;
                if (it_ != last_)
                    read_into_buf();
            }
            return *this;
        }

        /** This function is constexpr in C++14 and later. */
        BOOST_TEXT_CXX14_CONSTEXPR utf_32_to_8_iterator
        operator++(int)noexcept(!throw_on_error)
        {
            utf_32_to_8_iterator retval = *this;
            ++*this;
            return retval;
        }

        /** This function is constexpr in C++14 and later. */
        BOOST_TEXT_CXX14_CONSTEXPR utf_32_to_8_iterator &
        operator--() noexcept(!throw_on_error)
        {
            if (0 < index_) {
                --index_;
            } else {
                --it_;
                index_ = read_into_buf();
            }
            return *this;
        }

        /** This function is constexpr in C++14 and later. */
        BOOST_TEXT_CXX14_CONSTEXPR utf_32_to_8_iterator
        operator--(int)noexcept(!throw_on_error)
        {
            utf_32_to_8_iterator retval = *this;
            --*this;
            return retval;
        }

        template<
            typename Iter1,
            typename ErrorHandler1,
            typename Iter2,
            typename ErrorHandler2>
        friend BOOST_TEXT_CXX14_CONSTEXPR auto operator==(
            utf_32_to_8_iterator<Iter1, ErrorHandler1> lhs,
            utf_32_to_8_iterator<Iter2, ErrorHandler2> rhs) noexcept
            -> decltype(lhs.it_ == rhs.it_ && lhs.index_ == rhs.index_);

#ifndef BOOST_TEXT_DOXYGEN
    private:
        constexpr bool buf_empty() const noexcept { return index_ == 4; }

        constexpr bool at_buf_end() const noexcept
        {
            return buf_[index_] == '\0';
        }

        BOOST_TEXT_CXX14_CONSTEXPR int read_into_buf() noexcept(!throw_on_error)
        {
            uint32_t cp = static_cast<uint32_t>(*it_);
            index_ = 0;
            return detail::read_into_buf(cp, buf_.data());
        }

        Iter first_;
        Iter it_;
        Sentinel last_;
        int index_;
        std::array<char, 5> buf_;

        template<typename Iter2, typename Sentinel2, typename ErrorHandler2>
        friend struct utf_32_to_8_iterator;
#endif
    };

    /** Returns a utf_32_to_8_iterator<Iter> constructed from an Iter. */
    template<typename Iter, typename Sentinel>
    utf_32_to_8_iterator<Iter, Sentinel>
    make_utf_32_to_8_iterator(Iter first, Iter it, Sentinel last) noexcept
    {
        return utf_32_to_8_iterator<Iter, Sentinel>(first, it, last);
    }

    /** This function is constexpr in C++14 and later. */
    template<
        typename Iter1,
        typename ErrorHandler1,
        typename Iter2,
        typename ErrorHandler2>
    BOOST_TEXT_CXX14_CONSTEXPR auto operator==(
        utf_32_to_8_iterator<Iter1, ErrorHandler1> lhs,
        utf_32_to_8_iterator<Iter2, ErrorHandler2> rhs) noexcept
        -> decltype(lhs.it_ == rhs.it_ && lhs.index_ == rhs.index_)
    {
        return lhs.it_ == rhs.it_ && lhs.index_ == rhs.index_;
    }

    /** This function is constexpr in C++14 and later. */
    template<
        typename Iter1,
        typename ErrorHandler1,
        typename Iter2,
        typename ErrorHandler2>
    BOOST_TEXT_CXX14_CONSTEXPR auto operator!=(
        utf_32_to_8_iterator<Iter1, ErrorHandler1> lhs,
        utf_32_to_8_iterator<Iter2, ErrorHandler2> rhs) noexcept
        -> decltype(!(lhs == rhs))
    {
        return !(lhs == rhs);
    }


    /** A UTF-8 to UTF-32 converting iterator.  Set the ErrorHandler template
        parameter to control error handling.  The default ErrorHandler is
        use_replacement_character, which simply produces a replacement
        character.

        Iter must be a bidirectional iterator with a 1-byte char
        value_type. */
    template<
        typename Iter,
        typename Sentinel = Iter,
        typename ErrorHandler = use_replacement_character>
    struct utf_8_to_32_iterator
    {
        using value_type = uint32_t;
        using difference_type = int;
        using pointer = uint32_t *;
        using reference = uint32_t;
        using iterator_category = std::bidirectional_iterator_tag;

        static bool const throw_on_error =
            !noexcept(std::declval<ErrorHandler>()(0));

        constexpr utf_8_to_32_iterator() noexcept : first_(), it_(), last_() {}
        explicit constexpr utf_8_to_32_iterator(
            Iter first, Iter it, Sentinel last) noexcept :
            first_(first),
            it_(it),
            last_(last)
        {}
        template<typename Iter2, typename Sentinel2>
        constexpr utf_8_to_32_iterator(
            utf_8_to_32_iterator<Iter2, Sentinel2, ErrorHandler> const &
                other) noexcept :
            first_(other.first_),
            it_(other.it_),
            last_(other.last_)
        {}

        /** This function is constexpr in C++14 and later. */
        BOOST_TEXT_CXX14_CONSTEXPR reference operator*() const
            noexcept(!throw_on_error)
        {
            if (at_end(it_))
                return replacement_character();
            unsigned char curr_c = *it_;
            if (curr_c < 0x80)
                return curr_c;
            return get_value().value_;
        }

        /** This function is constexpr in C++14 and later. */
        BOOST_TEXT_CXX14_CONSTEXPR Iter base() const noexcept { return it_; }

        /** This function is constexpr in C++14 and later. */
        BOOST_TEXT_CXX14_CONSTEXPR utf_8_to_32_iterator &
        operator++() noexcept(!throw_on_error)
        {
            it_ = increment();
            return *this;
        }

        /** This function is constexpr in C++14 and later. */
        BOOST_TEXT_CXX14_CONSTEXPR utf_8_to_32_iterator
        operator++(int)noexcept(!throw_on_error)
        {
            utf_8_to_32_iterator retval = *this;
            ++*this;
            return retval;
        }

        /** This function is constexpr in C++14 and later. */
        BOOST_TEXT_CXX14_CONSTEXPR utf_8_to_32_iterator &
        operator--() noexcept(!throw_on_error)
        {
            it_ = detail::decrement(first_, it_);
            return *this;
        }

        /** This function is constexpr in C++14 and later. */
        BOOST_TEXT_CXX14_CONSTEXPR utf_8_to_32_iterator
        operator--(int)noexcept(!throw_on_error)
        {
            utf_8_to_32_iterator retval = *this;
            --*this;
            return retval;
        }

        /** This function is constexpr in C++14 and later. */
        friend BOOST_TEXT_CXX14_CONSTEXPR bool
        operator==(utf_8_to_32_iterator lhs, null_sentinel rhs) noexcept
        {
            return !*lhs.it_;
        }

        /** This function is constexpr in C++14 and later. */
        friend BOOST_TEXT_CXX14_CONSTEXPR bool
        operator==(null_sentinel lhs, utf_8_to_32_iterator rhs) noexcept
        {
            return rhs == lhs;
        }

        /** This function is constexpr in C++14 and later. */
        friend BOOST_TEXT_CXX14_CONSTEXPR bool
        operator!=(utf_8_to_32_iterator lhs, null_sentinel rhs) noexcept
        {
            return !(lhs == rhs);
        }

        /** This function is constexpr in C++14 and later. */
        friend BOOST_TEXT_CXX14_CONSTEXPR bool
        operator!=(null_sentinel lhs, utf_8_to_32_iterator rhs) noexcept
        {
            return !(rhs == lhs);
        }

#ifndef BOOST_TEXT_DOXYGEN
    private:
        struct get_value_result
        {
            reference value_;
            Iter it_;
        };

        BOOST_TEXT_CXX14_CONSTEXPR bool check_continuation(
            unsigned char c,
            unsigned char lo = 0x80,
            unsigned char hi = 0xbf) const noexcept(!throw_on_error)
        {
            if (continuation(c, lo, hi)) {
                return true;
            } else {
                ErrorHandler{}(
                    "Invalid UTF-8 sequence; an expected continuation "
                    "code unit is missing.");
                return false;
            }
        }

        BOOST_TEXT_CXX14_CONSTEXPR bool at_end(Iter it) const
            noexcept(!throw_on_error)
        {
            if (it == last_) {
                ErrorHandler{}(
                    "Invalid UTF-8 sequence; expected another code unit "
                    "before the end of string.");
                return true;
            } else {
                return false;
            }
        }

        BOOST_TEXT_CXX14_CONSTEXPR get_value_result get_value() const
            noexcept(!throw_on_error)
        {
        // It turns out that this naive implementation is faster than the
        // table implementation for the converting iterators.
#if 1
            /*
                Unicode 3.9/D92
                Table 3-7. Well-Formed UTF-8 Byte Sequences

                Code Points        First Byte Second Byte Third Byte Fourth Byte
                ===========        ========== =========== ========== ===========
                U+0000..U+007F     00..7F
                U+0080..U+07FF     C2..DF     80..BF
                U+0800..U+0FFF     E0         A0..BF      80..BF
                U+1000..U+CFFF     E1..EC     80..BF      80..BF
                U+D000..U+D7FF     ED         80..9F      80..BF
                U+E000..U+FFFF     EE..EF     80..BF      80..BF
                U+10000..U+3FFFF   F0         90..BF      80..BF     80..BF
                U+40000..U+FFFFF   F1..F3     80..BF      80..BF     80..BF
                U+100000..U+10FFFF F4         80..8F      80..BF     80..BF
            */

            uint32_t value = 0;
            Iter next = it_;
            unsigned char curr_c = *next;

            using detail::in;

            // One-byte case handled by caller

            // Two-byte
            if (in(0xc2, curr_c, 0xdf)) {
                value = curr_c & 0b00011111;
                ++next;
                if (at_end(next))
                    return get_value_result{replacement_character(), next};
                curr_c = *next;
                if (!check_continuation(curr_c))
                    return get_value_result{replacement_character(), next};
                value = (value << 6) + (curr_c & 0b00111111);
                ++next;
                // Three-byte
            } else if (curr_c == 0xe0) {
                value = curr_c & 0b00001111;
                ++next;
                if (at_end(next))
                    return get_value_result{replacement_character(), next};
                curr_c = *next;
                if (!check_continuation(curr_c, 0xa0, 0xbf))
                    return get_value_result{replacement_character(), next};
                value = (value << 6) + (curr_c & 0b00111111);
                ++next;
                if (at_end(next))
                    return get_value_result{replacement_character(), next};
                curr_c = *next;
                if (!check_continuation(curr_c))
                    return get_value_result{replacement_character(), next};
                value = (value << 6) + (curr_c & 0b00111111);
                ++next;
            } else if (in(0xe1, curr_c, 0xec)) {
                value = curr_c & 0b00001111;
                ++next;
                if (at_end(next))
                    return get_value_result{replacement_character(), next};
                curr_c = *next;
                if (!check_continuation(curr_c))
                    return get_value_result{replacement_character(), next};
                value = (value << 6) + (curr_c & 0b00111111);
                ++next;
                if (at_end(next))
                    return get_value_result{replacement_character(), next};
                curr_c = *next;
                if (!check_continuation(curr_c))
                    return get_value_result{replacement_character(), next};
                value = (value << 6) + (curr_c & 0b00111111);
                ++next;
            } else if (curr_c == 0xed) {
                value = curr_c & 0b00001111;
                ++next;
                if (at_end(next))
                    return get_value_result{replacement_character(), next};
                curr_c = *next;
                if (!check_continuation(curr_c, 0x80, 0x9f))
                    return get_value_result{replacement_character(), next};
                value = (value << 6) + (curr_c & 0b00111111);
                ++next;
                if (at_end(next))
                    return get_value_result{replacement_character(), next};
                curr_c = *next;
                if (!check_continuation(curr_c))
                    return get_value_result{replacement_character(), next};
                value = (value << 6) + (curr_c & 0b00111111);
                ++next;
            } else if (in(0xed, curr_c, 0xef)) {
                value = curr_c & 0b00001111;
                ++next;
                if (at_end(next))
                    return get_value_result{replacement_character(), next};
                curr_c = *next;
                if (!check_continuation(curr_c))
                    return get_value_result{replacement_character(), next};
                value = (value << 6) + (curr_c & 0b00111111);
                ++next;
                if (at_end(next))
                    return get_value_result{replacement_character(), next};
                curr_c = *next;
                if (!check_continuation(curr_c))
                    return get_value_result{replacement_character(), next};
                value = (value << 6) + (curr_c & 0b00111111);
                ++next;
                // Four-byte
            } else if (curr_c == 0xf0) {
                value = curr_c & 0b00000111;
                ++next;
                if (at_end(next))
                    return get_value_result{replacement_character(), next};
                curr_c = *next;
                if (!check_continuation(curr_c, 0x90, 0xbf))
                    return get_value_result{replacement_character(), next};
                value = (value << 6) + (curr_c & 0b00111111);
                ++next;
                if (at_end(next))
                    return get_value_result{replacement_character(), next};
                curr_c = *next;
                if (!check_continuation(curr_c))
                    return get_value_result{replacement_character(), next};
                value = (value << 6) + (curr_c & 0b00111111);
                ++next;
                if (at_end(next))
                    return get_value_result{replacement_character(), next};
                curr_c = *next;
                if (!check_continuation(curr_c))
                    return get_value_result{replacement_character(), next};
                value = (value << 6) + (curr_c & 0b00111111);
                ++next;
            } else if (in(0xf1, curr_c, 0xf3)) {
                value = curr_c & 0b00000111;
                ++next;
                if (at_end(next))
                    return get_value_result{replacement_character(), next};
                curr_c = *next;
                if (!check_continuation(curr_c))
                    return get_value_result{replacement_character(), next};
                value = (value << 6) + (curr_c & 0b00111111);
                ++next;
                if (at_end(next))
                    return get_value_result{replacement_character(), next};
                curr_c = *next;
                if (!check_continuation(curr_c))
                    return get_value_result{replacement_character(), next};
                value = (value << 6) + (curr_c & 0b00111111);
                ++next;
                if (at_end(next))
                    return get_value_result{replacement_character(), next};
                curr_c = *next;
                if (!check_continuation(curr_c))
                    return get_value_result{replacement_character(), next};
                value = (value << 6) + (curr_c & 0b00111111);
                ++next;
            } else if (curr_c == 0xf4) {
                value = curr_c & 0b00000111;
                ++next;
                if (at_end(next))
                    return get_value_result{replacement_character(), next};
                curr_c = *next;
                if (!check_continuation(curr_c, 0x80, 0x8f))
                    return get_value_result{replacement_character(), next};
                value = (value << 6) + (curr_c & 0b00111111);
                ++next;
                if (at_end(next))
                    return get_value_result{replacement_character(), next};
                curr_c = *next;
                if (!check_continuation(curr_c))
                    return get_value_result{replacement_character(), next};
                value = (value << 6) + (curr_c & 0b00111111);
                ++next;
                if (at_end(next))
                    return get_value_result{replacement_character(), next};
                curr_c = *next;
                if (!check_continuation(curr_c))
                    return get_value_result{replacement_character(), next};
                value = (value << 6) + (curr_c & 0b00111111);
                ++next;
            } else {
                value = ErrorHandler{}("Invalid initial UTF-8 code unit.");
                ++next;
            }
            return get_value_result{value, next};
#else
            Iter next = it_;
            uint32_t const value = detail::advance(next, last_);
            return get_value_result{value, next};
#endif
        }

        BOOST_TEXT_CXX14_CONSTEXPR Iter increment() const
            noexcept(!throw_on_error)
        {
            if (at_end(it_))
                return it_;
            unsigned char curr_c = *it_;
            if (curr_c < 0x80)
                return std::next(it_);
            return get_value().it_;
        }

        Iter first_;
        Iter it_;
        Sentinel last_;

        template<typename Iter2, typename Sentinel2, typename ErrorHandler2>
        friend struct utf_8_to_16_iterator;

        template<typename Iter2, typename Sentinel2, typename ErrorHandler2>
        friend struct utf_8_to_32_iterator;

#endif
    };

    /** Returns a utf_8_to_32_iterator<Iter> constructed from an Iter. */
    template<typename Iter, typename Sentinel>
    utf_8_to_32_iterator<Iter, Sentinel>
    make_utf_8_to_32_iterator(Iter first, Iter it, Sentinel last) noexcept
    {
        return utf_8_to_32_iterator<Iter, Sentinel>(first, it, last);
    }

    /** This function is constexpr in C++14 and later. */
    template<
        typename Iter1,
        typename ErrorHandler1,
        typename Iter2,
        typename ErrorHandler2>
    BOOST_TEXT_CXX14_CONSTEXPR auto operator==(
        utf_8_to_32_iterator<Iter1, ErrorHandler1> lhs,
        utf_8_to_32_iterator<Iter2, ErrorHandler2> rhs) noexcept
        -> decltype(lhs.base() == rhs.base())
    {
        return lhs.base() == rhs.base();
    }

    /** This function is constexpr in C++14 and later. */
    template<
        typename Iter1,
        typename ErrorHandler1,
        typename Iter2,
        typename ErrorHandler2>
    BOOST_TEXT_CXX14_CONSTEXPR auto operator!=(
        utf_8_to_32_iterator<Iter1, ErrorHandler1> lhs,
        utf_8_to_32_iterator<Iter2, ErrorHandler2> rhs) noexcept
        -> decltype(!(lhs == rhs))
    {
        return !(lhs == rhs);
    }


    /** A UTF-32 to UTF-16 converting iterator.  Set the ErrorHandler template
        parameter to control error handling.  The default ErrorHandler is
        use_replacement_character, which simply produces a replacement
        character.

        Iter must be a bidirectional iterator with a 4-byte integral
        value_type. */
    template<
        typename Iter,
        typename Sentinel = Iter,
        typename ErrorHandler = use_replacement_character>
    struct utf_32_to_16_iterator
    {
        using value_type = uint16_t;
        using difference_type = int;
        using pointer = uint16_t *;
        using reference = uint16_t;
        using iterator_category = std::bidirectional_iterator_tag;

        static bool const throw_on_error =
            !noexcept(std::declval<ErrorHandler>()(0));

        static_assert(
            std::is_same<
                typename std::iterator_traits<Iter>::iterator_category,
                std::bidirectional_iterator_tag>::value ||
                std::is_same<
                    typename std::iterator_traits<Iter>::iterator_category,
                    std::random_access_iterator_tag>::value,
            "utf_32_to_16_iterator requires its Iter parameter to be at "
            "least "
            "bidirectional.");
        static_assert(
            sizeof(typename std::iterator_traits<Iter>::value_type) == 4,
            "utf_32_to_16_iterator requires its Iter parameter to produce a "
            "4-byte value_type.");

        constexpr utf_32_to_16_iterator() noexcept :
            first_(),
            it_(),
            last_(),
            index_(2),
            buf_()
        {}
        explicit BOOST_TEXT_CXX14_CONSTEXPR
        utf_32_to_16_iterator(Iter first, Iter it, Sentinel last) noexcept :
            first_(first),
            it_(it),
            last_(last),
            index_(0),
            buf_()
        {
            if (it_ != last_)
                read_into_buf();
        }
        template<typename Iter2, typename Sentinel2>
        constexpr utf_32_to_16_iterator(
            utf_32_to_16_iterator<Iter2, Sentinel2, ErrorHandler> const &
                other) noexcept :
            first_(other.first_),
            it_(other.it_),
            last_(other.last_),
            index_(other.index_),
            buf_(other.buf_)
        {}

        /** This function is constexpr in C++14 and later. */
        BOOST_TEXT_CXX14_CONSTEXPR reference operator*() const
            noexcept(!throw_on_error)
        {
            return buf_[index_];
        }

        /** This function is constexpr in C++14 and later. */
        BOOST_TEXT_CXX14_CONSTEXPR Iter base() const noexcept { return it_; }

        /** This function is constexpr in C++14 and later. */
        BOOST_TEXT_CXX14_CONSTEXPR utf_32_to_16_iterator &
        operator++() noexcept(!throw_on_error)
        {
            ++index_;
            if (at_buf_end()) {
                ++it_;
                index_ = 0;
                if (it_ != last_)
                    read_into_buf();
            }
            return *this;
        }

        /** This function is constexpr in C++14 and later. */
        BOOST_TEXT_CXX14_CONSTEXPR utf_32_to_16_iterator
        operator++(int)noexcept(!throw_on_error)
        {
            utf_32_to_16_iterator retval = *this;
            ++*this;
            return retval;
        }

        /** This function is constexpr in C++14 and later. */
        BOOST_TEXT_CXX14_CONSTEXPR utf_32_to_16_iterator &
        operator--() noexcept(!throw_on_error)
        {
            if (0 < index_) {
                --index_;
            } else {
                --it_;
                index_ = read_into_buf();
            }
            return *this;
        }

        /** This function is constexpr in C++14 and later. */
        BOOST_TEXT_CXX14_CONSTEXPR utf_32_to_16_iterator
        operator--(int)noexcept(!throw_on_error)
        {
            utf_32_to_16_iterator retval = *this;
            --*this;
            return retval;
        }

        template<
            typename Iter1,
            typename ErrorHandler1,
            typename Iter2,
            typename ErrorHandler2>
        friend BOOST_TEXT_CXX14_CONSTEXPR auto operator==(
            utf_32_to_16_iterator<Iter1, ErrorHandler1> lhs,
            utf_32_to_16_iterator<Iter2, ErrorHandler2> rhs) noexcept
            -> decltype(lhs.it_ == rhs.it_ && lhs.index_ == rhs.index_);

#ifndef BOOST_TEXT_DOXYGEN
    private:
        constexpr bool at_buf_end() const noexcept { return buf_[index_] == 0; }

        BOOST_TEXT_CXX14_CONSTEXPR int read_into_buf() noexcept(!throw_on_error)
        {
            uint32_t const value = *it_;

            if (value < 0x10000) {
                buf_[0] = static_cast<uint16_t>(value);
                buf_[1] = 0;
                return 0;
            } else {
                buf_[0] =
                    static_cast<uint16_t>(value >> 10) + high_surrogate_base;
                buf_[1] =
                    static_cast<uint16_t>(value & 0x3ff) + low_surrogate_base;
                buf_[2] = 0;
                return 1;
            }
        }

        Iter first_;
        Iter it_;
        Sentinel last_;
        int index_;
        std::array<uint16_t, 4> buf_;

        static uint16_t const high_surrogate_base = 0xd7c0;
        static uint16_t const low_surrogate_base = 0xdc00;

        template<typename Iter2, typename Sentinel2, typename ErrorHandler2>
        friend struct utf_32_to_16_iterator;
#endif
    };

    /** Returns a utf_32_to_16_iterator<Iter> constructed from an Iter. */
    template<typename Iter, typename Sentinel>
    utf_32_to_16_iterator<Iter, Sentinel>
    make_utf_32_to_16_iterator(Iter first, Iter it, Sentinel last) noexcept
    {
        return utf_32_to_16_iterator<Iter, Sentinel>(first, it, last);
    }

    /** This function is constexpr in C++14 and later. */
    template<
        typename Iter1,
        typename ErrorHandler1,
        typename Iter2,
        typename ErrorHandler2>
    BOOST_TEXT_CXX14_CONSTEXPR auto operator==(
        utf_32_to_16_iterator<Iter1, ErrorHandler1> lhs,
        utf_32_to_16_iterator<Iter2, ErrorHandler2> rhs) noexcept
        -> decltype(lhs.it_ == rhs.it_ && lhs.index_ == rhs.index_)
    {
        return lhs.it_ == rhs.it_ && lhs.index_ == rhs.index_;
    }

    /** This function is constexpr in C++14 and later. */
    template<
        typename Iter1,
        typename ErrorHandler1,
        typename Iter2,
        typename ErrorHandler2>
    BOOST_TEXT_CXX14_CONSTEXPR auto operator!=(
        utf_32_to_16_iterator<Iter1, ErrorHandler1> lhs,
        utf_32_to_16_iterator<Iter2, ErrorHandler2> rhs) noexcept
        -> decltype(!(lhs == rhs))
    {
        return !(lhs == rhs);
    }


    /** A UTF-16 to UTF-32 converting iterator.  Set the ErrorHandler template
        parameter to control error handling.  The default ErrorHandler is
        use_replacement_character, which simply produces a replacement
        character.

        Iter must be a bidirectional iterator with a 2-byte char
        value_type. */
    template<
        typename Iter,
        typename Sentinel = Iter,
        typename ErrorHandler = use_replacement_character>
    struct utf_16_to_32_iterator
    {
        using value_type = uint32_t;
        using difference_type = int;
        using pointer = uint32_t *;
        using reference = uint32_t;
        using iterator_category = std::bidirectional_iterator_tag;

        static bool const throw_on_error =
            !noexcept(std::declval<ErrorHandler>()(0));

        static_assert(
            std::is_same<
                typename std::iterator_traits<Iter>::iterator_category,
                std::bidirectional_iterator_tag>::value ||
                std::is_same<
                    typename std::iterator_traits<Iter>::iterator_category,
                    std::random_access_iterator_tag>::value,
            "utf_16_to_32_iterator requires its Iter parameter to be at "
            "least "
            "bidirectional.");
        static_assert(
            sizeof(typename std::iterator_traits<Iter>::value_type) == 2,
            "utf_16_to_32_iterator requires its Iter parameter to produce a "
            "2-byte value_type.");

        constexpr utf_16_to_32_iterator() noexcept : first_(), it_(), last_() {}
        explicit constexpr utf_16_to_32_iterator(
            Iter first, Iter it, Sentinel last) noexcept :
            first_(first),
            it_(it),
            last_(last)
        {}
        template<typename Iter2, typename Sentinel2>
        constexpr utf_16_to_32_iterator(
            utf_16_to_32_iterator<Iter2, Sentinel2, ErrorHandler> const &
                other) noexcept :
            first_(other.first_),
            it_(other.it_),
            last_(other.last_)
        {}

        /** This function is constexpr in C++14 and later. */
        BOOST_TEXT_CXX14_CONSTEXPR reference operator*() const
            noexcept(!throw_on_error)
        {
            if (at_end(it_))
                return replacement_character();
            return get_value(*it_).value_;
        }

        /** This function is constexpr in C++14 and later. */
        BOOST_TEXT_CXX14_CONSTEXPR Iter base() const noexcept { return it_; }

        /** This function is constexpr in C++14 and later. */
        BOOST_TEXT_CXX14_CONSTEXPR utf_16_to_32_iterator &
        operator++() noexcept(!throw_on_error)
        {
            it_ = increment();
            return *this;
        }

        /** This function is constexpr in C++14 and later. */
        BOOST_TEXT_CXX14_CONSTEXPR utf_16_to_32_iterator
        operator++(int)noexcept(!throw_on_error)
        {
            utf_16_to_32_iterator retval = *this;
            ++*this;
            return retval;
        }

        /** This function is constexpr in C++14 and later. */
        BOOST_TEXT_CXX14_CONSTEXPR utf_16_to_32_iterator &
        operator--() noexcept(!throw_on_error)
        {
            if (boost::text::low_surrogate(*--it_)) {
                if (it_ != first_ &&
                    boost::text::high_surrogate(*std::prev(it_)))
                    --it_;
            }
            return *this;
        }

        /** This function is constexpr in C++14 and later. */
        BOOST_TEXT_CXX14_CONSTEXPR utf_16_to_32_iterator
        operator--(int)noexcept(!throw_on_error)
        {
            utf_16_to_32_iterator retval = *this;
            --*this;
            return retval;
        }

#ifndef BOOST_TEXT_DOXYGEN
    private:
        struct get_value_result
        {
            reference value_;
            Iter it_;
        };

        BOOST_TEXT_CXX14_CONSTEXPR bool at_end(Iter it) const
            noexcept(!throw_on_error)
        {
            if (it == last_) {
                ErrorHandler{}(
                    "Invalid UTF-16 sequence; expected another code unit "
                    "before the end of string.");
                return true;
            } else {
                return false;
            }
        }

        BOOST_TEXT_CXX14_CONSTEXPR get_value_result
        get_value(uint16_t curr) const noexcept(!throw_on_error)
        {
            uint32_t value = 0;
            Iter next = it_;

            if (high_surrogate(curr)) {
                value = (curr - high_surrogate_base) << 10;
                ++next;
                if (at_end(next)) {
                    return get_value_result{replacement_character(), next};
                }
                curr = *next;
                if (!low_surrogate(curr)) {
                    return get_value_result{replacement_character(), next};
                }
                value += curr - low_surrogate_base;
                ++next;
            } else if (low_surrogate(curr)) {
                value = ErrorHandler{}("Invalid initial UTF-16 code unit.");
                return get_value_result{replacement_character(), next};
            } else {
                value = curr;
                ++next;
            }

            if (!valid_code_point(value)) {
                value = ErrorHandler{}(
                    "UTF-16 sequence results in invalid UTF-32 code point.");
            }

            return get_value_result{value, next};
        }

        BOOST_TEXT_CXX14_CONSTEXPR Iter increment() const
            noexcept(!throw_on_error)
        {
            if (at_end(it_))
                return it_;
            return get_value(*it_).it_;
        }

        Iter first_;
        Iter it_;
        Sentinel last_;

        static uint16_t const high_surrogate_base = 0xd7c0;
        static uint16_t const low_surrogate_base = 0xdc00;

        template<typename Iter2, typename Sentinel2, typename ErrorHandler2>
        friend struct utf_32_to_16_iterator;

        template<typename Iter2, typename Sentinel2, typename ErrorHandler2>
        friend struct utf_16_to_32_iterator;

#endif
    };

    /** Returns a utf_16_to_32_iterator<Iter> constructed from an Iter. */
    template<typename Iter, typename Sentinel>
    utf_16_to_32_iterator<Iter, Sentinel>
    make_utf_16_to_32_iterator(Iter first, Iter it, Sentinel last) noexcept
    {
        return utf_16_to_32_iterator<Iter, Sentinel>(first, it, last);
    }

    /** This function is constexpr in C++14 and later. */
    template<
        typename Iter1,
        typename ErrorHandler1,
        typename Iter2,
        typename ErrorHandler2>
    BOOST_TEXT_CXX14_CONSTEXPR auto operator==(
        utf_16_to_32_iterator<Iter1, ErrorHandler1> lhs,
        utf_16_to_32_iterator<Iter2, ErrorHandler2> rhs) noexcept
        -> decltype(lhs.base() == rhs.base())
    {
        return lhs.base() == rhs.base();
    }

    /** This function is constexpr in C++14 and later. */
    template<
        typename Iter1,
        typename ErrorHandler1,
        typename Iter2,
        typename ErrorHandler2>
    BOOST_TEXT_CXX14_CONSTEXPR auto operator!=(
        utf_16_to_32_iterator<Iter1, ErrorHandler1> lhs,
        utf_16_to_32_iterator<Iter2, ErrorHandler2> rhs) noexcept
        -> decltype(!(lhs == rhs))
    {
        return !(lhs == rhs);
    }


    /** An insert-iterator analogous to std::insert_iterator, that also
        converts UTF-32 to UTF-16.*/
    template<typename Container>
    struct utf_32_to_16_insert_iterator
    {
        using value_type = void;
        using difference_type = void;
        using pointer = void;
        using reference = void;
        using iterator_category = std::output_iterator_tag;

        utf_32_to_16_insert_iterator(
            Container & c, typename Container::iterator it) noexcept :
            it_(c, it)
        {}

        utf_32_to_16_insert_iterator & operator=(uint32_t cp)
        {
            *it_ = static_cast<uint16_t>(cp >> 10) + high_surrogate_base;
            ++it_;
            *it_ = static_cast<uint16_t>(cp & 0x3ff) + low_surrogate_base;
            ++it_;
            return *this;
        }

        utf_32_to_16_insert_iterator & operator*() noexcept { return *this; }
        utf_32_to_16_insert_iterator & operator++() noexcept { return *this; }
        utf_32_to_16_insert_iterator operator++(int)noexcept { return *this; }

        std::insert_iterator<Container> base() const { return it_; }

    private:
        static uint16_t const high_surrogate_base = 0xd7c0;
        static uint16_t const low_surrogate_base = 0xdc00;

        std::insert_iterator<Container> it_;
    };

    /** Returns a utf_32_to_16_insert_iterator<Container> constructed from the
        given container and iterator. */
    template<typename Container>
    utf_32_to_16_insert_iterator<Container> utf_32_to_16_inserter(
        Container & c, typename Container::iterator it) noexcept
    {
        return utf_32_to_16_insert_iterator<Container>(c, it);
    }

    /** An insert-iterator analogous to std::back_insert_iterator, that also
        converts UTF-32 to UTF-16.*/
    template<typename Container>
    struct utf_32_to_16_back_insert_iterator
    {
        using value_type = void;
        using difference_type = void;
        using pointer = void;
        using reference = void;
        using iterator_category = std::output_iterator_tag;

        utf_32_to_16_back_insert_iterator(Container & c) noexcept : it_(c) {}

        utf_32_to_16_back_insert_iterator & operator=(uint32_t cp)
        {
            *it_ = static_cast<uint16_t>(cp >> 10) + high_surrogate_base;
            ++it_;
            *it_ = static_cast<uint16_t>(cp & 0x3ff) + low_surrogate_base;
            ++it_;
            return *this;
        }

        utf_32_to_16_back_insert_iterator & operator*() noexcept
        {
            return *this;
        }
        utf_32_to_16_back_insert_iterator & operator++() noexcept
        {
            return *this;
        }
        utf_32_to_16_back_insert_iterator operator++(int)noexcept
        {
            return *this;
        }

        std::back_insert_iterator<Container> base() const { return it_; }

    private:
        static uint16_t const high_surrogate_base = 0xd7c0;
        static uint16_t const low_surrogate_base = 0xdc00;

        std::back_insert_iterator<Container> it_;
    };

    /** Returns a utf_32_to_16_insert_iterator<Container> constructed from the
        given container and iterator. */
    template<typename Container>
    utf_32_to_16_back_insert_iterator<Container>
    utf_32_to_16_back_inserter(Container & c) noexcept
    {
        return utf_32_to_16_back_insert_iterator<Container>(c);
    }


    /** A UTF-16 to UTF-8 converting iterator.  Set the ErrorHandler template
        parameter to control error handling.  The default ErrorHandler is
        use_replacement_character, which simply produces a replacement
        character.

        Iter must be a bidirectional iterator with a 2-byte integral
        value_type. */
    template<
        typename Iter,
        typename Sentinel = Iter,
        typename ErrorHandler = use_replacement_character>
    struct utf_16_to_8_iterator
    {
        using value_type = char;
        using difference_type = int;
        using pointer = char *;
        using reference = char;
        using iterator_category = std::bidirectional_iterator_tag;

        static bool const throw_on_error =
            !noexcept(std::declval<ErrorHandler>()(0));

        static_assert(
            std::is_same<
                typename std::iterator_traits<Iter>::iterator_category,
                std::bidirectional_iterator_tag>::value ||
                std::is_same<
                    typename std::iterator_traits<Iter>::iterator_category,
                    std::random_access_iterator_tag>::value,
            "utf_16_to_8_iterator requires its Iter parameter to be at least "
            "bidirectional.");
        static_assert(
            sizeof(typename std::iterator_traits<Iter>::value_type) == 2,
            "utf_16_to_8_iterator requires its Iter parameter to produce a "
            "2-byte value_type.");

        constexpr utf_16_to_8_iterator() noexcept :
            first_(),
            it_(),
            last_(),
            index_(4),
            buf_()
        {}
        explicit BOOST_TEXT_CXX14_CONSTEXPR
        utf_16_to_8_iterator(Iter first, Iter it, Sentinel last) noexcept :
            first_(first),
            it_(it),
            last_(last),
            index_(0),
            buf_()
        {
            if (it_ != last_)
                read_into_buf();
        }
        template<typename Iter2, typename Sentinel2>
        constexpr utf_16_to_8_iterator(
            utf_16_to_8_iterator<Iter2, Sentinel2> const & other) noexcept :
            first_(other.first_),
            it_(other.it_),
            last_(other.last_),
            index_(other.index_),
            buf_(other.buf_)
        {}

        /** This function is constexpr in C++14 and later. */
        BOOST_TEXT_CXX14_CONSTEXPR reference operator*() const
            noexcept(!throw_on_error)
        {
            return buf_[index_];
        }

        /** This function is constexpr in C++14 and later. */
        BOOST_TEXT_CXX14_CONSTEXPR Iter base() const noexcept { return it_; }

        /** This function is constexpr in C++14 and later. */
        BOOST_TEXT_CXX14_CONSTEXPR utf_16_to_8_iterator &
        operator++() noexcept(!throw_on_error)
        {
            ++index_;
            if (at_buf_end()) {
                increment();
                index_ = 0;
                if (it_ != last_)
                    read_into_buf();
            }
            return *this;
        }

        /** This function is constexpr in C++14 and later. */
        BOOST_TEXT_CXX14_CONSTEXPR utf_16_to_8_iterator
        operator++(int)noexcept(!throw_on_error)
        {
            utf_16_to_8_iterator retval = *this;
            ++*this;
            return retval;
        }

        /** This function is constexpr in C++14 and later. */
        BOOST_TEXT_CXX14_CONSTEXPR utf_16_to_8_iterator &
        operator--() noexcept(!throw_on_error)
        {
            if (0 < index_) {
                --index_;
            } else {
                decrement();
                index_ = read_into_buf();
            }
            return *this;
        }

        /** This function is constexpr in C++14 and later. */
        BOOST_TEXT_CXX14_CONSTEXPR utf_16_to_8_iterator
        operator--(int)noexcept(!throw_on_error)
        {
            utf_16_to_8_iterator retval = *this;
            --*this;
            return retval;
        }

        template<
            typename Iter1,
            typename ErrorHandler1,
            typename Iter2,
            typename ErrorHandler2>
        friend BOOST_TEXT_CXX14_CONSTEXPR auto operator==(
            utf_16_to_8_iterator<Iter1, ErrorHandler1> lhs,
            utf_16_to_8_iterator<Iter2, ErrorHandler2> rhs) noexcept
            -> decltype(lhs.it_ == rhs.it_ && lhs.index_ == rhs.index_);

        /** This function is constexpr in C++14 and later. */
        friend BOOST_TEXT_CXX14_CONSTEXPR bool
        operator==(utf_16_to_8_iterator lhs, null_sentinel rhs) noexcept
        {
            return !*lhs.it_;
        }

        /** This function is constexpr in C++14 and later. */
        friend BOOST_TEXT_CXX14_CONSTEXPR bool
        operator==(null_sentinel lhs, utf_16_to_8_iterator rhs) noexcept
        {
            return rhs == lhs;
        }

        /** This function is constexpr in C++14 and later. */
        friend BOOST_TEXT_CXX14_CONSTEXPR bool
        operator!=(utf_16_to_8_iterator lhs, null_sentinel rhs) noexcept
        {
            return !(lhs == rhs);
        }

        /** This function is constexpr in C++14 and later. */
        friend BOOST_TEXT_CXX14_CONSTEXPR bool
        operator!=(null_sentinel lhs, utf_16_to_8_iterator rhs) noexcept
        {
            return !(rhs == lhs);
        }

#ifndef BOOST_TEXT_DOXYGEN
    private:
        BOOST_TEXT_CXX14_CONSTEXPR bool at_end() const noexcept(!throw_on_error)
        {
            if (it_ == last_) {
                ErrorHandler{}(
                    "Invalid UTF-16 sequence; expected another code unit "
                    "before the end of string.");
                return true;
            } else {
                return false;
            }
        }

        constexpr bool at_buf_end() const noexcept
        {
            return buf_[index_] == '\0';
        }

        BOOST_TEXT_CXX14_CONSTEXPR int read_into_buf() noexcept(!throw_on_error)
        {
            Iter next = it_;

            uint32_t first = static_cast<uint32_t>(*next);
            uint32_t second = 0;
            uint32_t cp = first;
            if (boost::text::high_surrogate(first)) {
                if (at_end())
                    cp = replacement_character();
                else {
                    second = static_cast<uint32_t>(*++next);
                    if (!boost::text::low_surrogate(second)) {
                        ErrorHandler{}(
                            "Invalid UTF-16 sequence; expected low surrogate "
                            "after high surrogate.");
                        cp = replacement_character();
                    } else {
                        cp = (first << 10) + second + surrogate_offset;
                    }
                }
            } else if (boost::text::surrogate(first)) {
                ErrorHandler{}("Invalid initial UTF-16 code unit.");
                cp = replacement_character();
            }

            return detail::read_into_buf(cp, buf_.data());
        }

        BOOST_TEXT_CXX14_CONSTEXPR void increment() noexcept
        {
            if (boost::text::high_surrogate(*it_)) {
                ++it_;
                if (it_ != last_ && boost::text::low_surrogate(*it_))
                    ++it_;
            } else {
                ++it_;
            }
        }

        BOOST_TEXT_CXX14_CONSTEXPR void decrement() noexcept
        {
            if (boost::text::low_surrogate(*--it_)) {
                if (it_ != first_)
                    --it_;
            }
        }

        Iter first_;
        Iter it_;
        Sentinel last_;
        int index_;
        std::array<char, 5> buf_;

        // Unicode 3.8/D71-D74

        static uint32_t const high_surrogate_min = 0xd800;
        static uint32_t const high_surrogate_max = 0xdbff;

        static uint32_t const low_surrogate_min = 0xdc00;
        static uint32_t const low_surrogate_max = 0xdfff;

        static uint32_t const surrogate_offset =
            0x10000 - (high_surrogate_min << 10) - low_surrogate_min;

        template<typename Iter2, typename Sentinel2, typename ErrorHandler2>
        friend struct utf_16_to_8_iterator;
#endif
    };

    /** Returns a utf_16_to_8_iterator<Iter> constructed from an Iter. */
    template<typename Iter, typename Sentinel>
    utf_16_to_8_iterator<Iter, Sentinel>
    make_utf_16_to_8_iterator(Iter first, Iter it, Sentinel last) noexcept
    {
        return utf_16_to_8_iterator<Iter, Sentinel>(first, it, last);
    }

    /** This function is constexpr in C++14 and later. */
    template<
        typename Iter1,
        typename ErrorHandler1,
        typename Iter2,
        typename ErrorHandler2>
    BOOST_TEXT_CXX14_CONSTEXPR auto operator==(
        utf_16_to_8_iterator<Iter1, ErrorHandler1> lhs,
        utf_16_to_8_iterator<Iter2, ErrorHandler2> rhs) noexcept
        -> decltype(lhs.it_ == rhs.it_ && lhs.index_ == rhs.index_)
    {
        return lhs.it_ == rhs.it_ && lhs.index_ == rhs.index_;
    }

    /** This function is constexpr in C++14 and later. */
    template<
        typename Iter1,
        typename ErrorHandler1,
        typename Iter2,
        typename ErrorHandler2>
    BOOST_TEXT_CXX14_CONSTEXPR auto operator!=(
        utf_16_to_8_iterator<Iter1, ErrorHandler1> lhs,
        utf_16_to_8_iterator<Iter2, ErrorHandler2> rhs) noexcept
        -> decltype(!(lhs == rhs))
    {
        return !(lhs == rhs);
    }


    template<typename Iter, typename Sentinel, typename ErrorHandler>
    struct utf_8_to_16_iterator
    {
        using value_type = uint16_t;
        using difference_type = int;
        using pointer = uint16_t *;
        using reference = uint16_t;
        using iterator_category = std::bidirectional_iterator_tag;

        static bool const throw_on_error =
            !noexcept(std::declval<ErrorHandler>()(0));

        constexpr utf_8_to_16_iterator() noexcept : it_(), index_(2), buf_() {}
        explicit BOOST_TEXT_CXX14_CONSTEXPR
        utf_8_to_16_iterator(Iter first, Iter it, Sentinel last) noexcept :
            it_(first, it, last),
            index_(0),
            buf_()
        {
            if (it_.it_ != it_.last_)
                read_into_buf();
        }
        template<typename Iter2, typename Sentinel2>
        constexpr utf_8_to_16_iterator(
            utf_8_to_16_iterator<Iter2, Sentinel2, ErrorHandler> const &
                other) noexcept :
            it_(other.it_),
            index_(other.index_),
            buf_(other.buf_)
        {}

        /** This function is constexpr in C++14 and later. */
        BOOST_TEXT_CXX14_CONSTEXPR reference operator*() const
            noexcept(!throw_on_error)
        {
            return buf_[index_];
        }

        /** This function is constexpr in C++14 and later. */
        BOOST_TEXT_CXX14_CONSTEXPR Iter base() const noexcept
        {
            return it_.base();
        }

        /** This function is constexpr in C++14 and later. */
        BOOST_TEXT_CXX14_CONSTEXPR utf_8_to_16_iterator &
        operator++() noexcept(!throw_on_error)
        {
            ++index_;
            if (at_buf_end()) {
                ++it_;
                index_ = 0;
                if (it_.it_ != it_.last_)
                    read_into_buf();
            }
            return *this;
        }

        /** This function is constexpr in C++14 and later. */
        BOOST_TEXT_CXX14_CONSTEXPR utf_8_to_16_iterator
        operator++(int)noexcept(!throw_on_error)
        {
            utf_8_to_16_iterator retval = *this;
            ++*this;
            return retval;
        }

        /** This function is constexpr in C++14 and later. */
        BOOST_TEXT_CXX14_CONSTEXPR utf_8_to_16_iterator &
        operator--() noexcept(!throw_on_error)
        {
            if (0 < index_) {
                --index_;
            } else {
                --it_;
                index_ = read_into_buf();
            }
            return *this;
        }

        /** This function is constexpr in C++14 and later. */
        BOOST_TEXT_CXX14_CONSTEXPR utf_8_to_16_iterator
        operator--(int)noexcept(!throw_on_error)
        {
            utf_8_to_16_iterator retval = *this;
            --*this;
            return retval;
        }

        template<
            typename Iter1,
            typename ErrorHandler1,
            typename Iter2,
            typename ErrorHandler2>
        friend BOOST_TEXT_CXX14_CONSTEXPR auto operator==(
            utf_8_to_16_iterator<Iter1, ErrorHandler1> lhs,
            utf_8_to_16_iterator<Iter2, ErrorHandler2> rhs) noexcept
            -> decltype(lhs.it_ == rhs.it_ && lhs.index_ == rhs.index_);

        /** This function is constexpr in C++14 and later. */
        friend BOOST_TEXT_CXX14_CONSTEXPR bool
        operator==(utf_8_to_16_iterator lhs, null_sentinel rhs) noexcept
        {
            return !*lhs.it_;
        }

        /** This function is constexpr in C++14 and later. */
        friend BOOST_TEXT_CXX14_CONSTEXPR bool
        operator==(null_sentinel lhs, utf_8_to_16_iterator rhs) noexcept
        {
            return rhs == lhs;
        }

        /** This function is constexpr in C++14 and later. */
        friend BOOST_TEXT_CXX14_CONSTEXPR bool
        operator!=(utf_8_to_16_iterator lhs, null_sentinel rhs) noexcept
        {
            return !(lhs == rhs);
        }

        /** This function is constexpr in C++14 and later. */
        friend BOOST_TEXT_CXX14_CONSTEXPR bool
        operator!=(null_sentinel lhs, utf_8_to_16_iterator rhs) noexcept
        {
            return !(rhs == lhs);
        }

#ifndef BOOST_TEXT_DOXYGEN
    private:
        constexpr bool at_buf_end() const noexcept { return buf_[index_] == 0; }

        BOOST_TEXT_CXX14_CONSTEXPR int read_into_buf() noexcept(!throw_on_error)
        {
            uint32_t const value = *it_;

            if (value < 0x10000) {
                buf_[0] = static_cast<uint16_t>(value);
                buf_[1] = 0;
                return 0;
            } else {
                buf_[0] =
                    static_cast<uint16_t>(value >> 10) + high_surrogate_base;
                buf_[1] =
                    static_cast<uint16_t>(value & 0x3ff) + low_surrogate_base;
                buf_[2] = 0;
                return 1;
            }
        }

        utf_8_to_32_iterator<Iter, Sentinel> it_;
        int index_;
        std::array<uint16_t, 4> buf_;

        static uint16_t const high_surrogate_base = 0xd7c0;
        static uint16_t const low_surrogate_base = 0xdc00;

        template<typename Iter2, typename Sentinel2, typename ErrorHandler2>
        friend struct utf_8_to_16_iterator;
#endif
    };

    /** Returns a utf_8_to_16_iterator<Iter> constructed from an Iter. */
    template<typename Iter, typename Sentinel>
    utf_8_to_16_iterator<Iter, Sentinel>
    make_utf_8_to_16_iterator(Iter first, Iter it, Sentinel last) noexcept
    {
        return utf_8_to_16_iterator<Iter, Sentinel>(first, it, last);
    }

    /** This function is constexpr in C++14 and later. */
    template<
        typename Iter1,
        typename ErrorHandler1,
        typename Iter2,
        typename ErrorHandler2>
    BOOST_TEXT_CXX14_CONSTEXPR auto operator==(
        utf_8_to_16_iterator<Iter1, ErrorHandler1> lhs,
        utf_8_to_16_iterator<Iter2, ErrorHandler2> rhs) noexcept
        -> decltype(lhs.it_ == rhs.it_ && lhs.index_ == rhs.index_)
    {
        return lhs.it_ == rhs.it_ && lhs.index_ == rhs.index_;
    }

    /** This function is constexpr in C++14 and later. */
    template<
        typename Iter1,
        typename ErrorHandler1,
        typename Iter2,
        typename ErrorHandler2>
    BOOST_TEXT_CXX14_CONSTEXPR auto operator!=(
        utf_8_to_16_iterator<Iter1, ErrorHandler1> lhs,
        utf_8_to_16_iterator<Iter2, ErrorHandler2> rhs) noexcept
        -> decltype(!(lhs == rhs))
    {
        return !(lhs == rhs);
    }


    /** An insert-iterator analogous to std::insert_iterator, that also
        converts UTF-32 to UTF-8.*/
    template<typename Container>
    struct utf_32_to_8_insert_iterator
    {
        using value_type = void;
        using difference_type = void;
        using pointer = void;
        using reference = void;
        using iterator_category = std::output_iterator_tag;

        utf_32_to_8_insert_iterator(
            Container & c, typename Container::iterator it) noexcept :
            it_(c, it)
        {}

        utf_32_to_8_insert_iterator & operator=(uint32_t cp)
        {
            uint32_t cps[1] = {cp};
            it_ = std::copy(
                utf_32_to_8_iterator<uint32_t const *>(cps, cps, cps + 1),
                utf_32_to_8_iterator<uint32_t const *>(cps, cps + 1, cps + 1),
                it_);
            return *this;
        }

        utf_32_to_8_insert_iterator & operator*() noexcept { return *this; }
        utf_32_to_8_insert_iterator & operator++() noexcept { return *this; }
        utf_32_to_8_insert_iterator operator++(int)noexcept { return *this; }

        std::insert_iterator<Container> base() const { return it_; }

    private:
        std::insert_iterator<Container> it_;
    };

    /** Returns a utf_32_to_8_insert_iterator<Container> constructed from the
        given container and iterator. */
    template<typename Container>
    utf_32_to_8_insert_iterator<Container> utf_32_to_8_inserter(
        Container & c, typename Container::iterator it) noexcept
    {
        return utf_32_to_8_insert_iterator<Container>(c, it);
    }

    /** An insert-iterator analogous to std::back_insert_iterator, that also
        converts UTF-32 to UTF-8.*/
    template<typename Container>
    struct utf_32_to_8_back_insert_iterator
    {
        using value_type = void;
        using difference_type = void;
        using pointer = void;
        using reference = void;
        using iterator_category = std::output_iterator_tag;

        utf_32_to_8_back_insert_iterator(Container & c) noexcept : it_(c) {}

        utf_32_to_8_back_insert_iterator & operator=(uint32_t cp)
        {
            uint32_t cps[1] = {cp};
            it_ = std::copy(
                utf_32_to_8_iterator<uint32_t const *>(cps, cps, cps + 1),
                utf_32_to_8_iterator<uint32_t const *>(cps, cps + 1, cps + 1),
                it_);
            return *this;
        }

        utf_32_to_8_back_insert_iterator & operator*() noexcept
        {
            return *this;
        }
        utf_32_to_8_back_insert_iterator & operator++() noexcept
        {
            return *this;
        }
        utf_32_to_8_back_insert_iterator operator++(int)noexcept
        {
            return *this;
        }

        std::back_insert_iterator<Container> base() const { return it_; }

    private:
        std::back_insert_iterator<Container> it_;
    };

    /** Returns a utf_32_to_8_insert_iterator<Container> constructed from the
        given container and iterator. */
    template<typename Container>
    utf_32_to_8_back_insert_iterator<Container>
    utf_32_to_8_back_inserter(Container & c) noexcept
    {
        return utf_32_to_8_back_insert_iterator<Container>(c);
    }

}}

#endif
