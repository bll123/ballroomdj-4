/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_WEBCLIENT_H
#define INC_WEBCLIENT_H

#include <stdbool.h>
#include <curl/curl.h>

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef void (*webclientcb_t)(void *userdata, const char *resp, size_t len);

typedef struct webclient webclient_t;

enum {
  WEB_OK = 200,
  WEB_BAD_REQUEST = 400,
  WEB_RESP_SZ = 512 * 1024,
};

webclient_t * webclientAlloc (void *userdata, webclientcb_t cb);
int   webclientGet (webclient_t *webclient, const char *uri);
int   webclientPost (webclient_t *webclient, const char *uri, const char *query);
int   webclientPostCompressed (webclient_t *webclient, const char *uri, const char *query);
int   webclientDownload (webclient_t *webclient, const char *uri, const char *outfile);
int   webclientUploadFile (webclient_t *webclient, const char *uri, const char *query [], const char *fn, const char *fnname);
void  webclientClose (webclient_t *webclient);
void  webclientSetTimeout (webclient_t *webclient, long timeout);
void  webclientSpoofUserAgent (webclient_t *webclient);
void  webclientCompressFile (const char *infn, const char *outfn);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_WEBCLIENT_H */
