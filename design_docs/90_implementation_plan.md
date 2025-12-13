# Implementation plan (iterative + checkable)

This file is meant to be the single “source of truth” for implementation progress. Follow the steps in order. Each step is atomic and should end with a concrete verification.

## Ground rules (to keep LLM work in context)

- Do not implement multiple modules at once unless the step explicitly says so.
- After every step:
  - run `make`
  - ensure no new compiler warnings are introduced
  - update this file by marking the step as done and adding notes if needed
- Keep the **Core Game** module UI-agnostic (no TTY, no keyboard, no sockets).
- Do not change collision rules without updating [design_docs/11_collision_rules.md](11_collision_rules.md).

## Milestones

- M1: Builds cleanly with module skeletons
- M2: Core rules work (unit-testable) + CLI debug output
- M3: Local singleplayer playable in terminal
- M4: Local multiplayer playable
- M5: Persistence (highscores/config)
- M6: Optional network client mode

---

## Step 0 — Repository scaffolding (M1)

1. [x] Add public header layout under `include/snake/` (empty skeletons only)
   - Create:
     - `include/snake/types.h`
     - `include/snake/game.h`
     - `include/snake/input.h`
     - `include/snake/render.h`
     - `include/snake/platform.h`
     - `include/snake/persist.h`
     - `include/snake/net.h`
     - `include/snake/utils.h`
   - Verify: `make` succeeds.

2. [x] Add minimal `.c` files per module with empty functions returning errors
   - Create one `.c` per folder in `src/*/` matching the headers.
   - Verify: `make` succeeds.

3. [x] Update Makefile to compile `src/**/*.c` automatically
   - Keep it simple: use `find` (or a curated list) to collect sources.
   - Verify: `make clean && make` builds the same `snake` binary.

---

## Step 1 — Shared types + utilities (M1)

4. [x] Define shared basic types in `include/snake/types.h`
   - Add:
     - `SnakePoint { int x, y; }`
     - `SnakeDir { UP, DOWN, LEFT, RIGHT }`
     - `GameStatus { RUNNING, PAUSED, GAME_OVER }`
   - Verify: `make`.

5. [x] Implement deterministic RNG utility (`src/utils/rng.c`)
   - API idea:
     - `void rng_seed(uint32_t *state, uint32_t seed)`
     - `uint32_t rng_next_u32(uint32_t *state)`
     - `int rng_range(uint32_t *state, int lo, int hi_inclusive)`
   - Verify: small local test in `main.c` prints stable sequence for seed 123.

6. [x] Implement bounds helpers (`src/utils/bounds.c`)
   - Examples:
     - `bool in_bounds(int x, int y, int w, int h)`
   - Verify: `make`.

---

## Step 2 — Core Game API design + skeleton (M1)

7. [ ] Finalize `GameState` struct (header-only) in `include/snake/game.h`
   - Include:
     - board width/height
     - tick rate (or tick_dt)
     - RNG state
     - food position
     - player states (start with max 2)
   - Verify: `make`.

8. [ ] Implement `game_init()` and `game_reset_player()` (no rendering)
   - Use RNG to pick valid spawn positions.
   - Enforce start length = 2.
   - Verify: run `./snake` and print the spawned coordinates (temporary debug output).

9. [ ] Implement snake body representation (ring buffer) (M2)
   - Decide max snake length (e.g., `board_w * board_h`).
   - Implement:
     - `snake_init()`
     - `snake_head()`
     - `snake_contains(point)` (for collision checks)
     - `snake_step(next_head, grow)`
   - Verify: write a small debug sequence that moves right for N ticks and prints head/tail positions.

---

## Step 3 — Collision rules implementation (M2)

10. [ ] Implement occupancy grid maintenance (recommended)
   - Maintain a grid for O(1) checks.
   - Verify: after init, grid contains exactly the snake cells.

11. [ ] Implement wall + self collision
   - Follow [design_docs/11_collision_rules.md](11_collision_rules.md).
   - Verify: force the snake into wall/self via debug input and observe immediate reset.

12. [ ] Implement snake-vs-snake collision (local multiplayer)
   - Implement deterministic evaluation:
     - compute next heads first
     - resolve collisions using documented ordering
   - Verify: scripted scenario where P1 runs into P2 body; only P1 resets.

13. [ ] Implement head-to-head rule explicitly
   - Choose one rule and document it (likely “both reset”).
   - Verify: scripted scenario with simultaneous head collision reproduces same outcome.

