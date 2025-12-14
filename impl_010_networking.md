# impl_010: Networking

**Header:** `include/snake/net.h`  
**Source:** `src/net/net.c`  
**Status:** ⏳ Stub (Optional Feature)

---

## Overview

Network multiplayer client. See `net.h` for function signatures (stub implementation).

**Not currently implemented.** Optional feature using PedroChat relay (message forwarder, not game simulation).

---

## Architecture

**PedroChat relay:** Message forwarder (no game logic). Host authoritative; non-host clients send inputs, receive state snapshots.

**Protocol:** JSON messages via TCP or WebSocket.

**Data flow:** Local input → InputState → game_tick() → GameState → render_draw() + broadcast.

---

## Status

**Stub implementation:** Returns false (not implemented).

**Reference:** See `Multiplayer_API/c_client/` and `Multiplayer_API/NetworkAPI_we_will_use.md`

**Current:** Local multiplayer only (2 players, same keyboard).

**Optional:** Steps 29-31 in original implementation plan.

---

## Called By (When Implemented)

- `main.c` - Optional network mode vs. local
