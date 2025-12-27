#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "net.h"
#include "game_internal.h"
#include <unistd.h>
#include <time.h>

static void* server_thread(void* arg) {
    int port = *(int*)arg;
    int l = socket(AF_INET, SOCK_STREAM, 0);
    assert(l >= 0);
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons((uint16_t)port);
    int opt = 1; setsockopt(l, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    assert(bind(l, (struct sockaddr*)&addr, sizeof(addr)) == 0);
    assert(listen(l, 1) == 0);
    int c = accept(l, NULL, NULL);
    assert(c >= 0);
    /* read length-prefixed input (client sends input) */
    uint32_t nsz = 0;
    ssize_t r = recv(c, &nsz, sizeof(nsz), MSG_WAITALL);
    assert(r == sizeof(nsz));
    nsz = ntohl(nsz);
    unsigned char inbuf[256];
    r = recv(c, inbuf, nsz, MSG_WAITALL);
    assert(r == (ssize_t)nsz);
    /* pack a small game state and send */
    GameState gs = {0};
    gs.width = 4; gs.height = 3; gs.status = GAME_STATUS_RUNNING; gs.num_players = 1; gs.food_count = 0;
    gs.max_players = 1;
    gs.players = calloc(1, sizeof(PlayerState));
    gs.players[0].active = true;
    gs.players[0].length = 1;
    gs.players[0].score = 0;
    unsigned char outbuf[256];
    size_t outsz = net_pack_game_state(&gs, outbuf, sizeof(outbuf));
    uint32_t outn = htonl((uint32_t)outsz);
    send(c, &outn, sizeof(outn), 0);
    send(c, outbuf, outsz, 0);
    free(gs.players);
    close(c);
    close(l);
    return NULL;
}

int main(void) {
    int port = 54321;
    pthread_t t;
    assert(pthread_create(&t, NULL, server_thread, &port) == 0);
    /* Wait a bit for server to start */
    /* sleep 1 second to allow server to bind */
    sleep(1);
    NetClient* c = net_connect("127.0.0.1", port);
    assert(c != NULL);
    InputState in = {0}; in.move_right = true;
    assert(net_send_input(c, &in));
    GameState gs = {0};
    assert(net_recv_state(c, &gs));
    net_free_unpacked_game_state(&gs);
    net_disconnect(c);
    pthread_join(t, NULL);
    printf("test_net_integration: OK\n");
    return 0;
}