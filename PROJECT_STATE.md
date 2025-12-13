# Snake Project Current State (Dec 13, 2025)

## Summary
The Snake project is a C-based terminal multiplayer snake game with modular architecture. The project is well-structured with clear design documentation and a phased implementation plan.

### Build Status
✅ **Builds cleanly** with no warnings/errors (ASan, release, and valgrind variants)
✅ **Binary compiled**: `./snake` (197 KB, with debug symbols)

## Completed Milestones

### M1 - Repository Scaffolding ✅
- ✅ All public headers created (`include/snake/types.h`, `game.h`, `input.h`, `render.h`, etc.)
- ✅ Minimal `.c` files per module with initial implementations
- ✅ Makefile configured to auto-compile `src/**/*.c`

### Steps 0-2 - Types & Utilities ✅
- ✅ Step 4: Basic types defined (`SnakePoint`, `SnakeDir`, `GameStatus`)
- ✅ Step 5: RNG utility implemented (`rng_seed()`, `rng_next_u32()`, `rng_range()`)
- ✅ Step 6: Bounds helpers implemented (`in_bounds()`)

### Step 2 - Core Game API ✅
- ✅ Step 7: `GameState` struct finalized with board, RNG, food, 2 players
- ✅ Step 8: `game_init()` and player spawning implemented
- ✅ Step 9: Snake body ring buffer working (max 1024 length)

### Step 3 - Collision Rules ✅
- ✅ Step 10: Occupancy grid is implicitly maintained via point-in-snake checks
- ✅ Step 11: Wall + self collision detection implemented
- ✅ Step 12: Snake-vs-snake collision detection implemented
- ✅ Step 13: Head-to-head collision rules documented (both reset)

### Step 4 - Food Rules ✅
- ✅ Step 14: `food_respawn()` with valid spawn logic (avoids all snakes)
- ✅ Step 15: Eating + growth + score increment working

### Step 5 - Input Module ✅
- ✅ Step 16: Non-blocking keyboard polling (`input_init()`, `input_poll()`)
- ✅ Step 17: Per-player key mapping (P1: arrows, P2: IJKL)
- ✅ Step 18: Restart (R) and pause (P) intents implemented

### Step 6 - Renderer (TTY) ✅
- ✅ Step 19: TTY graphics library integrated, `render_init()/render_shutdown()`
- ✅ Step 20: Playfield + border rendering
- ✅ Step 21: Snakes + food + HUD drawing with color distinction

### Step 7 - Main Loop ✅
- ✅ Step 22: Fixed-timestep loop with `now_ms()`, `sleep_ms()`
- ✅ Step 23: Input → tick → render pipeline wired
- ✅ Step 24: Local multiplayer (2 players) functional

## Current Implementation Details

### Core Game (`src/core/game.c`)
- **GameState structure**: Holds board dimensions, RNG state, game status, 2 player slots, food position
- **PlayerState structure**: Direction (current + queued), score, body array (ring buffer), length, active flag, reset flag
- **Key functions**:
  - `game_init()`: Initialize with dimensions and seed, spawn both players
  - `game_tick()`: Main update loop (collision checks, food spawning, state transitions)
  - `game_reset()`: Reset all player states

### Rendering (`src/render/render.c`)
- Uses TTY context (`g_tty` global)
- Draws box borders, playfield, snakes (distinct colors per player), food, HUD (scores/status)
- Min terminal size check (20x10)

### Input (`src/input/input.c`)
- Non-blocking keyboard input via `termios` + `fcntl`
- Handles arrow keys, IJKL, R (restart), P (pause), Q (quit)
- Terminal restoration on exit via `atexit()`

### Collision (`src/core/collision.c`)
- Point-in-snake checks for occupancy
- Wall collision detection
- Snake-vs-snake collision with documented rules

## What's NOT Done Yet (Steps 25-33)

