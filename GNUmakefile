# Standard stuff

.SUFFIXES:

MAKEFLAGS+= --no-builtin-rules  # Disable the built-in implicit rules.
MAKEFLAGS+= --warn-undefined-variables        # Warn when an undefined variable is referenced.

export CC:=gcc-16
export CXX:=g++-16
export CXXFLAGS:=-stdlib=libstdc++

.PHONY: all test check clean format find-package-test

all: build
	ninja -C build all # TODO: all_verify_interface_header_sets

build: GNUmakefile CMakeLists.txt
	cmake --version # NOTE: v4.4 expected! CK
	cmake -G Ninja -S . -B build --fresh \
	  -D CMAKE_SKIP_TEST_ALL_DEPENDENCY=NO \
	  -D FMT_PEDANTIC=YES \
	  -D FMT_MODULE=1 -D FMT_IMPORT_STD=0 -D CMAKE_CXX_STANDARD=20 \
	  -D CMAKE_EXPERIMENTAL_CXX_IMPORT_STD=f35a9ac6-8463-4d38-8eec-5d6008153e7d
	ln -fs build/compile_commands.json .

find-package-test: install
	cmake -G Ninja -S test/find-package-test -B build/find-package-build --fresh \
	  -D FMT_MODULE=1 -D FMT_IMPORT_STD=0 -D CMAKE_CXX_STANDARD=20 \
	  -D CMAKE_EXPERIMENTAL_CXX_IMPORT_STD=f35a9ac6-8463-4d38-8eec-5d6008153e7d
	ninja -C build/find-package-build -v
	ninja -C build/find-package-build -t deps CMakeFiles/module-test.dir/main.cc.o
	./build/find-package-build/module-test build/find-package-build/*-test

clean:
	rm -rf build .cache compile_commands.json
	find . -name .DS_Store -delete
	find . -name '*~' -delete

format:
	git ls-files ::*CMakeLists.txt ::*.cmake | xargs cmake-format -i

check: build
	run-clang-tidy src

test: build
	ninja -C build $(@)

# Anything we don't know how to build will use this rule.
% ::
	ninja -C build $(@)
