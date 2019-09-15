#ifndef BOOST_TEXT_UTILITY_HPP
#define BOOST_TEXT_UTILITY_HPP

#include <boost/text/transcode_iterator.hpp>
#include <boost/text/detail/algorithm.hpp>
#include <boost/text/detail/sentinel_tag.hpp>


namespace boost { namespace text {

    /** A range that adapts a sequence of `char const *` to a sequence of code
        points. */
    struct utf32_range
    {
        using iterator = utf_8_to_32_iterator<char const *>;

        utf32_range() :
            first_(nullptr, nullptr, nullptr),
            last_(nullptr, nullptr, nullptr)
        {}
        utf32_range(char const * f, char const * l) :
            first_(f, f, l),
            last_(f, l, l)
        {}
        utf32_range(iterator f, iterator l) : first_(f), last_(l) {}
        template<typename CharRange>
        utf32_range(CharRange const & r) :
            first_(std::begin(r), std::begin(r), std::end(r)),
            last_(std::begin(r), std::end(r), std::end(r))
        {}

        bool empty() const noexcept { return first_ == last_; }

        iterator begin() const noexcept { return first_; }
        iterator end() const noexcept { return last_; }

        friend bool operator==(utf32_range lhs, utf32_range rhs)
        {
            return lhs.first_ == rhs.first_ && lhs.last_ == rhs.last_;
        }
        friend bool operator!=(utf32_range lhs, utf32_range rhs)
        {
            return !(lhs == rhs);
        }

    private:
        iterator first_;
        iterator last_;
    };

    /** A range of code points. */
    template<typename CPIter, typename Sentinel = CPIter>
    struct cp_range
    {
        using iterator = CPIter;
        using sentinel = Sentinel;

        static_assert(
            detail::is_cp_iter<CPIter>::value,
            "CPIter must be a code point iterator");

        cp_range() {}
        cp_range(iterator first, sentinel last) : first_(first), last_(last) {}

        bool empty() const noexcept { return first_ == last_; }

        iterator begin() const { return first_; }
        sentinel end() const { return last_; }

        friend bool operator==(cp_range lhs, cp_range rhs)
        {
            return lhs.first_ == rhs.first_ && lhs.last_ == rhs.last_;
        }
        friend bool operator!=(cp_range lhs, cp_range rhs)
        {
            return !(lhs == rhs);
        }

    private:
        iterator first_;
        sentinel last_;
    };

    /** A generic range. */
    template<typename Iter, typename Sentinel = Iter>
    struct range
    {
        using iterator = Iter;
        using sentinel = Sentinel;

        range() {}
        range(iterator first, sentinel last) : first_(first), last_(last) {}

        bool empty() const noexcept { return first_ == last_; }

        iterator begin() const { return first_; }
        sentinel end() const { return last_; }

        friend bool operator==(range lhs, range rhs)
        {
            return lhs.first_ == rhs.first_ && lhs.last_ == rhs.last_;
        }
        friend bool operator!=(range lhs, range rhs) { return !(lhs == rhs); }

    private:
        iterator first_;
        sentinel last_;
    };

    namespace detail {
        template<typename T>
        using remove_cv_ref_t = typename std::remove_cv<
            typename std::remove_reference<T>::type>::type;

        template<typename Range>
        using iterator_t =
            remove_cv_ref_t<decltype(std::declval<Range>().begin())>;

        template<typename Range>
        using sentinel_t =
            remove_cv_ref_t<decltype(std::declval<Range>().end())>;

        template<
            template<class, class, class> class IterTemplate,
            typename Iter,
            typename Sentinel>
        struct make_range_impl_t
        {
            using iter_t =
                IterTemplate<Iter, Sentinel, use_replacement_character>;
            static range<iter_t, Sentinel>
            call(Iter first, Sentinel last) noexcept
            {
                return {iter_t{first, first, last}, last};
            }
        };

        template<
            template<class, class, class> class IterTemplate,
            typename Iter>
        struct make_range_impl_t<IterTemplate, Iter, Iter>
        {
            using iter_t = IterTemplate<Iter, Iter, use_replacement_character>;
            static range<iter_t, iter_t> call(Iter first, Iter last) noexcept
            {
                return {iter_t{first, first, last}, iter_t{first, last, last}};
            }
        };

        template<
            template<class, class, class> class IterTemplate,
            typename Range>
        struct make_range_t
        {
            using impl_t = make_range_impl_t<
                IterTemplate,
                iterator_t<Range const>,
                sentinel_t<Range const>>;
            static auto call(Range const & r) noexcept
                -> decltype(impl_t::call(std::begin(r), std::end(r)))
            {
                return impl_t::call(std::begin(r), std::end(r));
            }
        };
    }

#ifdef BOOST_TEXT_DOXYGEN

    /** Returns a range of code points transcoded from the given range of
        UTF-8 code units.

        This function only participates in overload resolution if `CharRange`
        models the CharRange concept. */
    template<typename CharRange>
    detail::unspecified make_to_utf32_range(CharRange const & r) noexcept;

    /** Returns a range of UTF-8 code units transcoded from the given range of
        code points.

        This function only participates in overload resolution if `CPRange`
        models the CPRange concept. */
    template<typename CPRange>
    detail::unspecified make_from_utf32_range(CPRange const & r) noexcept;

    /** Returns a range of UTF-16 code units transcoded from the given range
        of UTF-8 code units.

        This function only participates in overload resolution if `CharRange`
        models the CharRange concept. */
    template<typename CharRange>
    detail::unspecified make_to_utf16_range(CharRange const & r) noexcept;

    /** Returns a range of UTF-8 code units transcoded from the given range of
        UTF-16 code units.

        This function only participates in overload resolution if
        `Char16Range` is a range of 16-bit integral values, each of which is
        convertible to `uint16_t`. */
    template<typename Char16Range>
    detail::unspecified make_from_utf16_range(Char16Range const & r) noexcept;

#else

    template<typename CharRange>
    auto make_to_utf32_range(CharRange const & r) noexcept
        -> detail::rng_alg_ret_t<
            decltype(
                detail::make_range_t<utf_8_to_32_iterator, CharRange>::call(r)),
            CharRange>
    {
        return detail::make_range_t<utf_8_to_32_iterator, CharRange>::call(r);
    }

    template<typename CPRange>
    auto make_from_utf32_range(CPRange const & r) noexcept
        -> detail::cp_rng_alg_ret_t<
            decltype(
                detail::make_range_t<utf_32_to_8_iterator, CPRange>::call(r)),
            CPRange>
    {
        return detail::make_range_t<utf_32_to_8_iterator, CPRange>::call(r);
    }

    template<typename CharRange>
    auto make_to_utf16_range(CharRange const & r) noexcept
        -> detail::rng_alg_ret_t<
            decltype(
                detail::make_range_t<utf_8_to_16_iterator, CharRange>::call(r)),
            CharRange>
    {
        return detail::make_range_t<utf_8_to_16_iterator, CharRange>::call(r);
    }

    template<typename Char16Range>
    auto make_from_utf16_range(Char16Range const & r) noexcept
        -> detail::rng16_alg_ret_t<
            decltype(detail::make_range_t<utf_16_to_8_iterator, Char16Range>::
                         call(r)),
            Char16Range>
    {
        return detail::make_range_t<utf_16_to_8_iterator, Char16Range>::call(r);
    }

#endif

}}

#endif
