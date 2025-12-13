# 3D Renderer: Architectural Diagrams & Reference

## 1. Complete System Architecture Diagram

```
┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃                       SNAKE GAME MAIN                          ┃
┃  Orchestrates game loop, manages game state, calls rendering  ┃
┗━━━━━━━━━━━━━━━━━━━━━━━┳━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
                         │
                    ┌────┴────┐
                    │ Mode    │
                    │ 2D or 3D?
                    └────┬────┘
                    ┌────┴─────────────────┐
                    │                      │
                 ┌──▼──┐            ┌─────▼──┐
                 │ 2D  │            │ 3D     │
                 │Mode │            │Rendering
                 └─────┘            │Pipeline
                                    └─────┬──┘
                                          │
        ┌─────────────────┬───────────────┼────────────────┐
        │                 │               │                │
    ┌───▼────┐    ┌──────▼─────┐    ┌────▼────┐    ┌─────▼────┐
    │Camera  │    │Raycaster   │    │Projector│    │Texture   │
    │        │    │            │    │         │    │Shading   │
    │Config: │    │Config:     │    │Config:  │    │Config:   │
    │• x, y  │    │• Algorithm │    │• Screen │    │• Palette │
    │• angle │    │  (DDA)     │    │  dims   │    │• Chars   │
    │• FOV   │    │• Max dist  │    │• Scale  │    │• Levels  │
    │        │    │• Grid size │    │         │    │          │
    └────┬───┘    └──────┬─────┘    └────┬────┘    └────┬─────┘
         │                │             │              │
         │ ray_dir_x     │ distance    │ draw_start  │ char
         │ ray_dir_y     │ wall_x      │ draw_end    │ color
         │               │ side        │ tex_step    │
         │               │ map_x/map_y │ tex_pos     │
         └───────┬───────┴─────────────┴─────────────┘
                 │
         ┌───────▼───────┐
         │Sprite Engine  │
         │               │
         │• World→Screen │
         │• Z-buffer     │
         │• Sorting      │
         └───────┬───────┘
                 │
         ┌───────▼───────────┐
         │Display Backend 3D │
         │                   │
         │• Frame buffer     │
         │  (char + color)   │
         │• Column drawing   │
         │• HUD overlay      │
         │                   │
         └───────┬───────────┘
                 │
         ┌───────▼────────────┐
         │Terminal Display    │
         │(via display.h)     │
         └────────────────────┘
```

---

## 2. Data Flow Diagram (Per Frame)

```
FRAME START
    ├─ Input: game_state, camera_position, screen_width, screen_height
    │
    ├─────► Camera Update
    │       ├─ Input: active_player_pos, direction
    │       └─ Output: Camera3D (pos, angle, FOV, basis vectors)
    │
    ├─────► FOR EACH SCREEN COLUMN (0 to width-1)
    │       │
    │       ├─ Ray Generation (Camera)
    │       │  ├─ Input: column_index, FOV_basis
    │       │  └─ Output: ray_direction_x, ray_direction_y
    │       │
    │       ├─ Raycasting (DDA Algorithm)
    │       │  ├─ Input: ray_dir, camera_pos, game_geometry
    │       │  └─ Output: RayHit {distance, wall_x, side, map_x, map_y}
    │       │
    │       ├─ Projection (Distance → Screen Geometry)
    │       │  ├─ Input: ray_hit.distance, screen_height
    │       │  └─ Output: WallProjection {draw_start, draw_end, tex_step}
    │       │
    │       └─ FOR EACH PIXEL IN PROJECTION (draw_start to draw_end)
    │           │
    │           ├─ Texture Sampling
    │           │  ├─ Input: wall_side, wall_x, tex_v, distance
    │           │  └─ Output: TexelResult {character, color}
    │           │
    │           └─ Frame Buffer Write
    │               └─ frame[column][row] = {char, color}
    │
    ├─────► Sprite Rendering
    │       ├─ Build sprite list (snakes, food) from game_state
    │       ├─ Sort by depth (Z-buffer)
    │       └─ FOR EACH SPRITE (back to front)
    │           ├─ Project world position to screen
    │           ├─ Check visibility (in front, on-screen)
    │           └─ Draw billboard (overwrite frame buffer if not occluded)
    │
    ├─────► HUD Overlay
    │       ├─ Score, mode indicator, minimap
    │       └─ Overwrite top rows of frame buffer
    │
    └─────► Terminal Refresh
            └─ Send frame buffer to display.h abstraction
```

