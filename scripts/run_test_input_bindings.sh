#!/usr/bin/env bash
set -euo pipefail
CLANG=$(command -v clang || true)
GCC=$(command -v gcc || true)
PROJECT_ROOT=$(cd "$(dirname "$0")/.." && pwd)
# Use absolute include paths so compilation from a temp dir works reliably
CFLAGS='-std=c99 -g -O0 -fsanitize=address,undefined'
INCS="-I$PROJECT_ROOT/include -I$PROJECT_ROOT/include/snake"
TMPDIR=$(mktemp -d)
pushd "$TMPDIR" >/dev/null
# Build object files first then link, and retry linking a few times to avoid intermittent linker flakes.
OBJ_TEST=test_input_bindings_test.o
OBJ_INPUT=test_input_bindings_input.o
OBJ_PERSIST=test_input_bindings_persist.o
# Prefer clang for compilation, fall back to gcc
if [ -n "$CLANG" ]; then
    "$CLANG" -c $CFLAGS $INCS "$PROJECT_ROOT/tests/test_input_bindings.c" -o "$OBJ_TEST" || true
    "$CLANG" -c $CFLAGS $INCS "$PROJECT_ROOT/src/input/input.c" -o "$OBJ_INPUT" || true
    "$CLANG" -c $CFLAGS $INCS "$PROJECT_ROOT/src/persist/persist.c" -o "$OBJ_PERSIST" || true
fi
if [ ! -f "$OBJ_TEST" ] || [ ! -f "$OBJ_INPUT" ] || [ ! -f "$OBJ_PERSIST" ]; then
    if [ -n "$GCC" ]; then
        "$GCC" -c $CFLAGS $INCS "$PROJECT_ROOT/tests/test_input_bindings.c" -o "$OBJ_TEST"
        "$GCC" -c $CFLAGS $INCS "$PROJECT_ROOT/src/input/input.c" -o "$OBJ_INPUT"
        "$GCC" -c $CFLAGS $INCS "$PROJECT_ROOT/src/persist/persist.c" -o "$OBJ_PERSIST"
    else
        echo "No compiler found" >&2
        popd >/dev/null
        rm -rf "$TMPDIR"
        exit 1
    fi
fi
# Link using GCC (more stable on some systems) with a few retries
MAX_RETRIES=3
COUNT=0
LINK_OK=1
while [ $COUNT -lt $MAX_RETRIES ]; do
    if [ -n "$GCC" ]; then
        "$GCC" $CFLAGS $INCS "$OBJ_TEST" "$OBJ_INPUT" "$OBJ_PERSIST" -o test_input_bindings.out && LINK_OK=0 && break || true
    fi
    COUNT=$((COUNT + 1))
    sleep 0.1
done
if [ ! -x test_input_bindings.out ]; then
    echo "Link failed after $MAX_RETRIES attempts" >&2
    popd >/dev/null
    rm -rf "$TMPDIR"
    exit 1
fi
./test_input_bindings.out
EXIT_CODE=$?
popd >/dev/null
rm -rf "$TMPDIR"
exit $EXIT_CODE
