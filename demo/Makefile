CXX = g++
FMT_DIR = ..
CPPFLAGS += -I$(FMT_DIR)/include -DFMT_HEADER_ONLY -DFMT_COLOR_COMPACT_SGR=1
CXXFLAGS += -std=c++23 -Wall -Wextra -Werror

.PHONY: all run clean
all: demo

demo: FmtColorDemo2.cpp $(wildcard $(FMT_DIR)/include/fmt/*.h)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $< $(LDFLAGS) $(LDLIBS) -o $@

run: demo
	./demo

clean:
	$(RM) demo
