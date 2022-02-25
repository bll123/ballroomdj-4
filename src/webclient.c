#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include <curl/curl.h>

#include "log.h"
#include "sysvars.h"
#include "webclient.h"

static size_t webclientCallback (char *ptr, size_t size,
    size_t nmemb, void *userdata);

webclient_t *
webclientPost (webclient_t *webclient, char *uri, char *query,
    void *userdata, webclientcb_t callback)
{
  if (webclient == NULL) {
    char    tbuff [200];

    webclient = malloc (sizeof (webclient_t));
    assert (webclient != NULL);
    webclient->userdata = userdata;
    webclient->callback = callback;

    curl_global_init (CURL_GLOBAL_ALL);
    webclient->curl = curl_easy_init ();
    assert (webclient->curl != NULL);
#if 1
    if (logCheck (LOG_DBG, LOG_WEBCLIENT)) {
        /* can use CURLOPT_DEBUGFUNCTION */
      curl_easy_setopt (webclient->curl, CURLOPT_VERBOSE, 1);
    }
#endif
    curl_easy_setopt (webclient->curl, CURLOPT_TCP_KEEPALIVE, 1);
    curl_easy_setopt (webclient->curl, CURLOPT_WRITEDATA, webclient);
    curl_easy_setopt (webclient->curl, CURLOPT_WRITEFUNCTION, webclientCallback);
    snprintf (tbuff, sizeof (tbuff),
        "BallroomDJ/%s ( https://ballroomdj.org/ )", sysvarsGetStr (SV_BDJ4_VERSION));
    curl_easy_setopt (webclient->curl, CURLOPT_USERAGENT, tbuff);
  }

  curl_easy_setopt (webclient->curl, CURLOPT_URL, uri);
  curl_easy_setopt (webclient->curl, CURLOPT_POSTFIELDS, query);
  curl_easy_perform (webclient->curl);
  return webclient;
}

void
webclientClose (webclient_t *webclient)
{
  if (webclient != NULL) {
    if (webclient->curl != NULL) {
      curl_easy_cleanup (webclient->curl);
    }
    curl_global_cleanup ();
    free (webclient);
  }
}

static size_t
webclientCallback (char *ptr, size_t size, size_t nmemb, void *userdata)
{
  webclient_t     *webclient = userdata;

  webclient->callback (webclient->userdata, ptr, nmemb);
  return nmemb;
}
