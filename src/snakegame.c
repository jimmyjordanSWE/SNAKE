#include "snakegame.h"
#include "console.h"
#include "game.h"
#include "game_internal.h"
#include "input.h"
#include "mpapi_client.h"
#include "net_log.h"
#include "persist.h"
#include "platform.h"
#include "render.h"
#include "render_3d.h"
#include "tty.h"
#include "types.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Remote player data received from other clients */
#define MAX_REMOTE_PLAYERS 8
typedef struct {
    int x, y;
    int length;
    int score;
    char name[32];
    bool active;
} RemotePlayer;
static RemotePlayer g_remote_players[MAX_REMOTE_PLAYERS];
static int g_remote_player_count = 0;

/* Simple JSON field extraction (reusable) */
static int parse_json_int(const char* json, const char* key) {
    char needle[64];
    snprintf(needle, sizeof(needle), "\"%s\"", key);
    const char* p = strstr(json, needle);
    if (!p)
        return 0;
    p = strchr(p, ':');
    if (!p)
        return 0;
    p++;
    while (*p && isspace((unsigned char)*p))
        p++;
    return atoi(p);
}

static void parse_json_str(const char* json, const char* key, char* out, int maxlen) {
    out[0] = '\0';
    char needle[64];
    snprintf(needle, sizeof(needle), "\"%s\"", key);
    const char* p = strstr(json, needle);
    if (!p)
        return;
    p = strchr(p, ':');
    if (!p)
        return;
    p++;
    while (*p && isspace((unsigned char)*p))
        p++;
    if (*p != '"')
        return;
    p++;
    int i = 0;
    while (*p && *p != '"' && i < maxlen - 1) {
        if (*p == '\\' && p[1]) {
            p++;
        }
        out[i++] = *p++;
    }
    out[i] = '\0';
}

/* Parse incoming game state JSON from another client */
static void parse_remote_game_state(const char* json) {
    if (!json || !strstr(json, "\"type\":\"state\""))
        return;

    /* Find players array */
    const char* players = strstr(json, "\"players\":[");
    if (!players)
        return;
    players = strchr(players, '[');
    if (!players)
        return;
    players++;

    g_remote_player_count = 0;

    /* Parse each player object */
    while (*players && g_remote_player_count < MAX_REMOTE_PLAYERS) {
        const char* obj_start = strchr(players, '{');
        if (!obj_start)
            break;
        const char* obj_end = strchr(obj_start, '}');
        if (!obj_end)
            break;

        /* Extract a single player object */
        int len = (int)(obj_end - obj_start + 1);
        if (len > 0 && len < 512) {
            char obj[512];
            memcpy(obj, obj_start, (size_t)len);
            obj[len] = '\0';

            RemotePlayer* rp = &g_remote_players[g_remote_player_count];
            rp->x = parse_json_int(obj, "x");
            rp->y = parse_json_int(obj, "y");
            rp->length = parse_json_int(obj, "len");
            rp->score = parse_json_int(obj, "score");
            parse_json_str(obj, "name", rp->name, (int)sizeof(rp->name));
            rp->active = true;
            g_remote_player_count++;
        }

        players = obj_end + 1;
    }
}

/* JSON helpers for lightweight serialization of GameState */

static char* escape_json_str(const char* s) {
    if (!s)
        return strdup("");
    size_t need = 1;
    for (const unsigned char* p = (const unsigned char*)s; *p; ++p) {
        if (*p == '"' || *p == '\\' || *p == '\n' || *p == '\r' || *p == '\t')
            need += 2; /* escape */
        else if (*p < 0x20)
            need += 6; /* \u00xx */
        else
            need += 1;
    }
    char* out = malloc(need);
    if (!out)
        return NULL;
    char* q = out;
    for (const unsigned char* p = (const unsigned char*)s; *p; ++p) {
        unsigned char c = *p;
        if (c == '"') {
            *q++ = '\\';
            *q++ = '"';
        } else if (c == '\\') {
            *q++ = '\\';
            *q++ = '\\';
        } else if (c == '\n') {
            *q++ = '\\';
            *q++ = 'n';
        } else if (c == '\r') {
            *q++ = '\\';
            *q++ = 'r';
        } else if (c == '\t') {
            *q++ = '\\';
            *q++ = 't';
        } else if (c < 0x20) {
            int n = snprintf(q, 7, "\\u%04x", c);
            q += n;
        } else {
            *q++ = (char)c;
        }
    }
    *q = '\0';
    return out;
}

