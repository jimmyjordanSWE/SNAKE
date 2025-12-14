## Testing Implementation Plan

**Summary:**

This document outlines a practical, incremental plan to add comprehensive unit testing for the project's core logic (game rules, collision, utilities, input handling, persistence, networking, and platform abstractions). The 3D renderer and tests that require SDL/graphics or GPU access will be a separate test suite and are explicitly excluded from the "core" unit test scope.

**Note on code rewrites & module design:**

It is acceptable—and encouraged—to refactor or rewrite existing code where necessary to make modules independently testable and maintainable. Modules should be separate, have clear public headers, minimal global state, and well-defined interfaces. Follow professional C style (consistent naming, clear ownership semantics, defensive checks, and separation of concerns). Introduce small abstraction layers (e.g., for platform I/O, RNG, or rendering) and dependency injection where helpful to enable deterministic tests and easy mocking.

**Pre-test refactor pass (required):**

Before adding any unit tests there must be a deliberate, top-level pass across the codebase to make modules explicitly separable and testable. This pass must:
- Identify and enumerate modules and their public interfaces.
- Extract or create clear public headers for each module and ensure each module can be compiled independently.
- Remove or minimize global/static state; prefer explicit ownership and APIs that accept injected dependencies.
- Add or update build rules so modules and their tests can be built individually (object/library targets, test linking).
- Add small abstraction layers where needed (e.g., RNG, file I/O, platform/TTY) to enable deterministic tests and mocking.
- Produce a short `TESTING.md` or checklist documenting the gate criteria for when a module is considered testable.

- Verify each module is feature-complete (no missing or placeholder behavior) as part of the refactor pass; feature completeness is a prerequisite for writing unit tests.

No tests for a module should be written until that module meets these requirements; the refactor pass is an explicit quality gate.

**Planning status:**

This document is currently in the planning phase only. Do not start refactor or test work until module checkpoints (below) have been reviewed and the project maintainer explicitly moves work to the execution phase.

**Module checkpoints:**

Use these checkboxes to track a first-pass review for each module (modularity & correctness), a short "perform a feature complete check and implement any missing features" verification, and whether unit tests have been added. The module must perform a feature-complete check and implement any missing features before it moves to "Unit tests added".

- `src/core/game.c`
  - [x] First-pass modularity check completed
  - [x] Perform a detailed code review (see `C coding standards.md`) 
  - [x] perform a feature complete check and implement any missing features
  - [x] Unit tests added
  - Notes: public header `include/snake/game.h` exists; `GameState` is self-contained (no globals). RNG is injectable via `game_init` seed and stored in `GameState`, allowing deterministic tests. No blocking hidden state found; ready for test skeletons.
  - Feature completeness note: ensure all reasonable gameplay features (spawning, food respawn timing, scoring, end-of-game and tie rules, and edge cases) are implemented and verified before adding unit tests.
  - Refactor notes: small refactor completed to improve clarity and testability:
    - Introduced named constants (`SPAWN_MAX_ATTEMPTS`, `FOOD_RESPAWN_MIN`, `FOOD_RESPAWN_MAX`, `FOOD_MIN_ATTEMPTS`) to remove magic literals.
    - Added `game_set_food()` test helper to deterministically set food positions for unit tests.
    - Added brief documentation comments to public API in `include/snake/game.h`.
  - Feature-check changes: implemented `GAME_STATUS_GAME_OVER` when no players can be respawned; added `tests/game/test_game_over.c` to validate behavior; added Makefile target `test-game`.
- `src/core/collision.c`
  - [x] First-pass modularity check completed
  - [x] Perform a detailed code review (see `C coding standards.md`) 
  - [x] perform a feature complete check and implement any missing features
  - [x] Unit tests added
  - Notes: functions are pure and accept explicit `PlayerState`/`GameState` structures. No hidden globals found. Minor style/formatting suggestions applied to notes. Ready for collision unit tests (bounding, self, inter-snake, head-on tie cases).
  - Feature-check changes: allow stepping into an other player's tail if it will vacate (not growing), detect head-swap (A->B.head && B->A.head) as mutual death, and added tests `tests/collision/test_tail_vacate.c` and `tests/collision/test_head_swap.c` with Makefile target `test-collision`.

