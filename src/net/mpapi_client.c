#include "mpapi_client.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <ctype.h>
#include "platform.h"

#define MSG_QUEUE_SIZE 64
#define LINE_BUF_CAP 4096

struct mpclient {
    char server_host[128];
    uint16_t server_port;
    char identifier[37];
    int sockfd;
    pthread_t recv_thread;
    int running;
    pthread_mutex_t lock;
    /* simple ring buffer of strings (malloc'd) */
    char* msgs[MSG_QUEUE_SIZE];
    int head;
    int tail;
    /* session id assigned after host/join */
    char session[16];
};

static int connect_to_server(const char* host, uint16_t port) {
    if (!host) host = "127.0.0.1";
    char port_str[16];
    snprintf(port_str, sizeof(port_str), "%u", (unsigned int)port);
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    struct addrinfo* res = NULL;
    int err = getaddrinfo(host, port_str, &hints, &res);
    if (err != 0) return -1;
    int fd = -1;
    for (struct addrinfo* rp = res; rp != NULL; rp = rp->ai_next) {
        fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (fd == -1) continue;
        if (connect(fd, rp->ai_addr, rp->ai_addrlen) == 0) break;
        close(fd);
        fd = -1;
    }
    freeaddrinfo(res);
    return fd;
}

#include "net_log.h"

#include "console.h"

static int send_all(int fd, const char* buf, size_t len) {
    size_t sent = 0;
    while (sent < len) {
        ssize_t rc = send(fd, buf + sent, len - sent, 0);
        if (rc <= 0) return -1;
        /* log this chunk */
        net_log_send(fd, buf + sent, (size_t)rc, "mpclient send_all: chunk");
        sent += (size_t)rc;
    }
    return 0;
}

static void enqueue_msg(struct mpclient* c, const char* s) {
    if (!c || !s) return;
    pthread_mutex_lock(&c->lock);
    int next = (c->tail + 1) % MSG_QUEUE_SIZE;
    if (next == c->head) {
        /* full, drop oldest */
        free(c->msgs[c->head]);
        c->head = (c->head + 1) % MSG_QUEUE_SIZE;
    }
    c->msgs[c->tail] = strdup(s);
    c->tail = next;
    pthread_mutex_unlock(&c->lock);
}

int mpclient_poll_message(mpclient* c, char* out, int maxlen) {
    if (!c || !out || maxlen <= 0) return 0;
    pthread_mutex_lock(&c->lock);
    if (c->head == c->tail) {
        pthread_mutex_unlock(&c->lock);
        return 0;
    }
    char* s = c->msgs[c->head];
    c->msgs[c->head] = NULL;
    c->head = (c->head + 1) % MSG_QUEUE_SIZE;
    pthread_mutex_unlock(&c->lock);
    strncpy(out, s, (size_t)maxlen - 1);
    out[maxlen - 1] = '\0';
    free(s);
    return 1;
}

int mpclient_get_session(mpclient* c, char* out, int maxlen) {
    if (!c || !out || maxlen <= 0) return 0;
    pthread_mutex_lock(&c->lock);
    if (c->session[0]) {
        strncpy(out, c->session, (size_t)maxlen - 1);
        out[maxlen - 1] = '\0';
        pthread_mutex_unlock(&c->lock);
        return 1;
    }
    pthread_mutex_unlock(&c->lock);
    return 0;
}

int mpclient_has_session(mpclient* c) {
    if (!c) return 0;
    return c->session[0] != '\0';
}

static char* extract_json_field(const char* line, const char* key) {
    /* Find "key" and the following ':' then locate object or string */
    char needle[128];
    snprintf(needle, sizeof(needle), "\"%s\"", key);
    const char* p = strstr(line, needle);
    if (!p) return NULL;
    p = strchr(p, ':');
    if (!p) return NULL;
    p++;
    while (*p && isspace((unsigned char)*p)) p++;
    if (*p == '"') {
        p++;
        const char* start = p;
        while (*p && *p != '"') {
            if (*p == '\\' && p[1]) p += 2; else p++;
        }
        size_t len = (size_t)(p - start);
        char* out = (char*)malloc(len + 1);
        if (!out) return NULL;
        memcpy(out, start, len);
        out[len] = '\0';
        return out;
    } else if (*p == '{') {
        /* extract balanced object */
        const char* start = p;
        int depth = 0;
        while (*p) {
            if (*p == '{') depth++;
            else if (*p == '}') {
                depth--;
                if (depth == 0) { p++; break; }
            }
            p++;
        }
        size_t len = (size_t)(p - start);
        char* out = (char*)malloc(len + 1);
        if (!out) return NULL;
        memcpy(out, start, len);
        out[len] = '\0';
        return out;
    }
    return NULL;
}

