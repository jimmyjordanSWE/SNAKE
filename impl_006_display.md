# impl_006: Display Abstraction

**Header:** `include/snake/display.h`  
**Source:** `src/render/display_tty.c`  
**Status:** ✅ Complete

---

## Overview

Abstract display layer separating rendering from platform details. See `display.h` for function signatures.

Single implementation (TTY) provided; extensible for SDL, web, etc.

---

## Architecture

```
render.c ────────────────→ display.h (abstract API)
                              │
                              └──→ display_tty.c (TTY implementation)
                                     └──→ tty.h (terminal control)
```

Benefit: render.c is platform-agnostic; new backends only implement display.h interface.

---

## Display Interface
