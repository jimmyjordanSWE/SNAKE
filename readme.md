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

- Debug binary: `./snakegame`
- Release build: `CONFIG=release WERROR=0 make && ./snakegame`

**Note:** Binary is located at `./snakegame` in the project root (Dec 13, 2025 update)

## Documentation

Start with **`impl_00_index.md`** for module status overview, then read individual `impl_*.md` files for details:
- `impl_001_types.md` - Types & constants
- `impl_002_core_game.md` - Game logic & API
- `impl_003_collision.md` - Collision detection
- `impl_004_utils.md` - Utilities (RNG, bounds, platform)
- `impl_005_platform_tty.md` - Terminal control
- `impl_006_display.md` - Display abstraction
- `impl_007_render.md` - 2D rendering
- `impl_008_input.md` - Keyboard input
- `impl_009_persist.md` - High scores & config
- `impl_010_networking.md` - Network (PedroChat relay)
- `impl_011_3d_render.md` - 3D rendering (SDL optional)
	- Note: `Render3DConfig` now includes `show_sprite_debug` to toggle debug overlays/logging.

## Structure

- `include/snake/` - public headers (shared interfaces)
- `src/` - implementation split into independent modules
- `course_context/` - provided course/spec documents

## Multiplayer API reference

The instructor-provided reference implementation and protocol source lives in `Multiplayer_API/`.
