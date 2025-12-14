# Snake Implementation Status

**Last Updated:** December 14, 2025  
**Status:** Core gameplay complete, optional features pending

---

## Module Status

| # | Module | Header | Source | Status | Notes |
|---|--------|--------|--------|--------|-------|
| 001 | **Types & Constants** | `types.h` | — | ✅ | Board 40×20, enum SnakeDir/GameStatus |
| 002 | **Core Game Logic** | `game.h` | `game.c` | ✅ | GameState, tick loop, query API |
| 003 | **Collision Detection** | `collision.h` | `collision.c` | ✅ | Wall, self, food, snake-vs-snake |
| 004 | **Utilities** | `utils.h` | `rng.c`, `bounds.c`, `utils.c` | ✅ | RNG, bounds checking, platform calls |
| 005 | **Platform/TTY** | `platform.h`, `tty.h` | `platform.c`, `tty.c` | ✅ | Terminal mode, cursor, colors, timing |
| 006 | **Display Abstraction** | `display.h` | `display_tty.c` | ✅ | Abstract layer, color/glyph API |
| 007 | **Rendering** | `render.h` | `render.c` | ✅ | Board draw, HUD, glyph selection |
| 008 | **Input Handling** | `input.h` | `input.c` | ✅ | Keyboard polling, raw direction bools |
| 009 | **Persistence** | `persist.h` | `persist.c` | ✅ | High scores, config, atomic writes |
| 010 | **Networking** | `net.h` | `net.c` | ⏳ | PedroChat relay, optional multiplayer |
| 011 | **3D Renderer** | `render_3d.h` + 7 headers | 8 files | ⏳ | SDL backend, raycasting, sprites, texture |
| — | **Main Loop** | — | `main.c` | ✅ | Game orchestration, tick accumulator |

---

## Quick Facts

- **Lines of Code:** ~2,700 (core gameplay)
- **Compiler:** Strict flags (`-Wall -Wextra -Werror`)
- **Build Targets:** `debug` (ASan), `release`, `gdb`, `valgrind`
- **Code Quality:** clang-format ✅, clang-tidy ✅, 0 warnings ✅
- **Testing:** No unit tests yet

---

## Build & Run

```bash
make              # Debug build + ASan
./snake           # Play game

make release      # Release build (requires WERROR=0)
make format       # Apply clang-format
make tidy         # Static analysis
make clean        # Remove build artifacts
```

---

## See Also

- **impl_001** - Types & constants definitions
- **impl_002** - Core game logic and API
- **impl_003** - Collision detection rules
- **impl_004** - Utilities (RNG, bounds, platform)
- **impl_005** - Platform/TTY (terminal control)
- **impl_006** - Display abstraction layer
- **impl_007** - Rendering (board, HUD)
- **impl_008** - Input handling (keyboard)
- **impl_009** - Persistence (scores, config)
- **impl_010** - Networking (PedroChat)
- **impl_011** - 3D Rendering (SDL backend)

---

For details on any module, open the corresponding impl_*.md file.
