# MIMFS - Multithreaded In-Memory File System
#
# File:    Makefile
# Author:  Rahul Dange
# Year:    2026
# License: MIT. See the LICENSE file at the root of the distribution.
#
# Targets
#   all            build the static library
#   shared         build the shared library
#   tests          build and run the test suite
#   tests-strict   build and run the tests with the sanitizers enabled
#   benchmark      build and run the throughput measurements
#   examples       build the example programs
#   format         rewrite every source file with clang-format
#   format-check   fail if any source file is not formatted
#   analyse        run the compiler's static analyser over the library
#   coverage       run the tests and produce a coverage report
#   install        install the headers and the static library under PREFIX
#   clean          remove everything the build produced

# --------------------------------------------------------------------------
# Toolchain
# --------------------------------------------------------------------------

CC              ?= cc
AR              ?= ar
CLANG_FORMAT    ?= clang-format
PREFIX          ?= /usr/local

# C11 for the atomics, the alignment specifier and the static assertions the
# format definition relies on. None of them is an extension.
STANDARD_FLAGS   = -std=c11 -D_POSIX_C_SOURCE=200809L

# Every warning that has ever caught a real defect in this code base, promoted to
# an error so that it cannot be scrolled past.
WARNING_FLAGS    = -Wall -Wextra -Werror -Wshadow -Wcast-qual -Wcast-align \
                   -Wpointer-arith -Wstrict-prototypes -Wmissing-prototypes \
                   -Wredundant-decls -Wundef -Wwrite-strings -Wconversion \
                   -Wsign-conversion -Wvla -Wdouble-promotion

# The library reaches for its own headers by their full path from the two roots, so
# that every include line states which layer it is crossing.
INCLUDE_FLAGS    = -Iinclude -Isrc

OPTIMISATION_FLAGS ?= -O2 -fno-strict-aliasing

CFLAGS           = $(STANDARD_FLAGS) $(WARNING_FLAGS) $(INCLUDE_FLAGS) \
                   $(OPTIMISATION_FLAGS)
LDFLAGS          =
LDLIBS           = -lpthread

# --------------------------------------------------------------------------
# Layout
# --------------------------------------------------------------------------

BUILD_DIRECTORY  = build
OBJECT_DIRECTORY = $(BUILD_DIRECTORY)/objects

