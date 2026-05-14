// ... (existing content)
#include <type_traits>
#include <utility>

#ifdef __STDCPP_FLOAT16_T__
#  include <stdfloat>
#endif

// ... (inside the namespace fmt::detail)

template <typename T>
struct is_floating_point
    : std::integral_constant<bool, std::is_floating_point<T>::value
#ifdef __STDCPP_FLOAT16_T__
                                   || std::is_same<T, std::float16_t>::value
                                   || std::is_same<T, std::float32_t>::value
                                   || std::is_same<T, std::float64_t>::value
#endif
    > {};

// ... (rest of the file)