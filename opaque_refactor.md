 # Opaque Refactor Plan

 Purpose
 - Provide an incremental, low-risk plan to convert public struct definitions into opaque types (hide their layout in C files) while keeping the codebase buildable and testable at every step.

 Principles
 - Prefer hiding implementation details for types that are internal or have derived fields.
 - Prefer non-breaking, incremental changes: add constructors/accessors before removing field accessors.
 - Keep small PODs (value objects used broadly, e.g., `SnakePoint`, `HighScore`) public; do not convert them unless there is a strong reason.

 Inventory & Priority (recommended order)
 1. `render_3d_sprite` — Done (opaque, internal moved to `src/render/sprite.c`).
 2. Candidates (medium effort):
    - `Camera3D` (`include/snake/render_3d_camera.h`) — good candidate if you want to force callers to use camera APIs. Requires updating many tests and call sites. Priority: medium.
    - `Projection3D` (`include/snake/render_3d_projection.h`) — moderate scope; used across rendering. Priority: medium.
    - `Texture3D` (`include/snake/render_3d_texture.h`) — used in rendering; consider opaque if internal fields (pixels, img_w/img_h) should be hidden. Priority: low–medium.
 3. Keep public (do not convert unless needed): `SnakePoint`, `HighScore`, `GameConfig`, small out-params like `WallProjection`.

 Step-by-step refactor (repeat per module)

 Step 0 — Safety checklist
 - Ensure full test suite is green before starting.
 - Make one change at a time and run `make test` after each step.

 Step 1 — Design the new API (non-breaking)
 - Decide whether the type will be accessed through an allocated pointer (`Foo* foo_create(...)` / `foo_destroy(foo)`) or through accessors that operate on an opaque pointer the caller still owns.
 - Recommended: add a constructor + destructor and a minimal set of accessors that callers need (getters/setters). Document ownership in the header.

 Step 2 — Update the public header
 - Replace the full struct definition with a forward declaration and a `typedef struct Foo Foo;`.
 - Add declarations for the constructor/destructor and accessors you designed in Step 1.

 Step 3 — Implement the struct in the C file
 - Move the struct definition into the `.c` file and implement the constructor/destructor and accessors.

 Step 4 — Migrate call sites incrementally
 - Grep for uses of the struct fields and replace them with your accessors. Make the smallest number of changes per commit.
 - When callers need an instance on the stack, replace with `Foo* f = foo_create(...);` and `foo_destroy(f);` (or provide `foo_init(foo*, ...)` + `foo_free()` if you want to support stack allocation, but that requires callers to include an allocation method or an opaque fixed-size buffer -- usually more work).

 Step 5 — Add tests and run CI
 - Add unit tests for the public behaviors of the type through the new API.
 - Run `make test` and fix any regressions.

 Step 6 — Clean up and document
 - Remove any stale direct uses of fields, mark old macros or inline helpers deprecated, and update documentation comments in the header.

 Sample patch template (illustrative)

 - Header (before):
   ```c
   typedef struct {
       int screen_width;
       int screen_height;
       float fov_radians;
       int horizon_y;
       float wall_scale;
   } Projection3D;
   void projection_init(Projection3D* proj, int screen_width, int screen_height, float fov_radians, float wall_scale);
   ```

 - Header (after):
   ```c
   /* Projection3D is opaque; use the constructor and accessors. */
   typedef struct Projection3D Projection3D;
   Projection3D* projection_create(int screen_width, int screen_height, float fov_radians, float wall_scale);
   void projection_destroy(Projection3D* proj);
   void projection_init(Projection3D* proj, int screen_width, int screen_height, float fov_radians, float wall_scale);
   int projection_get_screen_width(const Projection3D* proj);
   int projection_get_screen_height(const Projection3D* proj);
   ```

 - C file (add at top):
   ```c
   struct Projection3D {
       int screen_width;
       int screen_height;
       float fov_radians;
       int horizon_y;
       float wall_scale;
   };

   Projection3D* projection_create(int screen_width, int screen_height, float fov_radians, float wall_scale) {
       Projection3D* p = calloc(1, sizeof(*p));
       if (!p) return NULL;
       projection_init(p, screen_width, screen_height, fov_radians, wall_scale);
       return p;
   }
   void projection_destroy(Projection3D* proj) { free(proj); }
   int projection_get_screen_width(const Projection3D* proj) { return proj ? proj->screen_width : 0; }
   ```

 Testing tips
 - Add unit tests that use only the public API (constructor/accessors) and verify identical behavior to the previous direct-field approach.
 - Use `git grep "proj\."` or `git grep "proj->"` to find and update call sites.

 Rollback strategy
 - Keep changes small and locally reversible. If a refactor breaks many call sites, revert and split into smaller commits.

 Notes on breaking changes
 - Making a struct opaque and requiring heap allocation is a breaking API change if callers rely on stack allocation of the concrete type. Plan migrations and deprecation cycles accordingly.

 Want me to start one refactor? I can open a patch for a chosen target (e.g. `Projection3D` or `Camera3D`) and update call sites incrementally with tests.
