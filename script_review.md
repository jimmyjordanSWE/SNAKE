# Static Analysis Script Review

> Generated: 2025-12-27

## Usefulness Ratings

| Script | Output | Rating | Justification |
|--------|--------|:------:|---------------|
| **summary.py** | `summary_out.txt` | ⭐⭐⭐⭐⭐ **10/10** | Exceptional high-level overview. Shows module purposes, public API lifecycle functions, and entry point call graph in ~30 lines. Perfect for LLM onboarding. |
| **tree_view.py** | `tree_view_out.txt` | ⭐⭐⭐⭐⭐ **9/10** | High-level directory structure visualization. Essential for understanding physical project layout. |
| **structure.py** | `structure_out.txt` | ⭐⭐⭐⭐⭐ **9/10** | Complete AST dump of all functions, structs, typedefs. 701 lines, but token-minimized with `*` prefix shorthand. Essential for navigation. |
| **call_chains.py** | `call_chains_out.txt` | ⭐⭐⭐⭐ **8/10** | Shows module flow, call tree from `main`, and entry points. 36 lines, very useful for understanding control flow. Could expand depth. |
| **dependencies.py** | `dependencies_out.txt` | ⭐⭐⭐⭐ **8/10** | Every file's `#include` list. 52 lines. Good for build order and detecting coupling. No cycle detection shown. |
| **data_flow.py** | `data_flow_out.txt` | ⭐⭐⭐⭐ **8/10** | Struct read/write counts and module data usage. 26 lines. Helpful for understanding ownership patterns (e.g., `GameState: r184 w41`). |
| **hotspots.py** | `hotspots_out.txt` | ⭐⭐⭐⭐ **7/10** | Identifies nested loops (potential O(n²) spots). 13 lines. Useful for performance review. Could add complexity estimates. |
| **memory_map.py** | `memory_map_out.txt` | ⭐⭐⭐⭐ **8/10** | Shows alloc/free patterns with legend. Reports total allocs/frees and file count. Token-minimized format: `line[op]` where op=m/c/r/f. |
| **invariants.py** | `invariants_out.txt` | ⭐⭐⭐⭐ **8/10** | Detects state transitions for status/mode/active/valid flags. Boolean, enum, and NULL assignments. 32 transitions across 7 files. |
| **macros.py** | `macros_out.txt` | ⭐⭐⭐⭐ **7/10** | Catalogs all macros (125 total, 7 func-like). Detects risky patterns: hidden control flow, multi-eval args. Stats summary when clean. |
| **errors.py** | `errors_out.txt` | ⭐⭐⭐⭐ **7/10** | Detects missing NULL checks and ignored returns. Void function whitelist reduces false positives. Reports stats when clean. |

---

## Tier Summary

### High Value (Always Useful)
- `summary.py` – Best-in-class LLM context
- `tree_view.py` – Directory structure overview
- `structure.py` – Complete codebase map
- `call_chains.py` – Control flow understanding

### Good Supporting Context
- `dependencies.py` – Include graph
- `data_flow.py` – Ownership patterns
- `hotspots.py` – Performance flags
- `errors.py` – Safety checks (NULL, ignored returns)
- `memory_map.py` – Allocation tracking with legend
- `invariants.py` – State transition detection
- `macros.py` – Macro catalog and risk detection

---

## Status

All scripts now rate **≥7/10**. Workflow complete.


