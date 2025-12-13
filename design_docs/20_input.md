# Input design (`src/input/`)

## Purpose
Provide responsive keyboard control with a clean API to the core game.

## Requirements
- Four direction commands: up/down/left/right.
- Direction change must take effect at next tick.

## Model
- Maintain per-player `current_dir` and `queued_dir`.
- On keypress, update `queued_dir` if valid.

## Validation rules (suggested)
- Ignore invalid keys.
- Optionally disallow immediate reversal (classic Snake):
  - if `queued_dir` would reverse `current_dir`, ignore it.

## API boundary
Input module outputs:
- For each player: latest queued direction intent since last tick.

Input module does not:
- Move the snake
- Detect collisions
- Render