static void process_line(struct mpclient* c, const char* line) {
    if (!c || !line) return;
    /* look for cmd */
    if (strstr(line, "\"cmd\":\"game\"") != NULL || strstr(line, "\"cmd\": \"game\"") != NULL) {
        char* data = extract_json_field(line, "data");
        if (data) {
            net_log_info("mpclient: recv cmd=game data_len=%zu", strlen(data));
            enqueue_msg(c, data);
            free(data);
        }
    } else if (strstr(line, "\"cmd\":\"host\"") != NULL || strstr(line, "\"cmd\": \"host\"") != NULL) {
        char* sid = extract_json_field(line, "session");
        if (sid) {
            snprintf(c->session, sizeof(c->session), "%s", sid);
            net_log_info("mpclient: recv cmd=host session=%s", sid);
            free(sid);
        }
    } else if (strstr(line, "\"cmd\":\"join\"") != NULL || strstr(line, "\"cmd\": \"join\"") != NULL) {
        char* sid = extract_json_field(line, "session");
        if (sid) {
            snprintf(c->session, sizeof(c->session), "%s", sid);
            net_log_info("mpclient: recv cmd=join session=%s", sid);
            free(sid);
        }
    } else if (strstr(line, "\"cmd\":\"list\"") != NULL || strstr(line, "\"cmd\": \"list\"") != NULL) {
        char* data = extract_json_field(line, "data");
        if (data) {
            net_log_info("mpclient: recv cmd=list data=%s", data);
            free(data);
        } else {
            net_log_info("mpclient: recv cmd=list (no data)");
        }
    }
}

static void* recv_thread_main(void* arg) {
    struct mpclient* c = (struct mpclient*)arg;
    if (!c) return NULL;
    char buf[LINE_BUF_CAP];
    size_t buf_len = 0;
    while (c->running) {
        char tmp[512];
        ssize_t rc = recv(c->sockfd, tmp, sizeof(tmp), 0);
        if (rc <= 0) {
            /* connection closed or error */
            net_log_info("mpclient: recv thread connection closed (fd=%d)", c->sockfd);
            break;
        }
        /* log the raw bytes received */
        net_log_recv(c->sockfd, tmp, (size_t)rc, "mpclient recv_thread: raw");
        for (ssize_t i = 0; i < rc; ++i) {
            char ch = tmp[i];
            if (ch == '\n') {
                buf[buf_len] = '\0';
                if (buf_len > 0) process_line(c, buf);
                buf_len = 0;
            } else if (buf_len + 1 < LINE_BUF_CAP) {
                buf[buf_len++] = ch;
            } else {
                /* overflow, reset */
                buf_len = 0;
            }
        }
    }
    c->running = 0;
    return NULL;
}

mpclient* mpclient_create(const char* host, uint16_t port, const char* identifier) {
    if (!host || !identifier) return NULL;
    struct mpclient* c = calloc(1, sizeof(*c));
    if (!c) return NULL;
    snprintf(c->server_host, sizeof(c->server_host), "%s", host);
    c->server_port = port;
    snprintf(c->identifier, sizeof(c->identifier), "%s", identifier);
    c->sockfd = -1;
    pthread_mutex_init(&c->lock, NULL);
    c->head = c->tail = 0;
    c->running = 0;
    c->session[0] = '\0';
    return c;
}

int mpclient_connect_and_start(mpclient* c) {
    if (!c) return -1;
    if (c->sockfd >= 0) return 0;
    net_log_info("mpclient: connecting to %s:%u..", c->server_host, (unsigned int)c->server_port);
    int fd = connect_to_server(c->server_host, c->server_port);
    if (fd < 0) {
        net_log_error("mpclient: connect failed to %s:%u", c->server_host, (unsigned int)c->server_port);
        return -1;
    }
    c->sockfd = fd;
    net_log_info("mpclient: connect success fd=%d", fd);
    c->running = 1;
    if (pthread_create(&c->recv_thread, NULL, recv_thread_main, c) != 0) {
        c->running = 0;
        console_warn("mpclient: failed to start recv thread");
        close(c->sockfd);
        c->sockfd = -1;
        return -1;
    }
    return 0;
}

