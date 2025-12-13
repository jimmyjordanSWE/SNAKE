# Snake (School Project)

Terminal Snake project designed to meet the course requirements (C/C++) and the Multiplayer Snake competition spec.

## Build

- Default (debug + ASan): `make`
- Every build runs `clang-format` (check) by default.
- Clean: `make clean`
- Release: `CONFIG=release WERROR=0 make` (GCC false positive in main.c requires WERROR=0)
- Debug with gdb: `make gdb`
- Valgrind run (builds without ASan): `make valgrind`

## Lint / format

- Check formatting: `make format-check`
- Apply formatting: `make format`
- Run clang-tidy: `make tidy`
- Run clang-tidy on every build: `make RUN_TIDY=1`
- Skip formatting check on build: `make RUN_FORMAT=0`

## Run

- Debug binary: `./snake`
- Release build: `CONFIG=release WERROR=0 make && ./snake`

**Note:** Binary is located at `./snake` in the project root (Dec 13, 2025 update)

## Structure

- `include/snake/` - public headers (shared interfaces)
- `src/` - implementation split into independent modules
- `design_docs/` - design notes per module
- `design_docs/60_networking.md` documents the PedroChat multiplayer connection method
- `course_context/` provided course/spec documents

## Multiplayer API reference

The instructor-provided reference implementation and protocol source lives in `Multiplayer_API/`.
