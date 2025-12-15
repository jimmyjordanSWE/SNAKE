# Changelog

## Unreleased

- Fixed `snake_rng_range` to handle full `INT_MIN..INT_MAX` range without modulo-by-zero and added tests (`tests/utils/test_rng.c`).
- Improved `tty` UTF-16 handling: detect surrogate pairs and encode them to UTF-8 (`src/platform/tty.c`) and added `tests/platform/test_tty_surrogate.c`.
- Adjusted input handling: `any_key` now only set for meaningful input (non-newline bytes and escape sequences) and added tests (`tests/test_input.c`).
- Removed redundant `len < 0` check in `draw_centered_string` (`src/render/render.c`).
- Updated `REVIEW_PROGRESS.md` to reflect applied fixes.
