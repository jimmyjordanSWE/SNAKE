# impl_011: 3D Rendering (SDL Backend)

**Headers:** `include/snake/render_3d*.h` (8 files)  
**Sources:** `src/render/camera.c`, `projection.c`, `raycast.c`, `sprite.c`, `texture.c`, `display_3d.c`, `render_3d.c`, `render_3d_sdl.c`  
**Status:** ⏳ Optional (Not core gameplay)

---

## Overview

Optional pseudo-3D rendering pipeline using SDL graphics backend. See `render_3d*.h` headers for function signatures.

**Status:** Not core gameplay; experimental feature for visual enhancement.

**Debugging:** `Render3DConfig` includes a `show_sprite_debug` flag to enable diagnostic overlays and gated `fprintf` logs for the raycaster and sprite system.

---

## Architecture

```
GameState
  │
  └→ render_3d.c [High-level API]
     │
     ├→ camera.c [View transformation]
     ├→ projection.c [3D→2D mapping]
     ├→ raycast.c [Wall rendering]
     ├→ sprite.c [Entity rendering]
     └→ texture.c [Texture lookup]
     │
     └→ display_3d.c [SDL drawing]
```

---

## Core Concepts