---

## 3. Module Dependency Graph (Acyclic)

```
┌──────────────────────────────────┐
│         main.c                   │
│    (Game loop orchestrator)      │
└──────────────┬───────────────────┘
               │
        ┌──────┴────────┐
        │               │
        ▼               ▼
    ┌────────────┐  ┌──────────┐
    │render.c    │  │render_3d.c
    │(2D mode)   │  │(3D mode) │
    └────────────┘  └────┬─────┘
                         │
         ┌───────────────┼───────────────┬──────────────┐
         │               │               │              │
         ▼               ▼               ▼              ▼
    ┌────────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐
    │camera.c    │  │raycast.c │  │project.c │  │texture.c │
    │            │  │          │  │          │  │          │
    │Pure math   │  │DDA algo  │  │Distance  │  │Shading   │
    │No state    │  │Grid step │  │→ height  │  │lookup    │
    │            │  │          │  │          │  │          │
    └────────────┘  └──────────┘  └──────────┘  └──────────┘
         │               │              │             │
         └───────────────┼──────────────┼─────────────┘
                         │              │
                         ▼              ▼
                    ┌──────────┐  ┌───────────┐
                    │sprite.c  │  │display_3d.│
                    │          │  │           │
                    │Billboard │  │Frame buf  │
                    │Z-buffer  │  │Column ops │
                    │          │  │HUD overlay│
                    └──────────┘  └───────────┘
                         │              │
                         └──────┬───────┘
                                │
                    ┌───────────▼────────────┐
                    │    display.h           │
                    │  (Abstraction layer)   │
                    │  • draw_at()           │
                    │  • clear()             │
                    │  • refresh()           │
                    └────────────────────────┘
                                │
                    ┌───────────▼────────────┐
                    │    tty.c               │
                    │  (Terminal backend)    │
                    └────────────────────────┘


DEPENDENCY SUMMARY:
- camera.c: depends on types.h, math.h (minimal)
- raycast.c: depends on types.h, game.h, camera.h
- project.c: depends on raycast.h, types.h
- texture.c: depends on raycast.h, project.h, display.h
- sprite.c: depends on camera.h, game.h, types.h, display.h
- display_3d.c: depends on display.h, types.h
- render_3d.c: orchestrates all above modules
```

---

## 4. Responsibility & Ownership Matrix

