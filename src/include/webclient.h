/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_WEBCLIENT_H
#define INC_WEBCLIENT_H

#include <stdbool.h>
#include <curl/curl.h>

typedef void (*webclientcb_t)(void *userdata, const char *resp, size_t len);

typedef struct webclient webclient_t;

#define WEB_RESP_SZ   (512*1024)

webclient_t * webclientAlloc (void *userdata, webclientcb_t cb);
void        webclientGet (webclient_t *webclient, const char *uri);
void        webclientPost (webclient_t *webclient, const char *uri, const char *query);
void        webclientDownload (webclient_t *webclient, const char *uri, const char *outfile);
void        webclientUploadFile (webclient_t *webclient, const char *uri, const char *query [], const char *fn, const char *fnname);
void        webclientClose (webclient_t *webclient);
char        * webclientGetLocalIP (void);
void        webclientCompressFile (const char *infn, const char *outfn);

#endif /* INC_WEBCLIENT_H */
