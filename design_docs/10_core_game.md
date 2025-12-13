# Core game engine design (`src/core/`)

## Purpose
Implement all Snake rules independent of UI, input method, and networking.

## Core requirements covered
- Finite board; snakes cannot leave the playfield.
- Tick-based movement; direction changes apply on next tick.
- Food consumption grows snake, increases score, respawns food at random valid position.
- Collision handling:
  - wall
  - self
  - snake vs snake
- Reset-on-death behavior (immediate reset, length back to 2, random valid start).

## Data model
- Board dimensions: width × height (finite).
- Player: score, current direction, queued direction, snake body.
- Snake body representation (recommended): circular buffer of points.
- Occupancy grid for O(1) collision checks.

## Determinism
- Store RNG state in `GameState`.
- All randomness (food spawn, reset spawn) goes through `rng_next(&state)`.
- Optional: accept a seed at new game start for reproducible tests.

## Tick contract
Input:
- Direction intents for each player since last tick.

Steps per tick:
1. Commit queued directions.
2. Compute candidate next head positions.
3. Apply collision resolution (deterministic ordering rule).
4. Move snakes; apply growth if food eaten.
5. Respawn food if consumed.
6. Update status/score.

Output:
- A read-only snapshot sufficient for rendering and network replication.

## Reset behavior
On death (wall/self/losing snake collision):
- Immediately reset snake length to 2.
- Place at new random valid start (in bounds, non-overlapping).
- Reset direction to a valid default.
