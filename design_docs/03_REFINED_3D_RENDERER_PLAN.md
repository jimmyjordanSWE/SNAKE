# Pseudo-3D Renderer: Refined Architecture & Optimization Plan

**Date:** December 13, 2025  
**Analysis Methodology:** Two-Pass Deep Refinement (Architecture → Optimization)

---

## EXECUTIVE SUMMARY

This document applies domain-driven design and algorithmic optimization principles to the Snake 3D renderer architecture. It produces:

1. **PASS 1 OUTPUT:** Architectural diagram, responsibility matrix, bounded contexts, data flow, dependency graph
2. **PASS 2 OUTPUT:** Optimized algorithms, data structures, complexity analysis, memory layout, parallelization opportunities

**KEY CHANGE:** The 3D renderer now runs as a **separate process** in an SDL window, communicating with the main game via IPC (Inter-Process Communication). The game continues running in the terminal as usual. This decoupling provides:
- Independent rendering pipeline (not constrained by terminal)
- True graphics capability (colors, higher resolution)
- Easier testing and debugging (can start/stop renderer independently)
- Scalability (renderer can run on different machine if network-based)

---

# PASS 1: ARCHITECTURE & DECOMPOSITION

## Domain Analysis & Ubiquitous Language

### Core Domain
- **3D Rendering Subdomain:** The computational heart—raycasting, projection, texture mapping
- **Supporting Subdomains:**
  - Camera Management (state, transforms)
  - Geometry Processing (world ↔ screen space)
  - Visual Effects (shading, depth effects)

### Ubiquitous Language

| Term | Definition | Scope |
|------|-----------|-------|
| **Ray** | Directional line from camera through screen pixel | Rendering |
| **Wall Hit** | Ray-wall intersection point with metadata | Rendering |
| **Perpendicular Distance** | Distance perpendicular to camera plane (fish-eye corrected) | Rendering |
| **Wall Projection** | Screen-space vertical span representing wall slice | Rendering |
| **Texture Texel** | Single character + color at specific wall location | Rendering |
| **Sprite Billboard** | World entity rendered as camera-facing quad | Rendering |
| **Z-Buffer** | Depth map for sprite sorting/occlusion | Rendering |
| **Shade Level** | Quantized distance band for character selection | Rendering |

### Bounded Contexts

```
┌─────────────────────────────────────────────────────────────┐
│                    PROCESS 1: SNAKE GAME                    │
│                        (Terminal)                           │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │ Game Logic   │  │ Input System │  │  Rendering   │      │
│  │ (game.c)     │  │ (input.c)    │  │ (render.c)   │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
│        │                                    │               │
│        │ Publishes state                    │ Renders 2D     │
│        │ (player, snake, food)              │ to terminal    │
│        │                                    │               │
│        ▼                                    ▼               │
│  ┌────────────────────────────────────────────────────┐    │
│  │ IPC Server (net.c or new comm module)             │    │
│  │ • Socket/pipe to renderer                          │    │
│  │ • Serializes game state → JSON/binary              │    │
│  │ • Sends at fixed interval (60 Hz)                  │    │
│  └────────────────┬───────────────────────────────────┘    │
│                   │ Network/IPC                             │
└───────────────────┼────────────────────────────────────────┘
                    │
       ┌────────────┴───────────────┐
       │  Protocol: GameState msgs  │
       │ {player_x, player_y, ...}  │
       │                            │
       ▼                            ▼
┌─────────────────────────────────────────────────────────────┐
│              PROCESS 2: 3D RENDERER                          │
│                    (SDL Window)                             │
│                                                              │
│  ┌──────────────────────────────────────────────────────┐  │
│  │ Renderer Service (render_3d.c)                       │  │
│  │ • Receives game state via IPC                        │  │
│  │ • Orchestrates rendering pipeline                    │  │
│  │ • Outputs to SDL window                              │  │
│  └──────────────────────────────────────────────────────┘  │
│   │                  │                 │                    │
│   ▼                  ▼                 ▼                    │
│  ┌─────────────┐  ┌──────────────┐  ┌────────────┐        │
│  │  Camera     │  │ Raycaster    │  │ Projector  │        │
│  │  (camera.c) │  │ (raycast.c)  │  │ (proj.c)   │        │
│  └─────────────┘  └──────────────┘  └────────────┘        │
│   • Transforms     • DDA Algorithm   • Wall Height         │
│   • Ray Generation • Wall Hits       • Clipping            │
│                                                             │
│   │                  │                 │                    │
│   └──────────────────┼─────────────────┘                   │
│                      ▼                                       │
│   ┌────────────────────────────────┐                       │
│   │ Texture/Shading (texture.c)    │                       │
│   │ • Distance shading             │                       │
│   │ • Color gradients              │                       │
│   │ • Wall side variation          │                       │
│   └────────────────────────────────┘                       │
│          │                                                  │
│          ▼                                                  │
│   ┌────────────────────────────────┐                       │
│   │ Sprite Renderer (sprite.c)     │                       │
│   │ • World→Screen transform       │                       │
│   │ • Z-buffering                  │                       │
│   │ • Billboard rendering          │                       │
│   └────────────────────────────────┘                       │
│          │                                                  │
│          ▼                                                  │
│   ┌────────────────────────────────┐                       │
│   │ SDL Display Backend (sdl.c)    │                       │
│   │ • Pixel buffer rendering       │                       │
│   │ • True color support           │                       │
│   │ • HUD overlay                  │                       │
│   └────────────────────────────────┘                       │
│          │                                                  │
│          ▼                                                  │
│   ┌────────────────────────────────┐                       │
│   │ SDL Window (High-res graphics) │                       │
│   └────────────────────────────────┘                       │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

### Relationship Patterns

| Contexts | Pattern | Type |
|----------|---------|------|
| Game ↔ 3D Renderer | **Published Language** | Asynchronous (Game publishes state; Renderer subscribes via IPC) |
| Camera ↔ Raycaster | **Shared Kernel** | Both need coordinate system, angle transforms |
| Raycaster ↔ Projector | **Customer-Supplier** | Raycaster (supplier) provides wall hits; Projector (customer) consumes |
| Projector ↔ Texture | **Customer-Supplier** | Projector specifies geometry; Texture applies visuals |
| Texture ↔ Display | **Conformist** | Display defines color/char interface; Texture conforms |
| Sprite ↔ Projector | **Separate Ways** | Sprites use camera transforms but independent rendering |

---

## Ownership & Responsibility Analysis

### Responsibility Matrix

| Module | Primary Responsibility | Data Ownership | Write Authority | Read-Only Access | Failure Domain |
|--------|------------------------|-----------------|------------------|------------------|-----------------|
| **Camera** | World↔Camera transforms, ray generation | Camera state (pos, angle, FOV) | camera_set_* functions | Game (read entity positions) | None (pure math) |
| **Raycaster** | Wall intersection detection, distance calc | Transient: ray state, DDA state | raycast() function | Game.width/height, game entity queries | Invalid grid coordinates |
| **Projector** | Ray distance → screen geometry | Transient: projection results | projection_wall() function | Raycaster results | Division by zero (distance=0) |
| **Texture** | Visual appearance (chars, colors) | Static: char patterns, color maps | initialization only | Game geometry, distance | Invalid shade level |
| **Sprite** | Entity rendering (snakes, food) | Transient: sprite list, Z-buffer | sprite_render_entity() | Game entity positions/states | Overflow in Z-buffer |
| **Display 3D** | Screen buffer management | Frame buffer (char + color grid) | draw_column(), draw_span() | Projector results, texture samples | Buffer overflow |
| **Render 3D** | Pipeline orchestration | Mode state | Calls sub-modules in sequence | All (orchestrates) | I/O to display |

### Single Responsibility Breakdown

**Camera Module:**
- ✅ Camera state (position, angle, FOV)
- ✅ View frustum calculations
- ✅ Ray direction generation per screen column
- ✅ World↔Camera coordinate transforms
- ❌ NOT: Raycasting (separate concern)
- ❌ NOT: Screen coordinate math (Projector's job)

**Raycaster Module:**
- ✅ DDA algorithm implementation
- ✅ Ray-wall intersection testing
- ✅ Perpendicular distance calculation (fish-eye correction)
- ✅ Wall side/texture coordinate determination
- ❌ NOT: Camera transforms (Camera's job)
- ❌ NOT: Screen rendering (Projector's job)

**Projector Module:**
- ✅ Distance → wall height calculation
- ✅ Screen-space geometry (draw_start, draw_end)
- ✅ Vertical centering (horizon)
- ✅ Texture coordinate mapping (UV generation)
- ✅ Clipping logic (visibility culling)
- ❌ NOT: Raycasting (Raycaster's job)
- ❌ NOT: Character selection (Texture's job)

**Texture Module:**
- ✅ Distance → shade level mapping
- ✅ Wall side → character pattern selection
- ✅ Texture coordinate → character lookup
- ✅ Color per shade level
- ❌ NOT: Projection math (Projector's job)
- ❌ NOT: Wall detection (Raycaster's job)

**Sprite Module:**
- ✅ World position → screen coordinates (using Camera)
- ✅ Billboard quad generation
- ✅ Z-buffer depth sorting
- ✅ Sprite visibility testing (camera-facing, screen-within-bounds)
- ❌ NOT: Character rendering (Display's job)
- ❌ NOT: Raycasting (Raycaster's job)

**Display 3D Module:**
- ✅ Frame buffer abstraction (char + color grid)
- ✅ Column and row rendering primitives
- ✅ HUD overlay rendering
- ✅ Screen refresh integration
- ❌ NOT: Geometry calculations (Projector's job)
- ❌ NOT: Character selection (Texture's job)

**Render 3D Module:**
- ✅ Per-frame orchestration (camera → raycaster → projector → texture)
- ✅ Mode management (2D vs 3D toggle)
- ✅ Pipeline integration (call sequence)
- ✅ Error handling (ray misses, invalid results)
- ❌ NOT: Individual algorithm details (delegated to sub-modules)

---

## Information Flow & Data Propagation

### Frame Rendering Pipeline (Detailed Data Flow)

```
┌─ Game Tick ─────────────────────────────────────────┐
│  GameState updated (snake moves, food eaten, etc)   │
└────────────────┬────────────────────────────────────┘
                 │
                 ▼
