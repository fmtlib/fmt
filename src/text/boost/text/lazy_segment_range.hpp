#ifndef BOOST_TEXT_LAZY_SEGMENT_RANGE_HPP
#define BOOST_TEXT_LAZY_SEGMENT_RANGE_HPP

#include <boost/text/utility.hpp>


namespace boost { namespace text {

    namespace detail {
        template<typename CPIter, typename CPRange>
        struct segment_arrow_proxy
        {
            explicit segment_arrow_proxy(CPRange value) : value_(value) {}

            CPRange * operator->() const noexcept
            {
                return &value_;
            }

        private:
            CPRange value_;
        };

        template<
            typename CPIter,
            typename Sentinel,
            typename NextFunc,
            typename CPRange>
        struct const_lazy_segment_iterator
        {
        private:
            NextFunc * next_func_;
            CPIter prev_;
            CPIter it_;
            Sentinel last_;

        public:
            using value_type = CPRange;
            using pointer = detail::segment_arrow_proxy<CPIter, CPRange>;
            using reference = value_type;
            using difference_type = std::ptrdiff_t;
            using iterator_category = std::forward_iterator_tag;

            const_lazy_segment_iterator() noexcept :
                next_func_(),
                prev_(),
                it_(),
                last_()
            {}

            const_lazy_segment_iterator(CPIter it, Sentinel last) noexcept :
                next_func_(),
                prev_(it),
                it_(),
                last_(last)
            {}

            const_lazy_segment_iterator(Sentinel last) noexcept :
                next_func_(),
                prev_(),
                it_(),
                last_(last)
            {}

            reference operator*() const noexcept
            {
                return value_type{prev_, it_};
            }

            pointer operator->() const noexcept { return pointer(**this); }

            const_lazy_segment_iterator & operator++() noexcept
            {
                auto const next_it = (*next_func_)(it_, last_);
                prev_ = it_;
                it_ = next_it;
                return *this;
            }

            void set_next_func(NextFunc * next_func) noexcept
            {
                next_func_ = next_func;
                it_ = (*next_func_)(prev_, last_);
            }

            friend bool operator==(
                const_lazy_segment_iterator lhs,
                const_lazy_segment_iterator rhs) noexcept
            {
                return lhs.prev_ == rhs.last_;
            }
            friend bool operator!=(
                const_lazy_segment_iterator lhs,
                const_lazy_segment_iterator rhs) noexcept
            {
                return !(lhs == rhs);
            }
        };

        template<typename CPIter, typename, typename PrevFunc, typename CPRange>
        struct const_reverse_lazy_segment_iterator
        {
        private:
            PrevFunc * prev_func_;
            CPIter first_;
            CPIter it_;
            CPIter next_;

        public:
            using value_type = CPRange;
            using pointer = detail::segment_arrow_proxy<CPIter, CPRange>;
            using reference = value_type;
            using difference_type = std::ptrdiff_t;
            using iterator_category = std::forward_iterator_tag;

            const_reverse_lazy_segment_iterator() noexcept :
                prev_func_(),
                first_(),
                it_(),
                next_()
            {}

            const_reverse_lazy_segment_iterator(
                CPIter first, CPIter it, CPIter last) noexcept :
                prev_func_(),
                first_(first),
                it_(it),
                next_(last)
            {}

            reference operator*() const noexcept
            {
                return value_type{it_, next_};
            }

            pointer operator->() const noexcept { return pointer(**this); }

            const_reverse_lazy_segment_iterator & operator++() noexcept
            {
                if (it_ == first_) {
                    next_ = first_;
                    return *this;
                }
                auto const prev_it =
                    (*prev_func_)(first_, std::prev(it_), next_);
                next_ = it_;
                it_ = prev_it;
                return *this;
            }

            void set_next_func(PrevFunc * prev_func) noexcept
            {
                prev_func_ = prev_func;
                ++*this;
            }

            friend bool operator==(
                const_reverse_lazy_segment_iterator lhs,
                const_reverse_lazy_segment_iterator rhs) noexcept
            {
                return lhs.next_ == rhs.first_;
            }
            friend bool operator!=(
                const_reverse_lazy_segment_iterator lhs,
                const_reverse_lazy_segment_iterator rhs) noexcept
            {
                return !(lhs == rhs);
            }
        };
    }

    /** Represents a range of non-overlapping subranges.  Each subrange
        represents some semantically significant segment, the semantics of
        which are controlled by the `NextFunc` template parameter.  For
        instance, if `NextFunc` is next_paragraph_break, the subranges
        produced by lazy_segment_range will be paragraphs.  Each subrange is
        lazily produced; an output subrange is not produced until a lazy range
        iterator is dereferenced. */
    template<
        typename CPIter,
        typename Sentinel,
        typename NextFunc,
        typename CPRange = cp_range<CPIter>,
        template<class, class, class, class> class IteratorTemplate =
            detail::const_lazy_segment_iterator,
        bool Reverse = false>
    struct lazy_segment_range
    {
        using iterator = IteratorTemplate<CPIter, Sentinel, NextFunc, CPRange>;

        lazy_segment_range() noexcept {}
        lazy_segment_range(
            NextFunc next_func, iterator first, iterator last) noexcept :
            next_func_(std::move(next_func)),
            first_(first),
            last_(last)
        {}

        iterator begin() const noexcept
        {
            const_cast<iterator &>(first_).set_next_func(
                const_cast<NextFunc *>(&next_func_));
            return first_;
        }
        iterator end() const noexcept { return last_; }

        /** Moves the contained `NextFunc` out of *this. */
        NextFunc && next_func() && noexcept { return std::move(next_func_); }

    private:
        NextFunc next_func_;
        iterator first_;
        iterator last_;
    };

}}

#endif