static int send_command(struct mpclient* c, const char* cmd, const char* session, const char* data_json) {
    if (!c || !cmd) return -1;
    char msg[2048];
    if (session && session[0]) {
        if (data_json)
            snprintf(msg, sizeof(msg), "{\"identifier\":\"%s\",\"session\":\"%s\",\"cmd\":\"%s\",\"data\":%s}\n", c->identifier, session, cmd, data_json);
        else
            snprintf(msg, sizeof(msg), "{\"identifier\":\"%s\",\"session\":\"%s\",\"cmd\":\"%s\",\"data\":{}}\n", c->identifier, session, cmd);
    } else {
        if (data_json)
            snprintf(msg, sizeof(msg), "{\"identifier\":\"%s\",\"cmd\":\"%s\",\"data\":%s}\n", c->identifier, cmd, data_json);
        else
            snprintf(msg, sizeof(msg), "{\"identifier\":\"%s\",\"cmd\":\"%s\",\"data\":{}}\n", c->identifier, cmd);
    }
    /* Log the outgoing command at a higher level (written to log only) */
    net_log_info("mpclient: sending cmd=%s session=%s len=%zu", cmd, session ? session : "(none)", strlen(msg));
    net_log_send(c->sockfd, msg, strlen(msg), "mpclient send_command: full msg");
    return send_all(c->sockfd, msg, strlen(msg));
}

int mpclient_auto_join_or_host(mpclient* c, const char* name) {
    if (!c || !name) return -1;
    /* ensure connected */
    if (mpclient_connect_and_start(c) != 0) return -1;
    /* Send list */
    if (send_command(c, "list", NULL, "{\"type\":\"sessions\"}") != 0) return -1;
    /* Wait briefly for response */
    int waited = 0;
    while (waited < 2000) {
        pthread_mutex_lock(&c->lock);
        pthread_mutex_unlock(&c->lock);
        if (c->session[0]) break;
        platform_sleep_ms(100);
        waited += 100;
    }
    if (c->session[0]) {
        /* If session already set (unlikely), join it */
        char payload[256];
        snprintf(payload, sizeof(payload), "{\"name\":\"%s\"}", name);
        if (send_command(c, "join", c->session, payload) != 0) return -1;
        return 0;
    }
    /* No session yet; host a new session */
    char payload[256];
    snprintf(payload, sizeof(payload), "{\"name\":\"%s\",\"private\":false}", name);
    if (send_command(c, "host", NULL, payload) != 0) return -1;
    /* wait for host response and session id to be populated */
    waited = 0;
    while (waited < 2000) {
        if (c->session[0]) return 0;
        platform_sleep_ms(100);
        waited += 100;
    }
    return 0;
}

int mpclient_join(mpclient* c, const char* sessionId, const char* name) {
    if (!c || !sessionId) return -1;
    if (mpclient_connect_and_start(c) != 0) return -1;
    char payload[256];
    snprintf(payload, sizeof(payload), "{\"name\":\"%s\"}", name ? name : "");
    if (send_command(c, "join", sessionId, payload) != 0) return -1;
    int waited = 0;
    while (waited < 2000) {
        if (c->session[0]) return 0;
        platform_sleep_ms(100);
        waited += 100;
    }
    return 0;
}

int mpclient_send_game(mpclient* c, const char* data_json) {
    if (!c || !data_json) return -1;
    if (send_command(c, "game", c->session[0] ? c->session : NULL, data_json) != 0) return -1;
    return 0;
}

void mpclient_stop(mpclient* c) {
    if (!c) return;
    c->running = 0;
    if (c->sockfd >= 0) {
        shutdown(c->sockfd, SHUT_RDWR);
        close(c->sockfd);
        c->sockfd = -1;
    }
    pthread_join(c->recv_thread, NULL);
}

void mpclient_destroy(mpclient* c) {
    if (!c) return;
    mpclient_stop(c);
    pthread_mutex_destroy(&c->lock);
    /* free queued messages */
    for (int i = 0; i < MSG_QUEUE_SIZE; ++i) if (c->msgs[i]) free(c->msgs[i]);
    free(c);
}
