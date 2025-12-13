# Networking client mode (`src/net/`)

## Goal
Support online multiplayer by connecting to the provided server (server is external to this repo).

## Architecture
- Local mode: core is authoritative.
- Network mode: server is authoritative; client sends inputs and renders server snapshots.

## Responsibilities
- Connect/disconnect.
- Non-blocking or buffered reads.
- Encode outgoing input messages.
- Decode incoming state messages.

## Security & robustness
- Treat all network input as untrusted.
- Validate:
  - message length
  - value ranges (board size, coordinates)
  - player count limits
- Handle partial reads/writes correctly.
- On malformed message: disconnect cleanly.

## Determinism
- Use the same collision rules defined in [design_docs/11_collision_rules.md](11_collision_rules.md).
