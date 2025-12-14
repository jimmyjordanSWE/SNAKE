#pragma once
#include "snake/game.h"
#include "snake/input.h"
#include <stdbool.h>
typedef struct NetClient NetClient;
NetClient* net_connect(const char* host, int port);
void net_disconnect(NetClient* client);
bool net_send_state(NetClient* client, const GameState* game);
bool net_recv_input(NetClient* client, InputState* out);
NetClient* net_connect(const char* host, int port);
void net_disconnect(NetClient* client);
bool net_send_input(NetClient* client, const InputState* input);
bool net_recv_state(NetClient* client, GameState* out_game);
