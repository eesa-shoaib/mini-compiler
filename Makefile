CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O2
CPPFLAGS += -Isrc

SRC := src/lexer.cpp src/parser.cpp src/codegen.cpp src/main.cpp
BIN := build/cmmc

.PHONY: all clean

all: $(BIN)

$(BIN): $(SRC) src/lexer.hpp src/parser.hpp src/codegen.hpp
	@mkdir -p build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(SRC) -DCMM_LEXER_NO_MAIN -o $@

clean:
	rm -rf build
