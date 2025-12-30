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
./snakegame.out
```

## Headless Mode
Run the game without graphics for automated testing or CI pipelines. Game state is printed to stdout each tick.

```bash
# Create a headless config
echo "headless = true" > headless.cfg
./snakegame.out headless.cfg
```

**Autoplay**: In headless mode, snakes automatically turn right every 3rd tick (going in circles) to avoid wall collisions. This is enabled by default for headless mode but can be disabled:

```bash
# Headless with autoplay disabled (snakes will crash into walls)
echo -e "headless = true\nautoplay = false" > headless_noauto.cfg
./snakegame.out headless_noauto.cfg

# Normal mode with autoplay enabled (for visual testing)
echo "autoplay = true" > autoplay.cfg
./snakegame.out autoplay.cfg
```

## Network logging

Enable capture of all network send/recv bytes in a log file. By default the log is written to `./net_io.log` in the current working directory. Set `SNAKE_NET_LOG` to change the path. Payloads are hex-dumped (up to 256 bytes) and truncated if larger.

Examples:

```bash
# write to ./net_io.log (default)
./snakegame

# write to a custom path
SNAKE_NET_LOG=/tmp/snake_net.log ./snakegame
```


## Configuration

Edit `snake.cfg` to customize:

Multiplayer (mpapi):
- `mp_enabled` = true|false
- `mp_server_host` = host (default mpapi.se)
- `mp_server_port` = tcp port (default 9001)
- `mp_identifier` = application identifier (must match server identifier)
- `mp_session` = optional session code to force-join (e.g. ABC123). Leave empty to auto-host/join.

## Local Multiplayer Testing (N Players)

Test multiplayer with multiple clients on localhost using the mpapi server.

### Step 1: Start the Server

```bash
# Start the mpapi server (requires Node.js)
make mpapi-start
# Wait for: "mpapi TCP server listening on port: 9001"
```

### Step 2: Start the Host (Player 1)

Create `mp_host.cfg`:
```
mp_enabled = 1
mp_server_host = 127.0.0.1
mp_server_port = 9001
mp_identifier = 67bdb04f-6e7c-4d76-81a3-191f7d78dd45
player_name = Player1
```

Run: `./snakegame.out mp_host.cfg`

**Note the Session ID** displayed at the bottom of the screen (e.g., `ABC123`).

### Step 3: Start Additional Players

Create `mp_join.cfg` (edit `mp_session` to match the Session ID from Step 2):
```
mp_enabled = 1
mp_server_host = 127.0.0.1
mp_server_port = 9001
mp_identifier = 67bdb04f-6e7c-4d76-81a3-191f7d78dd45
mp_session = ABC123
player_name = Player2
```

Run in separate terminals: `./snakegame.out mp_join.cfg`

Repeat for additional players (Player3, Player4, etc.).

### What You'll See

- Each client displays the Session ID at the bottom
- Remote players appear as **magenta/cyan circles** on your board
- Remote player names shown in side panel with `[MP]` prefix
- Game states are exchanged in real-time

### Server Options

To vendor a local copy of the mpapi server:
```bash
make vendor-mpapi
```

To start the vendored server:
```bash
make mpapi-start
```

Edit `snake.cfg` to customize:

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
| `headless` | Run without graphics (print state to stdout) | false |
| `autoplay` | Snake turns right every 3rd tick (testing). ON by default in headless, OFF in normal mode | auto |

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