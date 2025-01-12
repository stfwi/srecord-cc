#
# srecord-cc makefile.
#
CXX=g++
STD=c++17
CXXFLAGS=-std=${STD} -W -Wall -Wextra -pedantic -Werror -g
INCLUDES=-I./include

.PHONY: all clean mrproper dist example test default

default: test

all: clean | example test

dist:
	@echo "This library has no compiled distribution files, include <sw/srecord.hh> directly."

clean:
	@rm -rf ./build

mrproper: clean

test:
	@echo "[make] Building and running tests ..."
	@mkdir -p build
	@$(CXX) test/src/test.cc -o build/test $(CCFLAGS) $(INCLUDES) $(LDFLAGS)
	@cp -r test/res build/
	@cd ./build && ./test

example:
	@echo "[make] Building and running example ..."
	@mkdir -p build
	@$(CXX) test/src/example.cc -o build/example $(CCFLAGS) $(INCLUDES) $(LDFLAGS)
	@./build/example test/res/example.s19