```
┌─────────────┬────────────────────────┬──────────────────┬─────────────────┐
│ Module      │ Primary Responsibility │ Data Ownership   │ Failure Domain  │
├─────────────┼────────────────────────┼──────────────────┼─────────────────┤
│ camera.c    │ World↔Camera transform │ Camera state     │ None (pure math)│
│             │ Ray generation         │ (x, y, angle)    │                 │
│             │                        │                  │                 │
├─────────────┼────────────────────────┼──────────────────┼─────────────────┤
│ raycast.c   │ Wall intersection      │ Transient        │ Invalid grid    │
│             │ DDA algorithm          │ (ray state)      │ coordinates     │
│             │ Distance calculation   │                  │                 │
│             │                        │                  │                 │
├─────────────┼────────────────────────┼──────────────────┼─────────────────┤
│ project.c   │ Distance→geometry      │ Transient        │ Division by     │
│             │ Screen space mapping   │ (projection)     │ zero (distance) │
│             │ Clipping logic         │                  │                 │
│             │                        │                  │                 │
├─────────────┼────────────────────────┼──────────────────┼─────────────────┤
│ texture.c   │ Character selection    │ Static patterns  │ Invalid shade   │
│             │ Distance shading       │ Color maps       │ level           │
│             │ Visual appearance      │ (initialized     │                 │
│             │                        │ once)            │                 │
│             │                        │                  │                 │
├─────────────┼────────────────────────┼──────────────────┼─────────────────┤
│ sprite.c    │ Entity rendering       │ Transient        │ Z-buffer        │
│             │ World→Screen transform │ (sprite list,    │ overflow        │
│             │ Depth sorting          │ Z-buffer)        │                 │
│             │                        │                  │                 │
├─────────────┼────────────────────────┼──────────────────┼─────────────────┤
│ display_3d.c│ Frame buffer mgmt      │ Frame buffer     │ Buffer overflow │
│             │ Pixel writing          │ (char + color    │ Screen bounds   │
│             │ HUD rendering          │ grid)            │ exceeded        │
│             │                        │                  │                 │
├─────────────┼────────────────────────┼──────────────────┼─────────────────┤
│ render_3d.c │ Pipeline orchestration │ Mode state       │ Any downstream  │
│             │ Module coordination    │ (2D vs 3D)       │ errors          │
│             │                        │                  │                 │
└─────────────┴────────────────────────┴──────────────────┴─────────────────┘
```

---

## 5. Bounded Contexts & Integration Points

```
┌────────────────────────────────────────────────────────────────┐
│              SNAKE GAME CONTEXT (Existing)                     │
│                                                                │
│  ┌──────────┐     ┌──────────┐     ┌──────────┐              │
│  │ game.c   │────▶│  input.c │────▶│ render.c │              │
│  │ GameState│     │          │     │ (2D mode)│              │
│  └──────────┘     └──────────┘     └──────────┘              │
│        ▲                                   │                  │
│        │                                   │                  │
│        └─────── Query API (read-only) ────┘                  │
│                                                                │
│  ┌──────────────────────────────────────────┐               │
│  │        display.h (Abstraction)           │               │
│  │  • draw_at(x, y, char, color)            │               │
│  │  • clear()                               │               │
│  │  • refresh()                             │               │
│  └──────────────────────────────────────────┘               │
│                                                                │
└────────────────────────────────────────────────────────────────┘
                           △
                           │ Implements
                           │
┌────────────────────────────────────────────────────────────────┐
│         3D RENDERING CONTEXT (New Bounded Context)            │
│                                                                │
│  ┌──────────────────────────────────────────────────┐        │
│  │          render_3d.c (Service Orchestrator)      │        │
│  │  Coordinates pipeline, manages lifecycle         │        │
│  └──────────────────────┬───────────────────────────┘        │
│                         │                                    │
│         ┌───────────────┼───────────────┬──────────────┐    │
│         │               │               │              │    │
│    ┌────▼──┐     ┌──────▼──┐     ┌──────▼──┐    ┌─────▼───┐
│    │Camera │     │Raycaster│     │Projector│    │Texture  │
│    │       │     │         │     │         │    │         │
│    │Values:│     │Values:  │     │Values:  │    │Values:  │
│    │pos    │     │rays dist│     │height   │    │shade    │
│    │angle  │     │wall_x   │     │clipping │    │char     │
│    │FOV    │     │side     │     │tex coord│    │color    │
│    └────┬──┘     └────┬────┘     └────┬────┘    └────┬────┘
│         │             │              │             │
│         ├─────────────┼──────────────┼─────────────┤
│         │             │              │             │
│    ┌────▼──────┐  ┌──▼────────┐  ┌──▼─────────────┐
│    │ Sprite    │  │ Display 3D│  │ Integration   │
│    │ Renderer  │  │ Backend   │  │ with game     │
│    │           │  │           │  │               │
│    │• Z-buffer │  │• Frame buf│  │• Query mode   │
│    │• Billboard│  │• HUD      │  │• Toggle 2D/3D│
│    └───────────┘  └───────────┘  └───────────────┘
│                                                                │
└────────────────────────────────────────────────────────────────┘
```

