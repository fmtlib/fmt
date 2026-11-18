import fmt;

#ifdef FMT_IMPORT_STD
import std;
#endif

static void test() {
#ifdef FMT_IMPORT_STD
  auto now = std::chrono::system_clock::now();
  fmt::print("Date and time: {}\n", now);
  fmt::print("Time: {:%H:%M}\n", now);

  std::vector<int> v = {1, 2, 3};
  fmt::print("{}\n", v);

#  if FMT_USE_REFLECTION
  enum class[[= fmt::as_identifiers]] color { red, green, blue };
  fmt::println("{}", color::green);
#  endif

#endif
}

int main(int argc, char** argv) {
  for (int i = 0; i < argc; ++i) {
    fmt::print("{}: {}\n", i, argv[i]);
  }
  test();
}
