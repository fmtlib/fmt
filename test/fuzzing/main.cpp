#include <cassert>
#include <fstream>
#include <sstream>
#include <vector>
#include "fuzzer_common.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* Data, size_t Size);
int main(int argc, char* argv[]) {
  for (int i = 1; i < argc; ++i) {
    std::ifstream in(argv[i]);
    assert(in);
    in.seekg(0, std::ios_base::end);
    const auto pos = in.tellg();
    assert(pos >= 0);
    in.seekg(0, std::ios_base::beg);
    std::vector<char> buf(static_cast<size_t>(pos));
    in.read(buf.data(), static_cast<long>(buf.size()));
    assert(in.gcount() == pos);
    LLVMFuzzerTestOneInput(fmt_fuzzer::as_bytes(buf.data()), buf.size());
  }
}
