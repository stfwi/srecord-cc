#---------------------------------------------------------------------------------------------------
# Example Makefile include for compiling and running all tests
# with the pattern `<rootdir>/test/*/test.cc`.

# clang++/g++ sanitizer support (`make test WITH_SANITIZERS=1`)
#
# Requires GNU make version >= v4.0 and basic UNIX tools (for Windows
# simply install GIT globally, so that tools like rm.exe are in the PATH;
# Linux/BSD: no action needed).
#---------------------------------------------------------------------------------------------------
MAKEFLAGS+= --no-print-directory
MICROTEST_ROOT=./test/microtest/include
PRINT_RESULTS=0
WITH_ANSI_COLORS=1

# Test selection
wildcardr=$(foreach d,$(wildcard $1*),$(call wildcardr,$d/,$2) $(filter $(subst *,%,$2),$d))

TEST_SELECTION:=$(sort $(wildcard test/*$(TEST)*/))
TEST_BINARIES_SOURCES:=$(wildcard $(foreach F, $(filter test/%/ , $(TEST_SELECTION)), $Ftest.cc))
TEST_BINARIES:=$(patsubst %.cc,$(BUILDDIR)/%$(BINARY_EXTENSION),$(TEST_BINARIES_SOURCES))
TEST_BINARIES_RESULTS:=$(patsubst %.cc,$(BUILDDIR)/%.log,$(TEST_BINARIES_SOURCES))

# g++ pedantic test run options.
ifneq (,$(findstring g++,$(CXX)))
 # (Careful with formatting of this makefile, there are no tabs these blocks, indentation is with spaces)
 ifeq ($(WITH_SANITIZERS),1)
  TESTOPTS+=-g -ggdb -fsanitize=address -fno-omit-frame-pointer -fsanitize=undefined
  FLAGSLD+=-static-libstdc++ -static-libasan
	ifeq ($(MORE_SANITIZERS),1)
   TESTOPTS+=-fsanitize=pointer-compare -fsanitize=pointer-subtract -fsanitize=leak
	endif
 endif
endif

# clang++ pedantic test run options
ifneq (,$(findstring clang++,$(CXX)))
 ifeq ($(WITH_SANITIZERS),1)
  TESTOPTS+=-g -fsanitize=address -fsanitize=memory -fsanitize=memory-track-origins -fsanitize=thread
 endif
endif

ifeq ($(WITH_ANSI_COLORS),1)
 TESTOPTS+=-DWITH_MICROTEST_ANSI_COLORS_FORCED
endif

#---------------------------------------------------------------------------------------------------
# Tests
#---------------------------------------------------------------------------------------------------
.PHONY: test test-clean test-results
.PRECIOUS: %.log %.elf %.exe

# test-clean only removes the test build and result directory,
# so that other files in the build directory are not affected.
test-clean:
	@rm -rf $(BUILDDIR)/test

