#include "snake/persist.h"
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
config->render_mode= 1;
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
if(strcmp(key, "render_mode") == 0) {
for(char* p= value; *p; p++) *p= (char)tolower((unsigned char)*p);
if(strcmp(value, "2d") == 0 || strcmp(value, "console") == 0)
config->render_mode= 0;
else if(strcmp(value, "3d") == 0 || strcmp(value, "sdl") == 0)
config->render_mode= 1;
continue;
}
char* endptr= NULL;
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
config->screen_width= clamp_int((int)parsed_value, 20, 400);
else if(strcmp(key, "screen_height") == 0 || strcmp(key, "min_screen_height") == 0)
config->screen_height= clamp_int((int)parsed_value, 10, 200);
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
if(fprintf(fp, "render_mode=%s\n", (config->render_mode == 1) ? "3d" : "2d") < 0) goto write_fail;
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
write_fail:
fclose(fp);
(void)unlink(temp_filename);
return false;
}
