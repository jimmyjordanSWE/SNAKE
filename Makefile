SHELL := /bin/bash
.SHELLFLAGS := -eu -o pipefail -c

# Quiet, readable output by default.
MAKEFLAGS += --no-print-directory

CC ?= gcc

BUILD_ROOT ?= build
CONFIG ?= debug-asan

# NOTE: GNU make defines a built-in variable named `LINT` (default value: `lint`).
# Use explicit toggles to avoid collisions and keep builds fast.
RUN_FORMAT ?= 1
RUN_TIDY ?= 0

PREBUILD := $(if $(filter 1,$(RUN_FORMAT)),format-check,) $(if $(filter 1,$(RUN_TIDY)),tidy,)

CPPFLAGS_BASE ?= -Iinclude -D_POSIX_C_SOURCE=200809L
CFLAGS_BASE ?= -std=c99

# Keep code quality high from the start.
WARNINGS ?= \
	-Wall -Wextra -Wpedantic \
	-Wshadow -Wstrict-prototypes -Wmissing-prototypes \
	-Wcast-align -Wwrite-strings \
	-Wformat=2 -Wundef \
	-Wvla \
	-Wconversion -Wsign-conversion

# Treat warnings as errors by default (set WERROR=0 to relax).
WERROR ?= 1

ifeq ($(WERROR),1)
WARNINGS += -Werror
endif
LDFLAGS_BASE ?=
LDLIBS ?=

DEBUG_FLAGS ?= -O0 -g3
RELEASE_FLAGS ?= -O3 -DNDEBUG
ASAN_FLAGS ?= -fsanitize=address -fno-omit-frame-pointer

VALGRIND ?= valgrind
VALGRIND_ARGS ?= --leak-check=full --show-leak-kinds=all --track-origins=yes --error-exitcode=1

CLANG_FORMAT ?= clang-format
CLANG_TIDY ?= clang-tidy

TIDY_CHECKS ?= clang-analyzer-*,bugprone-*,performance-*
# Keep output readable: clang-tidy's underlying clang can be very noisy.
TIDY_EXTRA_ARGS ?= --quiet --extra-arg=-w

OK_MSG ?= all OK

ifeq ($(CONFIG),debug-asan)
BUILD_DIR := $(BUILD_ROOT)/debug-asan
CPPFLAGS := $(CPPFLAGS_BASE)
	CFLAGS := $(CFLAGS_BASE) $(WARNINGS) $(DEBUG_FLAGS) $(ASAN_FLAGS)
LDFLAGS := $(LDFLAGS_BASE) $(ASAN_FLAGS)
else ifeq ($(CONFIG),release)
BUILD_DIR := $(BUILD_ROOT)/release
CPPFLAGS := $(CPPFLAGS_BASE)
	CFLAGS := $(CFLAGS_BASE) $(WARNINGS) $(RELEASE_FLAGS)
LDFLAGS := $(LDFLAGS_BASE)
else ifeq ($(CONFIG),valgrind)
BUILD_DIR := $(BUILD_ROOT)/valgrind
CPPFLAGS := $(CPPFLAGS_BASE)
	CFLAGS := $(CFLAGS_BASE) $(WARNINGS) $(DEBUG_FLAGS) -fno-omit-frame-pointer
LDFLAGS := $(LDFLAGS_BASE)
else
$(error Unknown CONFIG '$(CONFIG)'. Use CONFIG=debug-asan|release|valgrind)
endif

OBJ_DIR := $(BUILD_DIR)/obj
BIN := $(BUILD_DIR)/snake
LOG_DIR := $(BUILD_ROOT)/logs

SRC := $(shell find src -name '*.c' -print) main.c
HDR := $(shell find include -name '*.h' -print)
FORMAT_FILES := $(SRC) $(HDR)
TIDY_FILES := $(SRC)
OBJ := $(patsubst %.c,$(OBJ_DIR)/%.o,$(SRC))
DEP := $(OBJ:.o=.d)

.DEFAULT_GOAL := debug

.PHONY: all build debug release valgrind gdb clean tools-check lint format-check format tidy

all: debug

build: $(BIN)

ifeq ($(RUN_FORMAT),1)
build: format-check
endif

ifeq ($(RUN_TIDY),1)
build: tidy
endif

debug:
	@$(MAKE) CONFIG=debug-asan $(PREBUILD) build
	@echo "$(OK_MSG)"

release:
	@$(MAKE) CONFIG=release $(PREBUILD) build
	@echo "$(OK_MSG)"

valgrind:
	@$(MAKE) CONFIG=valgrind $(PREBUILD) build
	@mkdir -p $(LOG_DIR)
	@$(VALGRIND) $(VALGRIND_ARGS) --quiet --log-file="$(LOG_DIR)/valgrind.log" ./$(BUILD_ROOT)/valgrind/snake \
		|| { echo "error: valgrind found issues (see $(LOG_DIR)/valgrind.log)"; tail -n 80 "$(LOG_DIR)/valgrind.log"; exit 1; }
	@echo "$(OK_MSG)"

gdb:
	@$(MAKE) CONFIG=debug-asan $(PREBUILD) build
	gdb ./$(BUILD_ROOT)/debug-asan/snake

tools-check:
	@command -v $(CLANG_FORMAT) >/dev/null 2>&1 || { echo "error: clang-format not found"; exit 1; }
	@command -v $(CLANG_TIDY) >/dev/null 2>&1 || { echo "error: clang-tidy not found"; exit 1; }

lint: tools-check format-check tidy
	@echo "$(OK_MSG)"

format-check: tools-check
	@$(CLANG_FORMAT) --dry-run --Werror $(FORMAT_FILES) \
		|| { echo "error: formatting check failed (run 'make format')"; exit 1; }

format: tools-check
	@$(CLANG_FORMAT) -i $(FORMAT_FILES)

tidy: tools-check
	@mkdir -p $(LOG_DIR)
	@: > "$(LOG_DIR)/clang-tidy.log"
	@set -e; \
	for f in $(TIDY_FILES); do \
		$(CLANG_TIDY) $(TIDY_EXTRA_ARGS) -checks='$(TIDY_CHECKS)' $$f -- $(CPPFLAGS_BASE) -std=c99 >>"$(LOG_DIR)/clang-tidy.log" 2>&1 \
			|| { echo "error: clang-tidy failed (see $(LOG_DIR)/clang-tidy.log)"; tail -n 80 "$(LOG_DIR)/clang-tidy.log"; exit 1; }; \
	done

$(BIN): $(OBJ)
	@mkdir -p $(dir $@)
	@$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c -o $@ $<

-include $(DEP)

clean:
	@rm -rf $(BUILD_ROOT)
