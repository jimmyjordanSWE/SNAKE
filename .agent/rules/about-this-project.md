---
trigger: always_on
---

The project follows a strict set of C coding standards, which are documented in `C coding standards.md`. Key conventions include:

*   **Opaque Pointers:** Hiding implementation details using opaque pointers.
*   **Error Handling:** Using an "error-out" pattern for cleanup.
*   **Dependency Injection:** Using capability structs to pass function pointers for dependencies.
*   **Memory Safety:** Strict rules for memory allocation and deallocation.

## Formatting

The project uses `clang-format` for code formatting. There are two formatting styles defined:

*   **`format-llm`:** A style optimized for LLM token usage.
*   **`format`:** A more human-readable style.

You can format the code using the following commands:

```bash
make format-llm
make format
```

# Project Context & Static Analysis

To provide superior context for LLMs, this project runs a suite of static analysis scripts on **every** `make` execution. These scripts generate token-minimized overviews of the codebase, highlighting structure, memory mapping, call chains, and more.

## Automated Analysis Suite

The project's analysis outputs are generated into `scripts/out/` and include:
- `structure_out.txt`: AST-based project structure
- `memory_map_out.txt`: Ownership and allocation patterns
- `call_chains_out.txt`: Function reachability and comprehensive call tree
- `errors_out.txt`: Error handling and safety checks
- (Additional outputs in `scripts/out/`)

To refresh the analysis run:

```bash
make analyze
```

