# impl_005: Platform & TTY

**Headers:** `include/snake/platform.h`, `include/snake/tty.h`  
**Sources:** `src/platform/platform.c`, `src/platform/tty.c`  
**Status:** ✅ Complete

---

## Overview

Platform abstraction and TTY control. See `platform.h`, `tty.h` for function signatures.

Handles terminal mode switching, cursor management, ANSI color codes. Decouples game from platform-specific I/O.

## Design Principles

**Raw mode vs. Canonical:** Game requires raw mode (character-by-character, no echo, no line buffering) for non-blocking arrow keys.

**Non-blocking I/O:** O_NONBLOCK flag allows game loop to continue even if no input pending.

**ANSI escape codes:** Standard sequences for cursor, color, formatting (`\x1b[...m` patterns).

**Graceful degradation:** Functions return bool; missing capabilities don't crash (e.g., unsupported color). Terminal size validation in main.c.
