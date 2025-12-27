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

## Static Analysis

Analysis runs automatically on every build. View results:

```bash
cat scripts/out/errors_out.txt     # Safety issues
cat scripts/out/memory_map_out.txt # Allocation tracking
cat PROJECT_CONTEXT.txt            # Full symbol tree
```

## Adding Code

1. **Headers** — One public header per module in `include/snake/`
2. **Implementation** — Corresponding `.c` in `src/<module>/`
3. **Internal** — Use `*_internal.h` for private types
4. **Tests** — Add targets to `Makefile`

## Commit Guidelines

- Build must pass with `make` (errors = failure)
- Empty `errors_out.txt` required
- Format code before committing

## LLM Agent Integration

This project embeds LLM context in `GEMINI.md`. When using AI assistants:

1. Ensure `GEMINI.md` is loaded as system context
2. Run `make analyze` to refresh `PROJECT_CONTEXT.txt`
3. Analysis scripts prevent redundant code generation
