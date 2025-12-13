# Snake (School Project)

Terminal Snake project designed to meet the course requirements (C/C++) and the Multiplayer Snake competition spec.

## Build

- Default (debug + ASan): `make`
- Every build runs `clang-format` (check) by default.
- Clean: `make clean`
- Release: `make release`
- Debug with gdb: `make gdb`
- Valgrind run (builds without ASan): `make valgrind`

## Lint / format

- Check formatting: `make format-check`
- Apply formatting: `make format`
- Run clang-tidy: `make tidy`
- Run clang-tidy on every build: `make RUN_TIDY=1`
- Skip formatting check on build: `make RUN_FORMAT=0`

## Run

- Default build: `./build/debug-asan/snake`
- Release build: `./build/release/snake`

## Structure

- `include/` public headers (shared interfaces)
- `src/` implementation split into independent modules
- `design_docs/` design notes per module
- `course_context/` provided course/spec documents
