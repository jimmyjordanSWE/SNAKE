#pragma once
#include <stddef.h>

/* Sanitize a player name input buffer.
 * - `input` may be NULL or empty
 * - `out` must point to a buffer of size `out_len` (>=1)
 * Returns 1 on success (out set), 0 on failure.
 */
int render_sanitize_player_name(const char* input, char* out, size_t out_len);
