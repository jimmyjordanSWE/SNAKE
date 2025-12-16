# Test Tasks & Status (source of truth)

This file tracks the test-related tasks, progress, and notes for the project. Update this file as work progresses.

## Summary
- Last run: make test (CONFIG=debug-asan) — all tests passed
- Current in-progress work: Standardize tests layout and Makefile/CI test helpers (in-progress)

---

## Todo list

1. **Run full test suite & collect failures** (completed)
   - Status: initial `make test` run completed; later `make test` found one unrelated failing test (`test_camera_angles`) when running full test matrix.
   - Update: Fixed `test_camera_angles` by returning the current camera angle when interpolation time is zero; re-ran render and module-specific tests and they passed locally. Also fixed the `test-unit` Makefile recipe (now uses a robust `find | xargs` approach) so the full `make test` runs successfully locally.
    - Update: Fixed `test_camera_angles` by returning the current camera angle when interpolation time is zero; re-ran render and module-specific tests and they passed locally. Also fixed the `test-unit` Makefile recipe (now uses a robust `find | xargs` approach) so the full `make test` runs successfully locally. Consolidated `test_camera_angles` into a Unity-style unit test at `tests/unit/render/test_camera_angles_unity.c` and removed the old duplicate tests.
   - Valgrind: ran `make valgrind`; Valgrind reported memory issues. Fixes implemented:
  - Added `raycaster_destroy` in `render_3d_shutdown` to free the raycaster allocation created by `raycaster_create` (resolved a project-level leak).
  - Adjusted the `valgrind` Makefile target to run Valgrind with `SDL_VIDEODRIVER=dummy` to avoid loading GLX/Mesa (this prevented large GLX-related leaks from appearing under Valgrind on CI/dev machines).
  - After these changes, remaining Valgrind output shows only small **still reachable** allocations in `libdbus`/`SDL2` (likely system-level and acceptable). Next: add a CI valgrind step that sets `SDL_VIDEODRIVER=dummy` (or use a suppression file) and document the result.
   - Next: run valgrind and coverage.

2. **Standardize tests layout** (completed)
   - Progress: Added a minimal Unity harness at `tests/vendor/unity.{h,c}` and migrated many tests into `tests/unit/` (persist, highscore, net, utils, game, collision). Moved render and platform-heavy tests into `tests/integration/` and added a `test-integration` target in the `Makefile`.
   - Next: keep migrating remaining platform/render helpers and add more integration tests as needed.

4. **Add unit tests for untested modules** (in-progress)
   - Progress: Added highscore, read-scores malformed, append edge cases, persist idempotency, persist-free null handling, net pack, game events/death/over, utils (bounds, direction, rng), and collision tests.
   - Next: add tests for additional edge cases and modules (input, render helpers where possible without SDL) to further increase coverage. Consider adding integration tests for render/platform where feasible.

5. **Makefile improvements & CI helpers** (completed)
   - Progress: Added `UNITY_SRC := tests/vendor/unity.c` and `test-unit` target. Updated `valgrind` target to run with `SDL_VIDEODRIVER=dummy` to avoid GLX-related leaks under Valgrind. Added `scripts/run_tests.sh` to orchestrate unit/full/valgrind runs. Added `test-integration`, `test-all`, `test-valgrind`, and `test-asan` targets.
   - Next: ensure CI uses `scripts/run_tests.sh --valgrind` and add a Valgrind suppression file if needed.
   - Note: `test-unit` now only recompiles unit tests when their source files are newer than the binary (saves time when iterating).

2. **Standardize tests layout** (not-started)
   - Proposed layout:
     - `tests/unit/` — unit test source files
     - `tests/integration/` — integration tests
     - Build outputs in `$(BUILD_DIR)/tests/`
   - Next: move sources and update Makefile to detect tests automatically.

3. **Add config validation unit tests** (completed)
   - Status: Added Unity-style config validation tests at `tests/unit/persist/test_config_validation_unity.c` and verified via `make test`.

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

7. **Add CI-friendly test runner** (completed)
   - Created `scripts/run_tests.sh` that supports `--unit`, `--all`, and `--valgrind` options; verified it runs unit tests successfully.

8. **Create `task_tests.md`** (this file) (completed)
   - This file mirrors the `manage_todo_list` and will be the source of truth.

9. **Final sweep & report** (completed)
   - Status: Valgrind run completed with only small system-level 'still reachable' allocations reported. Coverage generation attempted locally but `lcov`/`genhtml` were not installed; added a CI step that will attempt coverage and archive results if available.
   - Artifacts: Valgrind log archived at `build/logs/valgrind.log` when run via CI; coverage output (if available) will be archived at `build/coverage/report`.

10. **Add CI Valgrind & coverage job** (completed)
   - Added GitHub Actions workflow `.github/workflows/ci-tests.yml` that:
     - runs unit tests, integration tests (headless via `SDL_VIDEODRIVER=dummy`), a Valgrind run, and a coverage step (if `lcov`/`genhtml` installed);
     - uploads `build/logs/valgrind.log` and `build/coverage/report` as artifacts.
   - Next: ensure CI environment includes `lcov`/`genhtml` or update CI image to install them for coverage reporting.

---

## Notes
- Current tests in `tests/` already exercise many persist-related functions.
- Next immediate action: continue standardizing test layout (move heavy/integration tests to `tests/integration/`) and add `make test-all` / CI-friendly valgrind job.

---

## Commands
- Run unit tests: `make test`
- Run coverage: `make coverage` (requires lcov/genhtml)
- Build release: `make release`