┌─ Camera Update ──────────────────────────────────────┐
│  Input: Active player position & direction           │
│  Output: Camera3D (position, angle, FOV)            │
│  • World coordinates → Camera space basis            │
│  • Ray generation matrix pre-computed                │
└────────────────┬────────────────────────────────────┘
                 │
         ┌───────┴──────────────────────┐
         │                              │
         ▼                              ▼
    ┌─ For Each Screen Column ────────────────────────────┐
    │  (0 to screen_width-1)                             │
    │                                                     │
    │  Input: Column index, Camera3D                    │
    │                                                     │
    │  ┌─ Camera: Ray Generation ────────────────────┐  │
    │  │ Output: float ray_dir_x, ray_dir_y         │  │
    │  │ • Map column to [-1, 1]                     │  │
    │  │ • Compute direction using basis vectors    │  │
    │  └─────────────────────────────────────────────┘  │
    │                │                                    │
    │                ▼                                    │
    │  ┌─ Raycaster: DDA Stepping ───────────────────┐  │
    │  │ Output: RayHit (distance, side, wall_x)    │  │
    │  │ • Step through grid cells                   │  │
    │  │ • Check each cell for walls                 │  │
    │  │ • Calculate perpendicular distance          │  │
    │  │ • Compute texture coordinate                │  │
    │  └─────────────────────────────────────────────┘  │
    │                │                                    │
    │                ▼                                    │
    │  ┌─ Projector: Vertical Projection ─────────────┐ │
    │  │ Output: WallProjection (draw_start/end)     │ │
    │  │ • Distance → wall height                     │ │
    │  │ • Clamp height to screen                     │ │
    │  │ • Center on horizon                          │ │
    │  │ • Generate texture step size                 │ │
    │  └─────────────────────────────────────────────┘ │
    │                │                                    │
    │                ▼                                    │
    │  ┌─ For Each Screen Row ────────────────────────┐ │
    │  │ (draw_start to draw_end)                    │ │
    │  │                                              │ │
    │  │  ┌─ Texture: Sample ──────────────────────┐ │ │
    │  │  │ Input: WallSide, wall_x, tex_v        │ │ │
    │  │  │ Output: TexelResult (char, color)    │ │ │
    │  │  │ • Shade based on distance             │ │ │
    │  │  │ • Select char per side                │ │ │
    │  │  │ • Apply color                         │ │ │
    │  │  └──────────────────────────────────────┘ │ │
    │  │         │                                   │ │
    │  │         ▼                                   │ │
    │  │  ┌─ Display: Draw Pixel ─────────────────┐ │ │
    │  │  │ Frame buffer[row][column] = TexelResult
    │  │  └──────────────────────────────────────┘ │ │
    │  └──────────────────────────────────────────┘ │
    │                                                 │
    └─────────────────────────────────────────────────┘
                 │
                 ▼
    ┌─ Sprite Rendering ──────────────────────────────┐
    │  Input: GameState entities, Camera3D, Z-buffer │
    │  Output: Additional entries in frame buffer    │
    │  • Sort sprites by depth (Z-buffer)            │
    │  • Project each sprite to screen               │
    │  • Draw if not occluded by walls               │
    └─────────────────┬────────────────────────────────┘
                      │
                      ▼
    ┌─ HUD Overlay ──────────────────────────────────┐
    │  • Score display                               │
    │  • Minimap (optional)                          │
    │  • Mode indicator                              │
    └─────────────────┬────────────────────────────────┘
                      │
                      ▼
    ┌─ Display Flush ────────────────────────────────┐
    │  • Terminal refresh (display_render_frame())   │
    │  • Frame presented to user                     │
    └────────────────────────────────────────────────┘
