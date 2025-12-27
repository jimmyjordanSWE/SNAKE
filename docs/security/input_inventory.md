# Security: Input Inventory

**Status:** Updated — partial remediation in progress

## Summary
This document lists *external/untrusted input sources* found in the codebase, with a brief risk note and an action priority. This update records work completed during the audit (static analysis, sanitizer testing, unit test additions, short fuzz smoke runs, and formatting) and outstanding actions that remain.

## High-priority sources (status summary)

1. Network (src/net/net.c) — Status: Addressed (validation + tests + fuzz smoke) ✅
   - Actions completed: added strict length and integer-overflow checks, rejected non-positive `width`/`height` in `net_unpack_game_state()`, initialized variables, and improved overflow guards.
   - Tests added: `tests/test_net.c` (truncated payload, huge counts, zero-dimension checks).
   - Fuzzing: `fuzz_net` built and short smoke runs executed (no crashes observed in short runs).
   - Remaining: run longer fuzz campaigns, add CI fuzz monitoring, and map call chains for deeper analysis.

2. Persist / Config / Highscores (src/persist/persist.c) — Status: Partially addressed ⚠️
   - Actions completed: added unit tests `tests/test_persist.c` for malformed and boundary lines; built `fuzz_persist` and ran short smoke fuzz runs.
   - Remaining: review and harden `memcpy` usage and parsing logic (explicit bounds and NUL handling), and run longer fuzz campaigns; add regression tests for any fixes.

3. Image loader & vendor code (src/vendor/stb_image.c) — Status: Not addressed (high priority) ❌
   - Observations: vendor parser has complex binary parsing and many `memcpy`/`fread` sites.
   - Actions needed: add a dedicated fuzz harness targeting `stb_image` file ingestion, pin vendor revision and check upstream CVEs, consider swapping for a hardened alternative if issues found.

4. Texture loading and path building (src/render/texture.c) — Status: Needs remediation ⚠️
   - Observations: uses `snprintf` then `strcat` and `readlink`; some unguarded concatenation risks remain.
   - Actions needed: replace `strcat` with bounds-checked concatenation, consistently verify `snprintf` return values, and validate path components.

5. Platform TTY and buffers (src/platform/tty.c) — Status: Needs remediation ❌
   - Observations: uses `strncpy`/`strcpy` with manual NUL handling, and allocations depend on terminal dimensions.
   - Actions needed: avoid `strcpy`, validate `tty_path` length, cap allocations, and add integer-overflow checks when computing sizes.

6. Stdin / input polling (src/input/input.c, src/render/render.c) — Status: Needs tests & validation ❌
   - Observations: raw reads (`read(STDIN_FILENO, ...)`) are passed to parsers; we have not yet added exhaustive tests for very long/malformed input.
   - Actions needed: add targeted tests for malformed/oversized input and strengthen parser bounds checks.

7. Other notable uses — Status: Not fully audited ⚠️
   - Environment variables (getenv) in `src/render/render_3d.c` are treated as untrusted but need explicit validation.
   - Various `snprintf` calls across the codebase should be audited to ensure correct return checks and buffer sizing.

## What we completed during this audit ✅
- Static analysis: `bash scripts/run_static_analysis.sh` (clang-tidy outputs in `scripts/out/`).
- ASAN/UBSAN builds and unit tests: `make CONFIG=debug-asan build && bash scripts/run_unit_tests.sh` (tests pass).
- Unit tests: added parser edge-case tests for `net` and `persist`.
- Fuzz smoke: built fuzz targets (`fuzz_net`, `fuzz_persist`) and ran short smoke runs (no crashes observed).
- Formatting: applied `clang-format` repo-wide and validated that tests still pass.
- Audit doc: updated `docs/security/audit_report.md` with details, changelog, and next steps.

## Remaining high-priority tasks 🔜
- Add a dedicated fuzz harness for `src/vendor/stb_image.c` and run long fuzz campaigns (hours) — HIGH priority.
- Harden parsing in `src/persist/persist.c` (explicit NUL-termination and guarded `memcpy`).
- Replace unsafe `strcat`/`strcpy` uses in `src/render/texture.c` and `src/platform/tty.c` with bounded APIs and add tests.
- Add CI checks: `clang-format --check`, `make CONFIG=debug-asan test`, and short fuzz smoke runs on PRs.
- Map call chains from inputs to sinks for `net`, `persist`, and `stb_image` (trace and document transformations).

## Audit traceability
- Tests and fixes are present in `tests/` and source edits (see `src/net/net.c` changes).
- Short fuzz smoke artifacts and outputs are in `build/fuzz/` and were run locally; longer runs should be archived for traceability if executed in CI.
- See `docs/security/audit_report.md` for reproductions, commands, and a changelog of fixes made during this session.

---

*Updated: December 27, 2025 — summary of work done and remaining actions. If you want, I can start on the `stb_image` fuzz harness now (high priority) or add the CI pre-commit/PR checks next.*

