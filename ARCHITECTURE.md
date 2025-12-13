# Snake Game Architecture (Dec 13, 2025)

Comprehensive overview of the Snake project's design, implementation status, and module structure.

## Project Status

**Completion:** ✅ Core gameplay fully implemented (Steps 0-26)  
**Build Status:** ✅ Compiles cleanly with AddressSanitizer (debug) and release builds  
**Code Quality:** ✅ Passes clang-format and clang-tidy checks  

### Implemented Features
- ✅ Core 2-player game loop with collision detection
- ✅ Local multiplayer (2 players on keyboard)
- ✅ TTY rendering with colors and HUD
- ✅ Non-blocking keyboard input (arrows + P/R/Q)
- ✅ Food spawning, eating, and growth mechanics
- ✅ High-score persistence and configuration management
- ✅ Modular architecture with loose coupling

### Not Implemented (Optional)
- ⏳ Unit testing framework
- ⏳ Network multiplayer (PedroChat)

## Architecture Overview

### Design Principles

1. **Separation of Concerns** - Each module has single responsibility
2. **Loose Coupling** - Modules depend on abstractions, not implementations
3. **Testability** - Core logic can be tested in isolation
4. **Extensibility** - Easy to add new backends and features

### Module Dependency Graph

```
┌──────────────────────────────────────────┐
│              main.c                      │
│   (Orchestrates via clean interfaces)    │
└──────────────────────────────────────────┘
      │           │           │        │
      ▼           ▼           ▼        ▼
   ┌─────┬────────┬──────┬──────────┐
   │game │ render │input │ persist  │
   └─────┴────────┴──────┴──────────┘
        ▲           │
   Query API     ▼
            ┌────────────┐
            │  display   │ (Abstraction Layer)
            └────────────┘
                 │
                 ▼
            ┌────────────┐
            │   tty      │ (Platform Implementation)
            └────────────┘
```

**Legend:**
- Arrow down = "depends on" / "calls"
- Arrow up = "provides query API"
- "Abstraction Layer" = Platform-independent interface

## Detailed Module Documentation

### Core Game (`src/core/`)

**Responsibility:** Game logic, state management, collision detection

**Files:**
- `include/snake/game.h` - GameState and PlayerState structures, main API
- `src/core/game.c` - Core game loop implementation
- `include/snake/collision.h` - Collision detection API
- `src/core/collision.c` - Collision implementation
- `include/snake/types.h` - Shared types (SnakePoint, SnakeDir, GameStatus)

**Key Functions:**
- `game_init()` - Initialize game with dimensions and seed
- `game_tick()` - Main update loop (handles collisions, food, scoring)
- `game_reset()` - Reset game state
- Query functions: `game_get_num_players()`, `game_player_is_active()`, `game_player_current_score()`, etc.

**Coupling:**
- ✅ Decoupled from rendering (no TTY or display includes)
- ✅ Decoupled from input (no keyboard handling)
- ✅ Self-contained state management

**Data Structures:**
```c
typedef struct {
    int width, height;              // Fixed board dimensions
    uint32_t rng_state;            // Deterministic RNG state
    GameStatus status;             // RUNNING/PAUSED/GAME_OVER
    SnakePoint food;               // Food position
    bool food_present;
    PlayerState players[2];        // Max 2 players (can extend)
} GameState;

typedef struct {
    SnakeDir current_dir, queued_dir;  // Direction control
    int score;
    SnakePoint body[1024];             // Ring buffer (max length)
    int length;
    bool active, needs_reset;
} PlayerState;
```

---

### Rendering (`src/render/`)

**Responsibility:** Draw game state to display

**Files:**
- `include/snake/render.h` - Rendering API
- `src/render/render.c` - Core rendering logic
- `include/snake/display.h` - Display abstraction layer (NEW)
- `src/render/display_tty.c` - TTY implementation of display

