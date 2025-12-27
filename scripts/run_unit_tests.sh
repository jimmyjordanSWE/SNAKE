#!/bin/bash
set -euo pipefail
OUT_DIR="scripts/out"
mkdir -p "$OUT_DIR"

echo "Building test_persist..."
clang -std=c99 -g -O0 -fsanitize=address,undefined -Iinclude -Iinclude/snake tests/test_persist.c src/persist/persist.c src/utils/validate.c -o test_persist || gcc -std=c99 -g -O0 -fsanitize=address,undefined -Iinclude -Iinclude/snake tests/test_persist.c src/persist/persist.c src/utils/validate.c -o test_persist

echo "Running test_persist under ASAN"
./test_persist | tee "$OUT_DIR/test_persist.txt"

# test_persist_config
echo "Building test_persist_config..."
clang -std=c99 -g -O0 -fsanitize=address,undefined -Iinclude -Iinclude/snake tests/test_persist_config.c src/persist/persist.c src/utils/validate.c -o test_persist_config || gcc -std=c99 -g -O0 -fsanitize=address,undefined -Iinclude -Iinclude/snake tests/test_persist_config.c src/persist/persist.c src/utils/validate.c -o test_persist_config

./test_persist_config | tee "$OUT_DIR/test_persist_config.txt"

# test_persist_long_lines
echo "Building test_persist_long_lines..."
clang -std=c99 -g -O0 -fsanitize=address,undefined -Iinclude -Iinclude/snake tests/test_persist_long_lines.c src/persist/persist.c src/utils/validate.c -o test_persist_long_lines || gcc -std=c99 -g -O0 -fsanitize=address,undefined -Iinclude -Iinclude/snake tests/test_persist_long_lines.c src/persist/persist.c src/utils/validate.c -o test_persist_long_lines

./test_persist_long_lines | tee "$OUT_DIR/test_persist_long_lines.txt"

# test_persist_truncation
echo "Building test_persist_truncation..."
clang -std=c99 -g -O0 -fsanitize=address,undefined -Iinclude -Iinclude/snake tests/test_persist_truncation.c src/persist/persist.c src/utils/validate.c -o test_persist_truncation || gcc -std=c99 -g -O0 -fsanitize=address,undefined -Iinclude -Iinclude/snake tests/test_persist_truncation.c src/persist/persist.c src/utils/validate.c -o test_persist_truncation

./test_persist_truncation | tee "$OUT_DIR/test_persist_truncation.txt"

# test_texture_path
echo "Building test_texture_path..."
clang -std=c99 -g -O0 -fsanitize=address,undefined -D_POSIX_C_SOURCE=200809L -Iinclude -Iinclude/snake -Isrc/vendor tests/test_texture_path.c src/render/texture.c src/vendor/stb_image.c src/utils/validate.c -o test_texture_path -lz -lm || \
    gcc -std=c99 -g -O0 -fsanitize=address,undefined -D_POSIX_C_SOURCE=200809L -Iinclude -Iinclude/snake -Isrc/vendor tests/test_texture_path.c src/render/texture.c src/vendor/stb_image.c src/utils/validate.c -o test_texture_path -lz -lm

./test_texture_path | tee "$OUT_DIR/test_texture_path.txt"

# test_stb_chunk_size_limit
echo "Building test_stb_chunk_size_limit..."
clang -std=c99 -g -O0 -fsanitize=address,undefined -D_POSIX_C_SOURCE=200809L -Iinclude -Iinclude/snake -Isrc/vendor tests/test_stb_chunk_size_limit.c src/vendor/stb_image.c -o test_stb_chunk_size_limit -lz || \
    gcc -std=c99 -g -O0 -fsanitize=address,undefined -D_POSIX_C_SOURCE=200809L -Iinclude -Iinclude/snake -Isrc/vendor tests/test_stb_chunk_size_limit.c src/vendor/stb_image.c -o test_stb_chunk_size_limit -lz

./test_stb_chunk_size_limit | tee "$OUT_DIR/test_stb_chunk_size_limit.txt"

