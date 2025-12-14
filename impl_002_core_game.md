# impl_002: Core Game Logic

**Headers:** `include/snake/game.h`  
**Source:** `src/core/game.c`  
**Status:** ✅ Complete

---

## Overview

Main game loop, state management, collision handling, food spawning, scoring. Decoupled from rendering, input, and networking.

---

## Architecture

**Game state structures** defined in `game.h` (see header for full Doxygen docs).

- **PlayerState** - Per-player snake, direction, score, collision tracking
- **GameState** - Board, players, food, RNG state, game status

**Ring buffer** in PlayerState.body[] enables efficient movement and growth without reallocating.

---

## Game Loop

**game_tick()** is the main update function: executes direction, moves heads, detects collisions, handles growth/food, updates status.

**Query API** (game_get_*, game_player_*) provides read-only access to state. External code should not access GameState fields directly.

---

## Collision Rules

1. **Wall collision** - Head touches board edge → death
2. **Self collision** - Head touches own body (after growing from food) → death
3. **Snake-vs-snake collision** - Head touches opponent body → both die
4. **Food collision** - Head touches food → length increases, score++

---

## Key Behaviors

- **Deterministic RNG** - Same seed produces identical games
- **Max length capping** - Prevents infinite growth
- **Direction queueing** - Prevents reverse (180°) turns within single tick
- **Pause state** - Freezes movement, score unchanged
- **Multi-player support** - Up to 2 independent snakes, collision interaction

---

## Design Notes

- **Query API** - No direct field access from external code
- **Decoupled** - No includes for render, input, platform, net
- **Stateless main.c** - Game logic self-contained in this module
- **Ring buffer** - Body array circular for efficient growth/movement
- **Food spawning** - Uses internal RNG, retries until valid position found

---

## Dependencies

- `types.h` - SnakePoint, SnakeDir, GameStatus
- `collision.h` - Collision detection functions
- `utils.h` - RNG calls

---

## Called By

- `main.c` - Main game loop orchestration
- `render.c` - Query functions for display
- Tests/validation code