static char* game_state_to_json(const GameState* gs, int tick) {
    if (!gs)
        return strdup("{}");
    /* build JSON in a dynamic buffer */
    size_t cap = 1024;
    char* buf = malloc(cap);
    if (!buf)
        return NULL;
    size_t len = 0;
    int n = snprintf(buf + len, cap - len, "{\"type\":\"state\",\"tick\":%d,\"w\":%d,\"h\":%d,\"players\":[", tick,
                     gs->width, gs->height);
    if (n < 0) {
        free(buf);
        return NULL;
    }
    len += (size_t)n;
    for (int i = 0; i < gs->num_players; ++i) {
        const PlayerState* p = &gs->players[i];
        if (!p->active)
            continue;
        /* head */
        int hx = p->body && p->length > 0 ? p->body[0].x : 0;
        int hy = p->body && p->length > 0 ? p->body[0].y : 0;
        char* escaped = NULL;
        if (p->name[0])
            escaped = escape_json_str(p->name);
        if (!escaped)
            escaped = strdup("");
        n = snprintf(buf + len, cap - len, "%s{\"id\":%d,\"name\":\"%s\",\"x\":%d,\"y\":%d,\"len\":%d,\"score\":%d}",
                     (len > 0 && buf[len - 1] != '[') ? "," : "", i, escaped, hx, hy, p->length, p->score);
        free(escaped);
        if (n < 0) {
            free(buf);
            return NULL;
        }
        len += (size_t)n;
        if (len + 256 > cap) {
            cap *= 2;
            char* nb = realloc(buf, cap);
            if (!nb) {
                free(buf);
                return NULL;
            }
            buf = nb;
        }
    }
    n = snprintf(buf + len, cap - len, "]\",\"food\":[");
    if (n < 0) {
        free(buf);
        return NULL;
    }
    len += (size_t)n;
    for (int i = 0; i < gs->food_count; ++i) {
        n = snprintf(buf + len, cap - len, "%s{\"x\":%d,\"y\":%d}", (i > 0) ? "," : "", gs->food[i].x, gs->food[i].y);
        if (n < 0) {
            free(buf);
            return NULL;
        }
        len += (size_t)n;
        if (len + 128 > cap) {
            cap *= 2;
            char* nb = realloc(buf, cap);
            if (!nb) {
                free(buf);
                return NULL;
            }
            buf = nb;
        }
    }
    n = snprintf(buf + len, cap - len, "]}\n");
    if (n < 0) {
        free(buf);
        return NULL;
    }
    len += (size_t)n;
    /* shrink to fit */
    char* out = realloc(buf, len + 1);
    return out ? out : buf;
}

struct SnakeGame {
    GameConfig* cfg;
    Game* game;
    bool has_3d;
    bool headless;
    bool autoplay;
};

