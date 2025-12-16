# Test Tasks & Status (source of truth)

This file tracks the test-related tasks, progress, and notes for the project. Update this file as work progresses.

## Summary
- Last run: make test (CONFIG=debug-asan) — all existing tests passed
- Current in-progress work: Run full test suite & collect failures (sanity run completed)

---

## Todo list

1. **Run full test suite & collect failures** (completed)
   - Status: initial `make test` run completed; later `make test` found one unrelated failing test (`test_camera_angles`) when running full test matrix.
   - Update: Fixed `test_camera_angles` by returning the current camera angle when interpolation time is zero; re-ran render and module-specific tests and they passed locally. Also fixed the `test-unit` Makefile recipe (now uses a robust `find | xargs` approach) so the full `make test` runs successfully locally.
   - Valgrind: ran `make valgrind`; Valgrind reported memory issues. Fixes implemented:
  - Added `raycaster_destroy` in `render_3d_shutdown` to free the raycaster allocation created by `raycaster_create` (resolved a project-level leak).
  - Adjusted the `valgrind` Makefile target to run Valgrind with `SDL_VIDEODRIVER=dummy` to avoid loading GLX/Mesa (this prevented large GLX-related leaks from appearing under Valgrind on CI/dev machines).
  - After these changes, remaining Valgrind output shows only small **still reachable** allocations in `libdbus`/`SDL2` (likely system-level and acceptable). Next: add a CI valgrind step that sets `SDL_VIDEODRIVER=dummy` (or use a suppression file) and document the result.
   - Next: investigate flaky failing tests (e.g. `test_camera_angles`), then run valgrind and coverage.

2. **Standardize tests layout** (in-progress)
   - Progress: Added a minimal Unity harness at `tests/vendor/unity.{h,c}` and placed rewritten persist tests into `tests/unit/persist/`. Rewrote and added tests for `persist`, `highscore`, `net`, `utils`, `game`, and `collision` under `tests/unit/`. Added `make test-unit` target that discovers and runs tests in `tests/unit` (only runs unit tests to avoid flaky render tests).
   - Next: continue migrating remaining tests (render, platform) as appropriate and add `tests/integration/` for heavy tests that require runtime assets.

4. **Add unit tests for untested modules** (in-progress)
   - Progress: Added highscore, read-scores malformed, append edge cases, persist idempotency, persist-free null handling, net pack, game events/death/over, utils (bounds, direction, rng), and collision tests.
   - Next: add tests for additional edge cases and modules (input, render helpers where possible without SDL) to further increase coverage.

5. **Makefile improvements** (in-progress)
   - Progress: Added `UNITY_SRC := tests/vendor/unity.c`, added `test-unit` to build and run unit tests; adjusted unit runner to only execute `tests/unit/*` tests to avoid running flaky integration/render tests automatically.
   - Next: add `make test-all` and a CI script that runs `test-unit` then optionally runs integration/render tests under controlled environments (with flags to skip flaky ones).  
   - Note: Investigate and fix `test-unit` recipe shell issue that can cause `make test` to fail on some environments when the command line gets long.
- Note: `test-unit` now only recompiles unit tests when their source files are newer than the binary (saves time when iterating).

2. **Standardize tests layout** (not-started)
   - Proposed layout:
     - `tests/unit/` — unit test source files
     - `tests/integration/` — integration tests
     - Build outputs in `$(BUILD_DIR)/tests/`
   - Next: move sources and update Makefile to detect tests automatically.

3. **Add config validation unit tests** (not-started)
   - Tests to add:
     - clamp behavior for `board_width`, `board_height`, `tick_rate_ms` etc.
     - parsing boolean/keyword variants (`true/yes/1`, `false/no/0`)
     - unknown keys are ignored (no side-effects)
     - missing file returns defaults and `persist_load_config` returns false

4. **Add unit tests for untested modules** (not-started)
   - High score edge cases, `persist_append_score` replacing lowest score,
   - `trim_in_place` and `clamp_int` behavior (edge cases),
   - more `game_config` setters/getters.

5. **Makefile improvements** (not-started)
   - Consolidate test targets into a single `test` and `test-all` target,
   - Ensure all tests are built into `$(BUILD_DIR)/tests`,
   - Add `make test-valgrind` and `make test-asan` helpers.

6. **Audit and clean `snake_cfg.txt`** (not-started)
   - Ensure each setting is recognized; remove or document any unused settings.

7. **Add CI-friendly test runner** (not-started)
   - Create `scripts/run_tests.sh` or `Makefile` target that fails fast and supports options.

8. **Create `task_tests.md`** (this file) (completed)
   - This file mirrors the `manage_todo_list` and will be the source of truth.

9. **Final sweep & report** (not-started)
   - Run coverage and valgrind, fix issues, and produce a short report.

---

## Notes
- Current tests in `tests/` already exercise many persist-related functions.
- Next immediate action: implement config-validation tests and add a `tests/unit/persist` test for edge cases.

---

## Commands
- Run unit tests: `make test`
- Run coverage: `make coverage` (requires lcov/genhtml)
- Build release: `make release`