---

## 6. Data Structure Memory Layout (Optimized)

```
CAMERA3D (36 bytes, fits in 1 cache line)
┌─────────────────────────────────────────┐
│ x (4B)       y (4B)      angle (4B)     │  Hot path: read frequently
│ fov (4B)     dir_x (4B)  dir_y (4B)     │
│ plane_x (4B) plane_y (4B) proj_dist (4B)│
└─────────────────────────────────────────┘

RAYHIT (24 bytes)
┌──────────────────────────────────────┐
│ distance (4B) wall_x (4B)            │  Core data
│ map_x (4B) map_y (4B)                │
│ side (1B) hit (1B) padding (6B)      │
└──────────────────────────────────────┘

WALLPROJECTION (20 bytes)
┌──────────────────────────────────────┐
│ draw_start (4B) draw_end (4B)        │  Projection data
│ wall_height (4B)                      │
│ tex_step (4B) tex_pos (4B)            │
└──────────────────────────────────────┘

DISPLAY3D (Frame Buffer, Column-Major SoA)
┌──────────────────────────────────────┐
│ chars[width * height]    ← Sequential│  Cache-friendly writes
│ colors[width * height]               │  per-column rendering
│ width (4B) height (4B)               │
│ pitch_chars (4B) pitch_colors (4B)   │
└──────────────────────────────────────┘

TEXELRESULT (8 bytes)
┌──────────────────────────────────────┐
│ character (1B) bold (1B) padding(2B) │
│ color (2B) padding (2B)              │
└──────────────────────────────────────┘
```

---

## 7. Performance Profile (Target: 60 FPS = 16.7 ms per frame)

```
FRAME BUDGET BREAKDOWN
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Setup Phase                    Expected        Margin
─────────────────────────────────────────────────────────
  Camera update               0.5 ms    ───     1.0 ms
  Ray cache lookup/compute    0.2 ms    ───     0.3 ms
  Display buffer clear        0.1 ms    ───     0.2 ms
  ─────────────────────────────────────────────────
  Subtotal                    0.8 ms           1.5 ms (2x margin)

Main Rendering Phase         Expected        Margin
─────────────────────────────────────────────────────────
  Raycasting (80 cols)        8.0 ms    ───    16.0 ms
    (6 DDA steps avg/column)  (0.1 ms/col)
  Projection & texture        3.0 ms    ───     6.0 ms
    (2400 pixels sampled)     (1.25 µs/pixel)
  ─────────────────────────────────────────────────────
  Subtotal                   11.0 ms          22.0 ms (2x margin)

Sprite & HUD Phase           Expected        Margin
─────────────────────────────────────────────────────────
  Sprite rendering            1.5 ms    ───     3.0 ms
  HUD overlay                 0.5 ms    ───     1.0 ms
  ─────────────────────────────────────────────────────
  Subtotal                    2.0 ms           4.0 ms (2x margin)

Terminal I/O Phase           Expected        Margin
─────────────────────────────────────────────────────────
  Frame buffer management     0.5 ms    ───     1.0 ms
  Terminal write/refresh      0.7 ms    ───     1.4 ms (may stall)
  ─────────────────────────────────────────────────────
  Subtotal                    1.2 ms           2.4 ms (2x margin)

─────────────────────────────────────────────────────────
TOTAL                        15.0 ms         30.0 ms
HEADROOM                                     16.7 - 15.0 = 1.7 ms

STATUS: ⚠ TIGHT - Optimization focus on raycasting first
```

---

## 8. Optimization Priority Roadmap

