# impl_001: Types & Constants

**Header:** `include/snake/types.h`  
**Status:** ✅ Complete

---

## Overview

Shared type definitions and constants. Foundation for all other modules.

---

## Type Definitions

### `SnakePoint`
2D coordinate on board.

```c
typedef struct {
    int x, y;
} SnakePoint;
```

- **Usage:** Snake body segments, food position
- **Range:** x ∈ [0, 40), y ∈ [0, 20)

---

### `SnakeDir`
Cardinal direction (enum).

```c
typedef enum {
    SNAKE_DIR_UP,
    SNAKE_DIR_DOWN,
    SNAKE_DIR_LEFT,
    SNAKE_DIR_RIGHT
} SnakeDir;
```

- **Usage:** Snake movement direction per player
- **Invariant:** Each tick, queued direction must not be reverse of current

---

### `GameStatus`
Game state enum.

```c
typedef enum {
    GAME_STATUS_RUNNING,
    GAME_STATUS_PAUSED,
    GAME_STATUS_GAME_OVER
} GameStatus;
```

- **RUNNING:** Normal gameplay
- **PAUSED:** User pressed P, game frozen
- **GAME_OVER:** Snake died (last score saved)

---

## Constants

### Board Dimensions
```c
#define SNAKE_BOARD_WIDTH  40
#define SNAKE_BOARD_HEIGHT 20
```

- **Fixed size** (not configurable at compile-time)
- Terminal must be ≥80×30 for safe display

---

### Snake Properties
```c
#define SNAKE_START_LENGTH    2
#define SNAKE_MAX_LENGTH      (SNAKE_BOARD_WIDTH * SNAKE_BOARD_HEIGHT)
#define SNAKE_DEFAULT_TICK_MS 100
```

- **START_LENGTH:** Initial body size at spawn
- **MAX_LENGTH:** Upper bound for growth
- **TICK_MS:** Default game speed (100ms per tick)

---

### Player Limits
```c
#define SNAKE_MAX_PLAYERS 2
```

- Current implementation: 2 players (local multiplayer)
- Extensible for >2 by increasing array size in GameState

---

## Design Notes

- **No enums for board size** - Keeps memory footprint fixed
- **Minimal types** - Only essentials (coordinates, direction, status)
- **Forward-declared in other headers** - Used by game.h, collision.h, render.h, etc.
- **Platform-independent** - Pure C, no platform includes

---

## Dependencies

None (foundational module)

---

## Used By

- `game.h`, `collision.h`, `render.h`, `input.h`, `persist.h`, `net.h`, `display.h`
- All `.c` files include this header
