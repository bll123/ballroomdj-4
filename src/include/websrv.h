/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_WEBSRV_H
#define INC_WEBSRV_H

#include <stdint.h>

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef struct websrv websrv_t;
typedef void (*websrv_handler_t) (void *userdata, const char *query, const char *uri);

websrv_t *websrvInit (uint16_t listenPort, websrv_handler_t eventHandler, void *userdata);
void websrvFree (websrv_t *websrv);
void websrvProcess (websrv_t *websrv);
void websrvReply (websrv_t *websrv, int httpcode, const char *headers, const char *msg);
void websrvServeFile (websrv_t *websrv, const char *dir, const char *path);
void websrvGetUserPass (websrv_t *websrv, char *user, size_t usersz, char *pass, size_t passsz);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_WEBSRV_H */
