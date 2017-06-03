# Optimisation selection
ifeq ($(O),)
O=0
endif
CXX:=g++
STD=11

# g++ / clang++ base flags
ifneq (,$(filter $(CXX), g++ clang++))
CCFLAGS+=-std=c++${STD} -W -Wall -Wextra -pedantic -g -O${O} -DMICROTEST=1 -I../include
endif

.PHONY: clean all test

all: clean test example

clean:
	@rm -f bin/*

test:
	@mkdir -p bin
	$(CXX) src/test.cc -o bin/test $(CCFLAGS) $(LDFLAGS)
	./bin/test

example:
	@mkdir -p bin
	$(CXX) src/example.cc -o bin/example $(CCFLAGS) $(LDFLAGS)
	./bin/example res/example.s19
