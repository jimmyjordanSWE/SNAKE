#include "snake/net.h"
#include "snake/game_internal.h"
#include <arpa/inet.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
size_t
net_pack_input(const InputState* input, unsigned char* buf, size_t buf_size)
{
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
bool net_unpack_input(const unsigned char* buf,
                      size_t               buf_size,
                      InputState*          out)
{
    if (!buf || buf_size < 1 || !out)
        return false;
    unsigned char flags = buf[0];
    *out                = (InputState){0};
    out->quit           = !!(flags & 0x01);
    out->restart        = !!(flags & 0x02);
    out->pause_toggle   = !!(flags & 0x04);
    out->move_up        = !!(flags & 0x10);
    out->move_down      = !!(flags & 0x20);
    out->move_left      = !!(flags & 0x40);
    out->move_right     = !!(flags & 0x80);
    out->any_key        = buf_size > 0 && flags != 0;
    return true;
}
size_t
net_pack_game_state(const GameState* game, unsigned char* buf, size_t buf_size)
{
    if (!game || !buf)
        return 0;
    const size_t header_sz = 4 * 6;
    size_t       needed    = header_sz + (size_t)game->food_count * 8
                    + (size_t)game->num_players * 8;
    if (buf_size < needed)
        return 0;
    unsigned char* p = buf;
    uint32_t       v;
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
    for (int i = 0; i < game->food_count; i++)
    {
        v = htonl((uint32_t)game->food[i].x);
        memcpy(p, &v, 4);
        p += 4;
        v = htonl((uint32_t)game->food[i].y);
        memcpy(p, &v, 4);
        p += 4;
    }
    for (int i = 0; i < game->num_players; i++)
    {
        v = htonl((uint32_t)game->players[i].score);
        memcpy(p, &v, 4);
        p += 4;
        v = htonl((uint32_t)game->players[i].length);
        memcpy(p, &v, 4);
        p += 4;
    }
    return needed;
}
bool net_unpack_game_state(const unsigned char* buf,
                           size_t               buf_size,
                           GameState*           out)
{
    if (!buf || !out || buf_size < 24)
        return false;
    const unsigned char* p = buf;
    uint32_t             v;
    memcpy(&v, p, 4);
    out->width = (int)ntohl(v);
    p += 4;
    memcpy(&v, p, 4);
    out->height = (int)ntohl(v);
    p += 4;
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
    size_t expected =
        24 + (size_t)out->food_count * 8 + (size_t)out->num_players * 8;
    if (buf_size < expected)
        return false;
    if (out->food_count > 0)
    {
        out->max_food = out->food_count;
        out->food =
            (SnakePoint*)malloc(sizeof(SnakePoint) * (size_t)out->food_count);
        if (!out->food)
            return false;
    }
    if (out->num_players > 0)
    {
        out->max_players = out->num_players;
        out->players     = (PlayerState*)malloc(sizeof(PlayerState)
                                            * (size_t)out->num_players);
        if (!out->players)
        {
            free(out->food);
            out->food = NULL;
            return false;
        }
        for (int i = 0; i < out->num_players; i++)
        {
            out->players[i].body        = NULL;
            out->players[i].length      = 0;
            out->players[i].score       = 0;
            out->players[i].active      = false;
            out->players[i].needs_reset = false;
        }
    }
    for (int i = 0; i < out->food_count; i++)
    {
        memcpy(&v, p, 4);
        out->food[i].x = (int)ntohl(v);
        p += 4;
        memcpy(&v, p, 4);
        out->food[i].y = (int)ntohl(v);
        p += 4;
    }
    for (int i = 0; i < out->num_players; i++)
    {
        memcpy(&v, p, 4);
        out->players[i].score = (int)ntohl(v);
        p += 4;
        memcpy(&v, p, 4);
        out->players[i].length = (int)ntohl(v);
        p += 4;
    }
    return true;
}
void net_free_unpacked_game_state(GameState* out)
{
    if (!out)
        return;
    if (out->food)
    {
        free(out->food);
        out->food       = NULL;
        out->food_count = 0;
        out->max_food   = 0;
    }
    if (out->players)
    {
        free(out->players);
        out->players     = NULL;
        out->num_players = 0;
        out->max_players = 0;
    }
}
struct NetClient
{
    int unused;
};
NetClient* net_connect(const char* host, int port)
{
    (void)host;
    (void)port;
    return NULL;
}
void net_disconnect(NetClient* client)
{
    (void)client;
}
bool net_send_input(NetClient* client, const InputState* input)
{
    (void)client;
    (void)input;
    return false;
}
bool net_recv_state(NetClient* client, GameState* out_game)
{
    (void)client;
    (void)out_game;
    return false;
}
