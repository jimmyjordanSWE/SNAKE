# impl_008: Input Handling

**Header:** `include/snake/input.h`  
**Source:** `src/input/input.c`  
**Status:** ✅ Complete

---

## Overview

Keyboard input polling. See `input.h` for function signatures.

Non-blocking reads, ANSI escape sequence parsing. Returns raw direction booleans (move_up/down/left/right), not game-specific commands. main.c translates inputs to snake directions.---

## Key Mappings

| Input | Bool Field | Type |
|-------|-----------|------|
| ↑↓←→ | move_up/down/left/right | ANSI escape seq |
| P | pause_toggle | ASCII char |
| R | restart | ASCII char |
| Q | quit | ASCII char |

---

## Design

**Raw directions (not game-specific):** Reusable for other games; main.c translates to SnakeDir.

**ANSI escape sequence parsing:** Handles multi-byte sequences, partial reads, non-blocking edge case.

**Non-blocking I/O:** O_NONBLOCK on stdin; read() returns EAGAIN if no data (game loop doesn't stall).

**Latency:** ~10-50ms perceived by player at 100ms tick rate (acceptable for responsiveness).

**Simultaneous keys:** InputState captures multi-key presses in single poll.

---

## Integration

main.c loop translates inputs to game directions:
```
input_poll(&state)
if (state.move_up) game.players[0].queued_dir = SNAKE_DIR_UP;
game_tick()
render_draw()
```

---

## Dependencies

- `platform.h`, `tty.h` for terminal control

---

## Called By

- `main.c` - Every game tick for input polling
