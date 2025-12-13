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

static int clamp_int(int v, int lo, int hi) {
    if (v < lo) { return lo; }
    if (v > hi) { return hi; }
    return v;
}

/* ========== High Score Functions ========== */

int persist_read_scores(const char* filename, HighScore* scores, int max_count) {
    if (filename == NULL || scores == NULL || max_count <= 0) { return 0; }

    FILE* fp = fopen(filename, "r");
    if (fp == NULL) { return 0; /* File doesn't exist, return 0 scores */ }

    int count = 0;
    char buffer[PERSIST_SCORE_BUFFER];

    while (count < max_count && fgets(buffer, (int)sizeof(buffer), fp) != NULL) {
        /* Strip newline */
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
            len--;
        }

        /* Skip empty lines */
        if (len == 0) { continue; }

        /* Parse "NAME SCORE" format */
        char* space_pos = strchr(buffer, ' ');
        if (space_pos == NULL) {
            /* Invalid format, skip this line */
            continue;
        }

        size_t name_len = (size_t)(space_pos - buffer);
        if (name_len == 0 || name_len >= (size_t)PERSIST_NAME_MAX) {
            /* Invalid name length, skip */
            continue;
        }

        /* Copy name */
        memcpy(scores[count].name, buffer, name_len);
        scores[count].name[name_len] = '\0';

        /* Parse score */
        char* score_str = space_pos + 1;
        char* endptr = NULL;
        errno = 0;
        long score_long = strtol(score_str, &endptr, 10);

        if (errno != 0 || endptr == score_str || score_long < 0 || score_long > INT_MAX) {
            /* Invalid score, skip */
            continue;
        }

        scores[count].score = (int)score_long;
        count++;
    }

    fclose(fp);
    return count;
}