```

### Data Source of Truth & Consistency Boundaries

| Data | Source of Truth | Consistency Model | Synchronization |
|------|-----------------|-------------------|-----------------|
| **World Geometry (walls)** | GameState (fixed board boundaries) | Strong (immutable) | Once at startup |
| **Camera Position** | Active player position | Strong (per frame) | `camera_set_from_player()` |
| **Entity Positions** | GameState (snakes, food) | Strong (per frame) | Query API each frame |
| **Raycasting Results** | Computed per frame (transient) | None (temporary) | Discarded after frame |
| **Projected Geometry** | Computed from raycasting results | None (temporary) | Discarded after frame |
| **Frame Buffer** | Display module (transient) | None (temporary) | Cleared each frame |
| **Z-Buffer** | Sprite renderer (transient) | None (temporary) | Rebuilt each frame |

### Event-Driven Architecture Points

**Candidate Events:**
- `camera_rotated` - Camera angle changes (future: smooth interpolation)
- `wall_rendered_column` - Wall slice ready for display (opportunity: multi-threaded rendering)
- `sprite_rendered` - Entity rendered (opportunity: UI feedback)
- `frame_complete` - Rendering finished, ready for display

**Current Synchronous Model:**
- Game tick generates events (movement, food, collision)
- Rendering reads current state deterministically
- No need for async in latency-sensitive renderer

---

## Architectural Patterns & Module Interfaces

### Pattern Selection

| Pattern | Application | Rationale |
|---------|-------------|-----------|
| **Layered Architecture** | Core pipeline: Camera → Raycaster → Projector → Texture → Display | Clear separation of concerns, testability |
| **Hexagonal (Ports & Adapters)** | Game provides query API; Display abstract; 3D renderer implements | Loose coupling to existing system |
| **Screaming Architecture** | Folder structure: `src/render3d/` with files named after domain concepts | Intent obvious from file names |
| **Command-Query Separation** | Camera transforms (queries only, pure math); Rendering (commands, state-modifying) | No unexpected side effects in geometry |
| **Dependency Inversion** | Renderer depends on Display abstraction, not TTY implementation | Swappable backends |
| **Strategy Pattern** | Texture character set (UTF-8 vs ASCII via `texture_init()`) | Runtime algorithm selection |

### Module Interfaces (Public API)

**Camera Interface** (input: player state → output: rays)
```c
// Queries (pure functions, no state modification)
void camera_get_ray(const Camera3D* camera, int screen_x, int screen_width,
                    float* ray_dir_x, float* ray_dir_y);
bool camera_point_in_front(const Camera3D* camera, float x, float y);
void camera_world_to_camera(const Camera3D* camera, float world_x, float world_y,
                            float* cam_x, float* cam_z);

// Commands (state-modifying)
void camera_set_from_player(Camera3D* camera, int x, int y, SnakeDir dir);
void camera_set_angle(Camera3D* camera, float angle_radians);
```

**Raycaster Interface** (input: ray + world geometry → output: wall hit)
```c
// Query (deterministic, no side effects)
void raycast(const GameState* game, const Camera3D* camera,
             float ray_dir_x, float ray_dir_y, float max_distance,
             RayHit* hit);
```

**Projector Interface** (input: wall hit → output: screen geometry)
```c
// Queries
void projection_wall(const ProjectionParams* params, const RayHit* hit,
                    int screen_x, WallProjection* projection);
```

**Texture Interface** (input: wall geometry + distance → output: texel)
```c
// Queries
ShadeLevel texture_distance_to_shade(float distance);
void texture_sample_wall(const WallTexture* texture, WallSide side,
                        float wall_x, float tex_v, float distance,
                        TexelResult* result);
```

**Sprite Interface** (input: entity position + camera → output: screen projection)
```c
// Commands
void sprite_list_init(SpriteList* list, int max_sprites);
void sprite_list_add(SpriteList* list, int world_x, int world_y,
                    float depth, int type);
void sprite_list_sort_by_depth(SpriteList* list);

// Queries
bool sprite_get_screen_projection(const Camera3D* camera, float world_x,
                                 float world_y, int* screen_x, int* screen_y);
```

**Display 3D Interface** (input: frame buffer → output: rendered screen)
```c
// Commands
void display_3d_clear(Display3D* display);
void display_3d_set_pixel(Display3D* display, int x, int y,
                         char character, uint16_t color);
void display_3d_draw_column(Display3D* display, int x, int y_start, int y_end,
                           const char* chars, const uint16_t* colors);
void display_3d_render_hud(Display3D* display, const GameState* game);
```

**Render 3D Interface** (main integration point)
```c
// Commands (high-level orchestration)
void render_3d_init(Display3D* display, int screen_width, int screen_height);
void render_3d_frame(const GameState* game, const Camera3D* camera,
                    Display3D* display);
bool render_3d_toggle_mode(void);  // 2D ↔ 3D
```

---

## Module Boundaries & Dependency Analysis

### Dependency Graph (Acyclic)

```
                    main.c
                  /    |   \
                 /     |    \
        render_3d.c  (2D mode)
              |
         ┌────┼────┬─────────┐
         |    |    |         |
         ▼    ▼    ▼         ▼
    camera raycast projector texture sprite display_3d
         |    |      |       |        |
         └────┼──────┘       │        │
              |              │        │
              └──────────────┼────────┘
                             │
                        game.h (read-only query API)
                             │
                        display.h (abstraction)
