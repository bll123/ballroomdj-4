/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>
#include <math.h>

#include "bdj4.h"
#include "bdj4intl.h"
#include "bdjopt.h"
#include "log.h"
#include "mdebug.h"
#include "ui.h"
#include "callback.h"
#include "uisongedit.h"

uisongedit_t *
uisongeditInit (conn_t *conn, musicdb_t *musicdb,
    dispsel_t *dispsel, nlist_t *options)
{
  uisongedit_t  *uisongedit;

  logProcBegin (LOG_PROC, "uisongeditInit");

  uisongedit = mdmalloc (sizeof (uisongedit_t));
  assert (uisongedit != NULL);

  uisongedit->conn = conn;
  uisongedit->dispsel = dispsel;
  uisongedit->musicdb = musicdb;
  uisongedit->options = options;
  uisongedit->savecb = NULL;
  uisongedit->applyadjcb = NULL;
  uisongedit->statusMsg = NULL;

  uisongeditUIInit (uisongedit);

  logProcEnd (LOG_PROC, "uisongeditInit", "");
  return uisongedit;
}

void
uisongeditFree (uisongedit_t *uisongedit)
{
  logProcBegin (LOG_PROC, "uisongeditFree");

  if (uisongedit != NULL) {
    uisongeditUIFree (uisongedit);
    mdfree (uisongedit);
  }

  logProcEnd (LOG_PROC, "uisongeditFree", "");
}

void
uisongeditMainLoop (uisongedit_t *uisongedit)
{
  uisongeditUIMainLoop (uisongedit);
  return;
}

int
uisongeditProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
    bdjmsgmsg_t msg, char *args, void *udata)
{
  uisongedit_t  *uisongedit = udata;
  bool          dbgdisp = false;

  switch (route) {
    case ROUTE_NONE:
    case ROUTE_MANAGEUI: {
      switch (msg) {
        case MSG_BPM_SET: {
          uisongeditSetBPMValue (uisongedit, args);
          dbgdisp = true;
          break;
        }
        default: {
          break;
        }
      }
      break;
    }
    default: {
      break;
    }
  }

  if (dbgdisp) {
    logMsg (LOG_DBG, LOG_MSGS, "uisongedit: rcvd: from:%d/%s route:%d/%s msg:%d/%s args:%s",
        routefrom, msgRouteDebugText (routefrom),
        route, msgRouteDebugText (route), msg, msgDebugText (msg), args);
  }

  return 0;
}

void
uisongeditNewSelection (uisongedit_t *uisongedit, dbidx_t dbidx)
{
  song_t      *song;

  song = dbGetByIdx (uisongedit->musicdb, dbidx);
  uisongeditLoadData (uisongedit, song, dbidx, UISONGEDIT_ALL);
}

void
uisongeditSetSaveCallback (uisongedit_t *uisongedit, callback_t *uicb)
{
  if (uisongedit == NULL) {
    return;
  }

  uisongedit->savecb = uicb;
}

void
uisongeditSetApplyAdjCallback (uisongedit_t *uisongedit, callback_t *uicb)
{
  if (uisongedit == NULL) {
    return;
  }

  uisongedit->applyadjcb = uicb;
}