---

## Step 4 — Food rules + scoring (M2)

14. [ ] Implement `food_spawn_valid()` and `food_respawn()`
   - Must never spawn on any snake cell.
   - Verify: run 1000 respawns on small board; never invalid.

15. [ ] Implement eating + growth + score increment
   - If head moves onto food:
     - grow
     - increment score
     - respawn food
   - Verify: scripted move sequence increases length and score.

---

## Step 5 — Input module (M2 → M3)

16. [ ] Implement non-blocking keyboard polling in `src/input/`
   - Minimal API:
     - `input_init()` / `input_shutdown()`
     - `input_poll(InputState *out)`
   - Verify: `./snake` prints key codes without blocking.

17. [ ] Implement per-player key mapping → direction intents
   - P1: arrows or WASD; P2: IJKL (example).
   - Verify: direction intent changes only at next tick.

18. [ ] Add “restart” and “pause” intents
   - `R` for restart, `P` for pause.
   - Verify: pause stops ticks; restart resets state.

---

## Step 6 — Renderer integration (TTY) (M3)

19. [ ] Integrate TTY graphics library into `src/render/`
   - Create `render_init(min_w, min_h)` and `render_shutdown()`.
   - Verify: opening TTY clears screen and restores on exit.

20. [ ] Implement draw of playfield + border
   - Verify: border looks stable; resizing handled gracefully (even if by showing “terminal too small”).

21. [ ] Implement draw of snakes + food + HUD
   - Distinct colors per player.
   - HUD shows status + scores.
   - Verify: visible separation and readable text per spec.

---

## Step 7 — Main loop + timing (M3)

22. [ ] Implement fixed-timestep loop in `main.c` + `src/platform/`
   - `now_ms()` and `sleep_ms()` helpers.
   - Tick at chosen rate, render each frame.
   - Verify: movement speed stable across machines.

23. [ ] Wire together input → core tick → render
   - Verify: local singleplayer fully playable.

24. [ ] Enable local multiplayer (2 players)
   - Verify: collisions between snakes behave as specified.

---

## Step 8 — Persistence (M5)

25. [x] Implement highscore read/write (`src/persist/`)
   - Strict parsing and atomic writes.
   - Verify: scores persist across runs and corrupted file fails safely.

26. [x] Implement config file (optional)
   - Validate ranges.
   - Verify: bad config doesn’t crash; defaults apply.

---

## Step 9 — Testing (continuous; aligns with course) (M2+)

27. [ ] Add a minimal unit-test runner for core logic (no external frameworks)
   - A `tests/` folder or `make test` target.
   - Verify: `make test` runs and returns non-zero on failure.

28. [ ] Add tests for:
   - movement
   - growth
   - food spawn validity
   - wall/self collisions
   - snake-vs-snake collisions
   - reset invariants
   - Verify: tests cover the deterministic head-to-head rule.

---

## Step 10 — Networking mode (optional) (M6)

29. [ ] Implement PedroChat transport adapter (`src/net/`)
   - Follow the provided PedroChat protocol described in [design_docs/60_networking.md](60_networking.md).
   - Implement TCP newline-delimited JSON framing and strict line-length caps.
   - Implement the core commands:
     - `host`, `join`, `list`, `leave`, `game`
   - Verify: `./snake` can (temporarily) connect, list sessions, host, and join without blocking the main loop.

30. [ ] Implement strict JSON schema validation for Snake messages
   - Validate `data.type` and required fields for `input` and `state`.
   - Ignore/drop invalid messages without crashing.
   - Verify: fuzz with malformed JSON and oversized lines; process survives and disconnects cleanly as designed.

31. [ ] Implement online mode (host client authoritative)
   - Host runs `game_tick()` locally and broadcasts `state` snapshots via `cmd:"game"`.
   - Non-host clients send `input` messages to the host (targeted `destination=<hostClientId>`).
   - Verify: remote client can steer its snake and sees consistent snapshots from host.

---

## Documentation checklist (update as you go)

32. [ ] Update docs when behavior changes
   - If collision rules change: update [design_docs/11_collision_rules.md](11_collision_rules.md).
   - If module APIs change: update this file and the module design doc.

33. [ ] Keep a brief changelog section in this file
   - Add a dated note for significant design decisions (e.g., head-to-head rule chosen).
