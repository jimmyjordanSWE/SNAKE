# Optimization Plan & Todos ⚡️

This document lists prioritized optimization opportunities for the project, concrete tasks to implement them, and measurement steps to verify impact.

---

## Goals
- Improve runtime performance (wall time / CPU usage) of release builds with minimal risk.
- Make performance reproducible and measurable (benchmarks + regression thresholds).
- Prioritize low-risk toolchain changes first, then targeted code optimizations with tests and benchmarks.

---

## Basics
1. Implement average frame timer for 3D rendering. This is what we optimize. Actual game logic is trivial code.
2. Start game, run for 5 seconds. Observe printed avg frametime printed to the console. (or log to file for historical lookback of optimization effects)

## Overview & Priorities (short)
1. Build & toolchain (LTO, -march=native, PGO) — **low effort, high impact** ✅
2. Terminal rendering (`tty_flip`) micro-optimizations — **low effort, high ROI for tty mode** ✅
3. Hot-loop refactors (game loop, collision, rendering loops) — **medium effort, high ROI**
4. Sprite sorting & rendering pipeline improvements — **medium effort**
5. Memory / allocation improvements and reuse — **low-to-medium effort**
6. Continuous perf testing, PGO flow — **higher effort, large payoff**

---

## Measurement & Baseline (must do before changes)
- Create reproducible benchmark run for the game in headless mode:
  - Example script: `./scripts/bench/bench_run.sh`
  - Use SDL dummy driver: `SDL_VIDEODRIVER=dummy ./snakegame.out` or a test harness that exercises typical gameplay.
- Collect metrics:
  - `time -v ./snakegame.out` (wall time + CPU)
  - `perf stat -e cycles,instructions,cache-misses,branch-misses -r 5 ./snakegame.out`
  - `perf record` / `perf report` when needed for hotspots
- Store baseline logs in `build/logs/perf_baseline.txt`
- Acceptance threshold: any change should target measurable improvement (e.g., 5%+ wall-time or CPU cycles improvement), otherwise revert.

---

## Detailed Todos

### 1) Build Flags & Linker Optimizations (Quick win) 🔧
Priority: High | Risk: Low

- [ ] Modify `Makefile`:
    - Add to `RELEASE_FLAGS`:
      - `-march=native -flto -fdata-sections -ffunction-sections -fno-plt`
    - Add to `LDFLAGS`:
      - `-flto -Wl,--gc-sections`
    - Keep `-O3 -DNDEBUG` as-is.
- [ ] Run `make release` and verify build success.
- [ ] Run baseline benchmarks (see Measurement section) and compare.
- Acceptance: Build works and shows measurable improvement over baseline.

Notes:
- If cross-compiling for CI, guard `-march=native` with a `MAKE` variable or document opt-in.

Estimated effort: 1–2 hours

---

### 2) Profile Guided Optimization (PGO)
Priority: High | Risk: Medium

- [ ] Add PGO build steps to `Makefile` or CI scripts (optional separate `CONFIG=pgo-generate`):
    1. Compile with `-fprofile-generate -O3`.
    2. Run a representative workload that exercises common code paths (automated script).
    3. Rebuild with `-fprofile-use -O3`.
- [ ] Run benchmarks and compare to baseline.
- Acceptance: measurable improvement; ensure PGO profile data is cached in build artifacts for reproducible builds.

Estimated effort: 1 day to set up + runs

---

### 3) `tty_flip` micro-optimizations (Terminal rendering) ✨
Priority: High for tty mode | Risk: Low

Why: `scripts/out/hotspots_out.txt` shows `tty_flip` as a hotspot. Replacing `snprintf` calls and repeated buffer checks will reduce CPU and allocations.

- [x] Implement these sub-tasks:
  - [x] Replace `snprintf` in `tty_flip` with a lightweight integer-append routine to write `\x1b[%d;%dH` (avoid sprintf overhead).
  - [x] Precompute ANSI color escape strings for all palette entries (0–15) and index into them instead of `snprintf`.
  - [x] Replace repeated `if (n < 0 || (size_t)n >= remaining)` checks with a small helper that appends safely or returns an error early.
  - [ ] Add unit tests asserting that terminal sequences are correct for a few representative cases.
  - [x] Add microbench: run `tty_flip` repeatedly in a small harness and measure cycles.
- Acceptance: `tty_flip` CPU cycles reduced by measurable percent; no behavior regressions.

Estimated effort: 1–2 days

---

### Render benchmark and first optimization
Priority: High | Risk: Low

