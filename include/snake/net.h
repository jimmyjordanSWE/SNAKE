#pragma once

#include <stdbool.h>

#include "snake/game.h"
#include "snake/input.h"

typedef struct NetClient NetClient;

NetClient* net_connect(const char* host, int port);
void net_disconnect(NetClient* client);

bool net_send_input(NetClient* client, const InputState* input);
bool net_recv_state(NetClient* client, GameState* out_game);
