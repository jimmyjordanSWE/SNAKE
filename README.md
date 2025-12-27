# SNAKE 🐍

A C99 snake game implementation featuring dual rendering modes: pseudo-3D SDL2 graphics and terminal-based ASCII output.

## Features

- **Pseudo-3D Raycasting Engine** — Wolfenstein-style first-person view via SDL2
- **Terminal Mode** — Classic snake with Unicode/ASCII glyphs
- **Configurable** — Board size, tick rate, FOV, textures, keybindings
- **Multiplayer Ready** — Supports multiple players on same board
- **High Score Persistence** — Saves scores to `.snake_scores`

## Architecture

The codebase follows modular design principles and includes:
- **Opaque Pointers**: Encapsulation of internal state.
- **Error-Out Pattern**: Centralized resource management and cleanup.
- **Context Generation**: Scripts that produce structural metadata in `scripts/out/` to assist in project navigation.

## Prerequisites

```bash
# Debian/Ubuntu
sudo apt install build-essential libsdl2-dev pkg-config python3-venv

# Fedora
sudo dnf install gcc make SDL2-devel pkgconf python3
```

## Building

```bash
# Debug build with AddressSanitizer (default)
make

# Release build (optimized)
make release

# Valgrind memory check
make valgrind
```

## Running

```bash
./snakegame
```

## Configuration

Edit `snake_cfg.txt` to customize:

| Setting | Description | Default |
|---------|-------------|---------|
| `board_width`, `board_height` | Play area size | 18x18 |
| `tick_rate_ms` | Game speed (lower = faster) | 250 |
| `num_players` | Number of snakes | 1 |
| `max_length` | Maximum snake length | 64 |
| `max_food` | Food items on board | 20 |
| `fov_degrees` | 3D field of view | 80 |
| `screen_width`, `screen_height` | 3D window size | 1200x800 |
| `enable_external_3d_view` | Launch SDL window | true |
| `wall_texture`, `floor_texture` | Image paths | assets/*.png |
| `wall_height_scale`, `tail_height_scale` | 3D scaling | 1.5, 0.5 |
| `key_up/down/left/right/quit/restart/pause` | Key bindings | w,s,a,d,q,r,p |

## Controls

| Key | Action |
|-----|--------|
| A / D | Turn Left / Right (Relative, for 3D view) |
| Arrows | Cardinal Directions (Up, Down, Left, Right, for top-down) |
| P | Pause/Resume |
| R | Restart Session |
| Q | Quit to Terminal |

## Development

```bash
# Format code (human-readable)
make format

# Format code (LLM token-optimized)
make format-llm

# Regenerate static analysis
make analyze
```

See [ARCHITECTURE.md](docs/ARCHITECTURE.md) for technical details and [CONTRIBUTING.md](CONTRIBUTING.md) for development guidelines.