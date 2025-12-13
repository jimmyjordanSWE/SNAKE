# Snake Game - Module Dependency Graph

## Current Architecture (After Refactoring)

### High-Level View

```
                    ┌───────────────┐
                    │    main.c     │
                    │ (Application) │
                    └───────┬───────┘
                            │
            ┌───────────────┼───────────────┐
            │               │               │
            ▼               ▼               ▼
    ┌───────────┐   ┌───────────┐   ┌───────────┐
    │   game    │   │  render   │   │  persist  │
    │  (core/)  │   │ (render/) │   │(persist/) │
    └─────┬─────┘   └─────┬─────┘   └───────────┘
          │               │
          │               ▼
          │         ┌───────────┐
          │         │  display  │ ← Abstraction layer
          │         │ (display.h)│
          │         └─────┬─────┘
          │               │
          ▼               ▼
    ┌───────────┐   ┌───────────┐
    │   input   │   │   tty     │
    │ (input/)  │   │(platform/)│
    └─────┬─────┘   └───────────┘
          │
          ▼
    ┌───────────┐
    │   utils   │
    │ (utils/)  │
    └───────────┘
```

### Detailed Dependency Matrix

| Module | Depends On | Used By |
|--------|-----------|---------|
| **types.h** | (none) | game, collision |
| **utils/** | (none) | game, input (removed) |
| **platform/** | (none) | main, display_tty |
| **tty.h** | platform | display_tty |
| **display.h** | (abstraction) | render |
| **display_tty.c** | display.h, tty.h | (impl) |
| **collision.h** | types.h, game.h | game |
| **game.h** | types.h | main, render, collision |
| **input.h** | (none - decoupled!) | main |
| **persist.h** | (none) | main, render (type only) |
| **render.h** | game.h, persist.h (type) | main |
| **net.h** | game.h, input.h | (stub) |

### File-Level Includes

#### include/snake/types.h
```
(no includes - defines primitives)
```

#### include/snake/game.h
```
#include "snake/types.h"
```

#### include/snake/collision.h
```
#include "snake/game.h"
#include "snake/types.h"
```

#### include/snake/input.h
```
(no snake includes - fully decoupled!)
```

#### include/snake/render.h
```
#include "snake/game.h"
#include "snake/persist.h"  // For HighScore type only
```

#### include/snake/persist.h
```
(no snake includes - independent)
```

#### include/snake/display.h
```
(no snake includes - abstraction)
```

#### include/snake/platform.h
```
(no snake includes - platform primitives)
```

#### include/snake/tty.h
```
(no snake includes - platform specific)
```

### Implementation Dependencies

#### src/core/game.c
```
#include "snake/game.h"
#include "snake/collision.h"
#include "snake/utils.h"
```

#### src/core/collision.c
```
#include "snake/collision.h"
```

#### src/input/input.c
```
#include "snake/input.h"
// No game types!
```

#### src/render/render.c
```
#include "snake/render.h"
#include "snake/display.h"
#include "snake/persist.h"  // For HighScore type
// No tty.h!
```

#### src/render/display_tty.c (NEW)
```
#include "snake/display.h"
#include "snake/tty.h"
```

#### src/persist/persist.c
```
#include "snake/persist.h"
```

#### src/platform/platform.c
```
#include "snake/platform.h"
```

#### src/platform/tty.c
```
#include "snake/tty.h"
```

#### main.c
```
#include "snake/game.h"
#include "snake/input.h"
#include "snake/persist.h"
#include "snake/platform.h"
#include "snake/render.h"
#include "snake/types.h"
```

## Key Improvements

### 1. Display Abstraction
- **Before:** render.c → tty.h (tight coupling)
- **After:** render.c → display.h → display_tty.c → tty.h (loose coupling)
- **Benefit:** Can swap TTY for SDL, framebuffer, etc. without changing render.c

### 2. Input Independence
- **Before:** input.h → types.h (game-specific types)
- **After:** input.h → (no game includes)
- **Benefit:** Input module is completely reusable

### 3. Render Data Flow
- **Before:** render.c calls persist_read_scores()
- **After:** main.c loads scores, passes to render_draw()
- **Benefit:** Clear data flow, render is pure rendering

### 4. Game Encapsulation
- **Before:** main.c accesses game.players[i].field directly
- **After:** main.c uses game_player_*() query functions
- **Benefit:** Can change internal structure without breaking main

## Dependency Levels

```
Level 0 (No dependencies):
  - types.h
  - utils/
  - platform/
  - persist.h
  - input.h (✨ now decoupled!)

Level 1 (Depends on Level 0):
  - tty.h (→ platform)
  - display.h (abstraction)
  - game.h (→ types.h)

Level 2 (Depends on Level 1):
  - collision.h (→ game.h, types.h)
  - display_tty.c (→ display.h, tty.h)
  - render.h (→ game.h, persist.h)

Level 3 (Application):
  - main.c (→ everything)
```

## Circular Dependencies

### Before Refactoring
❌ None found (good!)

### After Refactoring  
✅ None added (still good!)

## Testability Matrix

| Module | Testable Alone? | Mock Requirements |
|--------|-----------------|-------------------|
| **utils** | ✅ Yes | None |
| **platform** | ✅ Yes | None |
| **input** | ✅ Yes | None (fully decoupled!) |
| **persist** | ✅ Yes | Filesystem only |
| **collision** | ✅ Yes | GameState struct |
| **game** | ✅ Yes | Collision, utils |
| **display** | ✅ Yes | Display impl |
| **render** | ✅ Yes | Mock display, GameState, scores |
| **tty** | ⚠️ Partial | Real terminal |
| **main** | ⚠️ Complex | All modules |

### Improvements in Testability

1. **Input Module**
   - Before: Needed game types → harder to test
   - After: No dependencies → easy to test

2. **Render Module**
   - Before: Needed TTY + filesystem → hard to test
   - After: Mock display + pass data → easy to test

3. **Game Module**
   - Before: Could access directly → no enforcement
   - After: Query functions → controlled access

## Anti-Patterns Eliminated

### ❌ Before: Function Call Coupling
```c
// render.c calling persist directly
int count = persist_read_scores(".snake_scores", scores, MAX);
```

### ✅ After: Data Parameter Coupling
```c
// render receives data as parameter
void render_draw(const GameState* game, const HighScore* scores, int count)
```

---

### ❌ Before: Platform-Specific Leakage
```c
// render.c directly using TTY
tty_context* g_tty;
tty_put_pixel(g_tty, x, y, pixel);
```

### ✅ After: Abstract Interface
```c
// render.c using abstraction
DisplayContext* g_display;
display_put_char(g_display, x, y, ch, fg, bg);
```

---

### ❌ Before: Type Coupling
```c
// input.h using game type
typedef struct {
    SnakeDir p1_dir;  // From types.h
} InputState;
```

### ✅ After: Raw State
```c
// input.h with no game knowledge
typedef struct {
    bool move_up, move_down, move_left, move_right;
} InputState;
```

---

### ❌ Before: Direct Field Access
```c
// main.c accessing internals
if (game.players[0].died_this_tick) { ... }
```

### ✅ After: Query Function
```c
// main.c using API
if (game_player_died_this_tick(&game, 0)) { ... }
```

## Coupling Metrics

### Afferent Coupling (Ca) - How many modules depend on this

| Module | Ca | Change Impact |
|--------|-----|---------------|
| types.h | 3 | Medium |
| game.h | 3 | Medium |
| display.h | 1 | Low |
| persist.h | 2 | Low |
| input.h | 1 | Very Low |
| render.h | 1 | Very Low |

### Efferent Coupling (Ce) - How many modules this depends on

| Module | Ce | Dependency Load |
|--------|-----|-----------------|
| main.c | 6 | High (expected) |
| render.c | 3 | Medium |
| game.c | 3 | Medium |
| input.c | 0 | ✅ None! |
| persist.c | 0 | ✅ None! |
| display_tty.c | 2 | Low |

### Instability (I = Ce / (Ce + Ca))

Lower is more stable (good for foundations), higher is more flexible (good for high-level code).

| Module | I | Interpretation |
|--------|---|----------------|
| input | 0.00 | Very stable (no dependencies) |
| persist | 0.00 | Very stable (no dependencies) |
| types.h | 0.00 | Very stable (primitive types) |
| game | 0.50 | Balanced |
| render | 0.60 | Flexible (good for UI) |
| main | 1.00 | Very unstable (expected for app layer) |

## Summary

The refactored architecture achieves:

✅ **Clear layering** - Dependencies flow in one direction  
✅ **Loose coupling** - Modules interact through narrow interfaces  
✅ **High cohesion** - Each module has single, clear purpose  
✅ **Good stability** - Core modules are stable, UI is flexible  
✅ **No cycles** - All dependencies are acyclic  
✅ **Testability** - Modules can be tested independently  

The dependency structure now follows best practices for maintainable C projects.