# Test invocation and summary (we can't use a js script to analyze the tests, so the unix tools
# that are available on linux and windows (with GIT) are the tools of choice).
test: $(TEST_BINARIES_SOURCES)
	@mkdir -p $(BUILDDIR)/test
	@rm -f $(BUILDDIR)/test/*.log
 ifneq ($(TEST),)
	@rm -f $(TEST_BINARIES_RESULTS)
 endif
	@$(MAKE) -j -k test-results | tee $(BUILDDIR)/test/summary.log 2>&1
	@if grep -e '^\[fail\]' -- $(BUILDDIR)/test/summary.log >/dev/null 2>&1; then echo "[FAIL] Total verdict: At least one test failed."; /bin/false; else echo "[PASS] Total verdict: All tests passed."; fi
 ifneq ($(TEST)$(PRINT_RESULTS),)
  ifneq ($(TEST_BINARIES_RESULTS),)
	-@echo "[----] Test detail listing:"
	-@echo "[----] --------------------------------------------------"
	-@cat $(TEST_BINARIES_RESULTS) 2>/dev/null
  endif
 endif

# Actual test compilations and runs, done in parallel if
# `make -j` is specified.
test-results: $(TEST_BINARIES) $(TEST_BINARIES_RESULTS)

# Test binaries (compile)
$(BUILDDIR)/test/%/test$(BINARY_EXTENSION): test/%/test.cc test/testenv.hh $(MICROTEST_ROOT)/microtest.hh $(HEADER_DEPS)
	@echo "[c++ ] $@"
	@mkdir -p $(dir $@)
	@cp -rf $(dir $<)/* $(dir $@)/
	@$(CXX) -o $@ $< $(FLAGSCXX) -I. -I./test $(FLAGSLD) $(LDSTATIC) $(LIBS) $(TESTOPTS) $(OPTS) -DSCM_COMMIT='"""$(SCM_COMMIT)"""' || echo "[fail] $@"
	@rm -f $(dir $@)/test.cc || /bin/true
	@[ -f test.gcno ] && mv test.gcno $(dir $@) || /bin/true

# Test runs
$(BUILDDIR)/test/%/test.log: $(BUILDDIR)/test/%/test$(BINARY_EXTENSION)
	@mkdir -p $(dir $@)
	@rm -f $@
 ifneq ($(OS),Windows_NT)
	@cd $(dir $<) && ./$(notdir $<) $(ARGS) </dev/null >$(notdir $@) 2>&1 && echo "[pass] $@" || echo "[fail] $@"
	@[ -f test.gcda ] && mv test.gcda $(dir $@) || /bin/true
 else
	@cd $(dir $<) && echo "" | "./$(notdir $<)" $(ARGS) >$(notdir $@) && echo "[pass] $@" || echo "[fail] $@"
 endif

#---------------------------------------------------------------------------------------------------
# Coverage (only available with gcov/lcov using linux g++)
#---------------------------------------------------------------------------------------------------
.PHONY: coverage coverage-runs coverage-files coverage-summary
LCOV_DATA_ROOT=$(BUILDDIR)/coverage

# Coverage compile flags for g++
ifneq (,$(WITH_COVERAGE))
 ifneq (Linux,$(shell uname))
 	$(error Coverage is supported under Linux with g++ and gcov/lcov)
 endif
 .NOTPARALLEL:
 TESTOPTS+=--coverage -ftest-coverage -fprofile-arcs -fprofile-abs-path -O0 -fprofile-filter-files='main\.cc;$(MICROTEST_ROOT)/.*\.hh'
endif

# Coverage main target (the one we invoke)
coverage:
	@$(MAKE) clean
	@$(MAKE) --jobs=1 coverage-runs WITH_COVERAGE=1
	@$(MAKE) coverage-files
	@$(MAKE) coverage-summary

# Selection of tests where c++ coverage makes sense
coverage-runs: $(TEST_BINARIES_RESULTS)

# All gcov files from the executed tests
coverage-files: $(patsubst %/test.log,%/test.gcov,$(TEST_BINARIES_RESULTS))

# Analysis of one test using gcov/lcov
$(BUILDDIR)/test/%/test.gcov: $(BUILDDIR)/test/%/test.log
	@echo "[gcov] $*"
	@cd $(dir $<) && gcov --source-prefix "$(CURDIR)" *.gcno > gcov.log 2>&1
	@cd $(dir $<) && gcov -m *.cc >> gcov.log 2>&1
	@[ ! -z "$(shell which lcov)" ] || echo "[warn] lcov is not installed."
	@cd $(dir $<) && lcov -c -d . -o lcov.info >> lcov.log 2>&1
	@cd $(dir $<) && lcov -r lcov.info "/usr*" -o lcov.info >> lcov.log 2>&1
	@mkdir -p $(BUILDDIR)/coverage/data
	@mv $(dir $<)/lcov.info $(BUILDDIR)/coverage/data/$*.info

# Combined coverage all executed tests
coverage-summary:
	@echo "[lcov] Summary"
	@cd $(LCOV_DATA_ROOT) && genhtml data/*.info --output-directory ./html >> lcov.log 2>&1

#---------------------------------------------------------------------------------------------------
# Dump environment
#---------------------------------------------------------------------------------------------------
.PHONY: show-vars

show-vars:
	@echo "TEST_SELECTION=$(TEST_SELECTION)"
	@echo "TEST_BINARIES_SOURCES='$(TEST_BINARIES_SOURCES)'"
	@echo "TEST_BINARIES='$(TEST_BINARIES)'"
	@echo "TEST_BINARIES_RESULTS='$(TEST_BINARIES_RESULTS)'"

#--
