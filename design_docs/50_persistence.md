# Persistence (files) (`src/persist/`)

## Purpose
Demonstrate file I/O and safe parsing while adding useful project features.

## Suggested persisted data
- Highscores per player name (or per session)
- Simple config: board size, tick rate, key binds

## Safety requirements
- Validate all parsed data (ranges, string lengths).
- Avoid unsafe functions (`gets`, unchecked `scanf`, unchecked `sprintf`).
- Atomic save pattern:
  - write to temp file
  - `fsync` (optional)
  - rename to target

## Format
- Text format is simplest to inspect and debug.
- Keep parsing strict and fail gracefully.
