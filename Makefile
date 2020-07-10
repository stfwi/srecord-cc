CXX=g++
CXXFLAGS=-Os -std=c++11 -Wall -Wextra -pedantic
ifeq ($(OS),Windows_NT)
BINARY=srecord.exe
CXXFLAGS+=-static -static-libgcc -static-libstdc++
else
BINARY=srecord
endif
ifeq ($(DEBUG),1)
CXXFLAGS+=-O0 -g -D_DEBUG
else
LDFLAGS+=-s
endif

.PHONY: all clean test run

all: test

test:
	@cd test; make -s

clean:
	@rm -f cli/*.o cli/$(BINARY)
	@cd test; make -s clean

run: cli/$(BINARY)
	@cd cli; ./$(BINARY)

cli/$(BINARY): cli/dev.cc include/sw/srecord.hh
	@${CXX} -o "$@" "$<" ${CXXFLAGS} ${LDFLAGS} -Iinclude
