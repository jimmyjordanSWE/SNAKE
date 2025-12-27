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

# test_texture_path
echo "Building test_texture_path..."
clang -std=c99 -g -O0 -fsanitize=address,undefined -D_POSIX_C_SOURCE=200809L -Iinclude -Iinclude/snake -Isrc/vendor tests/test_texture_path.c src/render/texture.c src/vendor/stb_image.c src/utils/validate.c -o test_texture_path -lz -lm || \
    gcc -std=c99 -g -O0 -fsanitize=address,undefined -D_POSIX_C_SOURCE=200809L -Iinclude -Iinclude/snake -Isrc/vendor tests/test_texture_path.c src/render/texture.c src/vendor/stb_image.c src/utils/validate.c -o test_texture_path -lz -lm

./test_texture_path | tee "$OUT_DIR/test_texture_path.txt"

# test_net
clang -std=c99 -g -O0 -fsanitize=address,undefined -Iinclude -Iinclude/snake -Isrc/core tests/test_net.c src/net/net.c -o test_net || gcc -std=c99 -g -O0 -fsanitize=address,undefined -Iinclude -Iinclude/snake -Isrc/core tests/test_net.c src/net/net.c -o test_net

./test_net | tee "$OUT_DIR/test_net.txt"

# net integration test (requires -pthread)
clang -std=c99 -g -O0 -fsanitize=address,undefined -Iinclude -Iinclude/snake -Isrc/core tests/test_net_integration.c src/net/net.c -o test_net_integration -lpthread || gcc -std=c99 -g -O0 -fsanitize=address,undefined -Iinclude -Iinclude/snake -Isrc/core tests/test_net_integration.c src/net/net.c -o test_net_integration -lpthread

./test_net_integration | tee "$OUT_DIR/test_net_integration.txt"

# collision tests
clang -std=c99 -g -O0 -fsanitize=address,undefined -Iinclude -Iinclude/snake -Isrc/core tests/test_collision.c src/core/collision.c -o test_collision || gcc -std=c99 -g -O0 -fsanitize=address,undefined -Iinclude -Iinclude/snake -Isrc/core tests/test_collision.c src/core/collision.c -o test_collision

./test_collision | tee "$OUT_DIR/test_collision.txt"