# test_texture_path_extra
echo "Building test_texture_path_extra..."
clang -std=c99 -g -O0 -fsanitize=address,undefined -D_POSIX_C_SOURCE=200809L -Iinclude -Iinclude/snake -Isrc/vendor tests/test_texture_path_extra.c src/render/texture.c src/vendor/stb_image.c src/utils/validate.c -o test_texture_path_extra -lz -lm || \
    gcc -std=c99 -g -O0 -fsanitize=address,undefined -D_POSIX_C_SOURCE=200809L -Iinclude -Iinclude/snake -Isrc/vendor tests/test_texture_path_extra.c src/render/texture.c src/vendor/stb_image.c src/utils/validate.c -o test_texture_path_extra -lz -lm

./test_texture_path_extra | tee "$OUT_DIR/test_texture_path_extra.txt"

# test_tty_path
echo "Building test_tty_path..."
clang -std=c99 -g -O0 -fsanitize=address,undefined -Iinclude -Iinclude/snake tests/test_tty_path.c src/platform/tty.c -o test_tty_path || \
    gcc -std=c99 -g -O0 -fsanitize=address,undefined -Iinclude -Iinclude/snake tests/test_tty_path.c src/platform/tty.c -o test_tty_path

./test_tty_path | tee "$OUT_DIR/test_tty_path.txt"

# test_tty_buffer_cap
echo "Building test_tty_buffer_cap..."
clang -std=c99 -g -O0 -fsanitize=address,undefined -Iinclude -Iinclude/snake tests/test_tty_buffer_cap.c src/platform/tty.c -o test_tty_buffer_cap || \
    gcc -std=c99 -g -O0 -fsanitize=address,undefined -Iinclude -Iinclude/snake tests/test_tty_buffer_cap.c src/platform/tty.c -o test_tty_buffer_cap

./test_tty_buffer_cap | tee "$OUT_DIR/test_tty_buffer_cap.txt"

# test_env
echo "Building test_env..."
clang -std=c99 -g -O0 -fsanitize=address,undefined -Iinclude -Iinclude/snake tests/test_env.c src/utils/env.c -o test_env || \
    gcc -std=c99 -g -O0 -fsanitize=address,undefined -Iinclude -Iinclude/snake tests/test_env.c src/utils/env.c -o test_env

./test_env | tee "$OUT_DIR/test_env.txt"

# test_highscore_name
echo "Building test_highscore_name..."
clang -std=c99 -g -O0 -fsanitize=address,undefined -Iinclude -Iinclude/snake tests/test_highscore_name.c src/render/render_input.c -o test_highscore_name || \
    gcc -std=c99 -g -O0 -fsanitize=address,undefined -Iinclude -Iinclude/snake tests/test_highscore_name.c src/render/render_input.c -o test_highscore_name

./test_highscore_name | tee "$OUT_DIR/test_highscore_name.txt"

# test_net
clang -std=c99 -g -O0 -fsanitize=address,undefined -Iinclude -Iinclude/snake -Isrc/core tests/test_net.c src/net/net.c -o test_net || gcc -std=c99 -g -O0 -fsanitize=address,undefined -Iinclude -Iinclude/snake -Isrc/core tests/test_net.c src/net/net.c -o test_net

./test_net | tee "$OUT_DIR/test_net.txt"

# net integration test (requires -pthread)
clang -std=c99 -g -O0 -fsanitize=address,undefined -Iinclude -Iinclude/snake -Isrc/core tests/test_net_integration.c src/net/net.c -o test_net_integration -lpthread || gcc -std=c99 -g -O0 -fsanitize=address,undefined -Iinclude -Iinclude/snake -Isrc/core tests/test_net_integration.c src/net/net.c -o test_net_integration -lpthread

./test_net_integration | tee "$OUT_DIR/test_net_integration.txt"

# collision tests
clang -std=c99 -g -O0 -fsanitize=address,undefined -Iinclude -Iinclude/snake -Isrc/core tests/test_collision.c src/core/collision.c -o test_collision || gcc -std=c99 -g -O0 -fsanitize=address,undefined -Iinclude -Iinclude/snake -Isrc/core tests/test_collision.c src/core/collision.c -o test_collision

./test_collision | tee "$OUT_DIR/test_collision.txt"