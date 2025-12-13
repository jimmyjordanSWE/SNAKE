# Pseudo-3D Renderer Implementation Plan
## Overview & Architecture

**Goal:** Implement a Doom-style pseudo-3D raycasting renderer for the Snake game that creates a first-person perspective view from one player's position.

**Rendering Strategy:** Raycasting (similar to Wolfenstein 3D / early Doom)
- Cast rays from camera position to determine wall distances
- Project walls vertically based on distance
- Draw floors/ceilings with gradients
- Render other entities (snakes, food) as sprites

## Visual Concept

### Current View (Top-Down 2D)
```
┌────────────────────────┐
│ # # # # # # # # # # # │  # = wall
│ #   F               # │  S = snake
│ #   S S S           # │  F = food
│ #       S     F     # │
│ #       S           # │
│ # # # # # # # # # # # │
└────────────────────────┘
```

### New View (Pseudo-3D First-Person)
```
┌────────────────────────────────────────┐
│            ...ceiling...               │  Upper: ceiling (fade)
├────────────────────────────────────────┤
│ ▓▓░░│  ║  │░░▓▓  sprite  ▓▓░░│  ║  │ │  Middle: walls (textured)
│ ▓▓░░│  ║  │░░▓▓    ▓▓     ▓▓░░│  ║  │ │  & entities (sprites)
├────────────────────────────────────────┤
│            ...floor...                 │  Lower: floor (fade)
└────────────────────────────────────────┘
```

## Core Components

### 1. Camera System (`src/render3d/camera.c`)
- Player position (x, y)
- Viewing angle (theta)
- Field of view (FOV)
- Projection plane calculations

### 2. Raycaster (`src/render3d/raycast.c`)
- DDA (Digital Differential Analyzer) algorithm
- Ray-wall intersection detection
- Distance calculations (with fish-eye correction)
- Hit point and wall side determination

### 3. 3D Projection (`src/render3d/projection.c`)
- Convert ray distances to wall heights
- Handle vertical centering
- Clipping and depth buffering

### 4. Texture/Pattern System (`src/render3d/texture.c`)
- ASCII/UTF-8 character patterns for walls
- Shading based on distance
- Different patterns for different wall types
- Simple dithering for depth perception

### 5. Sprite Rendering (`src/render3d/sprite.c`)
- Transform world coordinates to screen space
- Z-buffering (depth sorting)
- Render snakes and food as billboards
- Scaling based on distance

### 6. 3D Display Backend (`src/render/display_3d.c`)
- Frame buffer for character + color
- Column-based rendering
- Vertical line drawing primitives
- Integration with existing display abstraction

### 7. Mode Switcher (`src/render/render_3d.c`)
- Toggle between 2D and 3D views
- Separate render path for 3D mode
- Camera follows active player
- HUD overlay in 3D mode

## Technical Specifications

### Resolution & Performance
- Terminal columns: existing terminal width (e.g., 80-120 cols)
- Render columns: 1 ray per column (or skip columns for performance)
- Vertical resolution: terminal height (e.g., 24-40 rows)
- Target: 60 FPS on modern terminals

### Coordinate System
```
World Space (Game):     Camera Space:        Screen Space:
      y                      z (depth)            y
      ↑                      ↑                    ↑
      │                      │                    │
      └──→ x                 └──→ x               └──→ x

Board: 0,0 at top-left   Camera: look direction   Terminal: 0,0 at top-left
```

### Distance Shading
```
Near (0-3 units):   █ ▓ ▒ (bright, full chars)
Mid (4-7 units):    ░ ▒ ╱ ╲ (medium chars)
Far (8-12 units):   · ˙ ` (dim chars)
Very Far (13+):     (empty/fade to background)
```

## Rendering Pipeline

```
1. Game Tick (existing)
   ↓
2. Determine Camera Position & Angle
   ↓
3. For Each Screen Column:
   a. Cast Ray
   b. Calculate Wall Distance
   c. Determine Wall Height
   d. Apply Texture/Shading
   ↓
4. Sort Sprites by Distance
   ↓
5. Render Sprites Over Walls (with Z-buffer)
   ↓
6. Overlay HUD (score, minimap)
   ↓
7. Present Frame
```

## File Implementation Order

Implementation split into **7 modular files**, each independently implementable:

1. **File 1:** Camera System (`camera.c` + `camera.h`)
2. **File 2:** Raycasting Core (`raycast.c` + `raycast.h`)
3. **File 3:** 3D Projection Math (`projection.c` + `projection.h`)
4. **File 4:** Texture/Shading (`texture.c` + `texture.h`)
5. **File 5:** Sprite Rendering (`sprite.c` + `sprite.h`)
6. **File 6:** 3D Display Backend (`display_3d.c` + integration)
7. **File 7:** Mode Integration (`render_3d.c` + toggle system)

Each file has:
- Clear inputs/outputs
- Minimal dependencies
- Self-contained logic
- Unit-testable functions

## Integration Points

### With Existing Code
- `GameState`: Read-only access for world data
- `render.h`: New function `render_draw_3d()` alongside `render_draw()`
- `input.h`: New key binding (e.g., 'V' for view toggle)
- `display.h`: Extension with 3D display context

### Configuration
- Enable/disable 3D mode at startup
- FOV adjustment (60-120 degrees)
- Rendering quality settings (column skip, shading detail)

## Development Strategy

### Phase 1: Core Infrastructure (Files 1-3)
- Implement camera, raycasting, and projection
- Render simple wall silhouettes
- Verify math correctness with test cases

### Phase 2: Visual Quality (Files 4-5)
- Add textures and shading
- Implement sprite rendering
- Polish distance effects

### Phase 3: Integration (Files 6-7)
- Connect to display system
- Add mode switching
- HUD and minimap overlay

### Phase 4: Polish & Optimization
- Performance profiling
- Character set optimization for terminals
- Player feedback and tweaks

## Testing Strategy

Each module includes:
- Unit tests for math functions (camera transforms, ray intersections)
- Visual tests with known scenarios
- Performance benchmarks
- Terminal compatibility checks

## Next Steps

Proceed to individual implementation files:
- `96_file1_camera.md` - Camera system detailed spec
- `97_file2_raycast.md` - Raycasting algorithm spec
- `98_file3_projection.md` - Projection math spec
- `99_file4_texture.md` - Texture system spec
- `100_file5_sprite.md` - Sprite rendering spec
- `101_file6_display3d.md` - 3D display backend spec
- `102_file7_integration.md` - Mode integration spec
