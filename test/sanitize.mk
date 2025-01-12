#---------------------------------------------------------------------------------------------------
# Sanitizing
#---------------------------------------------------------------------------------------------------
MAKEFLAGS+= --no-print-directory
CLANG_CXX=$(firstword $(shell which clang++ 2>/dev/null) clang++-is-not-installed)
CLANG_FORMAT=$(firstword $(shell which clang-format 2>/dev/null) clang-format-is-not-installed)
CLANG_TIDY=$(firstword $(shell which clang-tidy 2>/dev/null) clang-tidy-is-not-installed)
SCAN_BUILD=$(firstword $(shell which scan-build 2>/dev/null) scan-build-is-not-installed)
CLANG_TIDY_CONFIG=$(abspath ./.clang-tidy)

wildcardr=$(foreach d,$(wildcard $1*),$(call wildcardr,$d/,$2) $(filter $(subst *,%,$2),$d))
SANITIZE_SOURCES=$(call wildcardr,test,*.hh) $(call wildcardr,test,*.h)
SANITIZE_TESTS_SOURCES=$(call wildcardr,test,*/test.cc)

#---------------------------------------------------------------------------------------------------
# Main targets
#---------------------------------------------------------------------------------------------------

.PHONY: sanitize code-analysis code-format format-check
sanitize: code-format code-analysis

#---------------------------------------------------------------------------------------------------
# Code formatting
#---------------------------------------------------------------------------------------------------

format-code: ./.clang-format
	@echo "[note] Formatting code ..."
	@$(CLANG_FORMAT) -i -style=file -- $(SANITIZE_SOURCES)
	@$(CLANG_FORMAT) -i -style=file -- $(SANITIZE_TESTS_SOURCES)

format-check: ./.clang-format
	@echo "[note] Checking code formatting ..."
	@$(CLANG_FORMAT) -i -style=file --dry-run --Werror -- $(SANITIZE_SOURCES)
	@$(CLANG_FORMAT) -i -style=file --dry-run --Werror -- $(SANITIZE_TESTS_SOURCES)

#---------------------------------------------------------------------------------------------------
# Code analysis
#---------------------------------------------------------------------------------------------------

SANITIZE_TIDY_RESULTS:=$(patsubst %.cc,$(BUILDDIR)/clang-tidy/%.tidy.log,$(SANITIZE_TESTS_SOURCES))

code-analysis: $(SANITIZE_TIDY_RESULTS)
	@echo "[note] Code analysis finished."

$(BUILDDIR)/clang-tidy/test/%/test.tidy.log: test/%/test.cc
	@echo "[note] Running clang-tidy on $< ..."
	@mkdir -p $(dir $@)
	@$(CLANG_TIDY) --config-file=$(CLANG_TIDY_CONFIG) -p="$(dir $@)" "$<" -- $(FLAGSCXX) -I. -I./test $(OPTS) >$@ 2>&1
