// Isolated allocation regression: replacing global new must not affect other tests.
#include "fmt/styled_text.h"

#include <cstdio>
#include <cstdlib>
#include <new>

namespace {
bool count_allocations = false;
size_t allocated_bytes = 0;
size_t allocation_count = 0;
size_t largest_allocation = 0;
size_t bookkeeping_size = 0;
size_t bookkeeping_count = 0;
}

#if defined(__GNUC__)
#  define FMTCOLOR_TEST_NOINLINE __attribute__((noinline))
#elif defined(_MSC_VER)
#  define FMTCOLOR_TEST_NOINLINE __declspec(noinline)
#else
#  define FMTCOLOR_TEST_NOINLINE
#endif

FMTCOLOR_TEST_NOINLINE void* operator new(size_t size) {
  if (count_allocations) {
    allocated_bytes += size;
    ++allocation_count;
    if (size > largest_allocation) largest_allocation = size;
    if (size <= bookkeeping_size) ++bookkeeping_count;
  }
  void* ptr = std::malloc(size == 0 ? 1 : size);
  if (!ptr) FMT_THROW(std::bad_alloc());
  return ptr;
}
FMTCOLOR_TEST_NOINLINE void operator delete(void* ptr) noexcept {
  std::free(ptr);
}
void* operator new[](size_t size) { return ::operator new(size); }
void operator delete[](void* ptr) noexcept { ::operator delete(ptr); }
#if defined(__cpp_sized_deallocation) && __cpp_sized_deallocation >= 201309L
void operator delete(void* ptr, size_t) noexcept { ::operator delete(ptr); }
void operator delete[](void* ptr, size_t) noexcept { ::operator delete(ptr); }
#endif

namespace {
void start_counting() {
  allocated_bytes = 0;
  allocation_count = 0;
  largest_allocation = 0;
  bookkeeping_count = 0;
  count_allocations = true;
}

struct allocation_snapshot {
  size_t count, bytes, bookkeeping;
};

auto snapshot() -> allocation_snapshot {
  return {allocation_count, allocated_bytes, bookkeeping_count};
}

// Only MSVC's iterator-debug mode permits extra container-proxy allocations.
// Keep other libraries/modes at their original exact allocation counts.
bool allocations_match(allocation_snapshot expected) {
#if defined(_MSC_VER) && defined(_ITERATOR_DEBUG_LEVEL) && \
    _ITERATOR_DEBUG_LEVEL != 0
  // Allow a bounded number of proxy allocations across the helper move chain.
  // Match all larger allocations exactly, so copying a child vector or a large
  // character buffer cannot be mistaken for iterator bookkeeping.
  constexpr size_t proxy_budget = 32;
  return allocation_count >= expected.count &&
      allocation_count - expected.count <= proxy_budget &&
      allocated_bytes >= expected.bytes &&
      allocated_bytes - expected.bytes <= proxy_budget * bookkeeping_size &&
      allocation_count - bookkeeping_count ==
          expected.count - expected.bookkeeping;
#else
  return allocation_count == expected.count;
#endif
}

template<typename Char>
bool check(const char* stage, bool valid,
           allocation_snapshot expected = {0, 0, 0}) {
  if (valid) return true;
  std::fprintf(stderr,
      "styled_text sharing [%s] %s failed: allocations=%zu, bytes=%zu, "
      "largest=%zu; baseline allocations=%zu, bytes=%zu; "
      "bookkeeping size=%zu, count=%zu\n",
      sizeof(Char) == 1 ? "char" : "wchar_t", stage,
      allocation_count, allocated_bytes, largest_allocation,
      expected.count, expected.bytes, bookkeeping_size, bookkeeping_count);
  return false;
}

template<typename Char>
void measure_bookkeeping() {
  bookkeeping_size = 0;
#if defined(_MSC_VER) && defined(_ITERATOR_DEBUG_LEVEL) && \
    _ITERATOR_DEBUG_LEVEL != 0
  // Empty containers have no element buffer. Their largest allocation measures
  // iterator-debug bookkeeping without depending on private STL proxy types.
  start_counting();
  const auto text = std::basic_string<Char>{};
  const auto children = std::vector<fmt::basic_styled_text<Char>>{};
  count_allocations = false;
  bookkeeping_size = largest_allocation;
  (void)text;
  (void)children;
#endif
}

template<typename Char>
bool check_sharing() {
  using node = fmt::basic_styled_text<Char>;
  measure_bookkeeping<Char>();
  const auto large = std::basic_string<Char>(100000, static_cast<Char>('x'));
  const auto source = node(large);
  // A large character-buffer allocation must still be detected and rejected,
  // including when MSVC iterator-debug bookkeeping is permitted. Call the
  // replacement directly: string copies may share storage or elide allocation.
  start_counting();
  auto* buffer = ::operator new(large.size() * sizeof(Char));
  count_allocations = false;
  ::operator delete(buffer);
  if (!check<Char>("large-buffer detection control",
                   allocated_bytes >= large.size() * sizeof(Char) &&
                   !allocations_match({0, 0, 0})))
    return false;

  start_counting();
  auto copy = source;
  auto assigned = node{};
  assigned = source;
  count_allocations = false;
  if (!check<Char>("leaf copy/assignment",
                   allocation_count == 0 && copy == source && assigned == source))
    return false;

  const auto branch = source + source;
  start_counting();
  auto branch_copy = branch;
  count_allocations = false;
  const auto branch_copy_allocations = snapshot();
  // Structure may allocate; copying either large character buffer must not.
  if (!check<Char>("branch copy",
                   allocated_bytes < large.size() * sizeof(Char) &&
                   branch_copy == branch))
    return false;

  const auto root_style = fmt::fg(fmt::color::red);
  auto expected = branch;
  expected.style(root_style);
  start_counting();
  branch_copy.overlay(root_style);
  count_allocations = false;
  if (!check<Char>("in-place overlay",
                   allocation_count == 0 && branch_copy == expected))
    return false;

  start_counting();
  auto styled_copy = fmt::overlay(root_style, branch);
  count_allocations = false;
  if (!check<Char>("copying overlay",
                   allocations_match(branch_copy_allocations) &&
                   styled_copy == expected && branch.style().empty(),
                   branch_copy_allocations))
    return false;

  start_counting();
  auto styled_move = fmt::strike(fmt::bold(
      fmt::overlay(root_style, std::move(branch_copy))));
  count_allocations = false;
  expected.style(root_style | fmt::bold() | fmt::strike());
  if (!check<Char>("moving overlay/emphasis chain",
                   allocations_match({0, 0, 0}) && branch_copy.empty() &&
                   styled_move == expected))
    return false;

  start_counting();
  auto empty = node{};
  auto explicit_empty = node(std::basic_string<Char>{});
  auto moved = node(std::move(copy));
  count_allocations = false;
  return check<Char>("empty construction/leaf move",
                     allocations_match({0, 0, 0}) && empty.empty() &&
                     explicit_empty.empty() && copy.empty() && moved == source);
}
} // namespace

int main() {
  const auto narrow = check_sharing<char>();
  const auto wide = check_sharing<wchar_t>();
  if (narrow && wide) return 0;
  std::fputs("styled_text sharing allocation regression failed\n", stderr);
  return 1;
}
