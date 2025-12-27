#!/bin/bash
set -euo pipefail
OUT_DIR="build/fuzz"
mkdir -p "$OUT_DIR"

# libFuzzer style build (clang required)
if command -v clang >/dev/null 2>&1; then
    echo "Building fuzz_net"
    clang -fsanitize=fuzzer,address,undefined -g -O1 -Iinclude -Iinclude/snake -Isrc/core fuzz/fuzz_net.c src/net/net.c -o "$OUT_DIR/fuzz_net"
    echo "Building fuzz_persist"
    clang -fsanitize=fuzzer,address,undefined -g -O1 -Iinclude -Iinclude/snake -Isrc/core fuzz/fuzz_persist.c src/persist/persist.c src/utils/validate.c -o "$OUT_DIR/fuzz_persist"
    echo "Building fuzz_stb_image"
    clang -fsanitize=fuzzer,address,undefined -g -O1 -Iinclude -Iinclude/snake -Isrc/core -Isrc/vendor fuzz/fuzz_stb_image.c src/vendor/stb_image.c -o "$OUT_DIR/fuzz_stb_image" -lz
else
    echo "clang not found; cannot build libFuzzer targets."
    exit 1
fi

echo "Fuzzers built in $OUT_DIR"