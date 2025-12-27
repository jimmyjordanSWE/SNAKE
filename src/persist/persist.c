#include "persist.h"
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#define PERSIST_SCORE_BUFFER 256
#define PERSIST_CONFIG_BUFFER 256
#define PERSIST_TEMP_FILENAME_MAX 512
static char* trim_in_place(char* s) {
    if (s == NULL)
        return NULL;
    while (*s && isspace((unsigned char)*s))
        s++;
    char* end = s + strlen(s);
    while (end > s && isspace((unsigned char)end[-1]))
        end--;
    *end = '\0';
    return s;
}
static int clamp_int(int v, int lo, int hi) {
    if (v < lo)
        return lo;
    if (v > hi)
        return hi;
    return v;
}
struct HighScore {
    char name[PERSIST_NAME_MAX];
    int score;
};
HighScore* highscore_create(const char* name, int score) {
    HighScore* hs = (HighScore*)calloc(1, sizeof(*hs));
    if (!hs)
        return NULL;
    if (name)
        snprintf(hs->name, PERSIST_NAME_MAX, "%s", name);
    hs->score = score;
    return hs;
}
void highscore_destroy(HighScore* hs) {
    if (!hs)
        return;
    free(hs);
}
const char* highscore_get_name(const HighScore* hs) {
    return hs ? hs->name : NULL;
}
int highscore_get_score(const HighScore* hs) {
    return hs ? hs->score : 0;
}
void highscore_set_name(HighScore* hs, const char* name) {
    if (!hs || !name)
        return;
    snprintf(hs->name, PERSIST_NAME_MAX, "%s", name);
}
void highscore_set_score(HighScore* hs, int score) {
    if (!hs)
        return;
    hs->score = score;
}
struct GameConfig {
    int board_width, board_height;
    int tick_rate_ms;
    int render_glyphs;
    int screen_width, screen_height;
    int enable_external_3d_view;
    uint32_t seed;
    float fov_degrees;
    int show_sprite_debug;
    int active_player;
    int num_players;
    char player_name[PERSIST_PLAYER_NAME_MAX];
    int max_players;
    int max_length;
    int max_food;
    float wall_height_scale;
    float tail_height_scale;
    float wall_texture_scale;
    float floor_texture_scale;
    char wall_texture[PERSIST_TEXTURE_PATH_MAX];
    char floor_texture[PERSIST_TEXTURE_PATH_MAX];
    char key_up;
    char key_down;
    char key_left;
    char key_right;
    char key_quit;
    char key_restart;
    char key_pause;
};
GameConfig* game_config_create(void) {
    GameConfig* c = (GameConfig*)calloc(1, sizeof(*c));
    if (!c)
        return NULL;
    c->board_width = PERSIST_CONFIG_DEFAULT_WIDTH;
    c->board_height = PERSIST_CONFIG_DEFAULT_HEIGHT;
    c->tick_rate_ms = PERSIST_CONFIG_DEFAULT_TICK_MS;
    c->render_glyphs = 0;
    c->screen_width = PERSIST_CONFIG_DEFAULT_SCREEN_WIDTH;
    c->screen_height = PERSIST_CONFIG_DEFAULT_SCREEN_HEIGHT;
    c->enable_external_3d_view = PERSIST_CONFIG_DEFAULT_ENABLE_EXTERNAL_3D_VIEW;
    c->seed = (uint32_t)PERSIST_CONFIG_DEFAULT_SEED;
    c->fov_degrees = (float)PERSIST_CONFIG_DEFAULT_FOV_DEGREES;
    c->show_sprite_debug = PERSIST_CONFIG_DEFAULT_SHOW_SPRITE_DEBUG;
    c->active_player = PERSIST_CONFIG_DEFAULT_ACTIVE_PLAYER;
    c->num_players = PERSIST_CONFIG_DEFAULT_NUM_PLAYERS;
    snprintf(c->player_name, PERSIST_PLAYER_NAME_MAX, "You");
    c->max_players = PERSIST_CONFIG_DEFAULT_MAX_PLAYERS;
    c->max_length = PERSIST_CONFIG_DEFAULT_MAX_LENGTH;
    c->max_food = PERSIST_CONFIG_DEFAULT_MAX_FOOD;
    c->wall_height_scale = (float)PERSIST_CONFIG_DEFAULT_WALL_SCALE;
    c->tail_height_scale = (float)PERSIST_CONFIG_DEFAULT_TAIL_SCALE;
    c->wall_texture_scale = (float)PERSIST_CONFIG_DEFAULT_WALL_TEXTURE_SCALE;
    c->floor_texture_scale = (float)PERSIST_CONFIG_DEFAULT_FLOOR_TEXTURE_SCALE;
    snprintf(c->wall_texture, PERSIST_TEXTURE_PATH_MAX, PERSIST_CONFIG_DEFAULT_WALL_TEXTURE);
    snprintf(c->floor_texture, PERSIST_TEXTURE_PATH_MAX, "%s", PERSIST_CONFIG_DEFAULT_FLOOR_TEXTURE);
    c->key_up = PERSIST_CONFIG_DEFAULT_KEY_UP;
    c->key_down = PERSIST_CONFIG_DEFAULT_KEY_DOWN;
    c->key_left = PERSIST_CONFIG_DEFAULT_KEY_LEFT;
    c->key_right = PERSIST_CONFIG_DEFAULT_KEY_RIGHT;
    c->key_quit = PERSIST_CONFIG_DEFAULT_KEY_QUIT;
    c->key_restart = PERSIST_CONFIG_DEFAULT_KEY_RESTART;
    c->key_pause = PERSIST_CONFIG_DEFAULT_KEY_PAUSE;
    return c;
}
void game_config_destroy(GameConfig* cfg) {
    if (!cfg)
        return;
    free(cfg);
}
void game_config_set_board_size(GameConfig* cfg, int w, int h) {
    if (!cfg)
        return;
    cfg->board_width = w;
    cfg->board_height = h;
}
void game_config_get_board_size(const GameConfig* cfg, int* w_out, int* h_out) {
    if (!cfg)
        return;
    if (w_out)
        *w_out = cfg->board_width;
    if (h_out)
        *h_out = cfg->board_height;
}
void game_config_set_tick_rate_ms(GameConfig* cfg, int ms) {
    if (!cfg)
        return;
    cfg->tick_rate_ms = ms;
}
int game_config_get_tick_rate_ms(const GameConfig* cfg) {
    return cfg ? cfg->tick_rate_ms : 0;
}
void game_config_set_screen_size(GameConfig* cfg, int w, int h) {
    if (!cfg)
        return;
    cfg->screen_width = w;
    cfg->screen_height = h;
}
void game_config_get_screen_size(const GameConfig* cfg, int* w_out, int* h_out) {
    if (!cfg)
        return;
    if (w_out)
        *w_out = cfg->screen_width;
    if (h_out)
        *h_out = cfg->screen_height;
}
void game_config_set_seed(GameConfig* cfg, uint32_t seed) {
    if (!cfg)
        return;
    cfg->seed = seed;
}
uint32_t game_config_get_seed(const GameConfig* cfg) {
    return cfg ? cfg->seed : 0;
}
void game_config_set_fov_degrees(GameConfig* cfg, float fov) {
    if (!cfg)
        return;
    cfg->fov_degrees = fov;
}
float game_config_get_fov_degrees(const GameConfig* cfg) {
    return cfg ? cfg->fov_degrees : 0.0f;
}
void game_config_set_player_name(GameConfig* cfg, const char* name) {
    if (!cfg || !name)
        return;
    snprintf(cfg->player_name, PERSIST_PLAYER_NAME_MAX, "%s", name);
}
const char* game_config_get_player_name(const GameConfig* cfg) {
    return cfg ? cfg->player_name : NULL;
}
void game_config_set_render_glyphs(GameConfig* cfg, int v) {
    if (!cfg)
        return;
    cfg->render_glyphs = (v != 0) ? 1 : 0;
}
int game_config_get_render_glyphs(const GameConfig* cfg) {
    return cfg ? cfg->render_glyphs : 0;
}
void game_config_set_show_sprite_debug(GameConfig* cfg, int v) {
    if (!cfg)
        return;
    cfg->show_sprite_debug = v;
}
int game_config_get_show_sprite_debug(const GameConfig* cfg) {
    return cfg ? cfg->show_sprite_debug : 0;
}
void game_config_set_num_players(GameConfig* cfg, int n) {
    if (!cfg)
        return;
    cfg->num_players = n;
}
int game_config_get_num_players(const GameConfig* cfg) {
    return cfg ? cfg->num_players : 0;
}
void game_config_set_max_players(GameConfig* cfg, int n) {
    if (!cfg)
        return;
    cfg->max_players = n;
}
int game_config_get_max_players(const GameConfig* cfg) {
    return cfg ? cfg->max_players : 0;
}
void game_config_set_max_length(GameConfig* cfg, int v) {
    if (!cfg)
        return;
    cfg->max_length = v;
}
int game_config_get_max_length(const GameConfig* cfg) {
    return cfg ? cfg->max_length : 0;
}
void game_config_set_max_food(GameConfig* cfg, int v) {
    if (!cfg)
        return;
    cfg->max_food = v;
}
int game_config_get_max_food(const GameConfig* cfg) {
    return cfg ? cfg->max_food : 0;
}
void game_config_set_wall_height_scale(GameConfig* cfg, float v) {
    if (!cfg)
        return;
    cfg->wall_height_scale = v;
}
float game_config_get_wall_height_scale(const GameConfig* cfg) {
    return cfg ? cfg->wall_height_scale : 0.0f;
}
void game_config_set_tail_height_scale(GameConfig* cfg, float v) {
    if (!cfg)
        return;
    cfg->tail_height_scale = v;
}
float game_config_get_tail_height_scale(const GameConfig* cfg) {
    return cfg ? cfg->tail_height_scale : 0.0f;
}
void game_config_set_wall_texture_scale(GameConfig* cfg, float v) {
    if (cfg)
        cfg->wall_texture_scale = v;
}
float game_config_get_wall_texture_scale(const GameConfig* cfg) {
    return cfg ? cfg->wall_texture_scale : 1.0f;
}
void game_config_set_floor_texture_scale(GameConfig* cfg, float v) {
    if (cfg)
        cfg->floor_texture_scale = v;
}
float game_config_get_floor_texture_scale(const GameConfig* cfg) {
    return cfg ? cfg->floor_texture_scale : 1.0f;
}
void game_config_set_wall_texture(GameConfig* cfg, const char* path) {
    if (!cfg || !path)
        return;
    snprintf(cfg->wall_texture, PERSIST_TEXTURE_PATH_MAX, "%s", path);
}
const char* game_config_get_wall_texture(const GameConfig* cfg) {
    return cfg ? cfg->wall_texture : NULL;
}
void game_config_set_floor_texture(GameConfig* cfg, const char* path) {
    if (!cfg || !path)
        return;
    snprintf(cfg->floor_texture, PERSIST_TEXTURE_PATH_MAX, "%s", path);
}
const char* game_config_get_floor_texture(const GameConfig* cfg) {
    return cfg ? cfg->floor_texture : NULL;
}
void game_config_set_key_up(GameConfig* cfg, char c) {
    if (!cfg)
        return;
    cfg->key_up = c;
}
char game_config_get_key_up(const GameConfig* cfg) {
    return cfg ? cfg->key_up : '\0';
}
void game_config_set_key_down(GameConfig* cfg, char c) {
    if (!cfg)
        return;
    cfg->key_down = c;
}
char game_config_get_key_down(const GameConfig* cfg) {
    return cfg ? cfg->key_down : '\0';
}
void game_config_set_key_left(GameConfig* cfg, char c) {
    if (!cfg)
        return;
    cfg->key_left = c;
}
char game_config_get_key_left(const GameConfig* cfg) {
    return cfg ? cfg->key_left : '\0';
}
void game_config_set_key_right(GameConfig* cfg, char c) {
    if (!cfg)
        return;
    cfg->key_right = c;
}
char game_config_get_key_right(const GameConfig* cfg) {
    return cfg ? cfg->key_right : '\0';
}
void game_config_set_key_quit(GameConfig* cfg, char c) {
    if (!cfg)
        return;
    cfg->key_quit = c;
}
char game_config_get_key_quit(const GameConfig* cfg) {
    return cfg ? cfg->key_quit : '\0';
}
void game_config_set_key_restart(GameConfig* cfg, char c) {
    if (!cfg)
        return;
    cfg->key_restart = c;
}
char game_config_get_key_restart(const GameConfig* cfg) {
    return cfg ? cfg->key_restart : '\0';
}
void game_config_set_key_pause(GameConfig* cfg, char c) {
    if (!cfg)
        return;
    cfg->key_pause = c;
}
char game_config_get_key_pause(const GameConfig* cfg) {
    return cfg ? cfg->key_pause : '\0';
}
void game_config_set_enable_external_3d_view(GameConfig* cfg, int v) {
    if (!cfg)
        return;
    cfg->enable_external_3d_view = v ? 1 : 0;
}
int game_config_get_enable_external_3d_view(const GameConfig* cfg) {
    return cfg ? cfg->enable_external_3d_view : 0;
}
void game_config_set_active_player(GameConfig* cfg, int v) {
    if (!cfg)
        return;
    cfg->active_player = v;
}
int game_config_get_active_player(const GameConfig* cfg) {
    return cfg ? cfg->active_player : 0;
}
int persist_read_scores(const char* filename, HighScore*** out_scores) {
    if (filename == NULL || out_scores == NULL)
        return 0;
    FILE* fp = fopen(filename, "r");
    if (fp == NULL) {
        *out_scores = NULL;
        return 0;
    }
    HighScore** arr = calloc((size_t)PERSIST_MAX_SCORES, sizeof *arr);
    if (!arr) {
        fclose(fp);
        *out_scores = NULL;
        return 0;
    }
    int count = 0;
    char buffer[PERSIST_SCORE_BUFFER];
    while (count < PERSIST_MAX_SCORES && fgets(buffer, (int)sizeof(buffer), fp) != NULL) {
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
            len--;
        }
        if (len == 0)
            continue;
        char* space_pos = strchr(buffer, ' ');
        if (space_pos == NULL)
            continue;
        size_t name_len = (size_t)(space_pos - buffer);
        if (name_len == 0 || name_len >= (size_t)PERSIST_NAME_MAX)
            continue;
        char name[PERSIST_NAME_MAX];
        memcpy(name, buffer, name_len);
        name[name_len] = '\0';
        char* score_str = space_pos + 1;
        char* endptr = NULL;
        errno = 0;
        long score_long = strtol(score_str, &endptr, 10);
        if (errno != 0 || endptr == score_str || score_long < 0 || score_long > INT_MAX)
            continue;
        arr[count] = highscore_create(name, (int)score_long);
        if (!arr[count])
            break;
        count++;
    }
    fclose(fp);
    if (count == 0) {
        free(arr);
        *out_scores = NULL;
        return 0;
    }
    *out_scores = arr;
    return count;
}
bool persist_write_scores(const char* filename, HighScore** scores, int count) {
    if (filename == NULL || scores == NULL || count < 0 || count > PERSIST_MAX_SCORES)
        return false;
    char temp_filename[PERSIST_TEMP_FILENAME_MAX];
    if (snprintf(temp_filename, sizeof(temp_filename), "%s.tmp", filename) < 0)
        return false;
    FILE* fp = fopen(temp_filename, "w");
    if (fp == NULL)
        return false;
    for (int i = 0; i < count; i++) {
        const char* name = highscore_get_name(scores[i]);
        int score = highscore_get_score(scores[i]);
        if (fprintf(fp, "%s %d\n", name ? name : "", score) < 0) {
            fclose(fp);
            (void)unlink(temp_filename);
            return false;
        }
    }
    if (fflush(fp) != 0) {
        fclose(fp);
        (void)unlink(temp_filename);
        return false;
    }
    if (fclose(fp) != 0) {
        (void)unlink(temp_filename);
        return false;
    }
    if (rename(temp_filename, filename) != 0) {
        (void)unlink(temp_filename);
        return false;
    }
    return true;
}
static int compare_scores(const void* a, const void* b) {
    const HighScore* const* pa = (const HighScore* const*)a;
    const HighScore* const* pb = (const HighScore* const*)b;
    const HighScore* score_a = *pa;
    const HighScore* score_b = *pb;
    if (score_a->score > score_b->score)
        return -1;
    if (score_a->score < score_b->score)
        return 1;
    return 0;
}
bool persist_append_score(const char* filename, const char* name, int score) {
    if (filename == NULL || name == NULL || score < 0)
        return false;
    HighScore** arr = NULL;
    int count = persist_read_scores(filename, &arr);
    if (count < 0)
        count = 0;
    if (count < PERSIST_MAX_SCORES) {
        HighScore** tmp = realloc(arr, (size_t)(count + 1) * sizeof *tmp);
        if (!tmp) {
            persist_free_scores(arr, count);
            return false;
        }
        arr = tmp;
        arr[count] = highscore_create(name, score);
        if (!arr[count]) {
            persist_free_scores(arr, count);
            return false;
        }
        count++;
    } else {
        qsort(arr, (size_t)count, sizeof(HighScore*), compare_scores);
        if (score > highscore_get_score(arr[count - 1])) {
            highscore_set_name(arr[count - 1], name);
            highscore_set_score(arr[count - 1], score);
        } else {
            persist_free_scores(arr, count);
            return false;
        }
    }
    qsort(arr, (size_t)count, sizeof(HighScore*), compare_scores);
    int final_count = (count > 5) ? 5 : count;
    bool ok = persist_write_scores(filename, arr, final_count);
    persist_free_scores(arr, count);
    return ok;
}
void persist_free_scores(HighScore** scores, int count) {
    if (!scores)
        return;
    for (int i = 0; i < count; i++) {
        if (scores[i])
            highscore_destroy(scores[i]);
    }
    free(scores);
}
bool persist_load_config(const char* filename, GameConfig** out_config) {
    if (out_config == NULL)
        return false;
    GameConfig* config = game_config_create();
    if (config == NULL)
        return false;
    if (filename == NULL) {
        *out_config = config;
        return false;
    }
    FILE* fp = fopen(filename, "r");
    if (fp == NULL) {
        *out_config = config;
        return false;
    }
    bool file_exists = true;
    char buffer[PERSIST_CONFIG_BUFFER];
    while (fgets(buffer, (int)sizeof(buffer), fp) != NULL) {
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
            len--;
        }
        if (len == 0 || buffer[0] == '#')
            continue;
        char* eq_pos = strchr(buffer, '=');
        if (eq_pos == NULL)
            continue;
        *eq_pos = '\0';
        char* key = trim_in_place(buffer);
        char* value = trim_in_place(eq_pos + 1);
        char* endptr = NULL;
        if (key == NULL || value == NULL || *key == '\0' || *value == '\0')
            continue;
        if (strcmp(key, "render_glyphs") == 0 || strcmp(key, "glyphs") == 0 || strcmp(key, "charset") == 0) {
            if (isdigit((unsigned char)value[0])) {
                char* endptr2 = NULL;
                errno = 0;
                long v = strtol(value, &endptr2, 10);
                if (errno != 0 || endptr2 == value)
                    continue;
                config->render_glyphs = clamp_int((int)v, 0, 1);
            } else {
                for (char* p = value; *p; p++)
                    *p = (char)tolower((unsigned char)*p);
                if (strcmp(value, "utf8") == 0 || strcmp(value, "unicode") == 0 || strcmp(value, "box") == 0)
                    config->render_glyphs = 0;
                else if (strcmp(value, "ascii") == 0 || strcmp(value, "legacy") == 0)
                    config->render_glyphs = 1;
            }
            continue;
        }
        if (strcmp(key, "enable_external_3d_view") == 0) {
            for (char* p = value; *p; p++)
                *p = (char)tolower((unsigned char)*p);
            if (strcmp(value, "true") == 0 || strcmp(value, "yes") == 0 || strcmp(value, "1") == 0)
                config->enable_external_3d_view = 1;
            else if (strcmp(value, "false") == 0 || strcmp(value, "no") == 0 || strcmp(value, "0") == 0)
                config->enable_external_3d_view = 0;
            continue;
        } else if (strcmp(key, "seed") == 0) {
            errno = 0;
            unsigned long v = strtoul(value, &endptr, 10);
            if (errno != 0 || endptr == value)
                continue;
            config->seed = (uint32_t)v;
            continue;
        } else if (strcmp(key, "fov_degrees") == 0) {
            char* endptr2 = NULL;
            errno = 0;
            double dv = strtod(value, &endptr2);
            if (errno != 0 || endptr2 == value)
                continue;
            config->fov_degrees = (float)dv;
            continue;
        } else if (strcmp(key, "player_name") == 0) {
            snprintf(config->player_name, PERSIST_PLAYER_NAME_MAX, "%s", value);
            continue;
        } else if (strcmp(key, "wall_height_scale") == 0) {
            char* endptr2 = NULL;
            errno = 0;
            double dv = strtod(value, &endptr2);
            if (errno != 0 || endptr2 == value)
                continue;
            config->wall_height_scale = (float)dv;
            continue;
        } else if (strcmp(key, "tail_height_scale") == 0) {
            char* endptr2 = NULL;
            errno = 0;
            double dv = strtod(value, &endptr2);
            if (errno != 0 || endptr2 == value)
                continue;
            config->tail_height_scale = (float)dv;
            continue;
        } else if (strcmp(key, "wall_texture_scale") == 0) {
            char* endptr2 = NULL;
            errno = 0;
            double dv = strtod(value, &endptr2);
            if (errno != 0 || endptr2 == value)
                continue;
            config->wall_texture_scale = (float)dv;
            continue;
        } else if (strcmp(key, "floor_texture_scale") == 0) {
            char* endptr2 = NULL;
            errno = 0;
            double dv = strtod(value, &endptr2);
            if (errno != 0 || endptr2 == value)
                continue;
            config->floor_texture_scale = (float)dv;
            continue;
        } else if (strcmp(key, "wall_texture") == 0) {
            snprintf(config->wall_texture, PERSIST_TEXTURE_PATH_MAX, "%s", value);
            continue;
        } else if (strcmp(key, "floor_texture") == 0) {
            snprintf(config->floor_texture, PERSIST_TEXTURE_PATH_MAX, "%s", value);
            continue;
        } else if (strcmp(key, "key_up") == 0 || strcmp(key, "key_down") == 0 || strcmp(key, "key_left") == 0 ||
                   strcmp(key, "key_right") == 0 || strcmp(key, "key_quit") == 0 || strcmp(key, "key_restart") == 0 ||
                   strcmp(key, "key_pause") == 0) {
            char c = value[0];
            if (strcmp(key, "key_up") == 0)
                config->key_up = c;
            else if (strcmp(key, "key_down") == 0)
                config->key_down = c;
            else if (strcmp(key, "key_left") == 0)
                config->key_left = c;
            else if (strcmp(key, "key_right") == 0)
                config->key_right = c;
            else if (strcmp(key, "key_quit") == 0)
                config->key_quit = c;
            else if (strcmp(key, "key_restart") == 0)
                config->key_restart = c;
            else if (strcmp(key, "key_pause") == 0)
                config->key_pause = c;
            continue;
        } else if (strcmp(key, "show_sprite_debug") == 0) {
            for (char* p = value; *p; p++)
                *p = (char)tolower((unsigned char)*p);
            int b = -1;
            if (strcmp(value, "true") == 0 || strcmp(value, "yes") == 0 || strcmp(value, "1") == 0)
                b = 1;
            else if (strcmp(value, "false") == 0 || strcmp(value, "no") == 0 || strcmp(value, "0") == 0)
                b = 0;
            if (b == -1)
                continue;
            config->show_sprite_debug = b;
            continue;
        } else if (strcmp(key, "active_player") == 0) {
            errno = 0;
            long v = strtol(value, &endptr, 10);
            if (errno != 0 || endptr == value)
                continue;
            config->active_player = (int)v;
            continue;
        } else if (strcmp(key, "num_players") == 0) {
            errno = 0;
            long v = strtol(value, &endptr, 10);
            if (errno != 0 || endptr == value)
                continue;
            config->num_players = (int)v;
            if (config->max_players < config->num_players)
                config->max_players = config->num_players;
            continue;
        } else if (strcmp(key, "max_players") == 0) {
            errno = 0;
            long v = strtol(value, &endptr, 10);
            if (errno != 0 || endptr == value)
                continue;
            config->max_players = (int)v;
            continue;
        } else if (strcmp(key, "max_length") == 0) {
            errno = 0;
            long v = strtol(value, &endptr, 10);
            if (errno != 0 || endptr == value)
                continue;
            config->max_length = (int)v;
            continue;
        } else if (strcmp(key, "max_food") == 0) {
            errno = 0;
            long v = strtol(value, &endptr, 10);
            if (errno != 0 || endptr == value)
                continue;
            config->max_food = (int)v;
            continue;
        }
        errno = 0;
        long parsed_value = strtol(value, &endptr, 10);
        if (errno != 0 || endptr == value)
            continue;
        if (parsed_value < (long)INT_MIN || parsed_value > (long)INT_MAX)
            continue;
        if (strcmp(key, "board_width") == 0)
            config->board_width = clamp_int((int)parsed_value, 20, 100);
        else if (strcmp(key, "board_height") == 0)
            config->board_height = clamp_int((int)parsed_value, 10, 100);
        else if (strcmp(key, "tick_rate_ms") == 0)
            config->tick_rate_ms = clamp_int((int)parsed_value, 10, 1000);
        else if (strcmp(key, "screen_width") == 0 || strcmp(key, "min_screen_width") == 0)
            config->screen_width = clamp_int((int)parsed_value, 20, 4096);
        else if (strcmp(key, "screen_height") == 0 || strcmp(key, "min_screen_height") == 0)
            config->screen_height = clamp_int((int)parsed_value, 10, 2160);
    }
    fclose(fp);
    *out_config = config;
    return file_exists;
}
bool persist_write_config(const char* filename, const GameConfig* config) {
    if (filename == NULL || config == NULL)
        return false;
    char temp_filename[PERSIST_TEMP_FILENAME_MAX];
    if (snprintf(temp_filename, sizeof(temp_filename), "%s.tmp", filename) < 0)
        return false;
    FILE* fp = fopen(temp_filename, "w");
    if (fp == NULL)
        return false;
    if (fprintf(fp, "# Snake Game Configuration\n") < 0)
        goto write_fail;
    if (fprintf(fp, "board_width=%d\n", config->board_width) < 0)
        goto write_fail;
    if (fprintf(fp, "board_height=%d\n", config->board_height) < 0)
        goto write_fail;
    if (fprintf(fp, "tick_rate_ms=%d\n", config->tick_rate_ms) < 0)
        goto write_fail;
    if (fprintf(fp, "render_glyphs=%s\n", (config->render_glyphs == 1) ? "ascii" : "utf8") < 0)
        goto write_fail;
    if (fprintf(fp, "screen_width=%d\n", config->screen_width) < 0)
        goto write_fail;
    if (fprintf(fp, "screen_height=%d\n", config->screen_height) < 0)
        goto write_fail;
    if (fprintf(fp, "enable_external_3d_view=%s\n", (config->enable_external_3d_view ? "true" : "false")) < 0)
        goto write_fail;
    if (fprintf(fp, "seed=%u\n", (unsigned int)config->seed) < 0)
        goto write_fail;
    if (fprintf(fp, "fov_degrees=%.2f\n", config->fov_degrees) < 0)
        goto write_fail;
    if (fprintf(fp, "show_sprite_debug=%s\n", config->show_sprite_debug ? "true" : "false") < 0)
        goto write_fail;
    if (fprintf(fp, "active_player=%d\n", config->active_player) < 0)
        goto write_fail;
    if (fprintf(fp, "num_players=%d\n", config->num_players) < 0)
        goto write_fail;
    if (fprintf(fp, "player_name=%s\n", config->player_name) < 0)
        goto write_fail;
    if (fprintf(fp, "max_players=%d\n", config->max_players) < 0)
        goto write_fail;
    if (fprintf(fp, "max_length=%d\n", config->max_length) < 0)
        goto write_fail;
    if (fprintf(fp, "max_food=%d\n", config->max_food) < 0)
        goto write_fail;
    if (fprintf(fp, "wall_height_scale=%.2f\n", config->wall_height_scale) < 0)
        goto write_fail;
    if (fprintf(fp, "wall_texture_scale=%.2f\n", config->wall_texture_scale) < 0)
        goto write_fail;
    if (fprintf(fp, "floor_texture_scale=%.2f\n", config->floor_texture_scale) < 0)
        goto write_fail;
    if (fprintf(fp, "wall_texture=%s\n", config->wall_texture) < 0)
        goto write_fail;
    if (fprintf(fp, "floor_texture=%s\n", config->floor_texture) < 0)
        goto write_fail;
    if (fprintf(fp, "key_up=%c\n", config->key_up) < 0)
        goto write_fail;
    if (fprintf(fp, "key_down=%c\n", config->key_down) < 0)
        goto write_fail;
    if (fprintf(fp, "key_left=%c\n", config->key_left) < 0)
        goto write_fail;
    if (fprintf(fp, "key_right=%c\n", config->key_right) < 0)
        goto write_fail;
    if (fprintf(fp, "key_quit=%c\n", config->key_quit) < 0)
        goto write_fail;
    if (fprintf(fp, "key_restart=%c\n", config->key_restart) < 0)
        goto write_fail;
    if (fprintf(fp, "key_pause=%c\n", config->key_pause) < 0)
        goto write_fail;
    if (fflush(fp) != 0) {
        fclose(fp);
        (void)unlink(temp_filename);
        return false;
    }
    if (fclose(fp) != 0) {
        (void)unlink(temp_filename);
        return false;
    }
    {
        struct stat st_new;
        struct stat st_old;
        bool identical = false;
        if (stat(temp_filename, &st_new) == 0 && stat(filename, &st_old) == 0) {
            if (st_new.st_size == st_old.st_size) {
                FILE* fnew = fopen(temp_filename, "rb");
                FILE* fold = fopen(filename, "rb");
                if (fnew && fold) {
                    identical = true;
                    size_t bufsize = 4096;
                    char* b1 = malloc(bufsize);
                    char* b2 = malloc(bufsize);
                    if (!b1 || !b2) {
                        identical = false;
                    } else {
                        size_t r1, r2;
                        do {
                            r1 = fread(b1, 1, bufsize, fnew);
                            r2 = fread(b2, 1, bufsize, fold);
                            if (r1 != r2 || (r1 > 0 && memcmp(b1, b2, r1) != 0)) {
                                identical = false;
                                break;
                            }
                        } while (r1 > 0 && r2 > 0);
                    }
                    free(b1);
                    free(b2);
                }
                if (fnew)
                    fclose(fnew);
                if (fold)
                    fclose(fold);
            }
        }
        if (identical) {
            (void)unlink(temp_filename);
            return true;
        }
        if (rename(temp_filename, filename) != 0) {
            (void)unlink(temp_filename);
            return false;
        }
        return true;
    }
write_fail:
    fclose(fp);
    (void)unlink(temp_filename);
    return false;
}
static bool is_known_config_key(const char* key) {
    if (key == NULL || *key == '\0')
        return false;
    if (strcmp(key, "render_glyphs") == 0 || strcmp(key, "glyphs") == 0 || strcmp(key, "charset") == 0)
        return true;
    if (strcmp(key, "enable_external_3d_view") == 0)
        return true;
    if (strcmp(key, "seed") == 0)
        return true;
    if (strcmp(key, "fov_degrees") == 0)
        return true;
    if (strcmp(key, "player_name") == 0)
        return true;
    if (strcmp(key, "wall_height_scale") == 0)
        return true;
    if (strcmp(key, "tail_height_scale") == 0)
        return true;
    if (strcmp(key, "wall_texture_scale") == 0)
        return true;
    if (strcmp(key, "floor_texture_scale") == 0)
        return true;
    if (strcmp(key, "wall_texture") == 0)
        return true;
    if (strcmp(key, "floor_texture") == 0)
        return true;
    if (strcmp(key, "key_up") == 0 || strcmp(key, "key_down") == 0 || strcmp(key, "key_left") == 0 ||
        strcmp(key, "key_right") == 0 || strcmp(key, "key_quit") == 0 || strcmp(key, "key_restart") == 0 ||
        strcmp(key, "key_pause") == 0)
        return true;
    if (strcmp(key, "show_sprite_debug") == 0)
        return true;
    if (strcmp(key, "active_player") == 0)
        return true;
    if (strcmp(key, "num_players") == 0)
        return true;
    if (strcmp(key, "max_players") == 0)
        return true;
    if (strcmp(key, "max_length") == 0)
        return true;
    if (strcmp(key, "max_food") == 0)
        return true;
    if (strcmp(key, "board_width") == 0)
        return true;
    if (strcmp(key, "board_height") == 0)
        return true;
    if (strcmp(key, "tick_rate_ms") == 0)
        return true;
    if (strcmp(key, "screen_width") == 0 || strcmp(key, "min_screen_width") == 0)
        return true;
    if (strcmp(key, "screen_height") == 0 || strcmp(key, "min_screen_height") == 0)
        return true;
    return false;
}
bool persist_config_has_unknown_keys(const char* filename) {
    if (filename == NULL)
        return false;
    FILE* fp = fopen(filename, "r");
    if (fp == NULL)
        return false;
    char buffer[PERSIST_CONFIG_BUFFER];
    while (fgets(buffer, (int)sizeof(buffer), fp) != NULL) {
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
            len--;
        }
        if (len == 0)
            continue;
        if (buffer[0] == '#')
            continue;
        char* eq_pos = strchr(buffer, '=');
        if (eq_pos == NULL)
            continue;
        *eq_pos = '\0';
        char* key = trim_in_place(buffer);
        if (key == NULL || *key == '\0')
            continue;
        if (!is_known_config_key(key)) {
            fclose(fp);
            return true;
        }
    }
    fclose(fp);
    return false;
}
