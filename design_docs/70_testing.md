# Testing strategy

## Goal
Show understanding of testing by validating the most important rules in isolation.

## What to unit test (pure core)
- Movement: head advances, body follows.
- Growth: food consumption increases length.
- Food spawn validity: never overlaps snakes, always in bounds.
- Collisions:
  - wall
  - self
  - snake vs snake
  - head-to-head rule (documented)
- Reset behavior:
  - length reset to 2
  - spawn in bounds
  - no overlap with other snakes

## Manual test checklist (UI)
- Direction changes apply on next tick.
- Game over messaging is clear.
- Restart without exiting works.
- Scores update and are readable.

## Optional
- Add a simple replay mode by logging RNG seed + inputs per tick.
