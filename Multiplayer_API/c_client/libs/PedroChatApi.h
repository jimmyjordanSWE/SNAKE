#ifndef PEDROCHAT_API_H
#define PEDROCHAT_API_H

#include <stdint.h>
#include "jansson/jansson.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PedroChatApi PedroChatApi;


typedef void (*PedroChatListener)(
    const char *event,
    int64_t messageId,
    const char *clientId,
    json_t *data,
    void *context
);


enum {
    MP_API_OK = 0,
    MP_API_ERR_ARGUMENT = 1,
    MP_API_ERR_STATE = 2,
    MP_API_ERR_CONNECT = 3,
    MP_API_ERR_PROTOCOL = 4,
    MP_API_ERR_IO = 5,
    MP_API_ERR_REJECTED = 6
};


PedroChatApi *mp_api_create(const char *server_host, uint16_t server_port, const char *identifier);


void mp_api_destroy(PedroChatApi *api);


int mp_api_host(PedroChatApi *api,
				json_t *data,
                char **out_session,
                char **out_clientId,
                json_t **out_data);


int mp_api_list(PedroChatApi *api,
                  json_t **out_list);


int mp_api_join(PedroChatApi *api,
                const char *sessionId,
                json_t *data,
                char **out_session,
                char **out_clientId,
                json_t **out_data);


int mp_api_game(PedroChatApi *api, json_t *data, const char* destination);


int mp_api_listen(PedroChatApi *api,
                  PedroChatListener cb,
                  void *context);


void mp_api_unlisten(PedroChatApi *api, int listener_id);

#ifdef __cplusplus
}
#endif

#endif
