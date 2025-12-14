# impl_007: Rendering

**Header:** `include/snake/render.h`  
**Source:** `src/render/render.c`  
**Status:** ✅ Complete

---

## Overview

High-level rendering. Converts GameState snapshot to visual representation. See `render.h` for function signatures.

Uses display abstraction (display.h); no direct TTY calls.

---

## Rendering Flow

1. **Initialization:** render_init() sets up display
2. **Glyph selection:** render_set_glyphs() chooses UTF8 or ASCII
3. **Frame render:** render_draw() draws entire scene (board, HUD, scores)
4. **Shutdown:** render_shutdown() cleans up

---

## Board Layout & Styling

**Board:** 40×20 game area, bordered, centered on screen.

**Snake:** Head distinctive char, body standard char; color per player (P1 green, P2 red).

**Food:** Single char per food item, contrasting color.

**HUD:** Scores, status (RUNNING/PAUSED/GAME_OVER), high score table, control hints.

---

## Design

**Stateless:** render_draw() deterministic for same inputs; no side effects.

**Decoupled:** 
- Takes GameState as parameter (immutable snapshot via query API)
- High scores passed as parameters (decoupled from persist)
- No direct TTY; uses display.h abstraction
- Fully buffered frame

**Glyph modes:** UTF8 (box-drawing) vs. ASCII for compatibility.

**Full redraw:** Every frame clears+redraws (acceptable at 100ms tick, no flicker).

---

## Dependencies

- `types.h`, `game.h`, `persist.h`, `display.h`

---

## Called By

- `main.c` - Every game tick after game_tick()
