# Project Suggestions & Prioritized Worklist

## Summary ✅
A concise set of high-impact improvements to increase reliability, maintainability, and security.

## Progress ✅
- **Header ownership docs** — Completed: short ownership/lifetime comments added to public headers for create/destroy/get APIs (e.g. `include/snake/*.h`).
- **Safe allocation idioms** — Completed: replaced casted `malloc`/`calloc` with `malloc(sizeof *ptr)` / `calloc(1, sizeof *ptr)` and removed extraneous casts in core modules (examples: `src/core/game.c`, `src/core/collision.c`, `src/net/net.c`, `src/render/*`, `src/platform/tty.c`).
- **Tests for error/oom paths** — Completed: added `tests/test_game_oom.c` (allocation-failure simulator using `dlsym(RTLD_NEXT, ...)`) and integrated it into `scripts/run_unit_tests.sh`; run under ASAN — OK.
- **Analysis & validation** — Completed: ran `make analyze` and the unit test suite; no regressions detected.
- **Standardize error-out patterns** — Completed: refactored `tty_open` (`src/platform/tty.c`), `render_3d_sdl_init` (`src/render/render_3d_sdl.c`), and `net_unpack_game_state` (`src/net/net.c`) to use a single `goto out` cleanup path; added small unit tests to exercise failure cases.

## Medium-priority items (robustness & testing) ⚙️
- **Safe allocation idioms**
  - Use `malloc(sizeof *ptr)` and avoid casting malloc results in C; ensure return values are checked and error paths free resources.
- **Tests for error/oom paths**
  - Add tests that simulate allocation failures (fault injection/shims) and verify error paths free memory (run under ASAN).

## Concrete code changes (examples) 🔧
- Replace: `Game* g = (Game*)malloc(sizeof(Game));`
- With: `Game* g = malloc(sizeof *g);` and `if (!g) return NULL;`
- Add header comment example:
  ```c
  // Returns a newly allocated Game; caller must call game_destroy() to free it.
  Game* game_create(const GameConfig* cfg, uint32_t seed_override);
  ```
- Standardize error-out (`goto out`) cleanup patterns and add unit tests covering those branches.

## Next steps ➕
- Add more OOM tests to cover other modules (render, net, persist, tty) and important allocation sites.
- Add CI job to run unit tests under ASAN and the OOM harness to prevent regressions.
- Consider small refactors to standardize `goto out` error-out patterns where it improves clarity and testability.

## Medium-priority items (robustness & testing) ⚙️
- **Safe allocation idioms**
  - Use `malloc(sizeof *ptr)` and avoid casting malloc results in C; ensure return values are checked and error paths free resources.
- **Tests for error/oom paths**
  - Add tests that simulate allocation failures (fault injection/shims) and verify error paths free memory (run under ASAN).

## Concrete code changes (examples) 🔧
- Replace: `Game* g = (Game*)malloc(sizeof(Game));`
- With: `Game* g = malloc(sizeof *g);` and `if (!g) return NULL;`
- Add header comment example:
  ```c
  // Returns a newly allocated Game; caller must call game_destroy() to free it.
  Game* game_create(const GameConfig* cfg, uint32_t seed_override);
  ```
- Standardize error-out (`goto out`) cleanup patterns and add unit tests covering those branches.