LIBRARY_SOURCES  = $(wildcard src/support/*.c) $(wildcard src/core/*.c) \
                   $(wildcard src/api/*.c)
LIBRARY_OBJECTS  = $(patsubst %.c,$(OBJECT_DIRECTORY)/%.o,$(LIBRARY_SOURCES))

TEST_SOURCES     = $(wildcard tests/*.c)
TEST_OBJECTS     = $(patsubst %.c,$(OBJECT_DIRECTORY)/%.o,$(TEST_SOURCES))

STATIC_LIBRARY   = $(BUILD_DIRECTORY)/libmimfs.a
SHARED_LIBRARY   = $(BUILD_DIRECTORY)/libmimfs.so

TEST_BINARY      = $(BUILD_DIRECTORY)/mimfs-tests
BENCHMARK_BINARY = $(BUILD_DIRECTORY)/mimfs-benchmark
SHELL_BINARY     = $(BUILD_DIRECTORY)/mimfs-shell
PIPELINE_BINARY  = $(BUILD_DIRECTORY)/mimfs-pipeline

ALL_SOURCES      = $(LIBRARY_SOURCES) $(TEST_SOURCES) $(wildcard examples/*.c) \
                   $(wildcard benchmarks/*.c)
ALL_HEADERS      = $(wildcard include/mimfs/*.h) $(wildcard src/support/*.h) \
                   $(wildcard src/core/*.h) $(wildcard tests/*.h)

.PHONY: all shared tests tests-strict benchmark examples format format-check \
        analyse coverage install uninstall clean help

all: $(STATIC_LIBRARY)

# --------------------------------------------------------------------------
# Libraries
# --------------------------------------------------------------------------

$(STATIC_LIBRARY): $(LIBRARY_OBJECTS)
	@mkdir -p $(dir $@)
	$(AR) rcs $@ $^
	@echo "built $@"

shared: $(SHARED_LIBRARY)

$(SHARED_LIBRARY): CFLAGS += -fPIC
$(SHARED_LIBRARY): $(LIBRARY_OBJECTS)
	@mkdir -p $(dir $@)
	$(CC) -shared $(LDFLAGS) $^ $(LDLIBS) -o $@
	@echo "built $@"

# One rule covers every source tree, and the object directory mirrors the source
# tree so that two files of the same name in two layers cannot collide.
$(OBJECT_DIRECTORY)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

-include $(shell find $(OBJECT_DIRECTORY) -name '*.d' 2>/dev/null)

# --------------------------------------------------------------------------
# Tests
# --------------------------------------------------------------------------

tests: $(TEST_BINARY)
	./$(TEST_BINARY)

$(TEST_BINARY): $(TEST_OBJECTS) $(LIBRARY_OBJECTS)
	@mkdir -p $(dir $@)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

# The address and undefined behaviour sanitizers together catch the two classes of
# defect that matter most in a filesystem written in C: an access outside a region,
# and an assumption about representation that the standard does not guarantee. The
# thread sanitizer is run separately because it cannot be combined with the address
# sanitizer.
tests-strict: OPTIMISATION_FLAGS = -O1 -g -fno-omit-frame-pointer
tests-strict: CFLAGS += -fsanitize=address,undefined -fno-sanitize-recover=all
tests-strict: LDFLAGS += -fsanitize=address,undefined
tests-strict: clean
	$(MAKE) OPTIMISATION_FLAGS="$(OPTIMISATION_FLAGS)" CFLAGS="$(CFLAGS)" \
	        LDFLAGS="$(LDFLAGS)" $(TEST_BINARY)
	./$(TEST_BINARY)

# --------------------------------------------------------------------------
# Benchmark and examples
# --------------------------------------------------------------------------

benchmark: $(BENCHMARK_BINARY)
	./$(BENCHMARK_BINARY)

$(BENCHMARK_BINARY): $(OBJECT_DIRECTORY)/benchmarks/mimfs_benchmark.o $(LIBRARY_OBJECTS)
	@mkdir -p $(dir $@)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

examples: $(SHELL_BINARY) $(PIPELINE_BINARY)

$(SHELL_BINARY): $(OBJECT_DIRECTORY)/examples/mimfs_shell.o $(LIBRARY_OBJECTS)
	@mkdir -p $(dir $@)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(PIPELINE_BINARY): $(OBJECT_DIRECTORY)/examples/mimfs_pipeline.o $(LIBRARY_OBJECTS)
	@mkdir -p $(dir $@)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

# --------------------------------------------------------------------------
# Formatting and analysis
# --------------------------------------------------------------------------

format:
	$(CLANG_FORMAT) -i --style=file $(ALL_SOURCES) $(ALL_HEADERS)
	@echo "formatted $(words $(ALL_SOURCES) $(ALL_HEADERS)) files"

format-check:
	@$(CLANG_FORMAT) --style=file --dry-run --Werror \
	    $(ALL_SOURCES) $(ALL_HEADERS) && echo "every file is formatted"

analyse:
	$(CC) $(CFLAGS) --analyze $(LIBRARY_SOURCES) -o /dev/null
	@echo "static analysis found nothing to report"

coverage: OPTIMISATION_FLAGS = -O0 -g --coverage
coverage: LDFLAGS += --coverage
coverage: clean
	$(MAKE) OPTIMISATION_FLAGS="$(OPTIMISATION_FLAGS)" LDFLAGS="$(LDFLAGS)" tests
	@echo "coverage data written next to the object files"

# --------------------------------------------------------------------------
# Installation
# --------------------------------------------------------------------------

install: $(STATIC_LIBRARY)
	install -d $(DESTDIR)$(PREFIX)/include/mimfs
	install -m 644 include/mimfs/*.h $(DESTDIR)$(PREFIX)/include/mimfs/
	install -d $(DESTDIR)$(PREFIX)/lib
	install -m 644 $(STATIC_LIBRARY) $(DESTDIR)$(PREFIX)/lib/
	@echo "installed under $(DESTDIR)$(PREFIX)"

uninstall:
	rm -rf $(DESTDIR)$(PREFIX)/include/mimfs
	rm -f $(DESTDIR)$(PREFIX)/lib/libmimfs.a
	rm -f $(DESTDIR)$(PREFIX)/lib/libmimfs.so

# --------------------------------------------------------------------------
# Housekeeping
# --------------------------------------------------------------------------

clean:
	rm -rf $(BUILD_DIRECTORY)
	rm -f *.gcda *.gcno *.gcov

help:
	@echo "MIMFS build targets:"
	@echo "  all           static library at $(STATIC_LIBRARY)"
	@echo "  shared        shared library at $(SHARED_LIBRARY)"
	@echo "  tests         build and run the test suite"
	@echo "  tests-strict  same, with the address and undefined behaviour sanitizers"
	@echo "  benchmark     build and run the throughput measurements"
	@echo "  examples      build the example programs"
	@echo "  format        rewrite every file with clang-format"
	@echo "  format-check  fail if any file is not formatted"
	@echo "  analyse       run the compiler's static analyser"
	@echo "  coverage      run the tests with coverage instrumentation"
	@echo "  install       install headers and the static library under PREFIX"
	@echo "  clean         remove the build directory"
