# Fast inverse-square-root (Carmack) — details

Date: 2025-12-28

Summary
- Added `include/snake/math_fast.h` with `fast_inv_sqrt(float)` using the classic Carmack magic constant and one Newton–Raphson iteration.
- Applied to rendering-sensitive sites without adding a toggle (user requested no toggle).

Files changed
- `include/snake/math_fast.h`
- `src/render/sprite.c` (used in normalized screen position computation)
- `src/render/camera.c` (used in `camera_distance_to_point`)

Rationale
- Sqrt is relatively expensive; rendering pipelines often benefit from faster approximations when exact distance precision is unnecessary.
- One NR iteration provides a good accuracy/perf tradeoff for screen-space computations.

Safety
- The helper checks for `x <= 0.0f` and returns `0.0f` to avoid undefined behavior.
- Visual/behavioral impact is expected to be negligible; however, please test edge cases.

Reproduce
- Build: `make release`
- Smoke-run: `./snakegame.out`

Notes
- If a later change requires higher accuracy, we can add a second NR iteration or make the helper configurable.
