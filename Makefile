SHELL := /bin/bash
.SHELLFLAGS := -eu -o pipefail -c

# Quiet, readable output by default.
MAKEFLAGS += --no-print-directory

CC ?= gcc

BUILD_ROOT ?= build
CONFIG ?= debug-asan

# NOTE: GNU make defines a built-in variable named `LINT` (default value: `lint`).
# Use explicit toggles to avoid collisions and keep builds fast.
PREBUILD :=

CPPFLAGS_BASE ?= -Iinclude -D_POSIX_C_SOURCE=200809L $(shell pkg-config --cflags sdl2)
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
LDLIBS ?= $(shell pkg-config --libs sdl2) -lm -lz

DEBUG_FLAGS ?= -O0 -g3
RELEASE_FLAGS ?= -O3 -DNDEBUG
ASAN_FLAGS ?= -fsanitize=address -fno-omit-frame-pointer

VALGRIND ?= valgrind
VALGRIND_ARGS ?= --leak-check=full --show-leak-kinds=all --track-origins=yes --error-exitcode=1

CLANG_FORMAT ?= clang-format
CLANG_FORMAT_STYLE_LLM ?= .clang-format_LLM
CLANG_FORMAT_STYLE_HUMAN ?= .clang-format
OK_MSG ?= all OK

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
BIN := snake
BIN_3D := snake_3d
LOG_DIR := $(BUILD_ROOT)/logs

# Include all source files including 3D rendering
SRC_ALL := $(shell find src -name '*.c' -print)
SRC := $(SRC_ALL) main.c
SRC_3D := $(shell find src -name '*.c' -print) main_3d.c
HDR := $(shell find include -name '*.h' -print)
OBJ := $(patsubst %.c,$(OBJ_DIR)/%.o,$(SRC))
OBJ_3D := $(patsubst %.c,$(OBJ_DIR)/%.o,$(SRC_3D))
DEP := $(OBJ:.o=.d)
DEP_3D := $(OBJ_3D:.o=.d)

.DEFAULT_GOAL := debug

.PHONY: all build debug release valgrind gdb clean test-3d format-llm format-human

all: debug

build: $(BIN)

debug:
	@$(MAKE) CONFIG=debug-asan $(PREBUILD) build
	@echo "$(OK_MSG)"

release:
	@$(MAKE) CONFIG=release $(PREBUILD) build
	@echo "$(OK_MSG)"

valgrind:
	@$(MAKE) CONFIG=valgrind $(PREBUILD) build
	@mkdir -p $(LOG_DIR)
	@$(VALGRIND) $(VALGRIND_ARGS) --quiet --log-file="$(LOG_DIR)/valgrind.log" ./snake \
		|| { echo "error: valgrind found issues (see $(LOG_DIR)/valgrind.log)"; tail -n 80 "$(LOG_DIR)/valgrind.log"; exit 1; }
	@echo "$(OK_MSG)"

gdb:
	@$(MAKE) CONFIG=debug-asan $(PREBUILD) build
	gdb ./snake

test-3d:
	@$(MAKE) CONFIG=debug-asan $(PREBUILD) $(BIN_3D)
	@echo "$(OK_MSG)"
	./$(BIN_3D)

test-sprite:
	@$(CC) $(CPPFLAGS) $(CFLAGS) -o test_sprite tests/test_sprite.c src/render/sprite.c src/render/camera.c src/render/projection.c src/render/render_3d_sdl.c $(LDLIBS)
	@echo "$(OK_MSG)"
	./test_sprite


test-input:
	@$(CC) $(CPPFLAGS) $(CFLAGS) -o test_input tests/test_input.c src/input/input.c $(LDLIBS)
	@echo "$(OK_MSG)"
	./test_input


test-collision:
	@$(CC) $(CPPFLAGS) $(CFLAGS) -o test_collision_tail tests/collision/test_tail_vacate.c src/core/collision.c src/core/game.c src/utils/rng.c $(LDLIBS)
	@$(CC) $(CPPFLAGS) $(CFLAGS) -o test_collision_swap tests/collision/test_head_swap.c src/core/collision.c src/core/game.c src/utils/rng.c $(LDLIBS)
	@echo "$(OK_MSG)"
	./test_collision_tail && ./test_collision_swap

