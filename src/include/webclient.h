/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include <stdbool.h>
#include <time.h>

#include <curl/curl.h>

#include "webresp.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef void (*webclientcb_t)(void *userdata, const char *resp, size_t len, time_t tm);

typedef struct webclient webclient_t;

enum {
  WEB_RESP_SZ = 512 * 1024,
};

webclient_t * webclientAlloc (void *userdata, webclientcb_t cb);
int   webclientHead (webclient_t *webclient, const char *uri);
int   webclientGet (webclient_t *webclient, const char *uri);
int   webclientPost (webclient_t *webclient, const char *uri, const char *query);
int   webclientPostCompressed (webclient_t *webclient, const char *uri, const char *query);
int   webclientDownload (webclient_t *webclient, const char *uri, const char *outfile);
int   webclientUploadFile (webclient_t *webclient, const char *uri, const char *query [], const char *fn, const char *fnname);
void  webclientClose (webclient_t *webclient);
void  webclientSetTimeout (webclient_t *webclient, long timeout);
void  webclientSpoofUserAgent (webclient_t *webclient);
void  webclientIgnoreCertErr (webclient_t *webclient);
void  webclientGetLocalIP (char *ip, size_t sz);
void  webclientCompressFile (const char *infn, const char *outfn);
void  webclientSetUserPass (webclient_t *webclient, const char *user, const char *pass);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

