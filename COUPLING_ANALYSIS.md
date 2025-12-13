# Snake Game - Coupling Analysis and Refactoring Plan

## Executive Summary

This document analyzes the current coupling issues in the Snake game codebase and proposes a refactored architecture to maximize module independence and minimize coupling.

## Current Architecture Analysis

### Module Overview

The codebase is organized into the following modules:

1. **core/** - game logic (game.c, collision.c)
2. **input/** - input handling (input.c)
3. **net/** - networking (net.c) - currently stub
4. **persist/** - persistence (persist.c)
5. **platform/** - platform abstraction (platform.c, tty.c)
6. **render/** - rendering (render.c)
7. **utils/** - utility functions (utils.c, bounds.c, rng.c)

### Current Dependency Graph

```
main.c
├── game.h
├── input.h
├── persist.h
├── platform.h
├── render.h
└── types.h

game.c
├── game.h
├── collision.h
└── utils.h

collision.c
└── collision.h

render.c
├── render.h
├── persist.h (COUPLING ISSUE!)
└── tty.h

input.c
└── input.h

persist.c
└── persist.h

net.c
└── net.h

platform.c
└── platform.h

tty.c
└── tty.h
```

## Identified Coupling Issues

### 1. **CRITICAL: Render Module Depends on Persist Module**

**Location:** [src/render/render.c](src/render/render.c#L2)

**Problem:** The render module directly includes and uses `persist.h` to read high scores during rendering.

```c
#include "snake/persist.h"
...
int score_count = persist_read_scores(".snake_scores", highscores, PERSIST_MAX_SCORES);
```

**Impact:**
- Violates separation of concerns
- Render module should only be responsible for drawing, not data access
- Makes render untestable without filesystem access
- Creates tight coupling between rendering and persistence

**Root Cause:** The render module is doing too much - it's both drawing AND fetching data.

---

### 2. **CRITICAL: Render Module Directly Depends on TTY (Platform-Specific Code)**

**Location:** [src/render/render.c](src/render/render.c#L3)

**Problem:** The render module directly includes `tty.h` and uses TTY-specific types and functions throughout.

```c
#include "snake/tty.h"
static tty_context* g_tty = NULL;
```

**Impact:**
- Render module is tied to terminal-based rendering
- Cannot easily add alternative rendering backends (e.g., SDL, web canvas)
- Platform-specific code leaks into higher-level module

**Root Cause:** Missing abstraction layer between rendering logic and platform-specific output.

---

### 3. **MODERATE: Main.c Has Too Many Direct Dependencies**

**Location:** [main.c](main.c)

**Problem:** main.c directly orchestrates everything:
- Terminal size checking (platform-specific)
- Game loop timing
- Score persistence
- Input polling
- Rendering

**Impact:**
- Game loop logic is not reusable
- Hard to test the main game flow
- Mixing high-level control flow with low-level details

**Root Cause:** No game controller or application layer to coordinate modules.

---

### 4. **MODERATE: Input Module Uses Types from game.h**

**Location:** [include/snake/input.h](include/snake/input.h)

**Problem:** InputState structure includes `SnakeDir` type from types.h, creating a dependency.

```c
#include "snake/types.h"

typedef struct {
    ...
    SnakeDir p1_dir;
    bool p1_dir_set;
} InputState;
```

**Impact:**
- Input module knows about game-specific types
- Input module cannot be reused in different contexts
- Coupling between input and game types

**Root Cause:** Input module is translating inputs into game-specific directions rather than providing raw input state.

---

### 5. **MODERATE: Net Module Exposes Game and Input Types**

**Location:** [include/snake/net.h](include/snake/net.h)

**Problem:** Network API directly uses GameState and InputState types.

```c
#include "snake/game.h"
#include "snake/input.h"

bool net_send_input(NetClient* client, const InputState* input);
bool net_recv_state(NetClient* client, GameState* out_game);
```

**Impact:**
- Network module is tightly coupled to game implementation
- Cannot change game state structure without affecting network protocol
- Makes network module difficult to test independently

**Root Cause:** Network layer is working at the wrong abstraction level - should work with serializable DTOs, not internal game state.

---

### 6. **MINOR: Collision Module Depends on Full game.h**

**Location:** [include/snake/collision.h](include/snake/collision.h)

**Problem:** collision.h includes full game.h for GameState and PlayerState.

```c
#include "snake/game.h"
```

**Impact:**
- Collision logic coupled to full game state structure
- Cannot test collision in isolation easily

**Root Cause:** Collision functions take full GameState instead of minimal required data.

---

### 7. **MINOR: Main.c Accesses Internal GameState Fields Directly**

**Location:** [main.c](main.c)

**Problem:** main.c directly accesses internal game state fields:

```c
for (int i = 0; i < game.num_players; i++) {
    if (game.players[i].died_this_tick && game.players[i].score_at_death > 0) {
        ...
    }
}
```

**Impact:**
- Breaks encapsulation
- Game module cannot change internal representation
- Logic about game state is scattered outside the game module

**Root Cause:** Missing query functions in game.h to check player state.

---

## Proposed Refactored Architecture

### Design Principles

1. **Dependency Inversion:** High-level modules should not depend on low-level modules. Both should depend on abstractions.
2. **Single Responsibility:** Each module should have one clear responsibility.
3. **Interface Segregation:** Modules should expose minimal, focused APIs.
4. **Explicit Data Flow:** Make data ownership and flow clear through function parameters.
5. **Testability:** Modules should be testable in isolation with minimal setup.

### New Module Structure and Dependencies

```
┌─────────────────────────────────────────────────────────────┐
│                         main.c                               │
│                    (Application Layer)                       │
└─────────────────────────────────────────────────────────────┘
                            │
        ┌───────────────────┼───────────────────┐
        │                   │                   │
        ▼                   ▼                   ▼
┌───────────────┐  ┌───────────────┐  ┌───────────────┐
│  Game Logic   │  │  UI/Render    │  │  Persistence  │
│  (core/)      │  │  (render/)    │  │  (persist/)   │
└───────────────┘  └───────────────┘  └───────────────┘
        │                   │
        │                   ▼
        │          ┌───────────────┐
        │          │  Display API  │  ← New abstraction
        │          │  (display.h)  │
        │          └───────────────┘
        │                   │
        ▼                   ▼
┌───────────────┐  ┌───────────────┐
│     Input     │  │   Platform    │
│   (input/)    │  │  (platform/)  │
└───────────────┘  └───────────────┘
        │
        ▼
┌───────────────┐
│     Utils     │
│   (utils/)    │
└───────────────┘
```

### Refactoring Strategy

#### Phase 1: Extract Display Abstraction Layer

**Goal:** Separate rendering logic from platform-specific TTY code.

**Changes:**
1. Create new `display.h` interface with abstract drawing operations
2. Move TTY-specific code behind display interface
3. Update render.c to use display API instead of TTY directly

**New Files:**
- `include/snake/display.h` - abstract display interface
- `src/render/display_tty.c` - TTY implementation of display interface

---

#### Phase 2: Remove Render → Persist Dependency

**Goal:** Render module should not fetch data, only draw what it's given.

**Changes:**
1. Remove `persist.h` include from render.c
2. Pass high scores as parameter to render_draw() function
3. Main.c or new controller is responsible for loading high scores

**Modified APIs:**
```c
// Old (coupled)
void render_draw(const GameState* game);

// New (decoupled)
void render_draw(const GameState* game, const HighScore* scores, int score_count);
```

---

#### Phase 3: Decouple Input from Game Types

**Goal:** Input module should provide raw input state, not game-specific commands.

**Changes:**
1. Remove SnakeDir from InputState
2. Replace with raw directional booleans or key codes
3. Move direction translation to game logic or main

**New InputState:**
```c
typedef struct {
    bool quit;
    bool restart;
    bool pause_toggle;
    bool any_key;
    
    // Raw directional input (not game-specific)
    bool move_up;
    bool move_down;
    bool move_left;
    bool move_right;
} InputState;
```

---

#### Phase 4: Add Game Query Functions

**Goal:** Encapsulate game state access behind proper API.

**Changes:**
1. Add query functions to game.h for checking player state
2. Remove direct field access from main.c
3. Make internal structures opaque if possible

**New Functions:**
```c
// Query functions for game state
bool game_player_died_this_tick(const GameState* game, int player_index);
int game_player_score_at_death(const GameState* game, int player_index);
int game_player_current_score(const GameState* game, int player_index);
bool game_player_is_active(const GameState* game, int player_index);
int game_get_num_players(const GameState* game);
```

---

#### Phase 5: Refactor Network Module (Future)

**Goal:** Network should work with serializable DTOs, not internal types.

**Changes:**
1. Create network-specific message types
2. Add serialization/deserialization functions
3. Keep network protocol independent of game implementation

**New Types:**
```c
// Network message types (in net.h)
typedef struct {
    bool up, down, left, right;
    uint32_t sequence;
} NetInputMessage;

typedef struct {
    int player_count;
    int scores[SNAKE_MAX_PLAYERS];
    // Serialized game state
} NetStateMessage;
```

---

#### Phase 6: Extract Game Controller

**Goal:** Move game loop coordination out of main.c.

**Changes:**
1. Create `game_controller.h` with high-level game flow functions
2. Move terminal size checking to platform module
3. Move score persistence logic to controller

**New Module:**
```c
// include/snake/controller.h
typedef struct GameController GameController;

GameController* controller_create(void);
void controller_destroy(GameController* ctrl);
void controller_run(GameController* ctrl);
```

---

## Refactoring Implementation Order

### Step 1: Add Game Query Functions (Low Risk)
- Add functions to game.h
- Update main.c to use them
- Keep old direct access for compatibility

### Step 2: Extract Display Abstraction (Medium Risk)
- Create display.h interface
- Implement display_tty.c
- Update render.c to use display API

### Step 3: Fix Render → Persist Coupling (Low Risk)
- Change render_draw signature
- Update main.c to load scores
- Pass scores to render_draw

### Step 4: Decouple Input from Game Types (Medium Risk)
- Change InputState structure
- Update input.c
- Update main.c to translate inputs

### Step 5: Cleanup and Test (Low Risk)
- Add unit tests for isolated modules
- Document new APIs
- Update design docs

### Step 6: Extract Game Controller (High Risk - Optional)
- Create controller module
- Move logic from main.c
- Extensive testing

---

## Benefits of Refactored Architecture

### Improved Testability
- Core game logic can be tested without rendering
- Render logic can be tested without filesystem
- Input can be tested without terminal

### Easier Extensions
- Can add new display backends (SDL, curses, etc.)
- Can add new input methods (gamepad, network)
- Can add new persistence formats

### Better Code Organization
- Clear module boundaries
- Explicit dependencies
- Single responsibility per module

### Reduced Coupling
- Modules depend on interfaces, not implementations
- Changes to one module don't cascade
- Easier to understand and maintain

---

## Migration Path

### Backward Compatibility
- Keep old APIs during transition
- Mark deprecated with comments
- Remove after all callers updated

### Testing Strategy
1. Add tests for existing behavior
2. Refactor one module at a time
3. Run tests after each change
4. Ensure game still compiles and runs

### Risk Mitigation
- Make small, incremental changes
- Commit after each working state
- Keep main.c working throughout

---

## Conclusion

The current codebase has several coupling issues that can be resolved through systematic refactoring:

1. **Most Critical:** Render depends on Persist - violates separation of concerns
2. **Most Critical:** Render depends on TTY - prevents alternative backends
3. **Important:** Input module coupled to game types - reduces reusability

The proposed refactoring introduces clear abstractions, improves testability, and enables future extensions while maintaining all existing functionality.

### Immediate Next Steps
1. Get approval for refactoring approach
2. Begin with low-risk changes (game query functions)
3. Proceed incrementally through phases
4. Test thoroughly at each step