```

**No Cycles:** ✅ All dependencies flow downward; no circular imports possible.

**Dependency Count per Module:**
- `camera.c` - 2 (types.h, math.h)
- `raycast.c` - 3 (types.h, game.h, camera.h)
- `projection.c` - 2 (raycast.h, stdint.h)
- `texture.c` - 4 (raycast.h, projection.h, display.h, stdint.h)
- `sprite.c` - 4 (camera.h, types.h, game.h, display.h)
- `display_3d.c` - 2 (display.h, types.h)
- `render_3d.c` - 7 (camera.h, raycast.h, projection.h, texture.h, sprite.h, display_3d.h, game.h)

### Stability Hierarchy (Depend on Stable Abstractions)

| Stability Rank | Module | Reason |
|----------------|--------|--------|
| 1 (Most Stable) | `types.h`, `game.h`, `display.h` | External APIs; breaking changes costly |
| 2 | `camera.c`, `raycast.c`, `projection.c` | Core algorithms; unlikely to change |
| 3 | `texture.c`, `sprite.c` | Visual details; may evolve |
| 4 | `display_3d.c` | Rendering backend; may swap out |
| 5 (Least Stable) | `render_3d.c` | Orchestration; evolves with features |

**Principle:** Stable modules should depend on nothing; unstable modules can depend on stable modules. ✅ Design follows this.

---

## Scalability & Performance Considerations

### Stateless vs. Stateful Components

| Module | State | Scope | Thread-Safe |
|--------|-------|-------|------------|
| Camera | Minimal (position, angle) | Per-frame | Read-only queries: ✅ Yes; Commands: ❌ No |
| Raycaster | None (per-ray computation) | Per-call | ✅ Yes (pure function) |
| Projector | None (per-hit computation) | Per-call | ✅ Yes (pure function) |
| Texture | Static (character patterns) | Global | ✅ Yes (read-only after init) |
| Sprite | Transient (list, Z-buffer) | Per-frame | ❌ No (frame-local) |
| Display 3D | Frame buffer | Per-frame | ❌ No (thread-unsafe writes) |

### Parallelization Opportunities

**SIMD (Single Instruction, Multiple Data):**
- Camera ray generation for multiple columns simultaneously
- Raycasting DDA steps (limited: dependency chain)
- Distance shading calculations (embarrassingly parallel)

**Multi-Threading:**
- Column-based rendering: split columns across threads, synchronize at display phase
- Sprite rendering: independent of wall rendering (only Z-buffer dependency)
- HUD rendering: can happen in parallel with wall rendering (separate buffer regions)

**Current Recommendation:** Single-threaded initially (simplicity, cache locality). Revisit if profiling shows bottlenecks (unlikely on terminals, ~60 FPS target).

### Read-Heavy vs. Write-Heavy

| Operation | Type | Volume | Optimization |
|-----------|------|--------|--------------|
| Ray generation | Read | O(screen_width) | Pre-compute camera basis (one-time) |
| Wall detection | Read | O(avg_ray_length * screen_width) | Early exit on hit; spatial indexing for large boards |
| Distance shading | Read | O(screen_width * screen_height) | Quantize distances to avoid floating-point ops |
| Frame buffer writes | Write | O(screen_width * screen_height) | Use column-oriented writes for cache efficiency |

### Memory Layout Optimization

**Current Frame Buffer Design (Suboptimal):**
```c
struct { char c; uint16_t color; } frame[height][width];  // Row-major
```
Problem: Poor cache locality for column-by-column writes.

**Optimized Design (Column-Oriented):**
```c
struct {
    char chars[height * width];       // Column-major: chars[col * height + row]
    uint16_t colors[height * width];  // Separate arrays for better packing
} frame_buffer;
```
Benefit: Sequential writes per column = sequential memory access = better cache.

---

## Integration with Existing Architecture

### Process Separation & IPC Protocol

**Architecture:** The 3D renderer runs as a **completely separate process** (`snake_3d_renderer`) in an SDL window. The main game process (`snake`) continues to run in the terminal. Communication occurs via IPC.

**Game Process Responsibilities:**
- Game logic (snake movement, food, collision)
- 2D terminal rendering (existing behavior)
- IPC Server: Publishes game state to renderer at **60 Hz**

**Renderer Process Responsibilities:**
- 3D raycasting pipeline
- SDL graphics rendering
- Receives game state updates via IPC client
- Independent frame rate (can differ from game)

### IPC Communication Protocol

**Message Format:** JSON over Unix socket (or TCP for future network play)

**Game State Message (sent at 60 Hz):**
```json
{
  "type": "game_state",
  "frame": 12345,
  "timestamp": 1702473600.123,
  "player": {
    "x": 10.5,
    "y": 8.5,
    "direction": 0,
    "angle": 1.5708
  },
  "snake": [
    {"x": 10, "y": 8},
    {"x": 10, "y": 9},
    {"x": 10, "y": 10}
  ],
  "food": [
    {"x": 20, "y": 15},
    {"x": 5, "y": 12}
  ],
  "obstacles": [
    {"x": 15, "y": 10, "type": "wall"}
  ],
  "board": {"width": 40, "height": 20}
}
```

**Renderer Response (optional):**
```json
{
  "type": "renderer_status",
  "frame_time_ms": 14.2,
  "fps": 60.0
}
```

### IPC Layer Implementation

**Location:** `src/ipc/` directory

**Files:**
- `ipc.h` / `ipc.c` - Core IPC API (socket creation, send, receive)
- `game_state_protocol.h` / `game_state_protocol.c` - Message serialization (JSON via jansson library)
- `ipc_server.h` / `ipc_server.c` - Game-side server (publishes state)
- `ipc_client.h` / `ipc_client.c` - Renderer-side client (receives state)

**API Example - Game Side (Server):**
```c
// In main game loop
IPCServer* ipc_server = ipc_server_create("/tmp/snake_game.sock");

// Each frame:
ipc_server_publish(&ipc_server, &game_state);

// On shutdown:
ipc_server_destroy(&ipc_server);
```

**API Example - Renderer Side (Client):**
```c
// In renderer startup
IPCClient* ipc_client = ipc_client_connect("/tmp/snake_game.sock");

// Each frame:
GameState state;
if (ipc_client_receive(&ipc_client, &state)) {
    // Render with updated state
    render_frame_3d(&state);
}

