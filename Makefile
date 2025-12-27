SHELL := /bin/bash
.SHELLFLAGS := -eu -o pipefail -c

# Quiet, readable output by default.
MAKEFLAGS += --no-print-directory
# Use all available CPU cores for parallel builds by default (top-level only).
ifeq ($(MAKELEVEL),0)
NPROC := $(shell nproc)
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
RELEASE_FLAGS ?= -O3 -DNDEBUG
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
BIN := snakegame.out
LOG_DIR := $(BUILD_ROOT)/logs

# Source file scanning
SRC_FILES := $(shell find src -type f -name "*.c")
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
	echo "Running unit tests under ASAN..."; \
	# test_persist
	clang -std=c99 -g -O0 -fsanitize=address,undefined -Iinclude -Iinclude/snake tests/test_persist.c src/persist/persist.c src/utils/validate.c -o test_persist.out || gcc -std=c99 -g -O0 -fsanitize=address,undefined -Iinclude -Iinclude/snake tests/test_persist.c src/persist/persist.c src/utils/validate.c -o test_persist.out; \
	./test_persist.out; \
	# test_persist_config
	clang -std=c99 -g -O0 -fsanitize=address,undefined -Iinclude -Iinclude/snake tests/test_persist_config.c src/persist/persist.c src/utils/validate.c -o test_persist_config.out || gcc -std=c99 -g -O0 -fsanitize=address,undefined -Iinclude -Iinclude/snake tests/test_persist_config.c src/persist/persist.c src/utils/validate.c -o test_persist_config.out; \
	./test_persist_config.out; \
	# test_persist_long_lines
	clang -std=c99 -g -O0 -fsanitize=address,undefined -Iinclude -Iinclude/snake tests/test_persist_long_lines.c src/persist/persist.c src/utils/validate.c -o test_persist_long_lines.out || gcc -std=c99 -g -O0 -fsanitize=address,undefined -Iinclude -Iinclude/snake tests/test_persist_long_lines.c src/persist/persist.c src/utils/validate.c -o test_persist_long_lines.out; \
	./test_persist_long_lines.out; \
	# test_persist_truncation
	clang -std=c99 -g -O0 -fsanitize=address,undefined -Iinclude -Iinclude/snake tests/test_persist_truncation.c src/persist/persist.c src/utils/validate.c -o test_persist_truncation.out || gcc -std=c99 -g -O0 -fsanitize=address,undefined -Iinclude -Iinclude/snake tests/test_persist_truncation.c src/persist/persist.c src/utils/validate.c -o test_persist_truncation.out; \
	./test_persist_truncation.out; \
	# test_texture_path
	clang -std=c99 -g -O0 -fsanitize=address,undefined -D_POSIX_C_SOURCE=200809L -Iinclude -Iinclude/snake -Isrc/vendor tests/test_texture_path.c src/render/texture.c src/vendor/stb_image.c src/utils/validate.c -o test_texture_path.out -lz -lm || \
	gcc -std=c99 -g -O0 -fsanitize=address,undefined -D_POSIX_C_SOURCE=200809L -Iinclude -Iinclude/snake -Isrc/vendor tests/test_texture_path.c src/render/texture.c src/vendor/stb_image.c src/utils/validate.c -o test_texture_path.out -lz -lm; \
	./test_texture_path.out; \
	# test_stb_chunk_size_limit
	clang -std=c99 -g -O0 -fsanitize=address,undefined -D_POSIX_C_SOURCE=200809L -Iinclude -Iinclude/snake -Isrc/vendor tests/test_stb_chunk_size_limit.c src/vendor/stb_image.c -o test_stb_chunk_size_limit.out -lz || \
	gcc -std=c99 -g -O0 -fsanitize=address,undefined -D_POSIX_C_SOURCE=200809L -Iinclude -Iinclude/snake -Isrc/vendor tests/test_stb_chunk_size_limit.c src/vendor/stb_image.c -o test_stb_chunk_size_limit.out -lz; \
	./test_stb_chunk_size_limit.out; \
	# test_texture_path_extra
	clang -std=c99 -g -O0 -fsanitize=address,undefined -D_POSIX_C_SOURCE=200809L -Iinclude -Iinclude/snake -Isrc/vendor tests/test_texture_path_extra.c src/render/texture.c src/vendor/stb_image.c src/utils/validate.c -o test_texture_path_extra.out -lz -lm || \
	gcc -std=c99 -g -O0 -fsanitize=address,undefined -D_POSIX_C_SOURCE=200809L -Iinclude -Iinclude/snake -Isrc/vendor tests/test_texture_path_extra.c src/render/texture.c src/vendor/stb_image.c src/utils/validate.c -o test_texture_path_extra.out -lz -lm; \
	./test_texture_path_extra.out; \
	# test_tty_path
	clang -std=c99 -g -O0 -fsanitize=address,undefined -Iinclude -Iinclude/snake tests/test_tty_path.c src/platform/tty.c -o test_tty_path.out || \
	gcc -std=c99 -g -O0 -fsanitize=address,undefined -Iinclude -Iinclude/snake tests/test_tty_path.c src/platform/tty.c -o test_tty_path.out; \
	./test_tty_path.out; \
	# test_tty_buffer_cap
	clang -std=c99 -g -O0 -fsanitize=address,undefined -Iinclude -Iinclude/snake tests/test_tty_buffer_cap.c src/platform/tty.c -o test_tty_buffer_cap.out || \
	gcc -std=c99 -g -O0 -fsanitize=address,undefined -Iinclude -Iinclude/snake tests/test_tty_buffer_cap.c src/platform/tty.c -o test_tty_buffer_cap.out; \
	./test_tty_buffer_cap.out; \
	# test_env
	clang -std=c99 -g -O0 -fsanitize=address,undefined -Iinclude -Iinclude/snake tests/test_env.c src/utils/env.c -o test_env.out || \
	gcc -std=c99 -g -O0 -fsanitize=address,undefined -Iinclude -Iinclude/snake tests/test_env.c src/utils/env.c -o test_env.out; \
	./test_env.out; \
	# test_highscore_name
	clang -std=c99 -g -O0 -fsanitize=address,undefined -Iinclude -Iinclude/snake tests/test_highscore_name.c src/render/render_input.c -o test_highscore_name.out || \
	gcc -std=c99 -g -O0 -fsanitize=address,undefined -Iinclude -Iinclude/snake tests/test_highscore_name.c src/render/render_input.c -o test_highscore_name.out; \
	./test_highscore_name.out; \
	# test_net
	clang -std=c99 -g -O0 -fsanitize=address,undefined -Iinclude -Iinclude/snake -Isrc/core tests/test_net.c src/net/net.c -o test_net.out || gcc -std=c99 -g -O0 -fsanitize=address,undefined -Iinclude -Iinclude/snake -Isrc/core tests/test_net.c src/net/net.c -o test_net.out; \
	./test_net.out; \
	# net integration test (requires -pthread)
	clang -std=c99 -g -O0 -fsanitize=address,undefined -Iinclude -Iinclude/snake -Isrc/core tests/test_net_integration.c src/net/net.c -o test_net_integration.out -lpthread || gcc -std=c99 -g -O0 -fsanitize=address,undefined -Iinclude -Iinclude/snake -Isrc/core tests/test_net_integration.c src/net/net.c -o test_net_integration.out -lpthread; \
	./test_net_integration.out; \
	# collision tests
	clang -std=c99 -g -O0 -fsanitize=address,undefined -Iinclude -Iinclude/snake -Isrc/core tests/test_collision.c src/core/collision.c -o test_collision.out || gcc -std=c99 -g -O0 -fsanitize=address,undefined -Iinclude -Iinclude/snake -Isrc/core tests/test_collision.c src/core/collision.c -o test_collision.out; \
	./test_collision.out; \
	# test_game_oom (simulate allocation failures)
	echo "Building test_game_oom..."; \
	clang -std=c99 -g -O0 -fsanitize=address,undefined -Iinclude -Iinclude/snake tests/test_game_oom.c src/core/game.c src/core/player.c src/core/collision.c src/utils/rng.c src/utils/direction.c src/persist/persist.c -o test_game_oom.out || \
    gcc -std=c99 -g -O0 -fsanitize=address,undefined -Iinclude -Iinclude/snake tests/test_game_oom.c src/core/game.c src/core/player.c src/core/collision.c src/utils/rng.c src/utils/direction.c src/persist/persist.c -o test_game_oom.out; \
	./test_game_oom.out; \
	# test_tty_open
	clang -std=c99 -g -O0 -fsanitize=address,undefined -Iinclude -Iinclude/snake tests/test_tty_open.c src/platform/tty.c -o test_tty_open.out || \
	gcc -std=c99 -g -O0 -fsanitize=address,undefined -Iinclude -Iinclude/snake tests/test_tty_open.c src/platform/tty.c -o test_tty_open.out; \
	./test_tty_open.out; \
	# test_net_unpack
	clang -std=c99 -g -O0 -fsanitize=address,undefined -Iinclude -Iinclude/snake -Isrc/core tests/test_net_unpack.c src/net/net.c -o test_net_unpack.out || \
    gcc -std=c99 -g -O0 -fsanitize=address,undefined -Iinclude -Iinclude/snake -Isrc/core tests/test_net_unpack.c src/net/net.c -o test_net_unpack.out; \
	./test_net_unpack.out; \
	# test_input_multi
	clang -std=c99 -g -O0 -fsanitize=address,undefined -Iinclude -Iinclude/snake tests/test_input_multi.c src/input/input.c -o test_input_multi.out || \
	gcc -std=c99 -g -O0 -fsanitize=address,undefined -Iinclude -Iinclude/snake tests/test_input_multi.c src/input/input.c -o test_input_multi.out; \
	./test_input_multi.out; \
	# test_game_multi
	clang -std=c99 -g -O0 -fsanitize=address,undefined -Iinclude -Iinclude/snake -Isrc/core tests/test_game_multi.c src/core/game.c src/core/player.c src/core/collision.c src/utils/rng.c src/utils/direction.c src/persist/persist.c -o test_game_multi.out || \
	gcc -std=c99 -g -O0 -fsanitize=address,undefined -Iinclude -Iinclude/snake -Isrc/core tests/test_game_multi.c src/core/game.c src/core/player.c src/core/collision.c src/utils/rng.c src/utils/direction.c src/persist/persist.c -o test_game_multi.out; \
	./test_game_multi.out; \
	echo "unit-tests completed"; \
	echo "OK"; \
	exit 0;

test: unit-tests
	@echo "All unit tests completed";

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
