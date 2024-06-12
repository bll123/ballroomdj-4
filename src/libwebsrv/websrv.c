/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h> // for mongoose
#include <dirent.h> // for mongoose

#include "mongoose.h"

#include "log.h"
#include "mdebug.h"
#include "websrv.h"

static void websrvLog (char c, void *userdata);

websrv_t *
websrvInit (uint16_t listenPort, mg_event_handler_t eventHandler,
    void *userdata)
{
  websrv_t        *websrv;
  char            tbuff [100];

  websrv = mdmalloc (sizeof (websrv_t));

  mg_log_set_fn (websrvLog, NULL);
  if (logCheck (LOG_DBG, LOG_WEBSRV)) {
    mg_log_set (3);
  }

  mg_mgr_init (&websrv->mgr);
  websrv->mgr.userdata = userdata;

  snprintf (tbuff, sizeof (tbuff), "http://0.0.0.0:%" PRIu16, listenPort);
  mg_http_listen (&websrv->mgr, tbuff, eventHandler, userdata);
  return websrv;
}

void
websrvFree (websrv_t *websrv)
{
  if (websrv != NULL) {
    mg_mgr_free (&websrv->mgr);
    mdfree (websrv);
  }
}

void
websrvProcess (websrv_t *websrv)
{
  mg_mgr_poll (&websrv->mgr, 10);
}

static void
websrvLog (char c, void *userdata)
{
  static char   wlogbuff [2048];
  static size_t len = 0;

  if (c == '\n' || len > sizeof (wlogbuff)) {
    wlogbuff [len] = '\0';
    logMsg (LOG_DBG, LOG_WEBSRV, "%s", wlogbuff);
    len = 0;
    return;
  }

  wlogbuff [len++] = c;
  wlogbuff [len] = '\0';
}