bool persist_write_scores(const char* filename, const HighScore* scores, int count) {
    if (filename == NULL || scores == NULL || count < 0 || count > PERSIST_MAX_SCORES) { return false; }

    /* Create temp filename */
    char temp_filename[PERSIST_TEMP_FILENAME_MAX];
    if (snprintf(temp_filename, sizeof(temp_filename), "%s.tmp", filename) < 0) { return false; }

    /* Write to temp file */
    FILE* fp = fopen(temp_filename, "w");
    if (fp == NULL) { return false; }

    for (int i = 0; i < count; i++) {
        if (fprintf(fp, "%s %d\n", scores[i].name, scores[i].score) < 0) {
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

    /* Atomically rename temp file to target */
    if (rename(temp_filename, filename) != 0) {
        (void)unlink(temp_filename);
        return false;
    }

    return true;
}

static int compare_scores(const void* a, const void* b) {
    const HighScore* score_a = (const HighScore*)a;
    const HighScore* score_b = (const HighScore*)b;
    /* Sort descending by score */
    if (score_a->score > score_b->score) { return -1; }
    if (score_a->score < score_b->score) { return 1; }
    return 0;
}

bool persist_append_score(const char* filename, const char* name, int score) {
    if (filename == NULL || name == NULL || score < 0) { return false; }

    /* Read existing scores */
    HighScore scores[PERSIST_MAX_SCORES];
    int count = persist_read_scores(filename, scores, PERSIST_MAX_SCORES);

    /* Check if this score value already exists (prevent duplicates) */
    for (int i = 0; i < count; i++) {
        if (scores[i].score == score) { return true; /* Already have this score, don't add */ }
    }

    /* Add new score if under limit */
    if (count < PERSIST_MAX_SCORES) {
        snprintf(scores[count].name, PERSIST_NAME_MAX, "%s", name);
        scores[count].score = score;
        count++;
    } else {
        /* Replace lowest score if new score is higher */
        qsort(scores, (size_t)count, sizeof(HighScore), compare_scores);
        if (score > scores[count - 1].score) {
            snprintf(scores[count - 1].name, PERSIST_NAME_MAX, "%s", name);
            scores[count - 1].score = score;
        } else {
            return false; /* Score not high enough to make the list */
        }
    }

    /* Sort by score descending */
    qsort(scores, (size_t)count, sizeof(HighScore), compare_scores);

    /* Keep only top 5 scores */
    int final_count = (count > 5) ? 5 : count;

    /* Write back top scores */
    return persist_write_scores(filename, scores, final_count);
}

bool persist_load_config(const char* filename, GameConfig* config) {
    if (config == NULL) { return false; }

    /* Apply defaults first */
    config->board_width = PERSIST_CONFIG_DEFAULT_WIDTH;
    config->board_height = PERSIST_CONFIG_DEFAULT_HEIGHT;
    config->tick_rate_ms = PERSIST_CONFIG_DEFAULT_TICK_MS;
    config->min_screen_width = PERSIST_CONFIG_DEFAULT_MIN_SCREEN_WIDTH;
    config->min_screen_height = PERSIST_CONFIG_DEFAULT_MIN_SCREEN_HEIGHT;

    if (filename == NULL) { return false; }

    FILE* fp = fopen(filename, "r");
    if (fp == NULL) { return false; /* File doesn't exist, defaults already applied */ }

    bool file_exists = true;
    char buffer[PERSIST_CONFIG_BUFFER];

    while (fgets(buffer, (int)sizeof(buffer), fp) != NULL) {
        /* Strip newline */
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
            len--;
        }

        /* Skip empty lines and comments */
        if (len == 0 || buffer[0] == '#') { continue; }

        /* Parse "KEY=VALUE" format */
        char* eq_pos = strchr(buffer, '=');
        if (eq_pos == NULL) { continue; }

        *eq_pos = '\0';
        const char* key = buffer;
        const char* value = eq_pos + 1;

        char* endptr = NULL;
        errno = 0;
        long parsed_value = strtol(value, &endptr, 10);

        if (errno != 0 || endptr == value) { continue; }
        if (parsed_value < (long)INT_MIN || parsed_value > (long)INT_MAX) { continue; }

        /* Validate and apply config values (clamped) */
        if (strcmp(key, "board_width") == 0) {
            config->board_width = clamp_int((int)parsed_value, 20, 100);
        } else if (strcmp(key, "board_height") == 0) {
            config->board_height = clamp_int((int)parsed_value, 10, 50);
        } else if (strcmp(key, "tick_rate_ms") == 0) {
            config->tick_rate_ms = clamp_int((int)parsed_value, 10, 1000);
        } else if (strcmp(key, "min_screen_width") == 0) {
            config->min_screen_width = clamp_int((int)parsed_value, 20, 400);
        } else if (strcmp(key, "min_screen_height") == 0) {
            config->min_screen_height = clamp_int((int)parsed_value, 10, 200);
        }
    }

    fclose(fp);
    return file_exists;
}

bool persist_write_config(const char* filename, const GameConfig* config) {
    if (filename == NULL || config == NULL) { return false; }

    /* Create temp filename */
    char temp_filename[PERSIST_TEMP_FILENAME_MAX];
    if (snprintf(temp_filename, sizeof(temp_filename), "%s.tmp", filename) < 0) { return false; }

    /* Write to temp file */
    FILE* fp = fopen(temp_filename, "w");
    if (fp == NULL) { return false; }

    if (fprintf(fp, "# Snake Game Configuration\n") < 0) { goto write_fail; }
    if (fprintf(fp, "board_width=%d\n", config->board_width) < 0) { goto write_fail; }
    if (fprintf(fp, "board_height=%d\n", config->board_height) < 0) { goto write_fail; }
    if (fprintf(fp, "tick_rate_ms=%d\n", config->tick_rate_ms) < 0) { goto write_fail; }
    if (fprintf(fp, "min_screen_width=%d\n", config->min_screen_width) < 0) { goto write_fail; }
    if (fprintf(fp, "min_screen_height=%d\n", config->min_screen_height) < 0) { goto write_fail; }

    if (fflush(fp) != 0) {
        fclose(fp);
        (void)unlink(temp_filename);
        return false;
    }

    if (fclose(fp) != 0) {
        (void)unlink(temp_filename);
        return false;
    }

    /* Atomically rename temp file to target */
    if (rename(temp_filename, filename) != 0) {
        (void)unlink(temp_filename);
        return false;
    }

    return true;

write_fail:
    fclose(fp);
    (void)unlink(temp_filename);
    return false;
}