/* Headless mode: print minimal game state to stdout */
static void headless_print_state(const GameState* gs, int tick) {
    if (!gs)
        return;
    printf("TICK=%05d", tick);
    for (int i = 0; i < gs->num_players && i < SNAKE_MAX_PLAYERS; ++i) {
        const PlayerState* p = &gs->players[i];
        int hx = (p->body && p->length > 0) ? p->body[0].x : 0;
        int hy = (p->body && p->length > 0) ? p->body[0].y : 0;
        printf(" | P%d:(%02d,%02d) s=%d l=%d%s", i, hx, hy, p->score, p->length, p->active ? "" : " DEAD");
    }
    printf("\n");
    (void)fflush(stdout);
}
SnakeGame* snake_game_new(const GameConfig* config_in, int* err_out) {
    int err = 0;
    if (err_out)
        *err_out = 1;
    if (!config_in)
        return NULL;
    SnakeGame* s = malloc(sizeof *s);
    if (!s)
        return NULL;
    s->cfg = game_config_create();
    if (!s->cfg) {
        free(s);
        return NULL;
    }
    s->headless = (game_config_get_headless(config_in) != 0);
    int bw = 0, bh = 0;
    game_config_get_board_size(config_in, &bw, &bh);
    game_config_set_board_size(s->cfg, bw, bh);
    int tick = game_config_get_tick_rate_ms(config_in);
    if (tick < 10)
        tick = 10;
    game_config_set_tick_rate_ms(s->cfg, tick);
    game_config_set_player_name(s->cfg, game_config_get_player_name(config_in));
    game_config_set_seed(s->cfg, game_config_get_seed(config_in));
    game_config_set_num_players(s->cfg, game_config_get_num_players(config_in));
    /* Copy multiplayer settings */
    game_config_set_mp_enabled(s->cfg, game_config_get_mp_enabled(config_in));
    game_config_set_mp_server(s->cfg, game_config_get_mp_server_host(config_in),
                              game_config_get_mp_server_port(config_in));
    game_config_set_mp_identifier(s->cfg, game_config_get_mp_identifier(config_in));
    game_config_set_mp_session(s->cfg, game_config_get_mp_session(config_in));
    int sw = 0, sh = 0;
    game_config_get_screen_size(config_in, &sw, &sh);
    game_config_set_screen_size(s->cfg, sw, sh);
    const int board_width = bw;
    const int board_height = bh;
    bool has_3d = false;

    /* Headless mode: skip all graphics and input initialization */
    if (s->headless) {
        fprintf(stderr, "HEADLESS MODE: game running without graphics\n");
        Game* game = game_create(config_in, game_config_get_seed(config_in));
        if (!game) {
            fprintf(stderr, "Failed to create game\n");
            err = 3;
            goto out_err;
        }
        s->game = game;
        s->has_3d = false;
        /* Autoplay: default to ON in headless mode. Config can override:
         * -1 = unset -> default ON in headless
         *  0 = explicitly disabled -> OFF
         *  1 = explicitly enabled -> ON */
        int ap_cfg = game_config_get_autoplay(config_in);
        s->autoplay = (ap_cfg < 0) ? true : (ap_cfg > 0);
        if (err_out)
            *err_out = 0;
        return s;
    }

    /* Normal mode: full graphics initialization */
    render_set_glyphs((game_config_get_render_glyphs(config_in) == 1) ? RENDER_GLYPHS_ASCII : RENDER_GLYPHS_UTF8);
    input_set_bindings_from_config(config_in);
    platform_winch_init();
    int term_w = 0, term_h = 0;
    while (1) {
        if (!platform_get_terminal_size(&term_w, &term_h)) {
            term_w = 120;
            term_h = 30;
        }
        if (tty_size_sufficient_for_board(term_w, term_h, board_width, board_height))
            break;
        console_box_too_small_for_game(term_w, term_h, board_width + 4, board_height + 4);
        platform_sleep_ms(500);
        (void)platform_was_resized();
    }
    console_info("Terminal size OK (%dx%d). Starting game...\n", term_w, term_h);
    int min_render_w = board_width + 10;
    int min_render_h = board_height + 5;
    if (!game_config_get_enable_external_3d_view(config_in)) {
        int sw2 = 0, sh2 = 0;
        game_config_get_screen_size(config_in, &sw2, &sh2);
        if (sw2 > min_render_w)
            min_render_w = sw2;
        if (sh2 > min_render_h)
            min_render_h = sh2;
    }
    if (!render_init(min_render_w, min_render_h)) {
        console_error("Failed to initialize rendering\n");
        err = 2;
        goto out_err;
    }
    Game* game = game_create(config_in, game_config_get_seed(config_in));
    if (!game) {
        console_error("Failed to create game\n");
        err = 3;
        goto cleanup_render;
    }
    Render3DConfig config_3d = {0};
    config_3d.active_player = game_config_get_active_player(config_in);
    config_3d.fov_degrees = game_config_get_fov_degrees(config_in);
    config_3d.show_sprite_debug = (game_config_get_show_sprite_debug(config_in) != 0);
    config_3d.wall_height_scale = game_config_get_wall_height_scale(config_in);
    config_3d.tail_height_scale = game_config_get_tail_height_scale(config_in);
    config_3d.wall_texture_scale = game_config_get_wall_texture_scale(config_in);
    config_3d.floor_texture_scale = game_config_get_floor_texture_scale(config_in);
    int sw3 = 0, sh3 = 0;
    game_config_get_screen_size(config_in, &sw3, &sh3);
    config_3d.screen_width = sw3;
    config_3d.screen_height = sh3;
    if (game_config_get_wall_texture(config_in) && game_config_get_wall_texture(config_in)[0])
        snprintf(config_3d.wall_texture_path, (int)sizeof(config_3d.wall_texture_path), "%s",
                 game_config_get_wall_texture(config_in));
    if (game_config_get_floor_texture(config_in) && game_config_get_floor_texture(config_in)[0])
        snprintf(config_3d.floor_texture_path, (int)sizeof(config_3d.floor_texture_path), "%s",
                 game_config_get_floor_texture(config_in));
    if (game_config_get_enable_external_3d_view(config_in)) {
        has_3d = render_3d_init(game_get_state(game), &config_3d);
        if (!has_3d)
            console_warn("Warning: external 3D view initialization failed, "
                         "continuing with 2D only\n");
        else {
            render_3d_set_tick_rate_ms(game_config_get_tick_rate_ms(config_in));
        }
    }
    if (!input_init()) {
        console_error("Failed to initialize input\n");
        err = 4;
        goto cleanup_game;
    }
    char player_name_buf[PERSIST_PLAYER_NAME_MAX] = {0};
    const char* cfg_name = game_config_get_player_name(config_in);
    if (cfg_name && cfg_name[0])
        snprintf(player_name_buf, sizeof(player_name_buf), "%s", cfg_name);
    render_draw_startup_screen(player_name_buf, (int)sizeof(player_name_buf));
    if (player_name_buf[0])
        game_config_set_player_name(s->cfg, player_name_buf);
    render_draw(game_get_state(game), game_config_get_player_name(s->cfg), NULL, 0);
    s->game = game;
    s->has_3d = has_3d;
    /* In normal mode, autoplay is OFF unless explicitly enabled in config (value > 0) */
    s->autoplay = (game_config_get_autoplay(config_in) > 0);
    if (err_out)
        *err_out = 0;
    return s;
cleanup_game:
    if (game)
        game_destroy(game);
cleanup_render:
    render_shutdown();
out_err:
    if (err_out)
        *err_out = err ? err : 1;
    if (s) {
        if (s->cfg)
            game_config_destroy(s->cfg);
        free(s);
    }
    return NULL;
}
int snake_game_run(SnakeGame* s) {
    if (!s)
        return 1;
    int err = 0;
    Game* game = s->game;
    GameConfig* cfg = s->cfg;
    int bw = 0, bh = 0;
    game_config_get_board_size(cfg, &bw, &bh);

    /* Multiplayer client (optional) */
    mpclient* mpc = NULL;
    if (game_config_get_mp_enabled(cfg)) {
        const char* host = game_config_get_mp_server_host(cfg);
        int port = game_config_get_mp_server_port(cfg);
        const char* ident = game_config_get_mp_identifier(cfg);
        if (!ident)
            ident = "67bdb04f-6e7c-4d76-81a3-191f7d78dd45";
        mpc = mpclient_create(host ? host : "127.0.0.1", (uint16_t)(port ? port : 9001), ident);
        if (mpc) {
            if (mpclient_connect_and_start(mpc) == 0) {
                const char* forced = game_config_get_mp_session(cfg);
                if (forced && forced[0]) {
                    (void)mpclient_join(mpc, forced, game_config_get_player_name(cfg));
                    if (!s->headless)
                        render_set_session_id(forced);
                } else {
                    mpclient_auto_join_or_host(mpc, game_config_get_player_name(cfg));
                }
            }
        }
    }

    /* HEADLESS MODE: simplified game loop with no graphics/input */
    if (s->headless) {
        int tick = 0;
        fprintf(stderr, "HEADLESS: game loop starting (tick_rate=%dms, autoplay=%s)\n",
                game_config_get_tick_rate_ms(cfg), s->autoplay ? "ON" : "OFF");
        while (game_get_status(game) != GAME_STATUS_GAME_OVER) {
            /* Autoplay: turn right every 3rd tick to circle and avoid walls */
            if (s->autoplay && (tick % 3) == 0) {
                int np = game_get_num_players(game);
                for (int i = 0; i < np; ++i) {
                    InputState auto_in = {0};
                    auto_in.turn_right = 1;
                    (void)game_enqueue_input(game, i, &auto_in);
                }
            }
            GameEvents events = {0};
            game_step(game, &events);
            headless_print_state(game_get_state(game), tick);

            /* Handle multiplayer message exchange */
            if (mpc) {
                char mpbuf[1024];
                while (mpclient_poll_message(mpc, mpbuf, (int)sizeof(mpbuf))) {
                    if (strstr(mpbuf, "\"type\":\"state\"")) {
                        parse_remote_game_state(mpbuf);
                    }
                }
                /* Send current game state */
                if (mpclient_has_session(mpc)) {
                    char* state_json = game_state_to_json(game_get_state(game), tick);
                    if (state_json) {
                        (void)mpclient_send_game(mpc, state_json);
                        free(state_json);
                    }
                }
            }

            /* Auto-restart if all players dead */
            const GameState* gs = game_get_state(game);
            int active = 0;
            for (int i = 0; i < gs->num_players; ++i)
                if (gs->players[i].active)
                    active++;
            if (active == 0) {
                fprintf(stderr, "HEADLESS: all players dead, restarting\n");
                game_reset(game);
            }

            platform_sleep_ms((uint32_t)game_config_get_tick_rate_ms(cfg));
            tick++;
        }
        if (mpc) {
            mpclient_stop(mpc);
            mpclient_destroy(mpc);
        }
        if (game) {
            game_destroy(game);
            s->game = NULL;
        }
        return 0;
    }

    /* NORMAL MODE: full graphics game loop */
    const int board_width = bw;
    const int board_height = bh;
    bool has_3d = s->has_3d;
    int tick = 0;
    HighScore** highscores = NULL;
    int highscore_count = persist_read_scores(".snake_scores", &highscores);
    render_draw(game_get_state(game), game_config_get_player_name(cfg), highscores, highscore_count);

    /* Track session id and reflect in HUD when it appears/changes */
    char prev_session[16] = {0};

    while (game_get_status(game) != GAME_STATUS_GAME_OVER) {
        if (platform_was_resized()) {
            int new_w = 0, new_h = 0;
            if (!platform_get_terminal_size(&new_w, &new_h)) {
                new_w = 120;
                new_h = 30;
            }
            if (!tty_size_sufficient_for_board(new_w, new_h, board_width, board_height)) {
                render_draw(game_get_state(game), game_config_get_player_name(cfg), highscores, highscore_count);
                console_box_paused_terminal_small(new_w, new_h, board_width + 4, board_height + 4);
                while (1) {
                    InputState in = {0};
                    input_poll(&in);
                    if (in.quit)
                        goto clean_done;
                    int check_w = 0, check_h = 0;
                    if (!platform_get_terminal_size(&check_w, &check_h)) {
                        check_w = 120;
                        check_h = 30;
                    }
                    if (tty_size_sufficient_for_board(check_w, check_h, board_width, board_height)) {
                        console_terminal_resized(check_w, check_h);
                        break;
                    }
                    platform_sleep_ms(250);
                }
            }
        }
        int num_players = game_get_num_players(game);
        InputState inputs[SNAKE_MAX_PLAYERS];
        for (int i = 0; i < num_players && i < SNAKE_MAX_PLAYERS; ++i)
            inputs[i] = (InputState){0};
        input_poll_all(inputs, num_players);
        if (num_players > 0 && inputs[0].quit)
            goto clean_done;
        for (int i = 0; i < num_players && i < SNAKE_MAX_PLAYERS; ++i)
            (void)game_enqueue_input(game, i, &inputs[i]);
        /* Autoplay: turn right every 3rd tick to circle and avoid walls */
        if (s->autoplay && (tick % 3) == 0) {
            for (int i = 0; i < num_players && i < SNAKE_MAX_PLAYERS; ++i) {
                InputState auto_in = {0};
                auto_in.turn_right = 1;
                (void)game_enqueue_input(game, i, &auto_in);
            }
        }
        GameEvents events = {0};
        game_step(game, &events);
        if (has_3d)
            render_3d_on_tick(game_get_state(game));
        if (highscores) {
            persist_free_scores(highscores, highscore_count);
            highscores = NULL;
        }
        bool auto_restart_after_highscore = false;
        highscore_count = persist_read_scores(".snake_scores", &highscores);
        for (int ei = 0; ei < events.died_count; ei++) {
            int death_score = events.died_scores[ei];
            int death_player = events.died_players[ei];
            if (death_score > 0) {
                HighScore** cur = NULL;
                int cur_cnt = persist_read_scores(".snake_scores", &cur);
                bool qualifies = false;
                if (cur_cnt < PERSIST_MAX_SCORES)
                    qualifies = true;
                for (int i = 0; cur && !qualifies && i < cur_cnt; i++) {
                    if (cur[i] && death_score > highscore_get_score(cur[i]))
                        qualifies = true;
                }
                char name_buf[PERSIST_PLAYER_NAME_MAX] = {0};
                if (qualifies) {
                    /* Default highscore name to player id (P1..P4) for now */
                    snprintf(name_buf, sizeof(name_buf), "P%d", death_player + 1);
                    if (death_player == 0)
                        auto_restart_after_highscore = true;
                } else {
                    snprintf(name_buf, sizeof(name_buf), "%s", game_config_get_player_name(cfg));
                }
                if (!persist_append_score(".snake_scores", name_buf, death_score)) {
                    console_warn("Failed to append score for %s\n", name_buf);
                }
                render_note_session_score(name_buf, death_score);
                if (cur) {
                    persist_free_scores(cur, cur_cnt);
                    cur = NULL;
                }
                if (highscores) {
                    persist_free_scores(highscores, highscore_count);
                    highscores = NULL;
                }
                highscore_count = persist_read_scores(".snake_scores", &highscores);
            }
        }
        /* Multiplayer mode: end when <= 1 active players remain (last man standing).
                   Handle end-of-match overlays and highscore saving to a multiplayer list. */
        if (game_get_num_players(game) > 1) {
            const GameState* gs = game_get_state(game);
            int active_count = 0;
            int last_active = -1;
            int np = game_get_num_players(game);
            for (int i = 0; i < np; i++) {
                if (gs->players[i].active) {
                    active_count++;
                    last_active = i;
                }
            }
            if (active_count <= 1) {
                /* Render final board */
                render_draw(game_get_state(game), game_config_get_player_name(cfg), highscores, highscore_count);
                if (has_3d) {
                    if (active_count == 1)
                        render_3d_draw_winner_overlay(gs, last_active, gs->players[last_active].score);
                    else
                        render_3d_draw_death_overlay(game_get_state(game), 0, true);
                } else {
                    if (active_count == 1)
                        render_draw_winner_overlay(game_get_state(game), last_active, gs->players[last_active].score);
                    else
                        render_draw_death_overlay(game_get_state(game), 0, true);
                }
                /* If there is a winner and a non-zero score, prompt for name and append to multiplayer scores. */
                if (active_count == 1) {
                    int final_score = gs->players[last_active].score;
                    if (final_score > 0) {
                        char name_buf[PERSIST_PLAYER_NAME_MAX] = {0};
                        render_prompt_for_highscore_name(name_buf, (int)sizeof(name_buf), final_score);
                        if (!persist_append_score(".snake_scores_multi", name_buf, final_score))
                            console_warn("Failed to append multiplayer highscore for %s\n", name_buf);
                        /* Treat the Enter used to confirm the name as an implicit restart command. */
                        auto_restart_after_highscore = true;
                    }
                }
                /* If player already confirmed name with Enter, restart immediately. */
                if (auto_restart_after_highscore) {
                    game_reset(game);
                    continue;
                }
                /* Wait for user input to restart or quit. */
                while (1) {
                    InputState in = {0};
                    input_poll(&in);
                    if (in.quit)
                        goto clean_done;
                    if (in.any_key) {
                        game_reset(game);
                        break;
                    }
                    if (has_3d) {
                        if (active_count == 1)
                            render_3d_draw_winner_overlay(gs, last_active, gs->players[last_active].score);
                        else
                            render_3d_draw_death_overlay(game_get_state(game), 0, true);
                    } else {
                        if (active_count == 1)
                            render_draw_winner_overlay(game_get_state(game), last_active,
                                                       gs->players[last_active].score);
                        else
                            render_draw_death_overlay(game_get_state(game), 0, true);
                    }
                    platform_sleep_ms(20);
                }
            }
        } else {
            /* Single-player existing behavior: if player 0 died, show death overlay and optionally restart. */
            bool player0_died = false;
            for (int ei = 0; ei < events.died_count; ei++)
                if (events.died_players[ei] == 0)
                    player0_died = true;
            if (player0_died) {
                render_draw(game_get_state(game), game_config_get_player_name(cfg), highscores, highscore_count);
                render_draw_death_overlay(game_get_state(game), 0, true);
                if (has_3d)
                    render_3d_draw_death_overlay(game_get_state(game), 0, true);
                if (auto_restart_after_highscore) {
                    game_reset(game);
                } else {
                    while (1) {
                        InputState in = {0};
                        input_poll(&in);
                        if (in.quit)
                            goto clean_done;
                        if (in.any_key) {
                            game_reset(game);
                            break;
                        }
                        if (has_3d)
                            render_3d_draw_death_overlay(game_get_state(game), 0, true);
                        platform_sleep_ms(20);
                    }
                }
            }
        }
        uint64_t frame_start = platform_now_ms();
        uint64_t frame_deadline = frame_start + (uint64_t)game_config_get_tick_rate_ms(cfg);
        uint64_t prev_frame = frame_start;
        while (platform_now_ms() < frame_deadline) {
            uint64_t now = platform_now_ms();
            float delta_s = (float)(now - prev_frame) / 1000.0f;
            prev_frame = now;
            /* Poll inputs for all players frequently so rapid taps for non-player-1 are captured.
               Previously we only polled single-player in this inner loop which ignored quick
               non-player-1 inputs that occurred after the initial per-tick sample. */
            if (num_players <= 1) {
                InputState in = {0};
                input_poll(&in);
                if (in.quit)
                    goto clean_done;
                (void)game_enqueue_input(game, 0, &in);
            } else {
                InputState ins[SNAKE_MAX_PLAYERS];
                for (int i = 0; i < SNAKE_MAX_PLAYERS; ++i)
                    ins[i] = (InputState){0};
                input_poll_all(ins, num_players);
                for (int i = 0; i < num_players && i < SNAKE_MAX_PLAYERS; ++i) {
                    if (ins[i].quit)
                        goto clean_done;
                    (void)game_enqueue_input(game, i, &ins[i]);
                }
            }
            render_draw(game_get_state(game), game_config_get_player_name(cfg), highscores, highscore_count);
            if (has_3d)
                render_3d_draw(game_get_state(game), game_config_get_player_name(cfg), highscores, highscore_count,
                               delta_s);

            /* Draw remote multiplayer players over the local board */
            if (g_remote_player_count > 0) {
                int remote_x[MAX_REMOTE_PLAYERS];
                int remote_y[MAX_REMOTE_PLAYERS];
                char remote_names[MAX_REMOTE_PLAYERS][32];
                for (int ri = 0; ri < g_remote_player_count && ri < MAX_REMOTE_PLAYERS; ri++) {
                    remote_x[ri] = g_remote_players[ri].x;
                    remote_y[ri] = g_remote_players[ri].y;
                    snprintf(remote_names[ri], sizeof(remote_names[ri]), "%s", g_remote_players[ri].name);
                }
                render_draw_remote_players(game_get_state(game), remote_x, remote_y, remote_names,
                                           g_remote_player_count);
            }

            /* Poll incoming multiplayer messages and show them on HUD */
            if (mpc) {
                char mpbuf[1024];
                while (mpclient_poll_message(mpc, mpbuf, (int)sizeof(mpbuf))) {
                    /* Parse as game state if it looks like one, otherwise show as message */
                    if (strstr(mpbuf, "\"type\":\"state\"")) {
                        parse_remote_game_state(mpbuf);
                        net_log_info("parsed remote state: %d players", g_remote_player_count);
                    } else {
                        render_push_mp_message(mpbuf);
                    }
                }

                /* Update displayed session id when it becomes available or changes */
                char cur_sess[16] = {0};
                if (mpclient_get_session(mpc, cur_sess, (int)sizeof(cur_sess)) && strcmp(prev_session, cur_sess) != 0) {
                    strncpy(prev_session, cur_sess, sizeof(prev_session) - 1);
                    prev_session[sizeof(prev_session) - 1] = '\0';
                    render_set_session_id(prev_session);
                }

                /* Send current game state as a 'game' message (JSON object) */
                if (mpc && mpclient_has_session(mpc)) {
                    const GameState* gs = game_get_state(game);
                    char* state_json = game_state_to_json(gs, tick);
                    if (state_json) {
                        net_log_info("sending game state: tick=%d len=%zu", tick, strlen(state_json));
                        (void)mpclient_send_game(mpc, state_json);
                        free(state_json);
                    }
                }
            }

            platform_sleep_ms(1);
        }
        tick++;
    }
clean_done:
    if (game_player_is_active(game, 0) && game_player_current_score(game, 0) > 0) {
        int final_score = game_player_current_score(game, 0);
        HighScore** cur = NULL;
        int cur_cnt = persist_read_scores(".snake_scores", &cur);
        bool qualifies = false;
        if (cur_cnt < PERSIST_MAX_SCORES)
            qualifies = true;
        for (int i = 0; cur && !qualifies && i < cur_cnt; i++) {
            if (cur[i] && final_score > highscore_get_score(cur[i]))
                qualifies = true;
        }
        char name_buf[PERSIST_PLAYER_NAME_MAX] = {0};
        if (qualifies) {
            /* For now, auto-name final highscores P1 until we add per-player names */
            snprintf(name_buf, sizeof(name_buf), "P1");
        } else {
            snprintf(name_buf, sizeof(name_buf), "%s", game_config_get_player_name(cfg));
        }
        if (!persist_append_score(".snake_scores", name_buf, final_score)) {
            console_warn("Failed to append final score for %s\n", name_buf);
        }
        render_note_session_score(name_buf, final_score);
        if (cur)
            persist_free_scores(cur, cur_cnt);
    }
    if (highscores) {
        persist_free_scores(highscores, highscore_count);
        highscores = NULL;
    }
    if (game) {
        game_destroy(game);
        s->game = NULL;
    }

    /* Cleanup multiplayer client if active */
    if (mpc) {
        mpclient_stop(mpc);
        mpclient_destroy(mpc);
        mpc = NULL;
    }

    input_shutdown();
    render_shutdown();
    if (has_3d)
        render_3d_shutdown();
    console_game_ran(tick);
    return err;
}
void snake_game_free(SnakeGame* s) {
    if (!s)
        return;
    if (s->game)
        game_destroy(s->game);
    input_shutdown();
    render_shutdown();
    if (s->has_3d)
        render_3d_shutdown();
    if (s->cfg)
        game_config_destroy(s->cfg);
    free(s);
}
