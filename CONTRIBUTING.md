# Contributing

Guidelines for contributing to the SNAKE project.

## Build Configurations

| Command | Description |
|---------|-------------|
| `make` | Debug + AddressSanitizer |
| `make release` | Optimized build |
| `make valgrind` | Memory leak detection |
| `make gdb` | Debug with GDB |

## Code Style

Follow [C coding standards.md](docs/C%20coding%20standards.md). Key rules:

- **Opaque pointers** — Hide implementation in `.c` files
- **Error-out pattern** — Use `goto out` for cleanup
- **Zero-init** — Always `struct foo f = {0};`
- **Bounded strings** — Use `snprintf`, never `sprintf`
- **No `_t` suffix** — POSIX reserved

## Formatting

```bash
make format      # Human-readable style
make format-llm  # Token-optimized for LLM
```

Formatting is enforced via `.clang-format`. Run before committing.

## LLM Context Generation

- Regenerate context with: `make analyze` (outputs go to `scripts/out/`).
- The Makefile calls the project's analysis scripts (Python) in `.venv` so the results are reproducible.

## Adding Code

1. **Headers** — One public header per module in `include/snake/`
2. **Implementation** — Corresponding `.c` in `src/<module>/`
3. **Internal** — Use `*_internal.h` for private types

## Commit Guidelines

- Build must pass with `make` (errors = failure)
- Empty `errors_out.txt` required
- Run `make format` before committing.
