SHELL := /bin/bash
.SHELLFLAGS := -eu -o pipefail -c

# Quiet, readable output by default.
MAKEFLAGS += --no-print-directory
# Use all available CPU cores for parallel builds by default (top-level only).
NPROC ?= $(shell nproc)
ifeq ($(MAKELEVEL),0)
MAKEFLAGS += -j$(NPROC)
endif

CC ?= gcc

BUILD_ROOT ?= build
CONFIG ?= debug-asan

# Python used for running analysis scripts (virtualenv)
VENV_PYTHON ?= .venv/bin/python3

# NOTE: GNU make defines a built-in variable named `LINT` (default value: `lint`).
# Use explicit toggles to avoid collisions and keep builds fast.
PREBUILD :=

CPPFLAGS_BASE ?= -Iinclude/snake -Isrc/core -Isrc/render -Isrc/render/internal -Isrc/vendor -D_POSIX_C_SOURCE=200809L $(shell pkg-config --cflags sdl2)
CFLAGS_BASE ?= -std=c99

# Keep code quality high from the start.
WARNINGS ?= \
	-Wall -Wextra -Wpedantic \
	-Wshadow -Wstrict-prototypes -Wmissing-prototypes \
	-Wcast-align -Wwrite-strings \
	-Wformat=2 -Wundef \
	-Wvla \
	-Wconversion -Wsign-conversion \
	-Wno-unused-parameter

# Treat warnings as errors by default (set WERROR=0 to relax).
WERROR ?= 1

ifeq ($(WERROR),1)
WARNINGS += -Werror
endif
LDFLAGS_BASE ?=
LDLIBS ?= $(shell pkg-config --libs sdl2) -lm -lz -ldl

DEBUG_FLAGS ?= -O0 -g3
RELEASE_FLAGS ?= -O3 -DNDEBUG -march=native -flto=$(NPROC) -fdata-sections -ffunction-sections -fno-plt
ASAN_FLAGS ?= -fsanitize=address -fno-omit-frame-pointer

VALGRIND ?= valgrind
VALGRIND_ARGS ?= --leak-check=full --show-leak-kinds=all --track-origins=yes

CLANG_FORMAT ?= clang-format
CLANG_FORMAT_STYLE_LLM ?= .clang-format_LLM
CLANG_FORMAT_STYLE_HUMAN ?= .clang-format
OK_MSG ?= ALL BUILDS OK

FORMAT_FILES = $(SRC) $(HDR)

ifeq ($(CONFIG),debug-asan)
BUILD_DIR := $(BUILD_ROOT)/debug-asan
CPPFLAGS := $(CPPFLAGS_BASE)
	CFLAGS := $(CFLAGS_BASE) $(WARNINGS) $(DEBUG_FLAGS) $(ASAN_FLAGS)
LDFLAGS := $(LDFLAGS_BASE) $(ASAN_FLAGS)
else ifeq ($(CONFIG),release)
BUILD_DIR := $(BUILD_ROOT)/release
CPPFLAGS := $(CPPFLAGS_BASE)
	CFLAGS := $(CFLAGS_BASE) $(WARNINGS) $(RELEASE_FLAGS)
LDFLAGS := $(LDFLAGS_BASE) -flto=$(NPROC) -Wl,--gc-sections
else ifeq ($(CONFIG),valgrind)
BUILD_DIR := $(BUILD_ROOT)/valgrind
CPPFLAGS := $(CPPFLAGS_BASE)
	CFLAGS := $(CFLAGS_BASE) $(WARNINGS) $(DEBUG_FLAGS) -fno-omit-frame-pointer
LDFLAGS := $(LDFLAGS_BASE)
else
$(error Unknown CONFIG '$(CONFIG)'. Use CONFIG=debug-asan|release|valgrind)
endif

OBJ_DIR := $(BUILD_DIR)/obj
BIN := snakegame.out
LOG_DIR := $(BUILD_ROOT)/logs

