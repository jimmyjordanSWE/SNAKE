#define _POSIX_C_SOURCE 200809L
#include "unity.h"
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include "net.h"
#include "game_internal.h"

static void* server_thread_fn(void* arg) {
    int port = *(int*)arg;
    int l = socket(AF_INET, SOCK_STREAM, 0);
    TEST_ASSERT_TRUE(l >= 0);
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons((uint16_t)port);
    int opt = 1; setsockopt(l, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    TEST_ASSERT_TRUE(bind(l, (struct sockaddr*)&addr, sizeof(addr)) == 0);
    TEST_ASSERT_TRUE(listen(l, 1) == 0);
    int c = accept(l, NULL, NULL);
    TEST_ASSERT_TRUE(c >= 0);
    uint32_t nsz = 0;
    ssize_t r = recv(c, &nsz, sizeof(nsz), MSG_WAITALL);
    TEST_ASSERT_EQUAL_INT((ssize_t)sizeof(nsz), r);
    nsz = ntohl(nsz);
    unsigned char inbuf[256];
    r = recv(c, inbuf, nsz, MSG_WAITALL);
    TEST_ASSERT_EQUAL_INT((ssize_t)nsz, r);
    GameState gs = {0};
    gs.width = 4; gs.height = 3; gs.status = GAME_STATUS_RUNNING; gs.num_players = 1; gs.food_count = 0;
    gs.max_players = 1;
    gs.players = calloc(1, sizeof(PlayerState));
    TEST_ASSERT_TRUE(gs.players != NULL);
    gs.players[0].active = true; gs.players[0].length = 1; gs.players[0].score = 0;
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

TEST(test_net_integration) {
    int port = 54321;
    pthread_t t;
    TEST_ASSERT_EQUAL_INT(0, pthread_create(&t, NULL, server_thread_fn, &port));
    sleep(1);
    NetClient* c = net_connect("127.0.0.1", port);
    TEST_ASSERT_TRUE(c != NULL);
    InputState in = {0}; in.move_right = true;
    TEST_ASSERT_TRUE(net_send_input(c, &in));
    GameState gs = {0};
    TEST_ASSERT_TRUE(net_recv_state(c, &gs));
    net_free_unpacked_game_state(&gs);
    net_disconnect(c);
    pthread_join(t, NULL);
}


