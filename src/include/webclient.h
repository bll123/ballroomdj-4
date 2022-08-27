#ifndef INC_WEBCLIENT_H
#define INC_WEBCLIENT_H

#include <stdbool.h>
#include <curl/curl.h>

typedef void (*webclientcb_t)(void *userdata, char *resp, size_t len);

typedef struct webclient webclient_t;

#define WEB_RESP_SZ   (512*1024)

webclient_t *webclientAlloc (void *userdata, webclientcb_t cb);
void        webclientGet (webclient_t *webclient, char *uri);
void        webclientPost (webclient_t *webclient, char *uri, char *query);
void        webclientDownload (webclient_t *webclient, char *uri, char *outfile);
void        webclientUploadFile (webclient_t *webclient, char *uri, const char *query [], char *fn);
void        webclientClose (webclient_t *webclient);
char *      webclientGetLocalIP (void);

#endif /* INC_WEBCLIENT_H */