- `src/utils/` (rng, bounds, helpers)
  - [x] First-pass modularity check completed
  - [x] Perform a detailed code review (see `C coding standards.md`) 
  - [x] perform a feature complete check and implement any missing features
  - [x] Unit tests added
  - Notes: `snake_rng_*` functions are deterministic and self-contained. `snake_in_bounds` is trivial and correct. No hidden state; ready for RNG and bounds tests.
  - Feature-check changes: added `tests/utils/test_rng.c` and `tests/utils/test_bounds.c` and Makefile target `test-utils`.
- `src/input/`
  - [x] First-pass modularity check completed
  - [x] Perform a detailed code review (see `C coding standards.md`) 
  - [x] perform a feature complete check and implement any missing features
  - [x] Unit tests added
  - Notes: original implementation directly read from `STDIN_FILENO` and used static terminal state. To improve testability, added `input_poll_from_buf(InputState*, const unsigned char*, size_t)` (in `include/snake/input.h` and `src/input/input.c`) so input parsing can be exercised in unit tests without terminal I/O. `input_init`/`input_shutdown` keep platform dependent behavior and remain separate.
  - Feature-check changes: added `tests/test_input.c` (existing) and verified parsing behavior.
- `src/persist/`
  - [x] First-pass modularity check completed
  - [x] Perform a detailed code review (see `C coding standards.md`) 
  - [x] perform a feature complete check and implement any missing features
  - [x] Unit tests added
  - Notes: parsing is defensive (trims, rejects malformed lines). `persist_read_scores` and `persist_write_scores` handle temp files safely. Consider adding small mocks for file IO in tests (or use temporary files). No blocking hidden state.
  - Feature-check changes: added `tests/persist/test_persist_io.c` and Makefile target `test-persist` using temp files.
- `src/net/`
  - [x] First-pass modularity check completed
  - [x] Perform a detailed code review (see `C coding standards.md`) 
  - [x] perform a feature complete check and implement any missing features (protocol helpers)
  - [x] Unit tests added
  - Notes: network API client functions (`net_connect`, `net_send_*`, `net_recv_*`) are intentionally left as minimal stubs — implementing a full transport depends on the chosen networking stack and is out of scope for core logic unit tests. The pack/unpack helpers are implemented and cover happy/error paths, enabling robust message parsing tests without a live network.
  - Feature-check changes: added simple (deterministic) pack/unpack helpers (`net_pack_*`, `net_unpack_*`) and `tests/net/test_net_pack.c` (including negative/error-case checks) with Makefile target `test-net`.
- `platform/` (tty and other abstractions)
  - [x] First-pass modularity check completed
  - [x] Perform a detailed code review  (see `C coding standards.md`) 
  - [x] perform a feature complete check and implement any missing features
  - [x] Unit tests added
  - Notes: `tty` exposes a stateful context relying on `ioctl`, `termios` and signals. To facilitate tests, added `tty_simulate_winch()` (in `include/snake/tty.h` and `src/platform/tty.c`) to simulate `SIGWINCH` and let tests exercise `tty_check_resize()` without sending real signals. Consider adding an injectable `get_terminal_size` wrapper for advanced mocking if needed.

  - Feature-check changes: added `tty_set_test_size()`/`tty_clear_test_size()` helpers to allow deterministic sizes in tests and added `tests/platform/test_tty.c` and Makefile target `test-tty`.

  - Feature completeness note: ensure resizing events and terminal-size changes are handled correctly (via `tty_check_resize()` and `tty_simulate_winch()`), and provide injectable terminal-size wrappers for deterministic testing before adding unit tests.

If additional modules are identified during the top-level review, add them to this checklist before beginning tests.

**Code review step:**