```
PHASE 1: INFRASTRUCTURE & FOUNDATIONAL OPTIMIZATIONS (Week 1-2)
┌──────────────────────────────────────┐
│ Priority │ Optimization              │ Benefit    │ Effort │ Impact  │
├──────────┼───────────────────────────┼────────────┼────────┼─────────┤
│    1     │ Column-major frame buffer │ 2x display │ Low    │ HIGH    │
│          │ (SoA layout)              │ writes     │        │ Must    │
├──────────┼───────────────────────────┼────────────┼────────┼─────────┤
│    2     │ Ray direction cache       │ 10% saved  │ Low    │ MED     │
│          │ (pre-compute per camera)  │ computation│        │ Should  │
├──────────┼───────────────────────────┼────────────┼────────┼─────────┤
│    3     │ Integer shade computation │ 5% faster  │ Low    │ LOW     │
│          │ (avoid float compares)    │ sampling   │        │ Nice    │
└──────────┴───────────────────────────┴────────────┴────────┴─────────┘

PHASE 2: ALGORITHMIC OPTIMIZATIONS (If Phase 1 insufficient)
┌──────────────────────────────────────┐
│ Priority │ Optimization              │ Benefit    │ Effort │ Impact  │
├──────────┼───────────────────────────┼────────────┼────────┼─────────┤
│    1     │ Adaptive ray resolution   │ 30% fewer  │ Medium │ HIGH    │
│          │ (skip distant columns)    │ rays       │        │ If slow │
├──────────┼───────────────────────────┼────────────┼────────┼─────────┤
│    2     │ Radix-sort sprites        │ 10x faster │ Low    │ LOW     │
│          │ (O(s) depth sort)         │ sorting    │        │ Nice    │
├──────────┼───────────────────────────┼────────────┼────────┼─────────┤
│    3     │ Early ray termination     │ Already    │ None   │ N/A     │
│          │ (in design)               │ included   │        │         │
└──────────┴───────────────────────────┴────────────┴────────┴─────────┘

PHASE 3: PARALLELIZATION (Only if Phase 1 & 2 insufficient)
┌──────────────────────────────────────┐
│ Priority │ Optimization              │ Benefit    │ Effort │ Impact  │
├──────────┼───────────────────────────┼────────────┼────────┼─────────┤
│    1     │ SIMD raycasting           │ 3-4x ray   │ High   │ HIGH    │
│          │ (SSE/AVX ray batches)     │ speedup    │        │ If slow │
├──────────┼───────────────────────────┼────────────┼────────┼─────────┤
│    2     │ Multi-thread columns      │ 2-4x wall  │ Medium │ MED     │
│          │ (OpenMP per-column)       │ rendering  │        │ If slow │
└──────────┴───────────────────────────┴────────────┴────────┴─────────┘

CURRENT RECOMMENDATION: Implement Phase 1 in all modules. Profile.
If 15.0 ms < 16.7 ms ✓, done. Otherwise proceed Phase 2.
```

---

## 9. File Structure & Organization

```
snake/
├── include/
│   └── snake/
│       ├── camera.h          ← New camera API
│       ├── raycast.h         ← New raycasting API
│       ├── projection.h      ← New projection API
│       ├── texture.h         ← New texture API
│       ├── sprite.h          ← New sprite API
│       ├── display_3d.h      ← New display backend
│       ├── render_3d.h       ← New render orchestrator
│       ├── game.h            ← Unchanged (read-only queries)
│       ├── display.h         ← Unchanged (abstraction)
│       └── [other headers]
│
├── src/
│   ├── core/                 ← Game logic (unchanged)
│   ├── render/
│   │   ├── render.c          ← Updated: add 2D/3D toggle
│   │   ├── display_tty.c     ← Unchanged
│   │   └── display_3d.c      ← New 3D backend
│   │
│   ├── render3d/             ← NEW: 3D renderer modules
│   │   ├── camera.c          ← Camera transforms
│   │   ├── raycast.c         ← DDA raycasting
│   │   ├── projection.c      ← Projection math
│   │   ├── texture.c         ← Shading & textures
│   │   ├── sprite.c          ← Sprite rendering
│   │   └── render_3d.c       ← Main orchestrator
│   │
│   ├── platform/             ← Platform (unchanged)
│   ├── input/                ← Input (unchanged)
│   ├── persist/              ← Persistence (unchanged)
│   └── utils/                ← Utils (unchanged)
│
├── design_docs/
│   ├── 03_REFINED_3D_RENDERER_PLAN.md    ← This document
│   ├── 03_ARCHITECTURE_DIAGRAMS.md       ← Supplementary diagrams
│   ├── 95_pseudo3d_renderer_overview.md  ← Original overview
│   ├── 96_file1_camera.md                ← Camera spec
│   ├── 97_file2_raycast.md               ← Raycasting spec
│   ├── 98_file3_projection.md            ← Projection spec
│   ├── 99_file4_texture.md               ← Texture spec
│   ├── 100_file5_sprite.md               ← Sprite spec
│   ├── 101_file6_display3d.md            ← Display spec
│   └── 102_file7_integration.md          ← Integration spec
│
├── Makefile
├── ARCHITECTURE.md
└── PROJECT_STATE.md
```