**Key Functions:**
- `render_init(width, height)` - Initialize rendering subsystem
- `render_draw(game, scores, score_count)` - Draw current frame
- `render_shutdown()` - Clean up resources
- `render_set_glyphs(UTF8|ASCII)` - Select character set

**Architecture:**
The render module uses a **Display Abstraction Layer** to separate rendering logic from platform details:

```
┌────────────────────────────────┐
│  render.c (game state → visual)│
└────────────────────────────────┘
         │
         │ Uses: display_* API
         ▼
┌────────────────────────────────┐
│  display.h (Abstract interface)│
│  (color constants, primitives) │
└────────────────────────────────┘
         │
         │ Implemented by:
         ▼
┌────────────────────────────────┐
│ display_tty.c (Terminal output)│
│ (ANSI colors, box drawing)     │
└────────────────────────────────┘
```

**Benefits:**
- ✅ Can easily add new backends (SDL, framebuffer, web canvas)
- ✅ Render logic is testable without terminal
- ✅ Platform code is isolated

**Coupling:**
- ✅ Takes game state as parameter (no direct access to GameState internals)
- ✅ Receives high scores as parameters (no filesystem calls)
- ✅ Uses display abstraction (no direct TTY calls)
- ✅ Only one persistent include: `persist.h` for HighScore type definition (acceptable)

---

### Input (`src/input/`)

**Responsibility:** Capture and decode raw keyboard input

**Files:**
- `include/snake/input.h` - Input API and InputState structure
- `src/input/input.c` - Keyboard polling implementation

**Key Functions:**
- `input_init()` - Set up non-blocking terminal input
- `input_poll(InputState* out)` - Get current input state
- `input_shutdown()` - Restore terminal

**Architecture (Refactored):**
The input module provides **raw input state**, not game-specific commands:

```
Raw Keyboard Input
      │
      ▼
input_poll() → parses escape sequences, maps keys
      │
      ▼
InputState {
    move_up, move_down, move_left, move_right  // Raw directional input
    restart, pause_toggle, quit                 // Global commands
}
      │
      ▼
main.c → translates to game directions
         (game.players[0].queued_dir = SNAKE_DIR_UP)
```

**Benefits:**
- ✅ Reusable in different game contexts
- ✅ No coupling to game types (SnakeDir)
- ✅ Easy to test without game logic
- ✅ Supports multiple players via same mechanism

**Coupling:**
- ✅ No dependency on game.h or types.h
- ✅ Only uses standard C libraries
- ✅ Completely independent module

---

### Persistence (`src/persist/`)

**Responsibility:** Load/save game state (scores, configuration)

**Files:**
- `include/snake/persist.h` - Persistence API and data structures
- `src/persist/persist.c` - File I/O implementation

**Key Functions:**
- `persist_read_scores(filename, scores[], max_count)` → count read
- `persist_write_scores(filename, scores[], count)` → bool
- `persist_append_score(filename, name, score)` → bool
- `persist_load_config(filename, config)` → bool
- `persist_write_config(filename, config)` → bool

**Data Structures:**
```c
typedef struct {
    char name[32];
    int score;
} HighScore;

typedef struct {
    int board_width, board_height;  // Game dimensions
    int tick_rate_ms;               // Game speed
    int render_glyphs;              // UTF8=0, ASCII=1
    int screen_width, screen_height; // Terminal size hints
} GameConfig;
```

**Features:**
- ✅ Atomic writes (write temp file, then rename)
- ✅ Strict parsing with error handling
- ✅ Graceful handling of corrupted files
- ✅ Sensible defaults if files missing

**Coupling:**
- ✅ Standalone module (only depends on libc)
- ✅ Used by render (for display) and main (for config)

---

### Utilities (`src/utils/`)

**Responsibility:** Shared helper functions

**Modules:**
- `rng.c` - Deterministic pseudo-random number generator
- `bounds.c` - Boundary checking utilities
- `utils.c` - Platform abstraction (timing, terminal size)

