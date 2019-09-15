#ifndef BOOST_TEXT_ALGORITHM_HPP
#define BOOST_TEXT_ALGORITHM_HPP

#include <boost/text/detail/sentinel_tag.hpp>

#include <cstddef>
#include <iterator>


namespace boost { namespace text {

    namespace detail {
        template<typename Iter>
        std::ptrdiff_t distance(Iter first, Iter last, non_sentinel_tag)
        {
            return std::distance(first, last);
        }

        template<typename Iter, typename Sentinel>
        std::ptrdiff_t distance(Iter first, Sentinel last, sentinel_tag)
        {
            std::ptrdiff_t retval = 0;
            while (first != last) {
                ++retval;
                ++first;
            }
            return retval;
        }
    }

    /** Range-friendly version of `std::distance()`, taking an iterator and a
        sentinel. */
    template<typename Iter, typename Sentinel>
    std::ptrdiff_t distance(Iter first, Sentinel last)
    {
        return detail::distance(
            first,
            last,
            typename std::conditional<
                std::is_same<Iter, Sentinel>::value,
                detail::non_sentinel_tag,
                detail::sentinel_tag>::type());
    }

    /** Range-friendly version of `std::find()`, taking an iterator and a
        sentinel. */
    template<typename BidiIter, typename Sentinel, typename T>
    BidiIter find(BidiIter first, Sentinel last, T const & x)
    {
        while (first != last) {
            if (*first == x)
                return first;
            ++first;
        }
        return first;
    }

    /** A range-friendly compliment to `std::find()`; returns an iterator to
        the first element not equal to `x`. */
    template<typename BidiIter, typename Sentinel, typename T>
    BidiIter find_not(BidiIter first, Sentinel last, T const & x)
    {
        while (first != last) {
            if (*first != x)
                return first;
            ++first;
        }
        return first;
    }

    /** Range-friendly version of `std::find_if()`, taking an iterator and a
        sentinel. */
    template<typename BidiIter, typename Sentinel, typename Pred>
    BidiIter find_if(BidiIter first, Sentinel last, Pred p)
    {
        while (first != last) {
            if (p(*first))
                return first;
            ++first;
        }
        return first;
    }

    /** Range-friendly version of `std::find_if_not()`, taking an iterator and
        a sentinel. */
    template<typename BidiIter, typename Sentinel, typename Pred>
    BidiIter find_if_not(BidiIter first, Sentinel last, Pred p)
    {
        while (first != last) {
            if (!p(*first))
                return first;
            ++first;
        }
        return first;
    }

    /** Analogue of `std::find()` that finds the last value in `[first, last)`
        equal to `x`. */
    template<typename BidiIter, typename T>
    BidiIter find_backward(BidiIter first, BidiIter last, T const & x)
    {
        auto it = last;
        while (it != first) {
            if (*--it == x)
                return it;
        }
        return last;
    }

    /** Analogue of `std::find()` that finds the last value in `[first, last)`
        not equal to `x`. */
    template<typename BidiIter, typename T>
    BidiIter find_not_backward(BidiIter first, BidiIter last, T const & x)
    {
        auto it = last;
        while (it != first) {
            if (*--it != x)
                return it;
        }
        return last;
    }

    /** Analogue of `std::find()` that finds the last value `v` in `[first,
        last)` for which `p(v)` is true. */
    template<typename BidiIter, typename Pred>
    BidiIter find_if_backward(BidiIter first, BidiIter last, Pred p)
    {
        auto it = last;
        while (it != first) {
            if (p(*--it))
                return it;
        }
        return last;
    }

    /** Analogue of `std::find()` that finds the last value `v` in `[first,
        last)` for which `p(v)` is false. */
    template<typename BidiIter, typename Pred>
    BidiIter find_if_not_backward(BidiIter first, BidiIter last, Pred p)
    {
        auto it = last;
        while (it != first) {
            if (!p(*--it))
                return it;
        }
        return last;
    }

    /** A utility range type returned by `foreach_subrange*()`. */
    template<typename Iter, typename Sentinel = Iter>
    struct foreach_subrange_range
    {
        using iterator = Iter;
        using sentinel = Sentinel;

        foreach_subrange_range() {}
        foreach_subrange_range(iterator first, sentinel last) :
            first_(first),
            last_(last)
        {}

        iterator begin() const noexcept { return first_; }
        sentinel end() const noexcept { return last_; }

    private:
        iterator first_;
        sentinel last_;
    };

    /** Calls `f(sub)` for each subrange `sub` in `[first, last)`.  A subrange
        is a contiguous subsequence of elements that each compares equal to
        the first element of the subsequence.  Subranges passed to `f` are
        non-overlapping. */
    template<typename FwdIter, typename Sentinel, typename Func>
    Func foreach_subrange(FwdIter first, Sentinel last, Func f)
    {
        while (first != last) {
            auto const & x = *first;
            auto const next = find_not(first, last, x);
            if (first != next)
                f(foreach_subrange_range<FwdIter, Sentinel>(first, next));
            first = next;
        }
        return f;
    }

    /** Calls `f(sub)` for each subrange `sub` in `[first, last)`.  A subrange
        is a contiguous subsequence of elements that for each element `e`,
        `proj(e)` each compares equal to `proj()` of the first element of the
        subsequence.  Subranges passed to `f` are non-overlapping. */
    template<typename FwdIter, typename Sentinel, typename Func, typename Proj>
    Func foreach_subrange(FwdIter first, Sentinel last, Func f, Proj proj)
    {
        using value_type = typename std::iterator_traits<FwdIter>::value_type;
        while (first != last) {
            auto const & x = proj(*first);
            auto const next = find_if_not(
                first, last, [&x, proj](const value_type & element) {
                    return proj(element) == x;
                });
            if (first != next)
                f(foreach_subrange_range<FwdIter, Sentinel>(first, next));
            first = next;
        }
        return f;
    }

    /** Calls `f(sub)` for each subrange `sub` in `[first, last)`.  A subrange
        is a contiguous subsequence of elements, each of which is equal to
        `x`.  Subranges passed to `f` are non-overlapping. */
    template<typename FwdIter, typename Sentinel, typename T, typename Func>
    Func foreach_subrange_of(FwdIter first, Sentinel last, T const & x, Func f)
    {
        while (first != last) {
            first = find(first, last, x);
            auto const next = find_not(first, last, x);
            if (first != next)
                f(foreach_subrange_range<FwdIter, Sentinel>(first, next));
            first = next;
        }
        return f;
    }

    /** Calls `f(sub)` for each subrange `sub` in `[first, last)`.  A subrange
        is a contiguous subsequence of elements `ei` for which `p(ei)` is
        true.  Subranges passed to `f` are non-overlapping. */
    template<typename FwdIter, typename Sentinel, typename Pred, typename Func>
    Func foreach_subrange_if(FwdIter first, Sentinel last, Pred p, Func f)
    {
        while (first != last) {
            first = boost::text::find_if(first, last, p);
            auto const next = boost::text::find_if_not(first, last, p);
            if (first != next)
                f(foreach_subrange_range<FwdIter, Sentinel>(first, next));
            first = next;
        }
        return f;
    }

    /** Sentinel-friendly version of `std::all_of()`. */
    template<typename Iter, typename Sentinel, typename Pred>
    bool all_of(Iter first, Sentinel last, Pred p)
    {
        for (; first != last; ++first) {
            if (!p(*first))
                return false;
        }
        return true;
    }

}}

#endif