# Source file scanning
SRC_FILES := $(shell find src -type f -name "*.c" ! -path "src/tools/*")
SRC := $(SRC_FILES) main.c
OBJ := $(patsubst %.c,$(OBJ_DIR)/%.o,$(SRC))
DEP := $(OBJ:.o=.d)

.DEFAULT_GOAL := debug

ifeq ($(MAKELEVEL),0)
MAYBE_ANALYZE := analyze
else
MAYBE_ANALYZE :=
endif

.PHONY: all build debug release valgrind gdb clean format-llm format-human context llvm-context unit-tests test analyze static-analysis run-sanitizers

all: $(MAYBE_ANALYZE) debug

build: $(MAYBE_ANALYZE) $(BIN)

debug: $(MAYBE_ANALYZE)
	@$(MAKE) CONFIG=debug-asan $(PREBUILD) build
	@echo "$(OK_MSG)"

release: $(MAYBE_ANALYZE)
	@$(MAKE) CONFIG=release $(PREBUILD) build
	@echo "$(OK_MSG)"

valgrind:
	@$(MAKE) CONFIG=valgrind $(PREBUILD) build
	@mkdir -p $(LOG_DIR)
	@SDL_VIDEODRIVER=dummy $(VALGRIND) $(VALGRIND_ARGS) --quiet --log-file="$(LOG_DIR)/valgrind.log" ./$(BIN) || true
	@echo "valgrind completed"

gdb:
	@$(MAKE) CONFIG=debug-asan $(PREBUILD) build
	gdb ./$(BIN)

$(BIN): $(OBJ)
	@mkdir -p $(dir $@)
	@$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c -o $@ $<

format-llm:
	@command -v $(CLANG_FORMAT) >/dev/null 2>&1 || { echo "error: clang-format not found"; exit 1; }
	@if [ -z "$(FORMAT_FILES)" ]; then echo "warning: no files to format"; exit 0; fi
	@if [ -f $(CLANG_FORMAT_STYLE_LLM) ]; then \
		$(CLANG_FORMAT) --style=file:$(CLANG_FORMAT_STYLE_LLM) -i $(FORMAT_FILES); \
	else \
		echo "warning: $(CLANG_FORMAT_STYLE_LLM) not found, using Google style"; \
		$(CLANG_FORMAT) --style=Google -i $(FORMAT_FILES); \
	fi

format:
	@command -v $(CLANG_FORMAT) >/dev/null 2>&1 || { echo "error: clang-format not found"; exit 1; }
	@if [ -z "$(FORMAT_FILES)" ]; then echo "warning: no files to format"; exit 0; fi
	@if [ -f $(CLANG_FORMAT_STYLE_HUMAN) ]; then \
		$(CLANG_FORMAT) --style=file:$(CLANG_FORMAT_STYLE_HUMAN) -i $(FORMAT_FILES); \
	else \
		echo "error: $(CLANG_FORMAT_STYLE_HUMAN) not found"; exit 1; \
	fi

analyze:
	@VENV_PYTHON="${VENV_PYTHON}"; SCRIPTS_DIR="scripts"; OUT_DIR="scripts/out"; \
	if [ ! -f "$$VENV_PYTHON" ]; then echo "Error: Virtual environment not found at $$VENV_PYTHON"; exit 1; fi; \
	mkdir -p "$$OUT_DIR"; \
	echo "Refreshing LLM Context (Static Analysis)..."; \
	echo "=== Structure ==="; \
	"$$VENV_PYTHON" "$$SCRIPTS_DIR/structure.py" > "$$OUT_DIR/structure_out.txt"; \
	for script in "memory_map.py" "call_chains.py" "errors.py" "data_flow.py" "hotspots.py" "invariants.py"; do \
		if [ -f "$$SCRIPTS_DIR/$$script" ]; then name=$${script%.py}; echo "- refreshing $$name context..."; "$$VENV_PYTHON" "$$SCRIPTS_DIR/$$script" > "$$OUT_DIR/$${name}_out.txt"; fi; \
	done; \
	echo "=== AST Context ==="; \
	echo -e "\nLLM Context Updated. Results in $$OUT_DIR/";

