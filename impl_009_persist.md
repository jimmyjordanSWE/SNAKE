# impl_009: Persistence

**Header:** `include/snake/persist.h`  
**Source:** `src/persist/persist.c`  
**Status:** ✅ Complete

---

## Overview

High score and game configuration file I/O. See `persist.h` for function signatures.

Atomic writes (temp file + rename), graceful error handling, sensible defaults.

---

## File Formats

**Scores:** Plain text, one entry per line: `name:score`

**Config:** Key=value pairs, one per line. Unknown keys ignored, malformed lines skipped.

**Storage:** Caller (main.c) provides paths (typically `~/.snake/scores.txt`, `~/.snake/config.txt`)

---

## Design

**Atomic writes:** Write temp file, fsync(), rename() to final (prevents corruption on crash/power loss).

**Graceful degradation:** Missing file → use defaults. Corrupted line → skip. I/O error → return false.

**No locking:** Assumes single-player instance at a time.

**Extensible format:** Easy to add new config keys.

---

## Dependencies

- POSIX I/O (stdio, unistd)

---

## Called By

- `main.c` - Load config at startup, save scores at shutdown
