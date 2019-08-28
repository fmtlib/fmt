
#include <fmt/core.h>

static_assert(__cplusplus >= 201402L, "expect C++ 2014");

auto make_message_cpp() -> std::string{
    return fmt::format("host compiler\t: __cplusplus == {}", __cplusplus);
}