Before a module's tests are added, perform a code review focused on correctness, style, and testability. The reviewer must confirm the module conforms to `C coding standards.md`, documents any exceptions, and either approve or request changes. Record the review outcome in the module checklist above.

**Goals & Acceptance Criteria:**
- Add reproducible unit tests that run quickly and deterministically and are runnable via `make test`.
- Tests cover core modules and edge cases; each tested module has a small, focused suite.
- CI runs tests and reports failures; coverage is tracked and improved incrementally.

**Scope (what to test now):**
- `src/core/game.c` — game state transitions, snake movement rules, scoring, and end-of-game conditions.
- `src/core/collision.c` — bounding checks, sprite collision detection, and edge cases.
- `src/utils/` — deterministic RNG behavior, bounds, and helper functions.
- `src/input/` — parsing and mapping of inputs; state transitions for pressed/released keys.
- `src/persist/` — save/load correctness, error handling for IO failures (use mocks).
- `src/net/` — basic message packing/unpacking and error paths (mock network layer).
- `platform/tty.c` and other platform abstractions — use stubs to avoid terminal dependencies.

- **Module design requirement:** Modules should be decoupled and written in a professional C style so they can be built and tested independently (small public headers, minimal static/global state, and clear error/ownership semantics).
  - When practical, ensure modules can be built as standalone objects/libraries and have no hidden runtime coupling (for example, unmockable globals or implicit init order).

**Out-of-scope (for core suite):**
- `render/` 3D renderer tests requiring SDL, GPU or heavy integration. These will live in `tests/3d/` and be run separately (e.g., in a different CI job or behind a feature flag).

**Framework & Tooling Recommendation:**
- Evaluate Criterion (recommended for C projects) for its ease of use and rich assertions; alternatives: Unity, Check, or a custom lightweight harness.
- Add framework as a dependency (submodule, vendor directory, or system package) and document install steps.

**Makefile / Runner changes:**
- Add `make test` target to build and run tests.
- Add `make test-coverage` (optional) that reports coverage using `gcov`/`lcov`.

**Test structure & examples:**
- Place tests alongside existing `tests/` directory, creating subdirs per module.
- Example tests to add first:
  - `tests/game/test_game_basic.c`: start game, move snake, apply input, check resulting state.
  - `tests/collision/test_collision_basic.c`: verify collisions between sprites and boundaries.
  - `tests/utils/test_rng.c`: seed RNG, check deterministic sequences.
  - `tests/persist/test_persist_io.c`: simulate save/load with file mocks and error cases.

**Mocks & Fixtures:**
- Provide small helper utilities to stub out file IO and platform functions.
- Provide deterministic fixtures for random sequences and controlled time sources.

**CI & Coverage:**
- Add a CI job to run `make test` on pushes and pull requests.
- Optionally run renderer/SDL tests only in special CI runners or on tagged jobs that have display access.

**Milestones & Timeline:**
1. Perform a mandatory top-level refactor pass to make modules independent and testable (this is a required gate before writing tests).
2. Add test framework dependency and `make test` target.
3. Add core test harness and first smoke tests for `game` and `collision` (minimal passing examples).
4. Expand test coverage across `utils`, `input`, `persist`, `net`, and platform abstractions.
5. Add CI integration and coverage reporting.
6. Create a separate 3D renderer suite and runner for more complex integration tests.

**Next actions (short-term):**
- Start the mandatory top-level refactor pass: enumerate modules, add or consolidate headers, remove hidden globals, and add test-build targets.
- After the refactor pass completes, pick and document the test framework (Criterion preferred) and implement `make test` with example tests.

**Notes:**
- Keep tests self-contained and fast — avoid launching the renderer, network servers, or heavy integration in the core suite.
- When renderer tests are added, ensure they can be executed independently (different target/CI label).
 - Rewriting or refactoring code is permitted to improve testability and code quality; please add small, well-documented commits for such changes.

--
Created as part of the test planning effort to add unit tests for core logic. If you'd like, I can now implement the test harness and add the first few tests (Criterion + `make test`) as a follow-up.
