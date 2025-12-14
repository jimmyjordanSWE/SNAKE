#include "snake/net.h"
#include <stddef.h>
struct NetClient {
int unused;
};
NetClient* net_connect(const char* host, int port) {
(void)host;
(void)port;
return NULL;
}
void net_disconnect(NetClient* client) { (void)client; }
bool net_send_input(NetClient* client, const InputState* input) {
(void)client;
(void)input;
return false;
}
bool net_recv_state(NetClient* client, GameState* out_game) {
(void)client;
(void)out_game;
return false;
}
