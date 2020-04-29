# FMT Fuzzer

Fuzzing has revealed [several bugs](https://github.com/fmtlib/fmt/issues?&q=is%3Aissue+fuzz)
in fmt. It is a part of the continous fuzzing at
[oss-fuzz](https://github.com/google/oss-fuzz).

The source code is modified to make the fuzzing possible without locking up on
resource exhaustion:
```cpp
#ifdef FMT_FUZZ
if(spec.precision>100000) {
  throw std::runtime_error("fuzz mode - avoiding large precision");
}
#endif
``` 
This macro `FMT_FUZZ` is enabled on OSS-Fuzz builds and makes fuzzing
practically possible. It is used in fmt code to prevent resource exhaustion in
fuzzing mode.  
The macro `FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION` is the
defacto standard for making fuzzing practically possible to disable certain
fuzzing-unfriendly features (for example, randomness), see [the libFuzzer
documentation](https://llvm.org/docs/LibFuzzer.html#fuzzer-friendly-build-mode).

## Running the fuzzers locally

There is a [helper script](build.sh) to build the fuzzers, which has only been
tested on Debian and Ubuntu linux so far. There should be no problems fuzzing on
Windows (using clang>=8) or on Mac, but the script will probably not work out of
the box.

Something along
```sh
mkdir build
cd build
export CXX=clang++
export CXXFLAGS="-fsanitize=fuzzer-no-link -DFUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION= -g"
cmake ..  -DFMT_SAFE_DURATION_CAST=On -DFMT_FUZZ=On -DFMT_FUZZ_LINKMAIN=Off -DFMT_FUZZ_LDFLAGS="-fsanitize=fuzzer"
cmake --build .
```
should work to build the fuzzers for all platforms which clang supports.

Execute a fuzzer with for instance
```sh
cd build
export UBSAN_OPTIONS=halt_on_error=1
mkdir out_chrono
bin/fuzzer_chrono_duration  out_chrono
```
