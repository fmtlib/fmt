#include <fmt/color.h>
#include <fmt/styled_text.h>
#include <fmt/format.h>

int main() {
  using namespace fmt;
  using enum fmt::term_color;

  // Starting with two styled children creates an unstyled parent. Appending
  // later spans then preserves their own colors and the plain separators.
  auto red_text = overlay(fg(red), "red text with "
      + bold("nested bold") + " still red");

  auto line = red_text
      + " | " + overlay(bg(blue), "blue background")
      + " | " + absolute(underline("reset+underline only"))
      + " | " + reverse(overlay(fg(red), "reverse red"));

  fmt::println("{}", line);

  auto line2 = strike(overlay(bg(gray_color{12}),
                             "This is part1.  " + line + "  This is part2."));

  fmt::println("{}", line2);
}
