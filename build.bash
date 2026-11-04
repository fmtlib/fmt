#!/bin/bash
set -euo pipefail

build_dir=build
fmt_doc=ON
cmake_args=()
if (( $# > 1 )); then
  echo "Usage: $0 [taskwarrior]" >&2
  exit 2
fi
case "${1:-}" in
  "") ;;
  taskwarrior)
    build_dir=build/taskwarrior
    fmt_doc=OFF
    cmake_args=(-DCMAKE_CXX_FLAGS=-DFMT_COLOR_COMPACT_SGR=1)
    ;;
  *)
    echo "Usage: $0 [taskwarrior]" >&2
    exit 2
    ;;
esac

set -x

doc_bin="$(pwd)/build/venv/bin"
export PATH="$doc_bin:$PATH"

# Follow support/release.py: isolate the pinned documentation tools and put
# them on PATH for this script, without activating them in the calling shell.
# Reuse the environment; do not delete existing local files.
if [[ ! -d build/venv ]]; then
  python -m venv build/venv
  "$doc_bin/python" -m pip install --require-hashes -r support/doc-requirements.txt
fi

# Skip tests incompatible with Android fdsan on Termux only.
fmtcolor_skip=OFF
if [[ -n ${TERMUX_VERSION:-} || ${PREFIX:-} == */com.termux/files/usr ]]; then
  fmtcolor_skip=ON
fi

cmake -S . -B "$build_dir" -U MKDOCS -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=g++ \
  -DCMAKE_CXX_STANDARD=23 -DFMT_TEST=ON -DFMT_DOC="$fmt_doc" -DFMTCOLOR_SKIP="$fmtcolor_skip" \
  "${cmake_args[@]}"
cmake --build    "$build_dir" --parallel 8
ctest --test-dir "$build_dir" --output-on-failure
if  [[ "$fmt_doc" == "ON" ]]; then
  cmake --build    "$build_dir" --target doc
fi
#cmake --install "$build_dir" --prefix ~/.local

# For Clang/libc++, add `-DCMAKE_CXX_COMPILER=clang++`
# and `-DCMAKE_CXX_FLAGS=-stdlib=libc++`.
# Consumers use `fmt::fmt` or `fmt::fmt-header-only`; both propagate `cxx_std_11`.
