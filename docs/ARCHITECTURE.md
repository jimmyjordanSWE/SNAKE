# Architecture

Technical deep-dive for developers working on the SNAKE codebase.

## Module Overview

```
src/
├── core/           Game state, collision detection, tick loop
├── render/         3D raycasting, TTY display, sprites, textures
├── input/          Terminal raw mode, key parsing
├── persist/        Config/score file I/O
├── platform/       Time, sleep, terminal size, signals
├── net/            State serialization (networking stub)
├── console/        Logging utilities
└── utils/          RNG, bounds, direction helpers
```

## Design Patterns

### Opaque Pointers
Public headers expose only API; implementation details hidden:
```c
// include/snake/game.h
typedef struct Game Game;
Game* game_create(const GameConfig* cfg);

// src/core/game.c
struct Game { GameState state; };
```

### Error-Out Pattern
Single cleanup path via `goto out`:
```c
int func(void) {
    int err = 0;
    resource* r = NULL;
    r = alloc_resource();
    if (!r) { err = -1; goto out; }
    // work...
out:
    free_resource(r);
    return err;
}
```

### Config Structs
Avoid long parameter lists:
```c
typedef struct { int width, height; } GameConfig;
Game* game_create(const GameConfig* cfg);
```

## 3D Rendering Pipeline

```
Camera → Raycaster → Projection → Texture Sampling → SDL Framebuffer
   ↓
Sprites → Depth Sort → Composite
```

| Component | File | Purpose |
|-----------|------|---------|
| Camera | `camera.c` | Position, angle, interpolation |
| Raycaster | `raycast.c` | DDA wall intersection |
| Projection | `projection.c` | Wall height from distance |
| Texture | `texture.c` | Image loading, sampling |
| Sprites | `sprite.c` | Billboard projection, depth sort |
| SDL | `render_3d_sdl.c` | Window, pixel buffer |

## Memory Ownership

- **Caller owns**: Configs passed to `*_create()`
- **Callee owns**: Returned opaque pointers (caller must call `*_destroy()`)

---

This project generates structural context to assist LLMs in understanding the codebase structure.

## Purpose

Instead of providing only raw source code to an LLM, these scripts produce:
- **Compressed symbol maps** — All functions, structs, typedefs in minimal tokens
- **Architectural views** — Call graphs, data flow, dependencies
- **Pattern summaries** — Memory ownership, state machines, hotspots

This allows an LLM to navigate and reason about the project at a high level, then request specific files only when needed.

## Orchestrator

**`make analyze`** — Runs the Python analysis scripts (using the project's venv) and emits outputs to `scripts/out/`.

```bash
make analyze  # Manual refresh (preferred)
```

## Script Registry

| Script | Output | Metadata Provided |
|--------|--------|------------------|
| `structure.py` | `structure_out.txt` | Symbol hierarchy |
| `call_chains.py` | `call_chains_out.txt` | Call tree and module flow |
| `data_flow.py` | `data_flow_out.txt` | Struct usage totals |
| `memory_map.py` | `memory_map_out.txt` | Allocation site tracking |
| `hotspots.py` | `hotspots_out.txt` | Loop nesting depths |
| `errors.py` | `errors_out.txt` | Allocation ownership patterns |
| `invariants.py` | `invariants_out.txt` | State variable usage |

## How LLMs Use This

1. **Load `structure_out.txt`** — See all modules, functions, types at a glance
2. **Browse specific outputs** — e.g., `call_chains_out.txt` to trace execution
3. **Request source only when needed** — Context allows targeted file requests

## Output Format (Token-Minimized)

```
# call_chains_out.txt  
Module Flow:
  render -> display, platform, render_3d
  render_3d -> camera, projection, raycast, sprite, texture
Call Tree (from main):
  main
    snake_game_run
      game_step
        game_tick

# data_flow_out.txt
Data Ownership (struct r/w totals):
  GameState: r(184) w(41)
  PlayerState: r(123) w(21)
Module Data Usage:
  core: GameState:r141w41, PlayerState:r82w21
```

## Adding New Context Generators

1. Create `scripts/new_extractor.py` using tree-sitter
2. Add to `scripts` array in `analyze_all.sh`
3. Output goes to `scripts/out/new_extractor_out.txt`

## Setup

```bash
python3 -m venv .venv
.venv/bin/pip install tree-sitter tree-sitter-c
```
