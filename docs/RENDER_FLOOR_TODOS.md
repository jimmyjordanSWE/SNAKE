# Render Floor TODOs & Plan ✅

Date: 2025-12-16

This document records the planned diagnostics and fixes for the floor-texture alignment / "floor sliding" issue.

---

## Summary

The renderer performs floor-casting using an unnormalized per-column ray and precomputed per-row distances. When the camera interpolates between coarse grid updates, we sometimes observe the floor texture moving independently of the walls (seams not lining up).

To diagnose and address this, we will implement a set of diagnostic checks, tests, and candidate fixes. Each item below is actionable and includes acceptance criteria.

---

## TODOs

- [ ] **Add deterministic integration test for floor/wall alignment**
  - File: `tests/integration/render/test_floor_alignment.c`
  - Description: Place the camera at known positions, compare floor UVs to wall-derived world coords; fail if mismatch > epsilon.
  - Acceptance: deterministic test that fails on current behavior when mismatch present.

- [ ] **Implement normalize-ray floor sampling (safe, medium effort)**
  - File: `src/render/render_3d.c` (change sampling) + `tests/unit/render/test_floor_normray.c`
  - Description: Normalize per-column ray_dir before multiplying by rowDist, or add a config toggle `floor_sampling=normalize`.
  - Acceptance: test passes and debug logs show curr_world ≈ wall_world.

- [ ] **Implement per-scanline start + x-step sampling (fast, numeric stability)**
  - File: `src/render/render_3d.c` (refactor loop) + `tests/perf/bench_floor_sampling.c`
  - Description: For each scanline compute a single world-start and per-column (dx,dy) step to reduce rounding differences and improve cache locality.
  - Acceptance: same visual result, improved perf in benchmarks.

- [ ] **Prototype visplane/span merging (Doom-style, high effort)**
  - File: `src/render/render_3d_visplane.c` (experimental)
  - Description: Group horizontal spans by plane and rasterize per-span; used for comparison and long-term performance goals.
  - Acceptance: experimental correctness and perf metrics collected.

- [ ] **Implement pd-based / unit-ray alternative (align to wall base)**
  - Description: Sample floor using unit ray (cos(ray_angle), sin(ray_angle)) * rowDist or using wall `pd` to compute base world coordinate so floor samples align to wall base.
  - Acceptance: debug output shows unit-world ≈ wall-world.

- [ ] **Add bilinear floor sampling + distance shading (low-effort mitigation)**
  - Description: Switch floor sampling from nearest to bilinear (`texture_sample(..., true)`) and apply distance shading (e.g., `texture_shade_from_distance`) to reduce perceived sliding.
  - Acceptance: perceived sliding reduced; configurable and easy to toggle.

- [ ] **Add enhanced floor diagnostics and test harness** *(PRIORITY - diagnostic in-progress)*
  - File: `src/render/render_3d.c`, `tests/integration/render/test_floor_debug.c`
  - Description: Enhance `SNAKE_DEBUG_FLOOR` output to print `curr_world`, `unit_world`, and `wall_world` for configurable columns/rows, and add a test harness to capture these logs deterministically.
  - Acceptance: reproducible debug lines showing mismatch when present.

- [ ] **Automated visual regression & frame-diff tests**
  - Description: Add end-to-end frame capture tests and frame diff scripts under `tests/integration/render/frame_diff/` to compare baseline vs fixes.
  - Acceptance: scripts can produce diffs and metrics; thresholds parameterized to avoid false positives.

- [ ] **Performance measurement & choose preferred approach**
  - File: `docs/RENDER_FLOOR_COMPARISON.md`
  - Description: Run benchmarks for each approach and summarize tradeoffs (accuracy vs CPU). Recommend a preferred solution.
  - Acceptance: documented metrics and recommendation.

- [ ] **Add config options & documentation**
  - Description: Expose fixes via config (e.g., `floor_sampling=normalize|step|visplane|pd|bilinear`) and document behavior in `ANIMATION_SMOOTHNESS_FIX.md` and README; add changelog entry.
  - Acceptance: documented and configurable options with examples.

---

## Notes

- Current in-progress item: **Add enhanced floor diagnostics and test harness** (see acceptance criteria above).
- Quick mitigations: bilinear sampling and distance shading can be applied quickly while we verify a permanent fix.

---

If you want, I can now:
- implement the diagnostics (recommended next step), or
- apply the bilinear+distance-shade mitigation for immediate improvement.

