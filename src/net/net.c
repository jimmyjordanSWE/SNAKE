#include "net.h"
#include "game_internal.h"
#include <arpa/inet.h>
#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
size_t net_pack_input(const InputState* input, unsigned char* buf, size_t buf_size) {
    if (!input || !buf || buf_size < 1)
        return 0;
    unsigned char flags = 0;
    flags |= (input->quit ? 0x01 : 0);
    flags |= (input->restart ? 0x02 : 0);
    flags |= (input->pause_toggle ? 0x04 : 0);
    flags |= (input->move_up ? 0x10 : 0);
    flags |= (input->move_down ? 0x20 : 0);
    flags |= (input->move_left ? 0x40 : 0);
    flags |= (input->move_right ? 0x80 : 0);
    buf[0] = flags;
    return 1;
}
bool net_unpack_input(const unsigned char* buf, size_t buf_size, InputState* out) {
    if (!buf || buf_size < 1 || !out)
        return false;
    unsigned char flags = buf[0];
    *out = (InputState){0};
    out->quit = !!(flags & 0x01);
    out->restart = !!(flags & 0x02);
    out->pause_toggle = !!(flags & 0x04);
    out->move_up = !!(flags & 0x10);
    out->move_down = !!(flags & 0x20);
    out->move_left = !!(flags & 0x40);
    out->move_right = !!(flags & 0x80);
    out->any_key = buf_size > 0 && flags != 0;
    return true;
}
size_t net_pack_game_state(const GameState* game, unsigned char* buf, size_t buf_size) {
    if (!game || !buf)
        return 0;
    const size_t header_sz = 4 * 6;
    size_t needed = header_sz + (size_t)game->food_count * 8 + (size_t)game->num_players * 8;
    if (buf_size < needed)
        return 0;
    unsigned char* p = buf;
    uint32_t v;
    v = htonl((uint32_t)game->width);
    memcpy(p, &v, 4);
    p += 4;
    v = htonl((uint32_t)game->height);
    memcpy(p, &v, 4);
    p += 4;
    v = htonl(game->rng_state);
    memcpy(p, &v, 4);
    p += 4;
    v = htonl((uint32_t)game->status);
    memcpy(p, &v, 4);
    p += 4;
    v = htonl((uint32_t)game->num_players);
    memcpy(p, &v, 4);
    p += 4;
    v = htonl((uint32_t)game->food_count);
    memcpy(p, &v, 4);
    p += 4;
    for (int i = 0; i < game->food_count; i++) {
        v = htonl((uint32_t)game->food[i].x);
        memcpy(p, &v, 4);
        p += 4;
        v = htonl((uint32_t)game->food[i].y);
        memcpy(p, &v, 4);
        p += 4;
    }
    for (int i = 0; i < game->num_players; i++) {
        v = htonl((uint32_t)game->players[i].score);
        memcpy(p, &v, 4);
        p += 4;
        v = htonl((uint32_t)game->players[i].length);
        memcpy(p, &v, 4);
        p += 4;
    }
    return needed;
}
bool net_unpack_game_state(const unsigned char* buf, size_t buf_size, GameState* out) {
    if (!buf || !out || buf_size < 24)
        return false;
    out->food = NULL;
    out->players = NULL;
    out->max_food = 0;
    out->max_players = 0;
    bool ok = false;
    const unsigned char* p = buf;
    uint32_t v;
    memcpy(&v, p, 4);
    out->width = (int)ntohl(v);
    p += 4;
    memcpy(&v, p, 4);
    out->height = (int)ntohl(v);
    p += 4;
    /* Reject non-positive dimensions early */
    if (out->width <= 0 || out->height <= 0)
        goto out;
    memcpy(&v, p, 4);
    out->rng_state = ntohl(v);
    p += 4;
    memcpy(&v, p, 4);
    out->status = (GameStatus)ntohl(v);
    p += 4;
    memcpy(&v, p, 4);
    out->num_players = (int)ntohl(v);
    p += 4;
    memcpy(&v, p, 4);
    out->food_count = (int)ntohl(v);
    p += 4;
    size_t expected = 24 + (size_t)out->food_count * 8 + (size_t)out->num_players * 8;
    if (buf_size < expected)
        goto out;
    if (out->food_count > 0) {
        if ((size_t)out->food_count > SIZE_MAX / sizeof(SnakePoint))
            goto out;
        out->food = malloc((size_t)out->food_count * sizeof *out->food);
        if (!out->food)
            goto out;
        out->max_food = out->food_count;
    }
    if (out->num_players > 0) {
        if ((size_t)out->num_players > SIZE_MAX / sizeof(PlayerState))
            goto out;
        out->players = malloc((size_t)out->num_players * sizeof *out->players);
        if (!out->players)
            goto out;
        out->max_players = out->num_players;
        for (int i = 0; i < out->num_players; i++) {
            out->players[i].body = NULL;
            out->players[i].length = 0;
            out->players[i].score = 0;
            out->players[i].active = false;
            out->players[i].needs_reset = false;
        }
    }
    for (int i = 0; i < out->food_count; i++) {
        memcpy(&v, p, 4);
        out->food[i].x = (int)ntohl(v);
        p += 4;
        memcpy(&v, p, 4);
        out->food[i].y = (int)ntohl(v);
        p += 4;
    }
    for (int i = 0; i < out->num_players; i++) {
        memcpy(&v, p, 4);
        out->players[i].score = (int)ntohl(v);
        p += 4;
        memcpy(&v, p, 4);
        out->players[i].length = (int)ntohl(v);
        p += 4;
    }
    ok = true;
out:
    if (!ok) {
        free(out->food);
        out->food = NULL;
        out->food_count = 0;
        out->max_food = 0;
        free(out->players);
        out->players = NULL;
        out->num_players = 0;
        out->max_players = 0;
    }
    return ok;
}
void net_free_unpacked_game_state(GameState* out) {
    if (!out)
        return;
    if (out->food) {
        free(out->food);
        out->food = NULL;
        out->food_count = 0;
        out->max_food = 0;
    }
    if (out->players) {
        free(out->players);
        out->players = NULL;
        out->num_players = 0;
        out->max_players = 0;
    }
}
struct NetClient {
    int fd;
};
NetClient* net_connect(const char* host, int port) {
    if (!host || port <= 0)
        return NULL;
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
        return NULL;
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);
    if (inet_pton(AF_INET, host, &addr.sin_addr) != 1) {
        close(fd);
        return NULL;
    }
    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        close(fd);
        return NULL;
    }
    NetClient* c = malloc(sizeof *c);
    if (!c) {
        close(fd);
        return NULL;
    }
    c->fd = fd;
    return c;
}
void net_disconnect(NetClient* client) {
    if (!client)
        return;
    close(client->fd);
    free(client);
}
#include "net_log.h"
bool net_send_input(NetClient* client, const InputState* input) {
    if (!client || !input)
        return false;
    unsigned char buf[256];
    size_t sz = net_pack_input(input, buf, sizeof(buf));
    if (sz == 0)
        return false;
    uint32_t nsz = htonl((uint32_t)sz);
    ssize_t r = send(client->fd, &nsz, sizeof(nsz), 0);
    if (r != sizeof(nsz))
        return false;
    /* log header */
    net_log_send(client->fd, &nsz, sizeof(nsz), "net_send_input: header");
    r = send(client->fd, buf, sz, 0);
    if (r != (ssize_t)sz)
        return false;
    /* log payload */
    net_log_send(client->fd, buf, sz, "net_send_input: payload");
    return true;
}
bool net_recv_state(NetClient* client, GameState* out_game) {
    if (!client || !out_game)
        return false;
    uint32_t nsz = 0;
    ssize_t r = recv(client->fd, &nsz, sizeof(nsz), MSG_WAITALL);
    if (r != sizeof(nsz))
        return false;
    /* log header */
    net_log_recv(client->fd, &nsz, sizeof(nsz), "net_recv_state: header");
    nsz = ntohl(nsz);
    if (nsz == 0 || nsz > 1000000)
        return false;
    unsigned char* buf = malloc(nsz);
    if (!buf)
        return false;
    r = recv(client->fd, buf, nsz, MSG_WAITALL);
    if (r != (ssize_t)nsz) {
        free(buf);
        return false;
    }
    /* log payload */
    net_log_recv(client->fd, buf, nsz, "net_recv_state: payload");
    bool ok = net_unpack_game_state(buf, (size_t)nsz, out_game);
    free(buf);
    return ok;
}
