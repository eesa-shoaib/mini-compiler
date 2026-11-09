CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O2
CPPFLAGS += -Isrc

SRC := src/lexer.cpp src/parser.cpp src/codegen.cpp src/main.cpp
BIN := build/cmmc

.PHONY: all debug clean

DEBUG_BIN := build/cmmc-debug

all: $(BIN)

debug: $(DEBUG_BIN)

$(DEBUG_BIN): $(SRC) src/lexer.hpp src/parser.hpp src/codegen.hpp
	@mkdir -p build
	$(CXX) $(CPPFLAGS) -std=c++17 -Wall -Wextra -g -O0 -fno-omit-frame-pointer $(SRC) -DCMM_LEXER_NO_MAIN -o $(DEBUG_BIN)

$(BIN): $(SRC) src/lexer.hpp src/parser.hpp src/codegen.hpp
	@mkdir -p build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(SRC) -DCMM_LEXER_NO_MAIN -o $@

clean:
	rm -rf build
