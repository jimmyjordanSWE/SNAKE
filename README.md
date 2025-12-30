# SNAKE 🐍

A C99 snake game implementation featuring dual rendering modes: pseudo-3D raycasting via SDL2 and a terminal-based ASCII interface.

## Features

- **Pseudo-3D Raycasting Engine** — Wolfenstein-style first-person view using SDL2.
- **Terminal Mode** — Classic snake gameplay with Unicode/ASCII glyphs.
- **Highly Configurable** — Customize board size, tick rate, FOV, textures, and keybindings via `snake.cfg`.
- **Multiplayer Ready** — Supports multiple players on the same board (local or networked).
- **High Score Persistence** — Automatically saves scores to `.snake_scores`.

## Architecture

The codebase adheres to strict C99 systems engineering standards:
- **Opaque Pointers**: Strict encapsulation of internal state.
- **Error-Out Pattern**: Centralized resource management and cleanup.
- **Context Generation**: Automated scripts in `scripts/out/` produce structural metadata to assist development.

## Controls

| Key | Action |
|-----|--------|
| A / D | Turn Left / Right (Relative, for 3D view) |
| Arrows | Cardinal Directions (Up, Down, Left, Right, for top-down) |
| P | Pause/Resume |
| R | Restart Session |
| Q | Quit to Terminal |

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
./snakegame.out
```

## Headless Mode

Run the game without graphics for automated testing or CI pipelines. Game state is serialized to stdout each tick.

```bash
# Create a headless config
echo "headless = true" > headless.cfg
./snakegame.out headless.cfg
```

**Autoplay**: In headless mode, snakes automatically turn right every 3rd tick (circular motion) to prevent immediate wall collisions. This is enabled by default in headless mode but can be explicitly toggled:

```bash
# Headless with autoplay disabled (snakes will crash)
echo -e "headless = true\nautoplay = false" > headless_noauto.cfg
./snakegame.out headless_noauto.cfg

# Normal mode with autoplay enabled (for visual debugging)
echo "autoplay = true" > autoplay.cfg
./snakegame.out autoplay.cfg
```

## Network Logging

Capture all network send/recv traffic in a log file. By default, logs are written to `logs/net_io.log`. Set `SNAKE_NET_LOG` to override the path. Payloads are hex-dumped (up to 256 bytes) and truncated if larger.

```bash
# Write to logs/net_io.log (default)
./snakegame.out

# Write to a custom path
SNAKE_NET_LOG=/tmp/snake_net.log ./snakegame.out
```


## Configuration

Edit `snake.cfg` to customize the game.

## Local Multiplayer Testing (N Players)

Test multiplayer with multiple clients on localhost using the included `mpapi` compatibility server.

### 1. Start the Server

```bash
# Requires Node.js
make mpapi-start
# Wait for: "mpapi TCP server listening on port: 9001"
```

### 2. Start the Host (Player 1)

Create `mp_host.cfg`:
```ini
mp_enabled = 1
mp_server_host = 127.0.0.1
mp_server_port = 9001
player_name = Player1
```

Run: `./snakegame.out mp_host.cfg`

**Note the Session ID** (e.g., `ABC123`) displayed at the bottom depending on the view.

### 3. Start Client(s)

Create `mp_join.cfg` (replace `mp_session` with the ID from the host):
```ini
mp_enabled = 1
mp_server_host = 127.0.0.1
mp_server_port = 9001
mp_session = ABC123
player_name = Player2
```

Run: `./snakegame.out mp_join.cfg`

Remote players appear as distinctively colored circles in the 3D view and glyphs in the terminal view.



### Server Options

To vendor a local copy of the mpapi server:
```bash
make vendor-mpapi
```

To start the vendored server:
```bash
make mpapi-start
```

## Development & Testing

```bash
# Format code (human-readable)
make format

# Format code (LLM token-optimized)
make format-llm

# Run static analysis (generates context in scripts/out/)
make analyze

# Run unit tests (with ASAN)
make unit-tests
```

### Notes
- **OOM Harness**: `tests/test_game_oom.c` simulates memory allocation failures (using `dlsym(RTLD_NEXT)`) to validate error handling paths.
- **Static Analysis**: The project uses a suite of Python scripts (`scripts/`) to generate high-level architectural context for LLM-assisted development.

See [ARCHITECTURE.md](docs/ARCHITECTURE.md) for technical design details and [CONTRIBUTING.md](CONTRIBUTING.md) for contribution guidelines.