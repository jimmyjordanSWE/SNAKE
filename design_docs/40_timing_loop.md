# Timing & game loop (`src/platform/` + main)

## Purpose
Keep gameplay smooth and predictable with a fixed tick rate.

## Requirements
- Tick frequency should feel consistent.
- Input direction changes apply on next tick.

## Recommended approach: fixed timestep
- Choose `tick_hz` (e.g., 10–20 Hz).
- Accumulate real time into `accumulator`.
- While `accumulator >= tick_dt`:
  - apply input intents
  - call `game_tick()`
  - decrement accumulator
- Render once per frame (or once per loop) using latest snapshot.

## Benefits
- Deterministic logic independent of render speed.
- Easier testing (tick-based).
