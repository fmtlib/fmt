#ifndef BOOST_TEXT_DETAIL_LZW_HPP
#define BOOST_TEXT_DETAIL_LZW_HPP

#include <boost/container/small_vector.hpp>

#include <vector>


namespace boost { namespace text { namespace detail {

    inline uint32_t bytes_to_uint32_t(unsigned char const * chars)
    {
        return chars[0] << 24 | chars[1] << 16 | chars[2] << 8 | chars[3] << 0;
    }

    inline uint32_t bytes_to_cp(unsigned char const * chars)
    {
        return chars[0] << 16 | chars[1] << 8 | chars[2] << 0;
    }

    inline uint32_t bytes_to_uint16_t(unsigned char const * chars)
    {
        return chars[0] << 8 | chars[1] << 0;
    }

    enum : uint16_t { no_predecessor = 0xffff, no_value = 0xffff };

    struct lzw_reverse_table_element
    {
        lzw_reverse_table_element(
            uint16_t pred = no_predecessor, uint16_t value = no_value) :
            pred_(pred),
            value_(value)
        {}
        uint16_t pred_;
        uint16_t value_;
    };

    using lzw_reverse_table = std::vector<lzw_reverse_table_element>;

    template<typename OutIter>
    OutIter
    copy_table_entry(lzw_reverse_table const & table, uint16_t i, OutIter out)
    {
        *out++ = table[i].value_;
        while (table[i].pred_ != no_predecessor) {
            i = table[i].pred_;
            *out++ = table[i].value_;
        }
        return out;
    }

    // Hardcoded to 16 bits.  Takes unsigned 16-bit LZW-compressed values as
    // input and writes the decompressed unsigned char values to out.
    template<typename Iter, typename OutIter>
    OutIter lzw_decompress(Iter first, Iter last, OutIter out)
    {
        lzw_reverse_table reverse_table(1 << 16);
        for (uint16_t i = 0; i < 256u; ++i) {
            reverse_table[i].value_ = i;
        }

        container::small_vector<unsigned char, 256> table_entry;

        uint32_t next_table_value = 256;
        uint32_t const end_table_value = 1 << 16;

        uint16_t prev_code = *first++;
        BOOST_ASSERT(prev_code < 256);
        unsigned char c = (unsigned char)prev_code;
        table_entry.push_back(c);
        *out++ = table_entry;

        while (first != last) {
            uint16_t const code = *first++;

            table_entry.clear();
            if (reverse_table[code].value_ == no_value) {
                table_entry.push_back(c);
                copy_table_entry(
                    reverse_table, prev_code, std::back_inserter(table_entry));
            } else {
                copy_table_entry(
                    reverse_table, code, std::back_inserter(table_entry));
            }

            *out++ = table_entry;
            c = table_entry.back();

            if (next_table_value < end_table_value) {
                reverse_table[next_table_value++] =
                    lzw_reverse_table_element{prev_code, c};
            }

            prev_code = code;
        }

        return out;
    }

}}}

#endif