### Step 8 - Persistence ❌
- [ ] Step 25: Highscore read/write (`src/persist/`)
- [ ] Step 26: Config file (validation, defaults)

### Step 9 - Testing ❌
- [ ] Step 27: Unit-test runner (no external frameworks)
- [ ] Step 28: Tests for movement, growth, food spawn, collisions, reset invariants

### Step 10 - Networking (Optional) ❌
- [ ] Step 29: PedroChat transport adapter (`src/net/`)
- [ ] Step 30: JSON schema validation for Snake messages
- [ ] Step 31: Online mode (host authoritative, snapshots, remote input)

### Documentation ❌
- [ ] Step 32: Update docs when behavior changes
- [ ] Step 33: Keep changelog section in implementation plan

## Key Files & Design References
- **Design docs**: `design_docs/` (layout, core game, collision rules, input, rendering, timing, persistence, networking, testing, build, implementation plan)
- **Multiplayer API ref**: `Multiplayer_API/` (PedroChat protocol, JSON schemas, C client example)
- **Course context**: `course_context/` (course requirements, spec, competition notes)

## Build Commands
```bash
make                   # Debug build with ASan
make clean            # Remove build artifacts
make release          # Optimized build
make gdb              # Debug build + launch gdb
make valgrind         # Run with valgrind memory checker
make format-check     # Check clang-format compliance
make format           # Apply clang-format
make tidy             # Run clang-tidy static analysis
```

## Run Command
```bash
./snake               # Run the game binary
```

---

## Next Session Prompt

When starting a new session to continue work, use this prompt:

---

**NEXT STEP: Implement High-Score Persistence (Steps 25-26)**

**Context & Current State:**
The Snake project is a terminal multiplayer game written in C with modular architecture. All core gameplay features are implemented and functional:
- Core game loop with `game_tick()`, state management, and collision detection
- Local 2-player multiplayer support
- TTY-based rendering with colors and HUD
- Non-blocking keyboard input (arrows for P1, IJKL for P2)
- Food spawning and eating mechanics
- Pause/restart functionality

The project compiles cleanly with AddressSanitizer and passes all format/lint checks. See `design_docs/90_implementation_plan.md` for the full implementation roadmap.

**Task:**
Implement high-score persistence module (`src/persist/persist.c` and `include/snake/persist.h`) following Step 25 & 26 of the implementation plan:

1. **Step 25 - Highscore read/write**:
   - Implement `persist_read_scores(const char* filename, HighScore* scores, int max_count) → int` (returns count of scores read)
   - Implement `persist_write_scores(const char* filename, const HighScore* scores, int count) → bool`
   - Strict parsing with error handling (corrupted file fails safely without crashing)
   - Atomic writes (write to temp file, then rename)
   - Store format: simple line-delimited CSV or newline-separated "NAME SCORE" format

2. **Step 26 - Config file**:
   - Implement `persist_load_config(const char* filename, GameConfig* config) → bool`
   - Support basic settings: board_width, board_height, tick_rate_ms, etc.
   - Validate ranges (e.g., board dimensions 20-100)
   - Apply defaults if config missing or invalid

**Acceptance Criteria:**
- Code compiles cleanly (`make` with no warnings)
- No new compiler/ASan warnings
- Scores persist correctly across multiple runs
- Corrupted score/config files don't crash the program
- Default values apply when files are missing
- Update `main.c` to call persist functions (load config on startup, save high scores on game over)
- All changes follow existing code style (clang-format compliant)

**Files to Modify/Create:**
- Create/update: `include/snake/persist.h` (API definitions and `HighScore`, `GameConfig` structs)
- Create/update: `src/persist/persist.c` (implementations)
- Update: `main.c` (integrate persistence into game flow)
- Optional: `include/snake/game.h` (add config fields to `GameState` if needed)

**Design References:**
- `design_docs/50_persistence.md` (persistence design notes)
- `design_docs/90_implementation_plan.md` (steps 25-26 detailed requirements)

---