---

## 10. Key Design Principles Summary

```
┌────────────────────────────────────────────────────────────┐
│ SEPARATION OF CONCERNS                                     │
├────────────────────────────────────────────────────────────┤
│ Each module has ONE clear responsibility                   │
│ • Camera = transforms only, no rendering                  │
│ • Raycaster = intersection only, no projection            │
│ • Projector = geometry only, no shading                   │
│ • Texture = appearance only, no math                      │
│ • Sprite = entities only, no walls                        │
│ • Display = I/O only, no algorithms                       │
└────────────────────────────────────────────────────────────┘

┌────────────────────────────────────────────────────────────┐
│ LOOSE COUPLING                                             │
├────────────────────────────────────────────────────────────┤
│ Modules depend on abstractions, not implementations        │
│ • render_3d.c calls camera_get_ray(), not internals      │
│ • raycast.c uses game query API, not game_state directly │
│ • texture.c receives wall_hit, doesn't generate it        │
│ • Enables swapping implementations without breaking       │
└────────────────────────────────────────────────────────────┘

┌────────────────────────────────────────────────────────────┐
│ ACYCLIC DEPENDENCIES                                       │
├────────────────────────────────────────────────────────────┤
│ Dependency graph forms DAG (no circular imports)           │
│ • No module imports each other mutually                   │
│ • Dependencies flow downward (ordered)                    │
│ • Enables parallel compilation & testing                 │
└────────────────────────────────────────────────────────────┘

┌────────────────────────────────────────────────────────────┐
│ PURE FUNCTIONS WHERE POSSIBLE                             │
├────────────────────────────────────────────────────────────┤
│ Queries have no side effects (testable, composable)        │
│ • camera_get_ray() - pure math function                   │
│ • raycast() - deterministic, no state modification        │
│ • texture_distance_to_shade() - lookup, no side effects   │
│ • Only render_3d orchestrates and modifies state          │
└────────────────────────────────────────────────────────────┘

┌────────────────────────────────────────────────────────────┐
│ STATIC ALLOCATION (No Runtime Fragmentation)              │
├────────────────────────────────────────────────────────────┤
│ All buffers pre-allocated, reused per frame               │
│ • Frame buffer allocated once, cleared/refilled each frame│
│ • Ray result array pre-sized for max columns              │
│ • Sprite list pre-sized for max entities                 │
│ • Avoids malloc/free in hot path                         │
└────────────────────────────────────────────────────────────┘

┌────────────────────────────────────────────────────────────┐
│ TESTABILITY & INSTRUMENTATION                             │
├────────────────────────────────────────────────────────────┤
│ Each module independently testable with unit tests        │
│ • Camera math verified with known test vectors            │
│ • Raycaster validated against expected hit patterns       │
│ • Projection bounds checked for clipping                 │
│ • Performance profiling built-in (optional instrumentation)│
└────────────────────────────────────────────────────────────┘
```

---

**END OF ARCHITECTURAL REFERENCE DIAGRAMS**
