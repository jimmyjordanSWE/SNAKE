# Pseudo-3D Renderer: Quick Reference & Implementation Guide

## Overview

This is a complete implementation plan for adding a Doom-style pseudo-3D renderer to the Snake game. The project is split into 7 independent, manageable files that can be implemented by an LLM with minimal context.

## File Structure

```
design_docs/
├── 95_pseudo3d_renderer_overview.md    ← Start here (architecture overview)
├── 96_file1_camera.md                  ← Camera system (150-200 LOC)
├── 97_file2_raycast.md                 ← Raycasting algorithm (120-150 LOC)
├── 98_file3_projection.md              ← 3D projection math (150-180 LOC)
├── 99_file4_texture.md                 ← Texture/shading system (200-250 LOC)
├── 100_file5_sprite.md                 ← Sprite rendering (250-300 LOC)
├── 101_file6_display3d.md              ← 3D display backend (200-250 LOC)
└── 102_file7_integration.md            ← Mode integration (400-500 LOC)
```

## Implementation Order

Implement in numerical order (File 1 → File 7) for minimal dependencies:

### Phase 1: Core Math & Infrastructure (Files 1-3)
1. **File 1 - Camera System** (`camera.c`)
   - Position, orientation, FOV
   - Ray generation
   - World-to-camera transforms
   - **Testing:** Unit tests with known coordinates
   - **Time:** 1-2 hours

2. **File 2 - Raycasting** (`raycast.c`)
   - DDA algorithm
   - Wall intersection detection
   - Fish-eye correction
   - **Testing:** Ray hits correct walls at correct distances
   - **Time:** 2-3 hours

3. **File 3 - Projection** (`projection.c`)
   - Distance to wall height
   - Vertical centering
   - Clipping calculations
   - **Testing:** Walls project to correct screen positions
   - **Time:** 1-2 hours

### Phase 2: Visual Quality (Files 4-5)
4. **File 4 - Textures** (`texture.c`)
   - ASCII/UTF-8 patterns
   - Distance-based shading
   - Floor/ceiling textures
   - **Testing:** Shading creates depth perception
   - **Time:** 2-3 hours

5. **File 5 - Sprites** (`sprite.c`)
   - Entity collection
   - Z-buffering
   - Billboard rendering
   - **Testing:** Sprites appear at correct positions, depth-sorted
   - **Time:** 2-3 hours

### Phase 3: Integration (Files 6-7)
6. **File 6 - Display Backend** (`display_3d.c`)
   - Framebuffer management
   - Double buffering
   - Drawing primitives
   - **Testing:** Screen updates correctly, no flicker
   - **Time:** 2-3 hours

7. **File 7 - Mode Integration** (`render_3d.c`)
   - Main rendering pipeline
   - Mode switching
   - HUD overlay
   - **Testing:** Full 3D rendering works, can toggle to 2D
   - **Time:** 3-4 hours

**Total Estimated Time:** 15-20 hours

## Files to Create/Modify

### New Files
```
include/snake/
├── camera.h
├── raycast.h
├── projection.h
├── texture.h
├── sprite.h
├── display_3d.h
└── render_3d.h

src/render3d/
├── camera.c
├── raycast.c
├── projection.c
├── texture.c
└── sprite.c

src/render/
├── display_3d.c
└── render_3d.c
```

### Modified Files
```
main.c           - Add 3D init, mode toggle handling
src/input/input.c - Add 'V' key for view toggle
src/render/render.c - Mode dispatch
Makefile         - Add new source files
```

## Dependency Graph

```
render_3d.c
    ↓
    ├─→ camera.c (no deps)
    ├─→ raycast.c (needs camera.h, game.h)
    ├─→ projection.c (needs raycast.h)
    ├─→ texture.c (needs raycast.h, projection.h, display.h)
    ├─→ sprite.c (needs camera.h, projection.h, game.h)
    └─→ display_3d.c (needs display.h)
```

## LLM Implementation Tips

Each file includes:
- ✅ **Complete function signatures** (copy-paste ready)
- ✅ **Implementation notes** (algorithm explanations)
- ✅ **Example code** (skeleton implementations)
- ✅ **Testing checklist** (verification criteria)
- ✅ **Minimal dependencies** (can implement in isolation)

### How to Use These Docs with an LLM

1. **Read overview** (File 95) to understand big picture
2. **Pick a file** (start with File 1)
3. **Provide to LLM:**
   ```
   Implement [File N] from the spec in design_docs/[filename].md
   
   Context:
   - Snake game is in C
   - Existing codebase has display.h, game.h, types.h
   - Board is FIXED_BOARD_WIDTH × FIXED_BOARD_HEIGHT
   - Terminal-based rendering
   
   Requirements:
   - Follow the spec exactly
   - Include all functions from "Public API" section
   - Add unit test scaffolding
   - Handle edge cases mentioned in spec
   ```
4. **Test the implementation** using checklist in spec
5. **Move to next file**

### Context Needed per File

