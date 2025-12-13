# Project layout & independent work packages

## Goal
Split the project into self-contained parts that can be implemented in parallel with clear ownership and stable interfaces.

## Module split (independent work packages)

### 1) Core game engine (`src/core/`)
**Owner can work without** terminal, keyboard, networking.

Responsibilities:
- Owns the authoritative `GameState` and all rules.
- Advances the game by a fixed-timestep `tick()`.
- Deterministic behavior via explicit RNG state/seed.

Inputs:
- Direction intents per player for this tick.

Outputs:
- Read-only snapshot of positions, food, scores, and status.

Deliverables:
- Pure functions and unit-testable behavior.

### 2) Input (`src/input/`)
**Owner can work without** render/network.

Responsibilities:
- Non-blocking keyboard input.
- Per-player key mapping.
- Converts keypresses into direction intents consumed on next tick.

### 3) Rendering (TTY) (`src/render/`)
**Owner can work without** real game logic.

Responsibilities:
- Draws a snapshot (board, snakes, food, status, scores).
- Uses the TTY library in [design_docs/TTY_graphics.md](TTY_graphics.md).

### 4) Platform/timing (`src/platform/`)
Responsibilities:
- Timekeeping (`now`, sleep) + fixed timestep accumulator.
- Signal/resize handling integration (if needed by TTY).

### 5) Persistence (`src/persist/`)
Responsibilities:
- Highscore + settings/config.
- Safe parsing and atomic writes.

### 6) Networking client (`src/net/`)
Responsibilities:
- Connect to the provided server.
- Encode outgoing inputs, decode incoming state.
- Strict validation of all received data.

### 7) Shared utilities (`src/utils/`)
Responsibilities:
- Small helpers used by multiple modules (bounds checks, RNG, ring-buffer helpers).

## Folder conventions
- `include/`: headers that define cross-module interfaces
- `src/<module>/`: implementation of each module
- `design_docs/`: design contracts (what a module must do + how to integrate)

## Definition of Done (per module)
- Compiles cleanly with `-Wall -Wextra -Wpedantic`
- Inputs/outputs documented in the module’s design doc
- Clear API boundaries (no hidden coupling)
- Manual test checklist for the module
