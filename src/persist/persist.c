#include "snake/persist.h"
#include <ctype.h>
#include <errno.h>
#include <limits.h>
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
if(s == NULL) return NULL;
while(*s && isspace((unsigned char)*s)) s++;
char* end= s + strlen(s);
while(end > s && isspace((unsigned char)end[-1])) end--;
*end= '\0';
return s;
}
static int clamp_int(int v, int lo, int hi) {
if(v < lo) return lo;
if(v > hi) return hi;
return v;
}
int persist_read_scores(const char* filename, HighScore* scores, int max_count) {
if(filename == NULL || scores == NULL || max_count <= 0) return 0;
FILE* fp= fopen(filename, "r");
if(fp == NULL) return 0;
int count= 0;
char buffer[PERSIST_SCORE_BUFFER];
while(count < max_count && fgets(buffer, (int)sizeof(buffer), fp) != NULL) {
size_t len= strlen(buffer);
if(len > 0 && buffer[len - 1] == '\n') {
buffer[len - 1]= '\0';
len--;
}
if(len == 0) continue;
char* space_pos= strchr(buffer, ' ');
if(space_pos == NULL) continue;
size_t name_len= (size_t)(space_pos - buffer);
if(name_len == 0 || name_len >= (size_t)PERSIST_NAME_MAX) continue;
memcpy(scores[count].name, buffer, name_len);
scores[count].name[name_len]= '\0';
char* score_str= space_pos + 1;
char* endptr= NULL;
errno= 0;
long score_long= strtol(score_str, &endptr, 10);
if(errno != 0 || endptr == score_str || score_long < 0 || score_long > INT_MAX) continue;
scores[count].score= (int)score_long;
count++;
}
fclose(fp);
return count;
}
bool persist_write_scores(const char* filename, const HighScore* scores, int count) {
if(filename == NULL || scores == NULL || count < 0 || count > PERSIST_MAX_SCORES) return false;
char temp_filename[PERSIST_TEMP_FILENAME_MAX];
if(snprintf(temp_filename, sizeof(temp_filename), "%s.tmp", filename) < 0) return false;
FILE* fp= fopen(temp_filename, "w");
if(fp == NULL) return false;
for(int i= 0; i < count; i++) {
if(fprintf(fp, "%s %d\n", scores[i].name, scores[i].score) < 0) {
fclose(fp);
(void)unlink(temp_filename);
return false;
}
}
if(fflush(fp) != 0) {
fclose(fp);
(void)unlink(temp_filename);
return false;
}
if(fclose(fp) != 0) {
(void)unlink(temp_filename);
return false;
}
if(rename(temp_filename, filename) != 0) {
(void)unlink(temp_filename);
return false;
}
return true;
}
static int compare_scores(const void* a, const void* b) {
const HighScore* score_a= (const HighScore*)a;
const HighScore* score_b= (const HighScore*)b;
if(score_a->score > score_b->score) return -1;
if(score_a->score < score_b->score) return 1;
return 0;
}
bool persist_append_score(const char* filename, const char* name, int score) {
if(filename == NULL || name == NULL || score < 0) return false;
HighScore scores[PERSIST_MAX_SCORES];
int count= persist_read_scores(filename, scores, PERSIST_MAX_SCORES);
if(count < PERSIST_MAX_SCORES) {
snprintf(scores[count].name, PERSIST_NAME_MAX, "%s", name);
scores[count].score= score;
count++;
} else {
qsort(scores, (size_t)count, sizeof(HighScore), compare_scores);
if(score > scores[count - 1].score) {
snprintf(scores[count - 1].name, PERSIST_NAME_MAX, "%s", name);
scores[count - 1].score= score;
} else {
return false;
}
}
qsort(scores, (size_t)count, sizeof(HighScore), compare_scores);
int final_count= (count > 5) ? 5 : count;
return persist_write_scores(filename, scores, final_count);
}
bool persist_load_config(const char* filename, GameConfig* config) {
if(config == NULL) return false;
config->board_width= PERSIST_CONFIG_DEFAULT_WIDTH;
config->board_height= PERSIST_CONFIG_DEFAULT_HEIGHT;
config->tick_rate_ms= PERSIST_CONFIG_DEFAULT_TICK_MS;
config->render_glyphs= 0;
config->screen_width= PERSIST_CONFIG_DEFAULT_SCREEN_WIDTH;
config->screen_height= PERSIST_CONFIG_DEFAULT_SCREEN_HEIGHT;
config->enable_external_3d_view= PERSIST_CONFIG_DEFAULT_ENABLE_EXTERNAL_3D_VIEW;
config->seed= (uint32_t)PERSIST_CONFIG_DEFAULT_SEED;
config->fov_degrees= (float)PERSIST_CONFIG_DEFAULT_FOV_DEGREES;
config->show_sprite_debug= PERSIST_CONFIG_DEFAULT_SHOW_SPRITE_DEBUG;
config->active_player= PERSIST_CONFIG_DEFAULT_ACTIVE_PLAYER;
config->num_players= PERSIST_CONFIG_DEFAULT_NUM_PLAYERS;
snprintf(config->player_name, PERSIST_PLAYER_NAME_MAX, "Player");
config->max_players= PERSIST_CONFIG_DEFAULT_MAX_PLAYERS;
config->max_length= PERSIST_CONFIG_DEFAULT_MAX_LENGTH;
config->max_food= PERSIST_CONFIG_DEFAULT_MAX_FOOD;
config->wall_height_scale= (float)PERSIST_CONFIG_DEFAULT_WALL_SCALE;
config->tail_height_scale= (float)PERSIST_CONFIG_DEFAULT_TAIL_SCALE;
snprintf(config->wall_texture, PERSIST_TEXTURE_PATH_MAX, "%s", PERSIST_CONFIG_DEFAULT_WALL_TEXTURE);
snprintf(config->floor_texture, PERSIST_TEXTURE_PATH_MAX, "%s", PERSIST_CONFIG_DEFAULT_FLOOR_TEXTURE);
config->key_up= PERSIST_CONFIG_DEFAULT_KEY_UP;
config->key_down= PERSIST_CONFIG_DEFAULT_KEY_DOWN;
config->key_left= PERSIST_CONFIG_DEFAULT_KEY_LEFT;
config->key_right= PERSIST_CONFIG_DEFAULT_KEY_RIGHT;
config->key_quit= PERSIST_CONFIG_DEFAULT_KEY_QUIT;
config->key_restart= PERSIST_CONFIG_DEFAULT_KEY_RESTART;
config->key_pause= PERSIST_CONFIG_DEFAULT_KEY_PAUSE;
if(filename == NULL) return false;
FILE* fp= fopen(filename, "r");
if(fp == NULL) return false;
bool file_exists= true;
char buffer[PERSIST_CONFIG_BUFFER];
while(fgets(buffer, (int)sizeof(buffer), fp) != NULL) {
size_t len= strlen(buffer);
if(len > 0 && buffer[len - 1] == '\n') {
buffer[len - 1]= '\0';
len--;
}
if(len == 0 || buffer[0] == '#') continue;
char* eq_pos= strchr(buffer, '=');
if(eq_pos == NULL) continue;
*eq_pos= '\0';
char* key= trim_in_place(buffer);
char* value= trim_in_place(eq_pos + 1);
char* endptr= NULL;
if(key == NULL || value == NULL || *key == '\0' || *value == '\0') continue;
if(strcmp(key, "render_glyphs") == 0 || strcmp(key, "glyphs") == 0 || strcmp(key, "charset") == 0) {
if(isdigit((unsigned char)value[0])) {
char* endptr2= NULL;
errno= 0;
long v= strtol(value, &endptr2, 10);
if(errno != 0 || endptr2 == value) continue;
config->render_glyphs= clamp_int((int)v, 0, 1);
} else {
for(char* p= value; *p; p++) *p= (char)tolower((unsigned char)*p);
if(strcmp(value, "utf8") == 0 || strcmp(value, "unicode") == 0 || strcmp(value, "box") == 0)
config->render_glyphs= 0;
else if(strcmp(value, "ascii") == 0 || strcmp(value, "legacy") == 0)
config->render_glyphs= 1;
}
continue;
}
if(strcmp(key, "enable_external_3d_view") == 0) {
for(char* p= value; *p; p++) *p= (char)tolower((unsigned char)*p);
if(strcmp(value, "true") == 0 || strcmp(value, "yes") == 0 || strcmp(value, "1") == 0)
config->enable_external_3d_view= 1;
else if(strcmp(value, "false") == 0 || strcmp(value, "no") == 0 || strcmp(value, "0") == 0)
config->enable_external_3d_view= 0;
continue;
} else if(strcmp(key, "seed") == 0) {
errno= 0;
unsigned long v= strtoul(value, &endptr, 10);
if(errno != 0 || endptr == value) continue;
config->seed= (uint32_t)v;
continue;
} else if(strcmp(key, "fov_degrees") == 0) {
char* endptr2= NULL;
errno= 0;
double dv= strtod(value, &endptr2);
if(errno != 0 || endptr2 == value) continue;
config->fov_degrees= (float)dv;
continue;
} else if(strcmp(key, "player_name") == 0) {
snprintf(config->player_name, PERSIST_PLAYER_NAME_MAX, "%s", value);
continue;
} else if(strcmp(key, "wall_height_scale") == 0) {
char* endptr2= NULL;
errno= 0;
double dv= strtod(value, &endptr2);
if(errno != 0 || endptr2 == value) continue;
config->wall_height_scale= (float)dv;
continue;
} else if(strcmp(key, "tail_height_scale") == 0) {
char* endptr2= NULL;
errno= 0;
double dv= strtod(value, &endptr2);
if(errno != 0 || endptr2 == value) continue;
config->tail_height_scale= (float)dv;
continue;
} else if(strcmp(key, "wall_texture") == 0) {
snprintf(config->wall_texture, PERSIST_TEXTURE_PATH_MAX, "%s", value);
continue;
} else if(strcmp(key, "floor_texture") == 0) {
snprintf(config->floor_texture, PERSIST_TEXTURE_PATH_MAX, "%s", value);
continue;
} else if(strcmp(key, "key_up") == 0 || strcmp(key, "key_down") == 0 || strcmp(key, "key_left") == 0 || strcmp(key, "key_right") == 0 || strcmp(key, "key_quit") == 0 || strcmp(key, "key_restart") == 0 || strcmp(key, "key_pause") == 0) {
char c= value[0];
if(strcmp(key, "key_up") == 0)
config->key_up= c;
else if(strcmp(key, "key_down") == 0)
config->key_down= c;
else if(strcmp(key, "key_left") == 0)
config->key_left= c;
else if(strcmp(key, "key_right") == 0)
config->key_right= c;
else if(strcmp(key, "key_quit") == 0)
config->key_quit= c;
else if(strcmp(key, "key_restart") == 0)
config->key_restart= c;
else if(strcmp(key, "key_pause") == 0)
config->key_pause= c;
continue;
} else if(strcmp(key, "show_sprite_debug") == 0) {
for(char* p= value; *p; p++) *p= (char)tolower((unsigned char)*p);
int b= -1;
if(strcmp(value, "true") == 0 || strcmp(value, "yes") == 0 || strcmp(value, "1") == 0)
b= 1;
else if(strcmp(value, "false") == 0 || strcmp(value, "no") == 0 || strcmp(value, "0") == 0)
b= 0;
if(b == -1) continue;
config->show_sprite_debug= b;
continue;
} else if(strcmp(key, "active_player") == 0) {
errno= 0;
long v= strtol(value, &endptr, 10);
if(errno != 0 || endptr == value) continue;
config->active_player= (int)v;
continue;
} else if(strcmp(key, "num_players") == 0) {
errno= 0;
long v= strtol(value, &endptr, 10);
if(errno != 0 || endptr == value) continue;
config->num_players= (int)v;
continue;
} else if(strcmp(key, "max_players") == 0) {
errno= 0;
long v= strtol(value, &endptr, 10);
if(errno != 0 || endptr == value) continue;
config->max_players= (int)v;
continue;
} else if(strcmp(key, "max_length") == 0) {
errno= 0;
long v= strtol(value, &endptr, 10);
if(errno != 0 || endptr == value) continue;
config->max_length= (int)v;
continue;
} else if(strcmp(key, "max_food") == 0) {
errno= 0;
long v= strtol(value, &endptr, 10);
if(errno != 0 || endptr == value) continue;
config->max_food= (int)v;
continue;
}
errno= 0;
long parsed_value= strtol(value, &endptr, 10);
if(errno != 0 || endptr == value) continue;
if(parsed_value < (long)INT_MIN || parsed_value > (long)INT_MAX) continue;
if(strcmp(key, "board_width") == 0)
config->board_width= clamp_int((int)parsed_value, 20, 100);
else if(strcmp(key, "board_height") == 0)
config->board_height= clamp_int((int)parsed_value, 10, 50);
else if(strcmp(key, "tick_rate_ms") == 0)
config->tick_rate_ms= clamp_int((int)parsed_value, 10, 1000);
else if(strcmp(key, "screen_width") == 0 || strcmp(key, "min_screen_width") == 0)
config->screen_width= clamp_int((int)parsed_value, 20, 4096);
else if(strcmp(key, "screen_height") == 0 || strcmp(key, "min_screen_height") == 0)
config->screen_height= clamp_int((int)parsed_value, 10, 2160);
}
fclose(fp);
return file_exists;
}
bool persist_write_config(const char* filename, const GameConfig* config) {
if(filename == NULL || config == NULL) return false;
char temp_filename[PERSIST_TEMP_FILENAME_MAX];
if(snprintf(temp_filename, sizeof(temp_filename), "%s.tmp", filename) < 0) return false;
FILE* fp= fopen(temp_filename, "w");
if(fp == NULL) return false;
if(fprintf(fp, "# Snake Game Configuration\n") < 0) goto write_fail;
if(fprintf(fp, "board_width=%d\n", config->board_width) < 0) goto write_fail;
if(fprintf(fp, "board_height=%d\n", config->board_height) < 0) goto write_fail;
if(fprintf(fp, "tick_rate_ms=%d\n", config->tick_rate_ms) < 0) goto write_fail;
if(fprintf(fp, "render_glyphs=%s\n", (config->render_glyphs == 1) ? "ascii" : "utf8") < 0) goto write_fail;
if(fprintf(fp, "screen_width=%d\n", config->screen_width) < 0) goto write_fail;
if(fprintf(fp, "screen_height=%d\n", config->screen_height) < 0) goto write_fail;
if(fprintf(fp, "enable_external_3d_view=%s\n", (config->enable_external_3d_view ? "true" : "false")) < 0) goto write_fail;
if(fprintf(fp, "seed=%u\n", (unsigned int)config->seed) < 0) goto write_fail;
if(fprintf(fp, "fov_degrees=%.2f\n", config->fov_degrees) < 0) goto write_fail;
if(fprintf(fp, "show_sprite_debug=%s\n", config->show_sprite_debug ? "true" : "false") < 0) goto write_fail;
if(fprintf(fp, "active_player=%d\n", config->active_player) < 0) goto write_fail;
if(fprintf(fp, "num_players=%d\n", config->num_players) < 0) goto write_fail;
if(fprintf(fp, "player_name=%s\n", config->player_name) < 0) goto write_fail;
if(fprintf(fp, "max_players=%d\n", config->max_players) < 0) goto write_fail;
if(fprintf(fp, "max_length=%d\n", config->max_length) < 0) goto write_fail;
if(fprintf(fp, "max_food=%d\n", config->max_food) < 0) goto write_fail;
if(fprintf(fp, "wall_height_scale=%.2f\n", config->wall_height_scale) < 0) goto write_fail;
if(fprintf(fp, "wall_texture=%s\n", config->wall_texture) < 0) goto write_fail;
if(fprintf(fp, "floor_texture=%s\n", config->floor_texture) < 0) goto write_fail;
if(fprintf(fp, "key_up=%c\n", config->key_up) < 0) goto write_fail;
if(fprintf(fp, "key_down=%c\n", config->key_down) < 0) goto write_fail;
if(fprintf(fp, "key_left=%c\n", config->key_left) < 0) goto write_fail;
if(fprintf(fp, "key_right=%c\n", config->key_right) < 0) goto write_fail;
if(fprintf(fp, "key_quit=%c\n", config->key_quit) < 0) goto write_fail;
if(fprintf(fp, "key_restart=%c\n", config->key_restart) < 0) goto write_fail;
if(fprintf(fp, "key_pause=%c\n", config->key_pause) < 0) goto write_fail;
if(fflush(fp) != 0) {
fclose(fp);
(void)unlink(temp_filename);
return false;
}
if(fclose(fp) != 0) {
(void)unlink(temp_filename);
return false;
}
{
struct stat st_new;
struct stat st_old;
bool identical= false;
if(stat(temp_filename, &st_new) == 0 && stat(filename, &st_old) == 0) {
if(st_new.st_size == st_old.st_size) {
FILE* fnew= fopen(temp_filename, "rb");
FILE* fold= fopen(filename, "rb");
if(fnew && fold) {
identical= true;
size_t bufsize= 4096;
char* b1= malloc(bufsize);
char* b2= malloc(bufsize);
if(!b1 || !b2) {
identical= false;
} else {
size_t r1, r2;
do {
r1= fread(b1, 1, bufsize, fnew);
r2= fread(b2, 1, bufsize, fold);
if(r1 != r2 || (r1 > 0 && memcmp(b1, b2, r1) != 0)) {
identical= false;
break;
}
} while(r1 > 0 && r2 > 0);
}
free(b1);
free(b2);
}
if(fnew) fclose(fnew);
if(fold) fclose(fold);
}
}
if(identical) {
(void)unlink(temp_filename);
return true;
}
if(rename(temp_filename, filename) != 0) {
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
