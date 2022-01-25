#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <sys/time.h> // for mongoose
#include <dirent.h> // for mongoose

#include "mongoose.h"

#include "log.h"
#include "websrv.h"

static void websrvLog (const void *msg, size_t len, void *userdata);

websrv_t *
websrvInit (uint16_t listenPort, mg_event_handler_t eventHandler,
    void *userdata)
{
  websrv_t        *websrv;
  char            tbuff [100];

  websrv = malloc (sizeof (websrv_t));
  assert (websrv != NULL);

  mg_log_set_callback (websrvLog, NULL);
  if (logCheck (LOG_DBG, LOG_WEBSRV)) {
    mg_log_set ("4");
  }

  mg_mgr_init (&websrv->mgr);
  websrv->mgr.userdata = userdata;

  snprintf (tbuff, sizeof (tbuff), "http://0.0.0.0:%d", listenPort);
  mg_http_listen (&websrv->mgr, tbuff, eventHandler, userdata);
  return websrv;
}

void
websrvFree (websrv_t *websrv)
{
  if (websrv != NULL) {
    mg_mgr_free (&websrv->mgr);
    free (websrv);
  }
}

void
websrvProcess (websrv_t *websrv)
{
  mg_mgr_poll (&websrv->mgr, 10);
}

static void
websrvLog (const void *tmsg, size_t len, void *userdata)
{
  const char  *msg = tmsg;
  logMsg (LOG_DBG, LOG_WEBSRV, "%.*s", len, msg);
}
