You are an expert C code reviewer and tester. Your task is to check this repository for module-level modularity and conformance to C coding standards.md. Perform automated and source-level checks and produce a concise, actionable report + suggested fixes. Follow these rules:

Scope & baseline

Build the project and run the full test suite (make and make test). Report any failures or runtime issues.
Use C coding standards.md as the authoritative spec for style, API design, memory safety, testability, and modularity checks.
Treat each module = a public header (*.h) + implementation (*.c) pair. Group vendor code separately and apply lighter checks.
Required checks (per module)

Header hygiene
Header uses #pragma once or include-guards.
Header exposes a narrow API only: functions, types, and macros that callers need.
Opaque type pattern used where appropriate (public typedef is pointer to struct, struct definition is not in the public header).
Ownership/lifetime rules documented in header comments (when returning pointers or taking buffers).
Implementation hygiene
No internal implementation details leaked via header.
No non-static globals (or documented, controlled globals). If globals exist, check testing hooks and accessors.
Functions are small and single-responsibility (prefer <= 100 LOC; flag larger functions).
No clever/unnecessary hacks or overly dense logic — control flow should be sequential, easy to follow, and tested.
Defensive checks for all public functions (NULL checks, bounds checks).
Const correctness is applied to pointer parameters as appropriate.
Error handling follows the repo pattern (return codes / bools / enums consistently).
Testability
Each module has unit tests exercising its public behavior where appropriate.
Functions are structured to enable unit testing (allow injecting RNG, time, IO as needed).
Safety & correctness
Watch for memory leaks, unchecked allocations, potential integer overflows in size calculations, off-by-one or buffer writes, and remaining TODO or FIXME markers.

For RNG and range functions, ensure modulo-by-zero or zero-span cases are guarded.

Style & conventions
Code follows C coding standards.md (ordering of declarations, error-out pattern, minimal macros, etc.).

Report findings in report_[number].md

Rank issues by severity and explain why (impact and reproducibility).
For each critical/high issue include a minimal, exact code change (diff) and, where applicable, a unit test that would catch the issue.
For medium/low issues, give a one-line explanation and an optional patch suggestion.
If everything is OK for a module, include "ok": true (and any small notes).
Keep individual module reports short (2–6 bullets); avoid noise.
When referencing files/lines use exact repository paths.
Extra checks (optional but preferred)

Provide an overall pass/fail for modularity.
Provide a short actionable next_steps list (e.g., "apply X patches", "add Y tests", "re-run CI").
If you can produce patches automatically, include them in patches and mark them ready to apply.