/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "conn.h"
#include "log.h"
#include "musicdb.h"
#include "playlist.h"
#include "tmutil.h"
#include "uimusicq.h"
#include "ui.h"

void
uimusicqQueueDanceProcess (uimusicq_t *uimusicq, nlistidx_t idx, int count)
{
  int           ci;
  char          tbuff [20];
  bdjmsgmsg_t   msg = MSG_QUEUE_DANCE;


  logProcBegin ();

  ci = uimusicq->musicqManageIdx;

  if (idx >= 0) {
    if (count > 1) {
      msg = MSG_QUEUE_DANCE_5;
    }
    snprintf (tbuff, sizeof (tbuff), "%d%c%d", ci, MSG_ARGS_RS, idx);
    connSendMessage (uimusicq->conn, ROUTE_MAIN, msg, tbuff);
  }
  logProcEnd ("");
}

void
uimusicqQueuePlaylistProcess (uimusicq_t *uimusicq, nlistidx_t idx)
{
  int           ci;
  char          tbuff [200];

  logProcBegin ();

  ci = uimusicq->musicqManageIdx;

  if (idx >= 0) {
    msgbuildQueuePlaylist (tbuff, sizeof (tbuff), ci,
        uiDropDownGetString (uimusicq->ui [ci].playlistsel), EDIT_FALSE);
    connSendMessage (uimusicq->conn, ROUTE_MAIN, MSG_QUEUE_PLAYLIST, tbuff);
  }
  logProcEnd ("");
}

void
uimusicqMoveTop (uimusicq_t *uimusicq, int mqidx, nlistidx_t idx)
{
  char        tbuff [40];

  uimusicq->changed = true;
  snprintf (tbuff, sizeof (tbuff), "%d%c%d", mqidx, MSG_ARGS_RS, idx);
  connSendMessage (uimusicq->conn, ROUTE_MAIN,
      MSG_MUSICQ_MOVE_TOP, tbuff);
}

void
uimusicqMoveUp (uimusicq_t *uimusicq, int mqidx, nlistidx_t idx)
{
  char        tbuff [40];

  uimusicq->changed = true;
  snprintf (tbuff, sizeof (tbuff), "%d%c%d", mqidx, MSG_ARGS_RS, idx);
  connSendMessage (uimusicq->conn, ROUTE_MAIN, MSG_MUSICQ_MOVE_UP, tbuff);
  logProcEnd ("");
}

void
uimusicqMoveDown (uimusicq_t *uimusicq, int mqidx, nlistidx_t idx)
{
  char        tbuff [40];

  uimusicq->changed = true;
  snprintf (tbuff, sizeof (tbuff), "%d%c%d", mqidx, MSG_ARGS_RS, idx);
  connSendMessage (uimusicq->conn, ROUTE_MAIN, MSG_MUSICQ_MOVE_DOWN, tbuff);
}

void
uimusicqTogglePause (uimusicq_t *uimusicq, int mqidx, nlistidx_t idx)
{
  char        tbuff [40];

  snprintf (tbuff, sizeof (tbuff), "%d%c%d", mqidx, MSG_ARGS_RS, idx);
  connSendMessage (uimusicq->conn, ROUTE_MAIN,
      MSG_MUSICQ_TOGGLE_PAUSE, tbuff);
}

void
uimusicqRemove (uimusicq_t *uimusicq, int mqidx, nlistidx_t idx)
{
  char        tbuff [40];

  uimusicq->changed = true;
  snprintf (tbuff, sizeof (tbuff), "%d%c%d", mqidx, MSG_ARGS_RS, idx);
  connSendMessage (uimusicq->conn, ROUTE_MAIN,
      MSG_MUSICQ_REMOVE, tbuff);
}

void
uimusicqSwap (uimusicq_t *uimusicq, int mqidx)
{
  char        tbuff [100];

  if (uimusicq->ui [mqidx].prevSelection < 0 ||
      uimusicq->ui [mqidx].currSelection < 0 ||
      uimusicq->ui [mqidx].prevSelection >= uimusicq->ui [mqidx].count ||
      uimusicq->ui [mqidx].currSelection >= uimusicq->ui [mqidx].count ||
      uimusicq->ui [mqidx].prevSelection == uimusicq->ui [mqidx].currSelection) {
    return;
  }

  uimusicq->changed = true;
  snprintf (tbuff, sizeof (tbuff), "%d%c%d%c%d", mqidx,
      MSG_ARGS_RS, uimusicq->ui [mqidx].prevSelection + 1,
      MSG_ARGS_RS, uimusicq->ui [mqidx].currSelection + 1);
  connSendMessage (uimusicq->conn, ROUTE_MAIN, MSG_MUSICQ_SWAP, tbuff);
}

void
uimusicqCreatePlaylistList (uimusicq_t *uimusicq)
{
  int               ci;
  slist_t           *plList;


  logProcBegin ();

  ci = uimusicq->musicqManageIdx;

  plList = playlistGetPlaylistList (PL_LIST_NORMAL, NULL);
  uiDropDownSetList (uimusicq->ui [ci].playlistsel, plList, NULL);
  slistFree (plList);
  logProcEnd ("");
}

void
uimusicqTruncateQueue (uimusicq_t *uimusicq, int mqidx, nlistidx_t idx)
{
  char          tbuff [40];

  snprintf (tbuff, sizeof (tbuff), "%d%c%d", mqidx, MSG_ARGS_RS, idx);
  connSendMessage (uimusicq->conn, ROUTE_MAIN, MSG_MUSICQ_TRUNCATE, tbuff);
}

void
uimusicqPlay (uimusicq_t *uimusicq, int mqidx, dbidx_t dbidx)
{
  char          tbuff [80];

  /* clear the playlist queue and music queue, current playing song */
  /* and insert the new song */
  snprintf (tbuff, sizeof (tbuff), "%d%c%d%c%d",
      mqidx, MSG_ARGS_RS, 99, MSG_ARGS_RS, dbidx);
  connSendMessage (uimusicq->conn, ROUTE_MAIN, MSG_QUEUE_CLEAR_PLAY, tbuff);
}

void
uimusicqQueue (uimusicq_t *uimusicq, int mqidx, dbidx_t dbidx)
{
  if (uimusicq->cbcopy [UIMUSICQ_CBC_QUEUE] != NULL) {
    callbackHandlerLongInt (uimusicq->cbcopy [UIMUSICQ_CBC_QUEUE], dbidx, mqidx);
  }
}

void
uimusicqSetPeerFlag (uimusicq_t *uimusicq, bool val)
{
  uimusicq->ispeercall = val;
}

bool
uimusicqSaveListCallback (void *udata, long dbidx)
{
  uimusicq_t  *uimusicq = udata;

  nlistSetStr (uimusicq->savelist, dbidx, NULL);
  return UICB_CONT;
}
