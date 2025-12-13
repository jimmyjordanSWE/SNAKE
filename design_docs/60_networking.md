# Networking client mode (`src/net/`) — PedroChat relay API

## Goal
Support optional online multiplayer by connecting to the provided PedroChat server (external to this repo). The server is a **relay** (message forwarder), not a game simulation.

Reference implementation lives in:
- [Multiplayer_API/c_client/libs/PedroChatApi.h](../Multiplayer_API/c_client/libs/PedroChatApi.h)
- [Multiplayer_API/backend/PedroChatServer.js](../Multiplayer_API/backend/PedroChatServer.js)

The provided URL is documented in:
- [Multiplayer_API/NetworkAPI_we_will_use.md](../Multiplayer_API/NetworkAPI_we_will_use.md)

## Architecture
- Local mode: core is authoritative.
- Network mode (PedroChat): **host client is authoritative**.
  - Host runs `game_tick()` and broadcasts periodic state snapshots.
  - Non-host clients send input intents to the host.
  - PedroChat relays JSON messages within a session.

This matches the actual server behavior (it forwards `cmd:"game"` payloads; it does not validate or simulate Snake rules).

## Transports
PedroChat supports two transports:

- WebSocket (used by the JS client): connect to `wss://<api-host>/net`.
  - Each message is one JSON object.
- TCP (used by the C example): connect to `<api-host>:9001`.
  - Each message is one JSON object terminated by `\n` (newline-delimited JSON).

This project is C, so the intended implementation is the TCP transport.

## Required field: `identifier`
Every outgoing message MUST include `identifier` (string). The server rejects messages without it.

`identifier` is a 36-character UUID string in the provided client. The server uses it to namespace sessions; `join` is rejected if the session’s identifier differs.

## Wire protocol

### Outgoing commands

#### `cmd: "host"`
Create a new session.

Client → server:
- `{"identifier": "...", "cmd": "host", "data": {"name": "...", "private": true|false}}`

Server → client response:
- `{"cmd":"host","session":"ABC123","clientId":"...","data":{...}}`

#### `cmd: "list"`
List public sessions.

Client → server:
- `{"identifier":"...","cmd":"list","data":{...}}`

Server → client response:
- `{"cmd":"list","data":{"list":[{"id":"ABC123","name":"...","clients":["..."]}]}}`

#### `cmd: "join"`
Join an existing session.

Client → server:
- `{"identifier":"...","session":"ABC123","cmd":"join","data":{...}}`

Server → client response (success):
- `{"cmd":"join","session":"ABC123","name":"...","host":"<clientId>","clients":["<clientId>",...],"clientId":"...","data":{...}}`

Server → client response (error):
- `{"cmd":"join","session":"ABC123","clientId":"...","data":{"status":"error","reason":"identifier_mismatch|already_joined|..."}}`

#### `cmd: "leave"`
Leave a session.

Client → server:
- `{"identifier":"...","session":"ABC123","cmd":"leave","data":{...}}`

Peers receive an event (see below).

#### `cmd: "game"`
Send game-specific payload within a session.

Client → server:
- `{"identifier":"...","session":"ABC123","cmd":"game","data":{...}}`
- Optional targeted send: include `"destination":"<clientId>"`.

The server relays the message to other clients in the session (broadcast by default).

### Incoming events
The JS server implementation sends these `cmd` values to connected peers:

- `cmd: "joined"` — a client joined
- `cmd: "left"` — a client left
- `cmd: "closed"` — host disconnected and the session was closed
- `cmd: "game"` — relayed game payload

`cmd:"game"` includes:
- `messageId`: monotonically increasing per-session integer (assigned by server)
- `clientId`: sender client id
- `broadcast`: boolean (false if `destination` was used)
- `data`: the original payload object

Note: the provided C example callback comments mention `"leaved"`, but the server actually uses `"left"`. When implementing `src/net/`, follow the server behavior.

## Snake message schemas (inside `data`)
PedroChat doesn’t define game schemas; we must define our own.

Minimal approach (host-authoritative):

- Input message (client → host, targeted using `destination=<hostClientId>`):
  - `{"type":"input","tick":123,"dir":"U|D|L|R"}`
- State snapshot (host → all, broadcast):
  - `{"type":"state","tick":123,"players":[...],"food":{"x":1,"y":2},"scores":[...],"status":"running|paused|over"}`

Validation rules:
- Require `type` string.
- Bounds-check all coordinates against configured board size.
- Cap array lengths (`players`, snake segments) to avoid memory abuse.

## Security & robustness
- Treat all network input as untrusted.
- For TCP:
  - implement a buffered reader (handle partial reads)
  - enforce a maximum line length; drop/disconnect on overflow
  - handle partial writes
- On JSON parse failure or invalid schema: ignore message (or disconnect, depending on severity) without crashing.

## Determinism
All clients must use the same collision rules defined in [design_docs/11_collision_rules.md](11_collision_rules.md).
