#ifndef INC_WEBCLIENT_H
#define INC_WEBCLIENT_H

#include <stdbool.h>

#include <curl/curl.h>

typedef void (*webclientcb_t)(void *userdata, char *resp, size_t len);

typedef struct {
  void            *userdata;
  webclientcb_t   callback;
  CURL            *curl;
} webclient_t;

webclient_t *webclientPost (webclient_t *webclient, char *uri, char *query,
    void *userdata, webclientcb_t callback);
void        webclientClose (webclient_t *webclient);
char *      webclientGetLocalIP (void);

#endif /* INC_WEBCLIENT_H */