- [x] Add `src/tools/render_bench.c` micro-benchmark that exercises raycasting, projection and texture sampling with a configurable screen size.
- [x] Add `make bench-render` target which saves `build/logs/perf_render_bench_latest.txt`.
- [x] Collect baseline (no precomputations): `make bench-render` → `render_bench: frames=200 screen=320x200 total_ms=36.281 avg_ms_per_frame=0.181407` (example run).
- [x] Implement small optimization: precompute per-column ray angle offsets using `camera_fill_ray_angle_offsets()` and reuse during frame.
- [x] Run optimized bench (enable with `env PRECOMP_ANGLE=1 make bench-render`) → `render_bench: frames=200 screen=320x200 total_ms=29.969 avg_ms_per_frame=0.149846` (example run).
- Acceptance: ~17% improvement on this microbench for the precompute optimization.

Notes:
- The optimization adds `camera_fill_ray_angle_offsets()` to `src/render/camera.c` and uses it in `src/tools/render_bench.c` under env toggle `PRECOMP_ANGLE`.
- Next step: integrate precomputed offsets into `render_3d_draw` (hoist to per-frame) and run full integration benchmarks. This will be implemented carefully to preserve semantics and stability.



---

### 4) Sprite rendering & sorting
Priority: Medium | Risk: Medium

- [ ] Investigate typical sprite counts and distribution.
- [ ] If small N (under ~64), consider insertion sort or optimized in-place stable sort with small fixed-size buffers.
- [ ] Consider bucket/radix sort if depth range is known and small.
- [ ] Reduce repeated memory allocations during sprite projection / draw.
- [ ] Add tests and benchmarks to compare qsort vs optimized approaches.
- Acceptance: Sorting time reduced for typical workloads with same visual results.

Estimated effort: 1–3 days depending on approach

---

### 5) Hot-loop optimizations in `snake_game_run`, `game_tick`, `collision_detect_and_resolve`
Priority: High | Risk: Medium

- [ ] Audit the functions flagged in `scripts/out/hotspots_out.txt`.
- [ ] Inline trivial helpers where appropriate and safe (adhere to coding standards).
- [ ] Remove redundant checks from inner loops, hoist invariants out of loops, minimize branching where safe.
- [ ] Add unit tests to preserve semantics.
- [ ] Benchmark before/after to quantify improvement.
- Acceptance: Reduced CPU cycles in critical loops without changing behavior.

Estimated effort: 1–4 days

---

### 6) Memory & Allocation improvements
Priority: Medium | Risk: Low

- [ ] Avoid per-frame allocation (e.g., reuse temporary buffers in `render` or `tty` subsystems).
- [ ] Use `calloc` only during init or resize; reuse existing buffers otherwise.
- [ ] Verify no memory leaks with ASAN/valgrind and ensure behavior unchanged.
- Acceptance: Reduced memory churn and fewer allocations during steady-state.

Estimated effort: 1–2 days

---

### 7) Continuous performance testing & CI
Priority: High (long term) | Risk: Low

- [ ] Add `scripts/bench/*` that run bench harnesses and generate perf output.
- [ ] Add a CI job that runs a small set of benchmarks on push to `main` and reports regressions (thresholds configurable).
- [ ] Store historical logs in `build/logs/perf_history/` and plot basic trends.
- Acceptance: CI job runs and alerts on regressions > X% (configurable).

Estimated effort: 2–3 days to set up

---

## Rollback & Safety
- Make small, incremental changes with measurement after each step.
- Keep unit tests and behavior tests passing. If speed improvements break rendering or gameplay, revert the change.
- Document all changes in PRs and link to bench logs and `perf` outputs.

---

## PR Checklist (for each optimization)
- [ ] Add/Update unit tests covering changed behavior
- [ ] Add benchmark that demonstrates improvement
- [ ] Provide before/after perf logs in PR description
- [ ] Ensure `make release` works locally
- [ ] Run `make unit-tests` and `make analyze`

---

## Next steps (suggested order)
1. Implement Makefile LTO and `-march=native` change and record baseline improvements. (Quick) ✅
2. Implement `tty_flip` micro-optimizations and measure terminal-mode improvements. (Medium)
3. Set up PGO pipeline and CI perf job. (Longer-term)
4. Tackle hot-loop and sorting optimizations with careful tests. (Medium)

---

## Notes / Resources
- Hotspots analysis available at `scripts/out/hotspots_out.txt`.
- Use `perf` and PGO as described above for profiling.

---

If you'd like, I can start with the Makefile changes (Option A) and run the baseline comparison automatically — say the word and I'll implement the low-risk build flags next.