#pragma once
#include <stdint.h>

typedef struct mpclient mpclient;

mpclient* mpclient_create(const char* host, uint16_t port, const char* identifier);
void mpclient_destroy(mpclient* c);

/* Connect to server and start receiver thread. Returns 0 on success. */
int mpclient_connect_and_start(mpclient* c);

/* Attempt to join first public session; if none exists, host one. Uses name as display name. Returns 0 on success. */
int mpclient_auto_join_or_host(mpclient* c, const char* name);

/* Join a specific session id (e.g. from config). Returns 0 on success. */
int mpclient_join(mpclient* c, const char* sessionId, const char* name);

/* Returns non-zero if client has an active session id assigned (host or joined). */
int mpclient_has_session(mpclient* c);
/* Returns non-zero if client is currently hosting a session. */
int mpclient_is_host(mpclient* c);

/* Retrieve current session id into out (null-terminated). Returns 1 if set, 0 if none. */
int mpclient_get_session(mpclient* c, char* out, int maxlen);
/* Send arbitrary JSON string as "data" in a game message. Returns 0 on success. */
int mpclient_send_game(mpclient* c, const char* data_json);

/* Non-blocking poll for next received game message. If a message is present, copies up to maxlen bytes into out (null-terminated) and returns 1. Returns 0 if no message available. */
int mpclient_poll_message(mpclient* c, char* out, int maxlen);

/* Returns non-zero if client has an active session id assigned (host or joined). */
int mpclient_has_session(mpclient* c);

/* Close socket and stop threads */
void mpclient_stop(mpclient* c);
