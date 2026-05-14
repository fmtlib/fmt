// ... (existing content)
#include <complex>
#include "fmt/format.h"

namespace fmt {
template <typename T, typename Char>
struct formatter<std::complex<T>, Char> {
  // Use the existing floating-point formatter for the real and imaginary parts.
  // std::complex<T> is formattable if T is formattable.
  formatter<T, Char> real_formatter;

  template <typename ParseContext>
  auto parse(ParseContext& ctx) -> decltype(ctx.begin()) {
    return real_formatter.parse(ctx);
  }

  template <typename FormatContext>
  auto format(const std::complex<T>& c, FormatContext& ctx) const -> decltype(ctx.out()) {
    auto out = ctx.out();
    *out++ = '(';
    out = real_formatter.format(c.real(), ctx);
    *out++ = ',';
    *out++ = ' ';
    ctx.advance_to(out);
    out = real_formatter.format(c.imag(), ctx);
    *out++ = ')';
    return out;
  }
};
}  // namespace fmt