#
# srecord-cc makefile.
#
TOOLCHAIN=

# Compiler and dependency selection
CXX=$(TOOLCHAIN)g++
LD=$(CXX)
CXX_STD=c++17
FLAGSCXX=-std=$(CXX_STD) -W -Wall -Wextra -pedantic
FLAGSCXX+=-I./include
SCM_COMMIT:=$(shell git log --pretty=format:%h -1 2>/dev/null || echo 0000000)
BUILD_DIRECTORY=build

# make command line overrides from the known terminologies (CXXFLAGS, LDFLAGS)
# without completely replacing the previous settings.
FLAGSCXX+=$(FLAGS)
FLAGSCXX+=$(CXXFLAGS)
FLAGSLD+=$(LDFLAGS)

# Pick windows or unix-like
ifeq ($(OS),Windows_NT)
 BUILDDIR:=./$(BUILD_DIRECTORY)
 BINARY_EXTENSION=.exe
 LIBS+=
else
 BUILDDIR:=./$(BUILD_DIRECTORY)
 BINARY_EXTENSION=.elf
 LIBS+=-lm -lrt
endif

#---------------------------------------------------------------------------------------------------
# make targets
#---------------------------------------------------------------------------------------------------
.PHONY: default clean all test mrproper help example

default:
	@$(MAKE) -j test

clean:
	-@$(MAKE) test-clean
	-@rm -rf $(BUILDDIR)

mrproper: clean

all: clean
	@$(MAKE) test-clean
	@mkdir -p $(BUILDDIR)
	@$(MAKE) -j test BUILD_DIRECTORY=$(BUILD_DIRECTORY)/std11 CXX_STD=c++11 TOOLCHAIN=$(TOOLCHAIN)
	@$(MAKE) -j test BUILD_DIRECTORY=$(BUILD_DIRECTORY)/std14 CXX_STD=c++14 TOOLCHAIN=$(TOOLCHAIN)
	@$(MAKE) -j test BUILD_DIRECTORY=$(BUILD_DIRECTORY)/std17 CXX_STD=c++17 TOOLCHAIN=$(TOOLCHAIN)
	@$(MAKE) -j test BUILD_DIRECTORY=$(BUILD_DIRECTORY)/std20 CXX_STD=c++20 TOOLCHAIN=$(TOOLCHAIN)

example:
	@echo "[make] Building and running example ..."
	@mkdir -p $(BUILDDIR)/example
	@$(CXX) test/example/example.cc -o $(BUILDDIR)/example/example -std=$(CXX_STD) $(FLAGSCXX) $(FLAGSLD)
	@cp -f test/example/example.s19 $(BUILDDIR)/example/example.s19
	@./$(BUILDDIR)/example/example $(BUILDDIR)/example/example.s19

help:
	@echo "Usage: make [ clean all test example ]"
	@echo ""
	@echo " - test:           Build test binaries, run all tests that have changed."
	@echo " - example:        Build and run example binaries."
	@echo " - clean:          Clean binaries, temporary files and tests."
	@echo " - all:            Run tests for standards c++11, c++14, c++17, c++20"
	@echo ""

include test/testenv.mk
include test/sanitize.mk

#--