**Key Functions:**
- RNG: `rng_seed()`, `rng_next_u32()`, `rng_range(lo, hi)`
- Bounds: `in_bounds(x, y, w, h)` → bool
- Platform: `platform_now_ms()`, `platform_sleep_ms()`

---

### Network (`src/net/`)

**Status:** Stub implementation only (optional feature)

**Files:**
- `include/snake/net.h` - Network API skeleton
- `src/net/net.c` - Empty implementation

**Planned (Not Implemented):**
- PedroChat transport adapter
- JSON message serialization
- Online multiplayer support

---

## Key Design Decisions

### Fixed vs. Dynamic Board Size

**Decision:** Fixed 40×20 board dimensions (set in `include/snake/types.h`)

**Rationale:**
- Simplifies game logic
- Ensures consistent gameplay experience
- Reduces complexity in collision detection

**Fallback:** Terminal size validation in main.c (minimum 80×30 recommended)

### Single-threaded Architecture

**Decision:** Main loop runs in single thread, input polling blocks briefly

**Rationale:**
- Simpler code, fewer concurrency bugs
- Sufficient performance for terminal game
- No need for synchronization primitives

### Deterministic RNG

**Decision:** Custom LCG RNG with explicit seed

**Rationale:**
- Reproducible games (same seed = same food spawns)
- No dependency on stdlib RNG
- Suitable for competition/testing

### Input Handling

**Decision:** Raw directional booleans, not game-specific directions

**Rationale:**
- Input module reusable in different contexts
- Main.c handles game-specific logic
- Cleaner separation of concerns

### Render Architecture

**Decision:** Display Abstraction Layer (display.h + display_tty.c)

**Rationale:**
- Platform independence
- Easy to add new backends
- Render logic testable without terminal

**Alternatives Enabled:**
- `display_sdl.c` - SDL graphics backend
- `display_curses.c` - Curses library backend
- `display_web.c` - WebSocket/canvas (future)

---

## Refactoring History

### Phase 1: Display Abstraction (Completed Dec 13)
- Created `display.h` interface
- Extracted TTY code to `display_tty.c`
- Render now uses abstract API

**Before:** `render.c` → `tty.h` (direct platform dependency)  
**After:** `render.c` → `display.h` → `display_tty.c` → `tty.h`

### Phase 2: Render-Persist Decoupling (Completed Dec 13)
- Changed `render_draw()` signature to accept scores as parameters
- Removed direct filesystem calls from render module

**Before:** `render_draw(game)` → calls `persist_read_scores()`  
**After:** `render_draw(game, scores, count)` (explicit data flow)

### Phase 3: Input Decoupling (Completed Dec 13)
- Removed SnakeDir from InputState
- Returns raw directional booleans instead
- Game logic translates inputs in main.c

**Before:** InputState has `p1_dir: SnakeDir`  
**After:** InputState has `move_up, move_down` booleans

### Phase 4: Game Encapsulation (Completed Dec 13)
- Added query functions to game.h
- Removed direct field access from main.c

**New API:**
```c
int game_get_num_players(const GameState* game);
bool game_player_is_active(const GameState* game, int player_index);
int game_player_current_score(const GameState* game, int player_index);
bool game_player_died_this_tick(const GameState* game, int player_index);
int game_player_score_at_death(const GameState* game, int player_index);
```

---

## Build System

**Tool:** GNU Make (standard Makefile)

**Build Variants:**
- `debug-asan` - Debug + AddressSanitizer (default)
- `release` - Optimized (requires `WERROR=0` for GCC false positive)
- `valgrind` - Debug + valgrind-compatible

**Key Targets:**
```bash
make                        # Build debug (default)
make release               # Build release (WERROR=0)
make clean                 # Remove build artifacts
make gdb                   # Build + launch debugger
make format                # Apply clang-format
make tidy                  # Run clang-tidy
make valgrind              # Run with valgrind
```

