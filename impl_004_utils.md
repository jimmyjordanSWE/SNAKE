# impl_004: Utilities

**Header:** `include/snake/utils.h`  
**Sources:** `src/utils/rng.c`, `src/utils/bounds.c`, `src/utils/utils.c`  
**Status:** ✅ Complete

---

## Overview

Shared utility functions. See `utils.h` for function signatures.

**Three components:** RNG (deterministic seeding), bounds checking, platform abstraction (timing, terminal size).

---

## RNG Algorithm

**Linear Congruential Generator (LCG):** next = (a × state + c) mod 2^32

Fast, deterministic, full 2^32 period. Quality sufficient for food spawning (not cryptographic).

---

## Design Notes

- **Platform-independent API** - Abstractions hide POSIX-specific calls
- **Minimal dependencies** - Only standard POSIX (no GLIBC extensions)
- **Determinism** - RNG reproducible per seed
- **No state** - Functions are mostly pure (except *_state parameters)

---

## Dependencies

- `types.h` (minimal include)
- POSIX system calls

---

## Called By

- `game.c` - RNG for food spawning
- `input.c` - Timing for input polling
- `main.c` - Timing accumulator, terminal size validation
- Platform/TTY initialization