unit-tests:
	@mkdir -p scripts/out; \
	set -e; \
	echo "Building unified test runner (ASAN)..."; \
	# Build a single unified test binary with all Unity tests and required code. Link with third-party libs as needed.
	$(CC) $(CPPFLAGS) $(CFLAGS) -Iinclude -Isrc -Isrc/vendor -Ivendor/unity tests/unity/*.c vendor/unity/unity.c $(filter-out main.c,$(SRC)) -o test_all.out $(LDLIBS) -lpthread -lz -lm -ldl || \
	($(CC) $(CPPFLAGS) $(CFLAGS) -Iinclude -Isrc -Isrc/vendor -Ivendor/unity tests/unity/*.c vendor/unity/unity.c $(filter-out main.c,$(SRC)) -o test_all.out $(LDLIBS) -lpthread -lz -lm -ldl); \
	# Run the unified test binary and capture logs
	./test_all.out 2>&1 | tee scripts/out/unit-tests.log; \
	echo "unit-tests completed"; \
	exit $${PIPESTATUS[0]};

test: unit-tests
	@echo "All unit tests completed"

.PHONY: bench-tty bench
bench-tty:
	@mkdir -p build
	@$(CC) $(CPPFLAGS) $(CFLAGS) -Iinclude -D_POSIX_C_SOURCE=200809L src/platform/tty.c src/tools/tty_bench.c -o build/tty_bench.out $(LDLIBS) || true
	@echo "built build/tty_bench.out"

bench:
	@mkdir -p build/logs
	@script -q -c "env SDL_VIDEODRIVER=dummy build/tty_bench.out" build/logs/perf_tty_bench_latest.txt || true
	@echo "bench completed: build/logs/perf_tty_bench_latest.txt";

bench-render:
	@mkdir -p build
	@$(CC) $(CPPFLAGS) $(CFLAGS) -Iinclude -Iinclude/snake -Isrc -Isrc/vendor -D_POSIX_C_SOURCE=200809L src/render/raycast.c src/render/projection.c src/render/texture.c src/render/camera.c src/tools/render_bench.c src/vendor/stb_image.c -o build/render_bench.out $(LDLIBS) || true
	@script -q -c "build/render_bench.out" build/logs/perf_render_bench_latest.txt || true
	@echo "bench-render completed: build/logs/perf_render_bench_latest.txt";
bench-texture:
	@mkdir -p build
	@$(CC) $(CPPFLAGS) $(CFLAGS) -Iinclude -D_POSIX_C_SOURCE=200809L src/render/texture.c src/tools/texture_bench.c src/vendor/stb_image.c -o build/texture_bench.out $(LDLIBS) || true
	@script -q -c "build/texture_bench.out" build/logs/perf_texture_bench_latest.txt || true
	@echo "bench-texture completed: build/logs/perf_texture_bench_latest.txt";

bench-sprite:
	@mkdir -p build
	@$(CC) $(CPPFLAGS) $(CFLAGS) -Iinclude -Iinclude/snake -Isrc -Isrc/vendor -D_POSIX_C_SOURCE=200809L src/render/sprite.c src/render/render_3d_sdl.c src/render/camera.c src/render/projection.c src/tools/sprite_bench.c -o build/sprite_bench.out $(LDLIBS) || true
	@script -q -c "env SNAKE_SPRITE_PROFILE=1 build/sprite_bench.out" build/logs/perf_sprite_bench_latest.txt || true
	@echo "bench-sprite completed: build/logs/perf_sprite_bench_latest.txt";
context: llvm-context

llvm-context:
	@./scripts/analyze_all.sh
	@./.venv/bin/python3 ./scripts/structure.py > PROJECT_CONTEXT.txt
	@echo "Context updated."

-include $(DEP)

clean:
	@rm -rf $(BUILD_ROOT) $(BIN)
	@rm -f test_*
	@echo "CLEAN OK"