**Compilation Flags:**
- `-std=c99` - C99 standard
- `-Wall -Wextra -Wpedantic` - Strict warnings
- `-D_POSIX_C_SOURCE=200809L` - POSIX features

---

## Testing Strategy

**Current Status:** No unit tests yet (Step 27 not implemented)

**Recommended Approach:**
1. Create `tests/` directory
2. Implement minimal test runner (no external frameworks)
3. Write tests for:
   - Snake movement and growth
   - Food spawning validity
   - Collision detection (wall, self, snake-vs-snake)
   - Game reset invariants
   - High score serialization

**Testing Layers:**
- **Unit:** Core game logic (game.c, collision.c) testable without rendering
- **Integration:** Full game loop with mocked display
- **Manual:** Run game and play

---

## Future Enhancements

### Easy to Add (Architecture Already Supports)
1. **Alternative Display Backends** - Implement new `display_*.c` files
2. **Gamepad Support** - Map buttons to move_* booleans in input
3. **AI Players** - Set move_* booleans programmatically
4. **Configurable Board Size** - Pass dimensions to game_init()
5. **Network Multiplayer** - Implement PedroChat adapter

### Moderate Effort
1. **Unit Test Framework** - Steps 27-28
2. **Performance Optimization** - Profile and optimize critical paths
3. **Advanced Collision** - Add physics/sliding mechanics
4. **Visual Effects** - Animations, particles, screenshake

### Higher Effort
1. **Web Version** - Compile to WebAssembly + display_web.c
2. **Multiplayer Lobby** - UI for hosting/joining games
3. **Replay System** - Record and playback games

---

## Code Quality Metrics

**Measured (Dec 13, 2025):**

| Metric | Value |
|--------|-------|
| Total lines of code | ~2,700 |
| Source files | 12 |
| Header files | 11 |
| Compiler warnings | 0 (with WERROR=0 for release) |
| Clang-format compliance | ✅ 100% |
| Clang-tidy issues | ✅ 0 (cleaned up) |
| Test coverage | 0% (no tests yet) |
| Module coupling | ✅ Low (refactored) |

---

## For Next Developers

### Getting Started
1. Clone repository
2. Run `make` to build
3. Run `./snake` to play
4. Check `readme.md` for commands

### Making Changes
1. Follow existing code style (clang-format automatic)
2. Keep modules loosely coupled
3. Avoid circular dependencies
4. Test changes with `make && ./snake`
5. Run `make format` and `make tidy` before committing

### Adding Features
1. **New Display Backend** - Copy `display_tty.c`, implement `display.h` interface
2. **New Game Feature** - Extend game.c/collision.c, update game.h
3. **New Tests** - Add to tests/ directory, update Makefile
4. **Network Play** - Implement net.c (see `design_docs/60_networking.md`)

### Documentation
- **Architecture overview** - This file (ARCHITECTURE.md)
- **Current status** - PROJECT_STATE.md
- **Build/run info** - readme.md
- **Implementation checklist** - design_docs/90_implementation_plan.md
- **Design decisions** - design_docs/*.md (10 files for each subsystem)
- **Coupling analysis** - COUPLING_ANALYSIS.md (shows problems that are now fixed)
- **Refactoring details** - REFACTORING_SUMMARY.md (shows work completed)

---

## Conclusion

The Snake project demonstrates clean C architecture with:
- ✅ Modular design (each module has single responsibility)
- ✅ Loose coupling (modules depend on abstractions)
- ✅ Extensibility (easy to add new features/backends)
- ✅ Code quality (strict compiler flags, formatted, no warnings)

The codebase is well-positioned for:
- Adding unit tests
- Implementing network multiplayer
- Supporting multiple display backends
- Being extended with new game mechanics

For any questions about design decisions, see the comprehensive `design_docs/` folder with detailed notes on each subsystem.

Last updated: **December 13, 2025**