| File | Context Required |
|------|-----------------|
| File 1 (Camera) | types.h only |
| File 2 (Raycast) | camera.h, game.h, types.h |
| File 3 (Projection) | raycast.h |
| File 4 (Texture) | raycast.h, projection.h, display.h |
| File 5 (Sprite) | camera.h, projection.h, game.h |
| File 6 (Display3D) | display.h |
| File 7 (Integration) | All previous files |

## Key Technical Details

### Coordinate Systems
- **World Space:** Top-left origin, X right, Y down (matches game board)
- **Camera Space:** Camera at origin, looking down +Z, X right, Y down
- **Screen Space:** Terminal coordinates, (0,0) at top-left

### Rendering Pipeline
```
1. Update camera from player position
2. For each screen column:
   a. Cast ray
   b. Find wall intersection
   c. Project to screen height
   d. Sample texture
   e. Store distance in Z-buffer
3. Collect sprites from game state
4. Transform sprites to screen space
5. Sort sprites by distance
6. Render sprites (check Z-buffer)
7. Draw HUD
8. Present frame
```

### Performance Targets
- **Frame Rate:** 30-60 FPS
- **Resolution:** 80×24 to 120×40 (terminal dependent)
- **Ray Count:** 1 ray per column (80-120 rays/frame)
- **Sprite Count:** < 200 entities typical

## Visual Examples

### Before (2D Top-Down)
```
┌────────────────────┐
│ # # # # # # # # # │
│ #   @         # # │
│ # S S S       # # │
│ #     S   F   # # │
│ # # # # # # # # # │
└────────────────────┘
```

### After (3D First-Person)
```
┌────────────────────────────┐
│      ...ceiling...         │
│ ║▓║│ ╱ @ ╲ │║▓║ F ║▓║│║▓║ │  ← Walls & sprites
│ ║▓║│╱     ╲│║▓║   ║▓║│║▓║ │
│      ...floor...           │
└────────────────────────────┘
Score: 42          Player1
```

## Common Pitfalls to Avoid

1. **Fish-eye distortion:** Use perpendicular distance, not Euclidean
2. **Division by zero:** Clamp distances to > 0.01
3. **Off-by-one errors:** Terminal coordinates are inclusive
4. **Buffer overruns:** Check array bounds before indexing
5. **Z-buffer issues:** Initialize to max distance, check before sprite draw
6. **Angle wrapping:** Normalize angles to [0, 2π)
7. **UTF-8 encoding:** Handle multibyte characters carefully

## Testing Strategy

### Per-File Testing
- Unit tests for math functions
- Visual tests with known scenarios
- Boundary condition checks
- Memory leak detection (valgrind)

### Integration Testing
- Toggle between 2D and 3D
- Walk around the board
- Verify perspective correctness
- Check sprite depth sorting
- Test with 1 and 2 players
- Performance profiling

### Acceptance Criteria
- ✅ Compiles without warnings
- ✅ No memory leaks
- ✅ Walls render with correct perspective
- ✅ Sprites appear at correct positions
- ✅ Depth sorting works (near objects occlude far)
- ✅ HUD displays correctly
- ✅ Mode toggle is seamless
- ✅ Frame rate > 30 FPS

## Build Integration

Add to Makefile:
```makefile
RENDER3D_SOURCES = \
    src/render3d/camera.c \
    src/render3d/raycast.c \
    src/render3d/projection.c \
    src/render3d/texture.c \
    src/render3d/sprite.c \
    src/render/display_3d.c \
    src/render/render_3d.c

SOURCES += $(RENDER3D_SOURCES)
```

## Quick Start Checklist

- [ ] Read overview (File 95)
- [ ] Create directory: `src/render3d/`
- [ ] Implement File 1 (Camera)
- [ ] Test File 1 independently
- [ ] Implement File 2 (Raycast)
- [ ] Test Files 1+2 together
- [ ] Implement File 3 (Projection)
- [ ] Test Files 1-3 with dummy renderer
- [ ] Implement File 4 (Texture)
- [ ] Implement File 5 (Sprite)
- [ ] Implement File 6 (Display3D)
- [ ] Test display backend standalone
- [ ] Implement File 7 (Integration)
- [ ] Full integration test
- [ ] Performance tuning
- [ ] Documentation and cleanup

## Support & References

### Mathematical Foundations
- Raycasting: Lode's Computer Graphics Tutorial
- Projection: Perspective transformation basics
- DDA: Digital Differential Analyzer algorithm

### Terminal Graphics
- ANSI escape codes for colors
- UTF-8 box drawing characters
- Double buffering for flicker-free updates

### Game-Specific
- Snake board is 40×20 (FIXED_BOARD_WIDTH × FIXED_BOARD_HEIGHT)
- Coordinate (0,0) is top-left
- Walls only at board edges in Snake

## Completion Reward

When all 7 files are implemented and integrated, you'll have:
- A fully functional pseudo-3D renderer
- First-person view of the Snake game
- Doom-like visual experience in a terminal
- Toggle between classic 2D and new 3D views
- Foundation for future 3D enhancements

**Estimated Final Result:** ~1,500-2,000 lines of C code across 7 modules

## Questions?

Each design doc includes:
- Detailed API documentation
- Implementation examples
- Testing guidance
- Integration notes
- Complexity estimates

Start with File 1 (camera.md) and work through sequentially!
