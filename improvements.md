# Improvements log (detailed) 📋

This file is a chronological log of *concrete* changes made for performance work. Each entry records:
- What I changed (files and short description)
- Reproduction command(s)
- Measured *before* and *after* numbers (when available)
- Notes and next steps

---

## [2025-12-28] 001 — Toolchain: LTO & CPU flags (low-risk)
- Files changed: `Makefile`
- Change: Added to `RELEASE_FLAGS`: `-march=native -flto -fdata-sections -ffunction-sections -fno-plt`; added to `LDFLAGS`: `-flto -Wl,--gc-sections`.
- Reproduce: `make release`
- Before: No link-time optimization / cpu-specific flags in release. (No microbench recorded.)
- After: Build completes with LTO enabled. Measure effect by running render/tty benches (next steps).
- Notes: Low-risk change; document in PR. For cross-builds, consider making `-march=native` opt-in.

---

## [2025-12-28] 002 — TTY: `tty_flip` micro-optimizations ⌨️
- Files changed: `src/platform/tty.c`
- Change: Replaced `snprintf` usage in hot-path with small `buf_append_uint`/`buf_safe_append_char` helpers; precomputed ANSI escape strings (`ANSI_FG`, `ANSI_BG`) and lengths; captured `write()` return values to satisfy `-Werror`.
- Reproduce:
  - Build: `make bench-tty`
  - Run: `script -q -c "env SDL_VIDEODRIVER=dummy build/tty_bench.out" build/logs/perf_tty_bench_latest.txt`
- Before: (no recorded prior numeric baseline in repo; observed earlier higher timings in unoptimized runs)
- After: Example run: `tty_bench: iters=2000 total_ms=7.020 avg_ms=0.003510 ms/flip` (results vary with PTY/terminal conditions)
- Notes: Added `src/tools/tty_bench.c` and Make targets; absolute times vary, but harness enables repeatable comparisons.

---

## [2025-12-28] 003 — Bench harness: `tty_bench` & `render_bench` 🧪
- Files added: `src/tools/tty_bench.c`, `src/tools/render_bench.c`; Makefile targets `bench-tty`, `bench-render`
- Purpose: Provide quick, reproducible microbenchmarks for terminal rendering and 3D rendering.
- Reproduce:
  - `make bench-tty` then `make bench` (TTY harness)
  - `make bench-render` (render microbench)
- Notes: Save logs under `build/logs/` for CI or historical comparison.

---

## [2025-12-28] 004 — Render micro-optimization: precompute per-column ray angle offsets (opt-in) 🧠
- Files changed: `src/render/camera.c`, `include/snake/render_3d_camera.h`, `src/tools/render_bench.c`
- Change: Added `camera_fill_ray_angle_offsets(const Camera3D*, float* out)` and optional use in bench when `PRECOMP_ANGLE=1` (bench-only opt-in for now).
- Reproduce:
  - Baseline: `make bench-render` -> example: `render_bench: frames=200 screen=320x200 total_ms=36.281 avg_ms_per_frame=0.181407`
  - Opt-in: `env PRECOMP_ANGLE=1 make bench-render` -> example: `render_bench: frames=200 screen=320x200 total_ms=29.969 avg_ms_per_frame=0.149846`
- Measured delta: ~17.3% total time reduction on this microbench
- Notes: Bench is synthetic but targets the heavy path (raycasting + projection + texture sampling). Next: integrate the precompute into `render_3d_draw` so the code benefits during real runs and measure again.

---

## [2025-12-28] 005 — Perf tooling note
- Action: Installed `linux-tools-common` and `linux-tools-generic`.
- Issue: On WSL, kernel-specific `linux-tools-<kernel>` is missing for the running kernel; `perf` warns and cannot run meaningful counters here.
- Notes: Use host Linux with matching `linux-tools` for cycle-accurate analysis or rely on bench harnesses until `perf` is available.

---

