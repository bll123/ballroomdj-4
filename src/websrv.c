#include "config.h"
#include "configssl.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "mongoose.h"

#include "websrv.h"

websrv_t *
websrvInit (uint16_t listenPort, mg_event_handler_t eventHandler,
    bool *done, void *userdata)
{
  websrv_t        *websrv;
  char            tbuff [100];

  websrv = malloc (sizeof (websrv_t));
  assert (websrv != NULL);

  mg_mgr_init (&websrv->mgr);
  websrv->mgr.userdata = userdata;
//  mg_log_set ("4");

  snprintf (tbuff, sizeof (tbuff), "http://0.0.0.0:%d", listenPort);
  mg_http_listen (&websrv->mgr, tbuff, eventHandler, &userdata);
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
  mg_mgr_poll (&websrv->mgr, 100);
}
