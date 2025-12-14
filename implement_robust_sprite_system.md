Implement Robust Sprite System
===============================

Overview
--------
Replace the current ad-hoc sprite code with a robust, simple billboard sprite system that:
- Projects sprites using the existing `Projection3D` helpers for vertical alignment.
- Occludes sprites using a cheap per-column perpendicular-depth buffer (filled by the raycaster).
- Uses integer pixel bounds and per-column drawing for correctness and performance.
- Provides a clear API for adding sprites (position, world height, pivot, optional texture/frame) and for projecting/drawing them.

Goals
-----
- Fix: sprites floating, disappearing at a distance, not aligning vertically with walls, and being drawn on top of walls.
- Keep changes minimal and compatible with current style (plain C, existing projection/camera/raycast code).
- Provide test coverage for important behaviors (occlusion, pivoting, camera-facing).

Design Summary
--------------
- Sprite model: a vertical billboard anchored to (world_x, world_y) with `world_height` and `pivot` (0.0 bottom to 1.0 top). Sprites can optionally `face_camera`.
- Projection: compute camera-space X/Y, reject behind-camera, compute perpendicular distance and call `projection_project_wall` to get vertical pixel baseline (draw_end) and wall height. Sprite pixel height = `proj.wall_height * (sprite.world_height / 1.0)`.
- Occlusion: during the existing raycast loop, record the perpendicular depth of the wall for each screen column in `column_depths[x]`. When drawing sprites, for each column `x` spanned by the sprite only render sprite pixels where `sprite_perp_distance < column_depths[x]`.
- Drawing primitive: per-column vertical spans (fast) or eventually column-texture sampling. Keep `render_3d_sdl_draw_filled_circle()` for debug only.

Data structures & API (proposed)
--------------------------------
```c
// header: include/snake/render_3d_sprite.h
typedef struct {
    float world_x, world_y;     // world cell center (x+0.5f, y+0.5f) convention
    float world_height;         // in world units (1.0 ~ wall height)
    float pivot;                // 0.0 = bottom-on-floor, 0.5 center, 1.0 top
    bool face_camera;           // true => always face camera (billboard)
    int texture_id;             // -1 => simple color
    int frame;                  // animation/frame index
    // derived values (filled by projection step)
    float perp_distance;       // perpendicular distance used for sizing
    int screen_x, screen_w, screen_h, screen_y_top;
    bool visible;
} Sprite3D;

void sprite_init(SpriteRenderer3D* sr, int max_sprites, const Camera3D* camera, const Projection3D* proj);
void sprite_clear(SpriteRenderer3D* sr);
bool sprite_add(SpriteRenderer3D* sr, float world_x, float world_y, float world_height, float pivot, bool face_camera, int texture_id, int frame);
void sprite_project_all(SpriteRenderer3D* sr);
void sprite_sort_by_depth(SpriteRenderer3D* sr);
void sprite_draw(SpriteRenderer3D* sr, SDL3DContext* ctx, const float* column_depths);
void sprite_shutdown(SpriteRenderer3D* sr);
```

Integration points & file changes
---------------------------------
- `include/snake/render_3d_sprite.h` — replace `SpriteProjection` with `Sprite3D`, add new prototypes.
- `src/render/sprite.c` — implement `sprite_project_all()` and `sprite_draw()`, keep `sprite_init()`, `sprite_add()`.
- `src/render/render_3d.c` — add `float* column_depths` to `Render3DContext`, allocate/free in init/shutdown, write `column_depths[x] = pd` in the raycast loop, and replace current ad-hoc sprite draw code with calls to `sprite_project_all()`, `sprite_sort_by_depth()`, `sprite_draw()`.
- `src/render/render_3d.c` — add `Render3DConfig.show_sprite_debug` to enable debug visuals and gate existing `fprintf` debug prints behind a debug flag.

Step-by-step implementation plan
--------------------------------
1. Define new `Sprite3D` struct & API (small)
   - Files: `include/snake/render_3d_sprite.h`, `src/render/sprite.c` (stubs)
   - Acceptance: compile; `sprite_add()` stores fields.
   - Status: **Completed** — header and implementation stubs added (`include/snake/render_3d_sprite.h`, `src/render/sprite.c`) and a test stub (`tests/test_sprite.c`). The `test-sprite` target builds and the test prints `test_sprite: OK` locally.

2. Add per-column depth buffer (small)
   - Files: `src/render/render_3d.c`
   - Changes: `float* column_depths` allocation in `render_3d_init()` and writes in raycast loop (`pd`), free in `render_3d_shutdown()`.
   - Acceptance: compiled; `column_depths` populated at render time.

3. Implement `sprite_project_all()` (medium)
   - Files: `src/render/sprite.c`
   - Behavior: for each sprite, compute camera-space coords, reject behind camera, compute perp-distance, call projection to compute `proj.wall_height`, compute `screen_h = proj.wall_height * (sprite.world_height)`, compute horizontal pixel center and bounds, set `visible`.
   - Acceptance: sprites in front of camera get `visible=1` and `screen_h>0`.

4. Implement `sprite_draw()` with occlusion (medium)
   - Files: `src/render/sprite.c`, `src/render/render_3d.c`
   - Behavior: for each visible sprite, iterate its `x` pixel range; for each column, if `sprite_perp_distance < column_depths[x]` then draw (for now a solid-colored vertical span sized to sprite). Use integer bounds and per-column writes.
   - Acceptance: sprites are occluded by nearer walls; sprite bottoms align with wall `draw_end` as per pivot.

5. Integrate and remove old code + debug (small)
   - Replace existing ad-hoc sprite code in `render_3d_draw()` with the new pipeline. Remove stray debug prints or gate them behind `show_sprite_debug`.
   - Acceptance: no stray `[sprite-debug]` or `[ray-debug]` prints; new pipeline active.

6. Tests & debugging (small)
   - Add tests: occlusion, pivot alignment, camera-facing, behind-camera rejection.
   - Acceptance: tests pass locally.

Edge cases & tests
------------------
- Occlusion: wall closer than sprite → sprite not drawn for that column. Test with a short wall and sprite behind it.
- Camera-facing: `face_camera=true` makes billboard face camera; check horizontal center mapping (screen_x). 
- Pivot anchoring: `pivot=0.0` bottom at projected `draw_end`, `pivot=0.5` centered. Test pixel top/bottom positions.
- Behind-camera and near-plane: camera at same position/range → sprite ignored; test no crash.
- Very small sprites: enforce minimal visible size (>=1px) when projection yields >0.

Extra recommendations
---------------------
- Keep per-column drawing to be fast and cache-friendly; avoid per-pixel loops where possible.
- Keep `render_3d_sdl_draw_filled_circle()` for debug only.
- Add `Render3DConfig.show_sprite_debug` for toggleable overlays; gate `fprintf` logs behind debug flags and remove noisy logs.
- If later adding textures, implement per-column texture sampling and alpha handling for transparent sprites.

Estimated effort
----------------
- Small: header and scaffolding, debug gating (2–4 hours). 
- Medium: projection + occlusion implementation, tests (1–2 days).
- Large (optional): textured sprites, animation frames (2–3 days).

Next step
---------
If you approve, I will proceed to Step 2 (add per-column depth buffer) next — I'll add `column_depths` to `Render3DContext`, allocate/free it in init/shutdown, and write `column_depths[x] = pd` in the raycast loop. Tell me to proceed when you're ready.