// On shutdown:
ipc_client_disconnect(&ipc_client);
```

### Build System Changes

**Two Build Targets:**
1. `snake` - Original game executable (terminal 2D rendering)
   - Links existing game code + IPC server
   - Minimal changes to main game loop

2. `snake_3d_renderer` - New renderer executable (SDL window)
   - Links 3D rendering code + IPC client
   - Runs standalone; connects to game via IPC
   - Can be started/stopped independently

**Makefile Rules:**
```makefile
# Game target (existing)
snake: $(GAME_OBJS) $(IPC_SERVER_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

# Renderer target (new)
snake_3d_renderer: $(RENDERER_OBJS) $(IPC_CLIENT_OBJS) $(SDL_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) $(SDL_LDFLAGS)
```

### Running Both Processes

**Terminal 1 - Game:**
```bash
./snake
```

**Terminal 2 - Renderer (starts automatically or manually):**
```bash
./snake_3d_renderer
```

Renderer waits for game to publish state; displays nothing until connection established.

---

## Configuration & Feature Toggles

### Compile-Time Configuration

```c
// render3d_config.h
#define RENDER_3D_ENABLED          1        // Feature flag
#define RENDER_3D_DEFAULT_FOV      60.0f    // Degrees
#define RENDER_3D_MAX_RAY_DISTANCE 50.0f    // Grid units
#define RENDER_3D_USE_UTF8         1        // Character set
#define RENDER_3D_COLUMN_SKIP      1        // 1=every column, 2=skip, etc.
```

### Runtime Configuration

```c
struct {
    float fov;              // 60-120 degrees
    float max_distance;     // 10-100 units
    bool use_utf8;          // ASCII vs UTF-8 mode
    int column_skip;        // Render quality
    bool show_minimap;      // HUD option
} render_3d_config;

void render_3d_config_set(const Render3DConfig* config);
```

---

## Testing & Validation Strategy

### Unit Tests per Module

**Camera Module:**
- ✅ FOV computation correct
- ✅ Direction vectors normalized
- ✅ Ray generation produces correct spread
- ✅ World-to-camera transform invertible
- ✅ Angle wrapping works (0, 2π)

**Raycaster Module:**
- ✅ DDA grid traversal correct
- ✅ Wall detection returns correct side
- ✅ Perpendicular distance accurate
- ✅ Texture coordinate in [0, 1]
- ✅ No infinite loops (max_distance works)

**Projector Module:**
- ✅ Distance → height monotonic
- ✅ Draw bounds within screen
- ✅ Horizon centered
- ✅ Clipping doesn't produce negative heights

**IPC Protocol Tests (NEW):**
- ✅ Game state serialization produces valid JSON
- ✅ Deserializer handles all message types
- ✅ Socket connection/disconnection graceful
- ✅ Message loss doesn't crash renderer
- ✅ Timestamp ordering correct across messages
- ✅ Renderer handles stale state (frame skips)
- ✅ 60 Hz publication rate maintained
- ✅ Large game boards don't overflow message buffer

**Integration Tests:**
- ✅ Full frame renders without crashes
- ✅ IPC communication established before rendering
- ✅ Renderer waits gracefully for first game state
- ✅ Player movement reflected in 3D view within 1 frame
- ✅ Food/snakes visible as sprites at correct positions
- ✅ No frame tears or artifacts in SDL window
- ✅ Both processes can be restarted independently
- ✅ Game continues while renderer is offline
- ✅ Renderer reconnects if game restarts

### Performance Targets

| Metric | Target | Rationale |
|--------|--------|----------| 
| **Game Process** | | |
| Frame Time | < 16.7 ms (60 FPS) | Terminal refresh rate |
| IPC Serialization | < 1 ms | JSON serialization |
| IPC Publish | < 1 ms | Socket write |
| **Renderer Process** | | |
| IPC Receive | < 1 ms | Socket read |
| Ray Casting | < 10 ms | 80% of render budget |
| Projection | < 2 ms | Simple math |
| Texture Sampling | < 2 ms | Lookup tables |
| Sprite Rendering | < 1 ms | Few entities |
| SDL Present | < 2 ms | GPU sync |
| **Inter-Process** | | |
| Message Latency | < 5 ms | IPC + network |
| State Freshness | < 16 ms | One frame skew acceptable |

### Profiling Instrumentation

```c
// Per-frame timing - Game side
#ifdef RENDER_3D_PROFILE
    uint64_t t_ipc_serialize = profile_end() - profile_start();
    uint64_t t_ipc_send = ...
    // Print to stderr
#endif

// Per-frame timing - Renderer side
#ifdef RENDER_3D_PROFILE
    uint64_t t_ipc_recv = profile_end() - profile_start();
    uint64_t t_raycast = ...
    // Print to SDL overlay or stderr
#endif
```

---

# PASS 2: ALGORITHM & DATA STRUCTURE OPTIMIZATION

## Complexity Analysis & Optimization Roadmap

### Current Algorithms: Time Complexity

| Operation | Algorithm | Time Complexity | Space | Notes |
|-----------|-----------|-----------------|-------|-------|
| Ray generation | Direct formula | O(1) per ray, O(w) per frame | O(1) | Can be vectorized |
| Raycasting (DDA) | Grid stepping | O(d) where d = distance in cells | O(1) | Already optimal for grid |
| Wall projection | Arithmetic | O(1) per ray | O(1) | Trivial |
| Texture sampling | Lookup | O(1) | O(1) | Hash table could be overkill |
| Sprite rendering | Depth sort | O(s log s) where s = # sprites | O(s) | Radix sort option if s >> |
| Frame buffer write | Direct access | O(w*h) per frame | O(w*h) | Column-major for cache |
| Total per frame | Dominated by raycasting | **O(w*d)** | O(w*h) | w=width, d=avg ray distance, h=height |

### Bottleneck Identification

**Primary Bottleneck:** Raycasting (70-80% of frame time)
- Per-column: ~5-10 DDA steps average (grid cells crossed)
- 80 columns × 10 steps = 800 ray-grid checks per frame
- Each check: 1 array lookup (grid cell)

**Secondary Bottleneck:** Texture sampling (10-15%)
- Per-pixel: distance shading calculation
- 80 × 30 = 2400 pixels per frame

**Tertiary Bottleneck:** Display I/O (5-10%)
- Terminal write latency

### Optimization Techniques

#### 1. Raycasting Optimizations

**a) Early Ray Termination (Already in design)**
```c
// If ray travels max_distance, stop
// Avoids unnecessary grid steps
// Saves ~30-40% of ray casts that hit far walls
```

**b) Vertical Ray Grouping (New)**
```c
// Rays in similar direction can reuse DDA state
// Example: rays for columns 40-42 might share first 5 DDA steps
// Opportunity: SIMD vectorize 4-8 rays simultaneously
// Complexity: Requires synchronized stepping
// Benefit: 2-3x speedup for ray batches
```

**c) Spatial Index for Walls (Future, if needed)**
```c
// Current: Check each grid cell, O(d)
// With index: Query possible hits in ray direction, O(log n)
// Current design adequate for Snake's 40x20 board
// Revisit if board scales to 200x200+
```

**d) Adaptive Ray Resolution (New)**
```c
// Don't cast rays for every column when board distant
// Skip columns at distance > 20 units
// Fill skipped columns via interpolation
// Benefit: 30-40% fewer rays cast at distance
// Trade-off: Slight visual quality loss (imperceptible at terminal resolution)
```

**Code Impact (Medium effort):**
```c
// render_3d.c - main loop
for (int col = 0; col < screen_width; col += skip_factor) {
    // Cast ray
    raycast(..., &hit);
    
    // Interpolate if skip_factor > 1
    if (skip_factor > 1 && col > 0) {
        interpolate_between_rays(prev_hit, hit, skip_factor, results);
    }
}
```

#### 2. Texture Sampling Optimizations

**a) Shade Level Quantization (Already in design)**
```c
// Distance to shade level: continuous → discrete
// 5-10 levels instead of float gradient
// Opportunity: Use integer math instead of float comparison
```

**b) Character Set Compression (New)**
```c
// Current: Full UTF-8 character set
// Optimized: Reduced set (most common chars only)
// Use bit-packed lookup: 3 bits (0-7) maps to char
// Saves memory, improves cache locality

// Old
char shade_chars[SHADE_COUNT][10] = { "█▓▒░...", ... };
// Lookup: shade_chars[level][pos]  // String indexing

// New
typedef uint8_t CharPacked;  // 3 bits = index, 5 bits spare
CharPacked shade_chars_packed[SHADE_COUNT] = { 0b00101010, ... };
char decode_char(CharPacked p) { return CHAR_TABLE[p & 0x07]; }
// Lookup: decode_char(shade_chars_packed[level])  // 1 array access
```

**Benefit:** 1 memory access instead of string lookup; better cache.

**c) Shade Computation via Lookup Table (New)**
```c
// Current: Floating-point comparisons
// ShadeLevel shade = texture_distance_to_shade(distance);
// if (distance < 3.0f) return SHADE_NEAR;
// else if (distance < 7.0f) return SHADE_MID;
// ...

// Optimized: Pre-compute thresholds, use integer arithmetic
typedef struct {
    float thresholds[SHADE_COUNT];  // Pre-computed: 0-3, 3-7, 7-12, 12+
} ShadeLUT;

ShadeLevel shade = 0;
while (shade < SHADE_COUNT && distance >= lut->thresholds[shade]) {
    shade++;
}
// O(SHADE_COUNT) = O(5-10) instead of floating-point checks
```

**Or, simpler:** Use integer distance (multiply by 10, avoid float):
```c
int dist_int = (int)(distance * 10);  // 0-300 range
int shade = (dist_int < 30) ? SHADE_NEAR
          : (dist_int < 70) ? SHADE_MID
          : (dist_int < 120) ? SHADE_FAR
          : SHADE_VERY_FAR;  // Branch-free option: use LUT
```

#### 3. Data Structure Optimizations

**a) Frame Buffer Layout**

Current (Row-Major):
```c
struct {
    char chars[height][width];
    uint16_t colors[height][width];
} frame;
```
Problem: Writing column j = scattered memory access (bad for column-by-column rendering).

Optimized (Column-Major, SoA - Structure of Arrays):
```c
struct {
    char* chars;          // chars[col * height + row]
    uint16_t* colors;     // colors[col * height + row]
    int width, height;
} frame;

void frame_set_pixel(Frame* f, int col, int row, char c, uint16_t color) {
    int idx = col * f->height + row;
    f->chars[idx] = c;
    f->colors[idx] = color;
}
```
Benefit: Sequential writes per column = prefetcher-friendly = ~2x faster writes.

**b) Sprite List (Z-Buffer)**

Current (Simple array):
```c
Sprite sprites[MAX_SPRITES];  // Unsorted
// Sort before rendering: O(s log s)
```

Optimized (Radix sort for depth):
```c
// Depth ranges: [0, max_distance] → quantize to 256 bins
// Radix sort: O(s + 256) = O(s) for typical s < 256 sprites
// Benefit: 10-100x faster for small s (Snake: typically 5-10 entities)
```

**c) Ray Direction Cache**

Current (Computed per ray):
```c
for (int col = 0; col < width; col++) {
    float ray_x = camera.dir_x + camera.plane_x * (2.0f * col / width - 1.0f);
    float ray_y = camera.dir_y + camera.plane_y * (2.0f * col / width - 1.0f);
    raycast(..., ray_x, ray_y);
}
```
Problem: Float division in loop (expensive).

Optimized (Pre-compute step):
```c
struct {
    float ray_dx[MAX_SCREEN_WIDTH];  // Pre-computed for each column
    float ray_dy[MAX_SCREEN_WIDTH];
    bool dirty;                       // Set when camera moves
} ray_cache;

void camera_invalidate_ray_cache(void) {
    ray_cache.dirty = true;
}

void ensure_ray_cache_valid(Camera3D* cam) {
    if (ray_cache.dirty) {
        for (int col = 0; col < screen_width; col++) {
            camera_get_ray(cam, col, screen_width, 
                          &ray_cache.ray_dx[col], &ray_cache.ray_dy[col]);
        }
        ray_cache.dirty = false;
    }
}
```
Benefit: Compute ray directions once per camera change, not per frame; saves ~10% per frame.

---

## Memory Optimization

### Memory Layout & Allocation Strategy

**Static Allocation (Preferred):**
```c
// All buffers allocated once at init, reused per frame
struct {
    Camera3D camera;
    RayHit rays[MAX_SCREEN_WIDTH];      // Reusable ray results
    WallProjection projections[MAX_SCREEN_WIDTH];
    TexelResult texels[MAX_SCREEN_WIDTH * MAX_SCREEN_HEIGHT];
    Sprite sprites[MAX_SPRITES];
    char frame_chars[MAX_SCREEN_WIDTH * MAX_SCREEN_HEIGHT];
    uint16_t frame_colors[MAX_SCREEN_WIDTH * MAX_SCREEN_HEIGHT];
} render_3d_state;
```
Benefit: No allocations in frame loop; predictable memory (stack or pre-allocated heap).

**Stack vs Heap:**
- Camera: **Stack** (240 bytes) - frequently accessed
- Ray results: **Stack** (per-function locals) or **Static** (frame-global)
- Frame buffer: **Heap** (or very large static array if terminal size known)

**Recommended Structure:**
```c
// render_3d_state.h - Frame-global state
extern Camera3D g_camera;
extern RayHit g_rays[MAX_SCREEN_WIDTH];
extern Display3D g_display;

// Allocation happens once in render_3d_init()
void render_3d_init(int screen_width, int screen_height) {
    g_display.chars = malloc(screen_width * screen_height);
    g_display.colors = malloc(screen_width * screen_height * sizeof(uint16_t));
}
```

### Cache Optimization

**Cache Line Size:** Typically 64 bytes

**Data Structure Alignment:**
```c
// Unoptimized (Cache thrashing)
struct {
    float angle;           // 4 bytes → cache line 1
    float dir_x;           // 4 bytes → cache line 1 (wasted)
    float dir_y;           // 4 bytes → cache line 1 (wasted)
    float plane_x;         // 4 bytes → cache line 1 (wasted)
    float plane_y;         // 4 bytes → cache line 1 (wasted)
    float x;               // 4 bytes → cache line 1 (wasted)
    float y;               // 4 bytes → cache line 1 (wasted)
    float projection_dist; // 4 bytes → cache line 2
    float fov;             // 4 bytes → cache line 2
    // Total: 36 bytes, fits 1-2 cache lines ✓ OK
} Camera3D;
```
This is already decent. Verify with `-std=c11` alignment attributes if needed.

**Hot Path Data:**
- Keep `Camera3D` in L1 cache (frequently read)
- Keep `projection_params` in L1 cache
- Frame buffer in L2/L3 (large, sequential access)

---

## Parallelization Strategy

### Current State: Single-Threaded
Rationale: Simple, high performance on modern CPUs (out-of-order execution), no synchronization overhead.

### Future Parallelization (If Needed)

**Phase 1: Column Parallelization (SIMD)**
```c
// Pseudo-code: Vectorize 4 rays simultaneously
__m256 ray_x = _mm256_set_ps(camera.dir_x + camera.plane_x * t[0], ..., t[3]);
__m256 ray_y = _mm256_set_ps(camera.dir_y + camera.plane_y * t[0], ..., t[3]);
// 4 raycasts in parallel via SIMD
raycast_simd(&ray_x, &ray_y, hits);
```
Benefit: 2-4x speedup for raycasting.
Effort: Medium (requires SIMD intrinsics knowledge).

**Phase 2: Multi-Thread Column Ranges**
```c
// Thread pool: 4 threads, each handles 20 columns (80 total)
#pragma omp parallel for
for (int col = 0; col < screen_width; col++) {
    raycast(...);
    project(...);
    texture_sample(...);
}
```
Benefit: 2-4x speedup (if CPU cores available).
Effort: Low (uses OpenMP, portable).
Caveat: Frame buffer writes must be synchronized (column-based = no contention).

**Current Recommendation:** Stay single-threaded. Terminal render speed (~60 FPS) doesn't require parallelization. Revisit only if profiling shows > 16 ms frames.

---

## Algorithmic Improvements

### Raycasting Enhancements

**Current: DDA with Perpendicular Distance**
- Accurate, standard approach
- ~O(d) where d = distance in grid cells
- No visible issues

**Alternative: Supersampling (for visual quality)**
- Cast 2-4 rays per column, average results
- Reduces aliasing artifacts
- Cost: 2-4x more raycasts
- Benefit: Smoother wall boundaries (terminal limitation, maybe not worth it)

**Alternative: Ray Coherence Optimization**
- Rays differing by angle < threshold share initial DDA steps
- Example: Batch rays in 5-degree cones
- Cost: Complex implementation, state management
- Benefit: 30-40% fewer ray iterations
- Verdict: Only if profiling proves raycasting is true bottleneck

### Projection Enhancements

**Current: Linear wall height scaling**
```c
wall_height = screen_height / distance;
```

**Alternative: Non-Linear (Perspective Correction)**
- Apply inverse transformation for better depth perception
- Too subtle for ASCII art; skip for now

### Texture Enhancements

**Current: Simple shade bands (0-3, 3-7, 7-12, 13+)**
- 4 levels
- Adequate for ASCII

**Alternative: Dithering (for smooth gradients)**
- Combine multiple characters to simulate intermediate shades
- Example: Mix "░" (25% filled) and "▒" (50% filled) for 37.5%
- Cost: More complex lookup
- Benefit: Smoother distance gradients
- Verdict: Nice-to-have; implement in Phase 2

---

## Performance Targets & Profiling Plan

### Frame Budget (60 FPS = 16.7 ms per frame)

| Phase | Component | Target | Margin |
|-------|-----------|--------|--------|
| Setup | Camera update | 0.5 ms | 0.5x |
| Main | Raycasting (80 cols × 7 cells avg) | 8.0 ms | 2x |
| Main | Projection | 1.0 ms | 2x |
| Main | Texture sampling | 2.0 ms | 2x |
| Sprite | Rendering | 1.0 ms | 2x |
| Display | HUD, frame buffer management | 1.0 ms | 2x |
| I/O | Terminal flush | 2.0 ms | 2x (may stall) |
| **Total** | **Frame render** | **15.5 ms** | **1.08x** |

Headroom is tight; focus optimizations on raycasting first.

### Instrumentation Code

```c
#include <time.h>

struct ProfileData {
    uint64_t t_raycast;
    uint64_t t_project;
    uint64_t t_texture;
    uint64_t t_sprite;
    uint64_t t_display;
    uint64_t t_total;
} profile;

#define PROFILE_START(name) uint64_t t_##name = get_ns()
#define PROFILE_END(name) profile.t_##name = get_ns() - t_##name

uint64_t get_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

void profile_print(void) {
    fprintf(stderr, "Raycast: %llu µs, Proj: %llu µs, Tex: %llu µs, "
                    "Sprite: %llu µs, Total: %llu µs\n",
            profile.t_raycast / 1000, profile.t_project / 1000, 
            profile.t_texture / 1000, profile.t_sprite / 1000, 
            profile.t_total / 1000);
}
```

---

## Data Structure Reference

### Final Optimized Layouts

**Camera3D** (Optimized for ray generation)
```c
typedef struct {
    // Core state (always accessed together)
    float x, y;              // Position (8 bytes)
    float angle;             // Viewing angle (4 bytes)
    float fov;               // Field of view (4 bytes)
    
    // Pre-computed from angle (cache-friendly)
    float dir_x, dir_y;      // Direction (8 bytes)
    float plane_x, plane_y;  // Camera plane (8 bytes)
    float projection_dist;   // For reference (4 bytes)
    
    // Total: 36 bytes (fits in 1 cache line)
} Camera3D;
```

**RayHit** (Optimized for raycasting results)
```c
typedef struct {
    float distance;          // Perpendicular distance (4 bytes)
    float wall_x;            // Texture coordinate (4 bytes)
    int map_x, map_y;        // Grid cell (8 bytes)
    WallSide side;           // uint8_t (1 byte)
    bool hit;                // (1 byte)
    uint8_t padding[6];      // Alignment (6 bytes)
    // Total: 24 bytes
} RayHit;
```

**WallProjection** (Minimized)
```c
typedef struct {
    int draw_start, draw_end;     // Screen span (8 bytes)
    int wall_height;              // Height in pixels (4 bytes)
    float tex_step, tex_pos;      // Texture mapping (8 bytes)
    // Total: 20 bytes
} WallProjection;
```

**Frame Buffer** (SDL Pixel Buffer - Updated for Renderer Process)
```c
// SDL-based rendering uses pixel buffers instead of character arrays
typedef struct {
    SDL_Texture* texture;        // SDL texture for rendering
    uint32_t* pixels;            // Pixel array [col * height + row]
    int width, height;
    int pitch;                   // Bytes per scanline
    SDL_Renderer* renderer;      // SDL renderer context
} Display3D;

void display_3d_set_pixel(Display3D* disp, int col, int row, uint32_t color) {
    int idx = col * disp->height + row;
    disp->pixels[idx] = color;
}

void display_3d_present(Display3D* disp) {
    SDL_UpdateTexture(disp->texture, NULL, disp->pixels, disp->pitch);
    SDL_RenderCopy(disp->renderer, disp->texture, NULL, NULL);
    SDL_RenderPresent(disp->renderer);
}
```

---

## Summary Table: Optimization Checklist

| Optimization | Category | Effort | Benefit | Priority | Status |
|--------------|----------|--------|---------|----------|--------|
| Column-major frame buffer | Memory layout | Low | 2x display writes | **Phase 1** | ✅ Include in design |
| Ray direction cache | Computation | Low | 10% speedup | **Phase 1** | ✅ Include in design |
| Integer shade computation | Computation | Low | 5% speedup | **Phase 1** | ✅ Include in design |
| Adaptive ray resolution | Algorithm | Medium | 30-40% rays | **Phase 2** | ⏳ Optional |
| SIMD raycasting | Parallelization | High | 3-4x speedup | **Phase 3** | ⏳ Only if bottleneck |
| Multi-thread columns | Parallelization | Medium | 2-4x speedup | **Phase 3** | ⏳ Only if bottleneck |
| Radix-sort sprites | Algorithm | Low | 10x sort | **Phase 2** | ⏳ Nice-to-have |
| Dithering textures | Visual | High | Better gradients | **Phase 2** | ⏳ Polish feature |

---

# INTEGRATION CHECKLIST

- [ ] **Architecture** - Module boundaries, interfaces, dependencies finalized
- [ ] **Pass 1 Output** - Architectural diagram, responsibility matrix, bounded contexts created
- [ ] **Pass 2 Output** - Complexity analysis, data structure optimizations documented
- [ ] **Performance Targets** - Frame budget (15.5 ms), profiling instrumentation defined
- [ ] **Memory Strategy** - Static allocation, cache optimization planned
- [ ] **Testing Plan** - Unit tests per module, integration tests, performance targets specified
- [ ] **Configuration** - Compile-time flags, runtime settings designed
- [ ] **Code Review** - Peer review of architecture against principles
- [ ] **Prototype** - Implement Camera module first as proof-of-concept
- [ ] **Iteration** - Build, profile, optimize per phase

---

# NEXT STEPS (IMPLEMENTATION ROADMAP)

## Phase 0: IPC Infrastructure (Week 1)

### Create IPC Layer
1. Create `src/ipc/` directory
2. Create header files:
   - `ipc.h` - Core IPC API (sockets, send/receive)
   - `game_state_protocol.h` - Message serialization (using jansson)
   - `ipc_server.h` - Game-side server
   - `ipc_client.h` - Renderer-side client
3. Implement `.c` files with:
   - Unix socket creation and management
   - JSON serialization/deserialization of GameState
   - Circular buffer for message queueing
   - Non-blocking I/O for both sender and receiver
4. Unit tests for protocol and socket operations
5. Update `Makefile` with IPC compilation rules and two targets: `snake` and `snake_3d_renderer`

### Create Renderer Directory Structure
1. Create `src/render/` subdirectories (reorganize existing render files)
2. Create `src/render_3d/` directory for 3D-specific code
3. Create new main file `main_3d.c` as renderer entry point

## Phase 1: Core 3D Pipeline + IPC Integration (Weeks 2-4)

### Game Process Integration
1. Modify `main.c` to initialize IPC server
2. Add IPC state publishing to game loop (60 Hz)
3. Serialize GameState to JSON via game_state_protocol
4. Handle IPC errors gracefully (game continues if renderer offline)
5. Unit tests for game-side IPC

### Renderer Process Core
1. **Create `main_3d.c`** - Renderer entry point
   - Initialize SDL window and renderer
   - Connect to IPC server
   - Main loop: receive state → render → present
2. **Implement Camera** (`camera.c`) - 2 hours
3. **Implement Raycaster** (`raycast.c`) - 4 hours
4. **Implement Projector** (`projection.c`) - 2 hours
5. **Implement Display3D** with SDL (`display_3d.c`) - 2 hours
6. **Unit tests** for each module - 2 hours

## Phase 2: Visual Quality & Polish (Weeks 5-6)
1. **Implement Texture** (`texture.c`) - 2 hours
   - Adapt character-based shading to SDL pixel colors
2. **Implement Sprite** (`sprite.c`) - 3 hours
   - Render snake/food as colored shapes
3. **Rendering optimization** - 2 hours
   - Profile both processes independently
   - Optimize hot paths (raycasting, projection)
4. **Visual testing & refinement** - 3 hours
   - Verify 3D view matches game state
   - Ensure latency is imperceptible

## Phase 3: Advanced Features & Deployment (Week 7+)

### Robustness
1. **Process lifecycle** - Both processes handle crashes gracefully
2. **Reconnection logic** - Renderer reconnects if game restarts
3. **Message buffering** - Handle bursts of updates
4. **Large board support** - Message size scaling

### Optional Enhancements
1. **Network IPC** - Replace Unix socket with TCP for remote rendering
2. **Adaptive ray resolution** (if renderer FPS < 60)
3. **Renderer-side interpolation** - Smooth movement between game updates
4. **Performance profiling** - Separate per-process instrumentation

### Build System
1. Update `Makefile`:
   - `make` builds both `snake` and `snake_3d_renderer`
   - `make snake` - game only
   - `make snake_3d_renderer` - renderer only
   - `make clean` - remove both
2. Create launch script to start both processes:
   ```bash
   #!/bin/bash
   ./snake &
   sleep 0.5
   ./snake_3d_renderer
   ```

## Build Targets Summary

**Target 1: `snake`** (Existing 2D game)
- Source: `main.c`, `src/core/`, `src/input/`, `src/net/`, `src/persist/`, `src/platform/`, `src/render/`, `src/utils/`, `src/ipc/ipc_server.c`
- Output: `./snake` executable
- Behavior: Runs in terminal, publishes game state via IPC

**Target 2: `snake_3d_renderer`** (New 3D renderer)
- Source: `main_3d.c`, `src/render_3d/`, `src/ipc/ipc_client.c`
- Dependencies: SDL2, jansson
- Output: `./snake_3d_renderer` executable
- Behavior: Runs in SDL window, receives game state via IPC, renders 3D view

**Linking:**
- Game: Uses jansson for IPC server (lightweight)
- Renderer: Uses jansson + SDL2 (graphics-heavy)

---

**END OF REFINED ARCHITECTURE & OPTIMIZATION PLAN**
