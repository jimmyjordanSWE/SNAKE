#pragma once
#include "snake/game.h"
#include "snake/input.h"
#include <stdbool.h>
typedef struct NetClient NetClient;
NetClient* net_connect(const char* host, int port);
void net_disconnect(NetClient* client);
bool net_send_input(NetClient* client, const InputState* input);
bool net_recv_state(NetClient* client, GameState* out_game);
/* NOTE: `net_unpack_game_state` will allocate `out->food` and `out->players`
 * when it returns successfully. Callers are responsible for freeing those
 * allocations (e.g. `free(out->food); free(out->players);`) when they are
 * no longer needed. */
void net_free_unpacked_game_state(GameState* out);
/* Serialization helpers for testing and transport (simple, deterministic binary
 * format). Return number of bytes written or 0 on error. */
size_t net_pack_input(const InputState* input, unsigned char* buf, size_t buf_size);
bool net_unpack_input(const unsigned char* buf, size_t buf_size, InputState* out);
size_t net_pack_game_state(const GameState* game, unsigned char* buf, size_t buf_size);
bool net_unpack_game_state(const unsigned char* buf, size_t buf_size, GameState* out);