## [2025-12-28] 006 — Integration: precomputed angles used per-frame in `render_3d_draw` 🔬
- Files changed: `src/render/render_3d.c`
- Change: Added a static `s_angle_offsets[]` buffer sized to `screen_w` and call `camera_fill_ray_angle_offsets()` once per frame; used `ray_angle = interp_cam_angle + s_angle_offsets[x]` when available to avoid per-column trig.
- Reproduce (raw data saved):
  - Before (raw): `build/logs/perf_render_before_integration.txt` — sample: `render_bench: frames=200 screen=320x200 total_ms=33.225 avg_ms_per_frame=0.166125`
  - After (raw): `build/logs/perf_render_after_integration.txt` — appended runs show e.g. `33.482`, `32.970`, `33.872`, `33.406` ms
  - Precompute-opt (bench-only raw): `build/logs/perf_render_precomp_opt.txt` — e.g. `render_bench: frames=200 total_ms=29.969` when opt-in
- Measured effect: integrated change produced *no meaningful improvement* on the full render bench (after average ~33.43 ms vs before ~33.225 ms). The bench-only opt-in still shows improvement in isolation; suggests other pipeline costs dominate.
- Notes & next steps:
  - Save raw logs for inspection: `build/logs/perf_render_before_integration.txt`, `build/logs/perf_render_after_integration.txt`, `build/logs/perf_render_precomp_opt.txt`.
  - Next: profile inner hotspots (texture sampling, sprite pipeline) and apply targeted micro-optimizations, recording raw logs for each change.

---

## Repro quick commands
- Build release: `make release`
- Run TTY bench: `make bench-tty && make bench` (logs -> `build/logs/perf_tty_bench_latest.txt`)
- Run render bench (baseline): `make bench-render` (logs -> `build/logs/perf_render_bench_latest.txt`)
- Run render bench (precomputed angles): `env PRECOMP_ANGLE=1 make bench-render`

---

## Next steps / TODO (short)
- Integrate precomputed column angles into `render_3d_draw` per-frame and re-measure (I can implement this next). ✅
- Optimize sprite sorting (bucket/radix) and inner texture sampling if further gains are needed.
- Add PGO and CI perf job to capture regressions and track improvements over time.

---

If you'd like, I can open a PR with the above changes and attach these logs and bench outputs. Let me know if you prefer the log entries to include exact git SHAs or PR links (I can add those when I open the PR).

- **New:** Added a fast inverse-square-root implementation and applied it to rendering paths (no toggle). See `docs/fast_inv_sqrt.md` for details.

---

## [2025-12-29] 007 — Texture sampling: fast integer bilinear sampler (low-risk, measurable)
- Files changed: `src/render/texture.c`, `include/snake/render_3d_texture.h`
- Change: Added `sample_bilinear_fast()` (Q8 fixed-point integer bilinear interpolation) and a fast-path in `sample_bilinear()` for coordinates in [0,1), falling back to the original float implementation otherwise.
- Reproduce (raw data saved):
  - Texture microbench (before): `build/logs/perf_texture_before.txt` -> `texture_bench: iters=2000000 total_ms=135.545 avg_ns=67.772`
  - Texture microbench (after):  `build/logs/perf_texture_after_fast.txt` -> `texture_bench: iters=2000000 total_ms=128.446 avg_ns=64.223`
- Measured delta (texture microbench): ~5.2% faster (135.545 -> 128.446 ms)
- Render bench effect (full pipeline): multiple runs saved in `build/logs/perf_render_after_texture_opt.txt` show mostly ~32.5 ms total (avg of stable runs ≈ 32.53 ms) vs prior integrated average ≈ 33.43 ms → **~2.7% improvement** on full render bench (after removing outliers and averaging stable runs).
- Raw logs:
  - `build/logs/perf_texture_before.txt`
  - `build/logs/perf_texture_after_fast.txt`
  - `build/logs/perf_render_after_texture_opt.txt`
- Notes / Next steps:
  - Texture sampling improvement is modest but measurable in isolation (~5%), and yields a small (~2.7%) win in the full renderer after averaging stable runs.
  - Next target: `sprite_project_all`, `sprite_sort_by_depth`, and `sprite_draw` (high impact according to hotspots). I'll add a bench or instrumentation to count time spent in those functions and iterate.