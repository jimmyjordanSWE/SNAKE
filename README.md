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

## Network logging 🔧

Enable capture of all network send/recv bytes in a log file. By default the log is written to `./net_io.log` in the current working directory. Set `SNAKE_NET_LOG` to change the path. Payloads are hex-dumped (up to 256 bytes) and truncated if larger.

Examples:

```bash
# write to ./net_io.log (default)
./snakegame

# write to a custom path
SNAKE_NET_LOG=/tmp/snake_net.log ./snakegame
```


## Configuration

Edit `snake_cfg.txt` to customize:

Multiplayer (mpapi):
- `mp_enabled` = true|false
- `mp_server_host` = host (default mpapi.se)
- `mp_server_port` = tcp port (default 9001)
- `mp_identifier` = application identifier (must match server identifier)
- `mp_session` = optional session code to force-join (e.g. ABC123). Leave empty to auto-host/join.

To run a local test server (requires node):

    scripts/run_mpapi_server.sh [HTTP_PORT] [TCP_PORT]

To vendor a local copy of the mpapi server (so you don't need network access), run:

    make vendor-mpapi

To start the vendored server:

    make mpapi-start

Then set `mp_enabled=true` in `snake_cfg.txt` and run two instances of the game to test client-to-client messages.

Note about VS Code / WSL2 terminals
- On Windows (WSL2) the integrated terminal or task runner may hang or return a non-zero exit code even when `make` prints successful output. If you see `The terminal process "/bin/bash '-c', 'make'" terminated with exit code: 2` but the build output shows `ALL BUILDS OK`, use one of the following:
  - Run `make quick` in the integrated terminal (new target that skips long analysis steps).
  - Run `make` directly in an external terminal (outside VS Code) or a separate WSL terminal.
  - If a task seems stuck, kill and restart the terminal (or use an external terminal) — some VS Code/WSL versions have known issues with long-running tasks.


Edit `snake_cfg.txt` to customize:

| Setting | Description | Default |
|---------|-------------|---------|
| `board_width`, `board_height` | Play area size | 18x18 |
| `tick_rate_ms` | Game speed (lower = faster) | 250 |
| `num_players` | Number of snakes | 2 |
| `max_length` | Maximum snake length | 64 |
| `max_food` | Food items on board | 20 |
| `fov_degrees` | 3D field of view | 80 |
| `screen_width`, `screen_height` | 3D window size | 1200x800 |
| `enable_external_3d_view` | Launch SDL window | true |
| `wall_texture`, `floor_texture` | Image paths | assets/*.png |
| `wall_height_scale`, `tail_height_scale` | 3D scaling | 1.5, 0.5 |
| `p{N}_left/p{N}_right/quit/restart/pause` | Key bindings | p1 uses Arrow keys by default; p1_left..p4_right override, ESC, r, p |

## Controls

| Key | Action |
|-----|--------|
| A / D | Turn Left / Right (Relative, for 3D view) |
| Arrows | Cardinal Directions (Up, Down, Left, Right, for top-down) |
| P | Pause/Resume |
| R | Restart Session |
| Q | Quit to Terminal |

## Development & Testing

```bash
# Format code (human-readable)
make format

# Format code (LLM token-optimized)
make format-llm

# Run static analysis (generates token-minimized outputs in scripts/out/)
make analyze

# Run unit tests (AddressSanitizer enabled)
make unit-tests
# or
make test
```

Notes:
- Unit tests include an OOM harness (`tests/test_game_oom.c`) that simulates allocation failures (using `dlsym(RTLD_NEXT, ...)`) and runs under ASAN to validate cleanup/error paths.

See [ARCHITECTURE.md](docs/ARCHITECTURE.md) for technical details and [CONTRIBUTING.md](CONTRIBUTING.md) for development guidelines.