test-game:
	@$(CC) $(CPPFLAGS) $(CFLAGS) -o test_game_over tests/game/test_game_over.c src/core/game.c src/core/collision.c src/utils/rng.c $(LDLIBS)
	@echo "$(OK_MSG)"
	./test_game_over


test-utils:
	@$(CC) $(CPPFLAGS) $(CFLAGS) -o test_rng tests/utils/test_rng.c src/utils/rng.c $(LDLIBS)
	@$(CC) $(CPPFLAGS) $(CFLAGS) -o test_bounds tests/utils/test_bounds.c src/utils/bounds.c $(LDLIBS)
	@echo "$(OK_MSG)"
	./test_rng && ./test_bounds

test-persist:
	@$(CC) $(CPPFLAGS) $(CFLAGS) -o test_persist tests/persist/test_persist_io.c src/persist/persist.c $(LDLIBS)
	@echo "$(OK_MSG)"
	./test_persist

test-net:
	@$(CC) $(CPPFLAGS) $(CFLAGS) -o test_net_pack tests/net/test_net_pack.c src/net/net.c src/core/game.c src/core/collision.c src/utils/rng.c $(LDLIBS)
	@echo "$(OK_MSG)"
	./test_net_pack

test-tty:
	@$(CC) $(CPPFLAGS) $(CFLAGS) -o test_tty tests/platform/test_tty.c src/platform/tty.c $(LDLIBS)
	@echo "$(OK_MSG)"
	./test_tty

test-render:
	@$(CC) $(CPPFLAGS) $(CFLAGS) -o test_camera_orient tests/render/test_camera_orient.c src/render/camera.c src/render/projection.c src/render/sprite.c src/render/render_3d_sdl.c $(LDLIBS) || true
	@$(CC) $(CPPFLAGS) $(CFLAGS) -o test_camera_interp tests/render/test_camera_interp.c src/render/camera.c $(LDLIBS) || true
	@$(CC) $(CPPFLAGS) $(CFLAGS) -o test_camera_world_transform tests/render/test_camera_world_transform.c src/render/camera.c $(LDLIBS) || true
	@$(CC) $(CPPFLAGS) $(CFLAGS) -o test_camera_angles tests/render/test_camera_angles.c src/render/camera.c $(LDLIBS) || true
	@$(CC) $(CPPFLAGS) $(CFLAGS) -o test_texture tests/render/test_texture.c src/render/texture.c src/vendor/stb_image.c src/render/raycast.c $(LDLIBS) || true
	@$(CC) $(CPPFLAGS) $(CFLAGS) -o test_texture_file tests/render/test_texture_file.c src/render/texture.c src/vendor/stb_image.c $(LDLIBS) || true
	@$(CC) $(CPPFLAGS) $(CFLAGS) -o test_texture_search tests/render/test_texture_search.c src/render/texture.c src/vendor/stb_image.c $(LDLIBS) || true
	@$(CC) $(CPPFLAGS) $(CFLAGS) -o test_texture_assets tests/render/test_texture_assets.c src/render/texture.c src/vendor/stb_image.c $(LDLIBS) || true
	@echo "$(OK_MSG)"
	./test_camera_orient || true
	./test_camera_interp || true
	./test_camera_world_transform || true
	./test_camera_angles || true
	./test_texture || true
	./test_texture_file || true
	./test_texture_search || true
	./test_texture_assets
.PHONY: test all build debug release valgrind gdb clean test-3d format-llm format-human
test: test-input test-collision test-game test-utils test-persist test-net test-tty

$(BIN): $(OBJ)
	@mkdir -p $(dir $@)
	@$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(BIN_3D): $(OBJ_3D)
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

-include $(DEP)
-include $(DEP_3D)

clean:
	@rm -rf $(BUILD_ROOT) $(BIN)
