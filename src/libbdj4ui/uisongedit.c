/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
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

  logProcBegin ();

  uisongedit = mdmalloc (sizeof (uisongedit_t));

  uisongedit->conn = conn;
  uisongedit->dispsel = dispsel;
  uisongedit->musicdb = musicdb;
  uisongedit->options = options;
  uisongedit->savecb = NULL;
  uisongedit->applyadjcb = NULL;
  uisongedit->statusMsg = NULL;

  uisongeditUIInit (uisongedit);

  logProcEnd ("");
  return uisongedit;
}

void
uisongeditFree (uisongedit_t *uisongedit)
{
  logProcBegin ();

  if (uisongedit != NULL) {
    uisongeditUIFree (uisongedit);
    mdfree (uisongedit);
  }

  logProcEnd ("");
}

void
uisongeditSetDatabase (uisongedit_t *uisongedit, musicdb_t *musicdb)
{
  if (uisongedit == NULL || musicdb == NULL) {
    return;
  }

  uisongedit->musicdb = musicdb;
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

  if (msg == MSG_BPM_SET) {
    logMsg (LOG_DBG, LOG_MSGS, "uisongedit: rcvd: from:%d/%s route:%d/%s msg:%d/%s args:%s",
        routefrom, msgRouteDebugText (routefrom),
        route, msgRouteDebugText (route), msg, msgDebugText (msg), args);
  }

  switch (route) {
    case ROUTE_NONE:
    case ROUTE_MANAGEUI: {
      switch (msg) {
        case MSG_BPM_SET: {
          uisongeditSetBPMValue (uisongedit, atoi (args));
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

  return 0;
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

