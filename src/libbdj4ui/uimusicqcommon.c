/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <unistd.h>

#include "conn.h"
#include "log.h"
#include "musicdb.h"
#include "playlist.h"
#include "uimusicq.h"
#include "uiplaylist.h"
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
    snprintf (tbuff, sizeof (tbuff), "%d%c%" PRId32, ci, MSG_ARGS_RS, idx);
    connSendMessage (uimusicq->conn, ROUTE_MAIN, msg, tbuff);
  }
  logProcEnd ("");
}

void
uimusicqQueuePlaylistProcess (uimusicq_t *uimusicq, const char *fn)
{
  int           ci;
  char          tbuff [300];

  logProcBegin ();

  ci = uimusicq->musicqManageIdx;

  if (fn != NULL) {
    msgbuildQueuePlaylist (tbuff, sizeof (tbuff), ci, fn, EDIT_FALSE);
    connSendMessage (uimusicq->conn, ROUTE_MAIN, MSG_QUEUE_PLAYLIST, tbuff);
  }
  logProcEnd ("");
}

void
uimusicqMoveTop (uimusicq_t *uimusicq, int mqidx, nlistidx_t idx)
{
  char        tbuff [40];

  uimusicq->changed = true;
  snprintf (tbuff, sizeof (tbuff), "%d%c%" PRId32, mqidx, MSG_ARGS_RS, idx);
  connSendMessage (uimusicq->conn, ROUTE_MAIN, MSG_MUSICQ_MOVE_TOP, tbuff);
}

void
uimusicqMoveUp (uimusicq_t *uimusicq, int mqidx, nlistidx_t idx)
{
  char        tbuff [40];

  uimusicq->changed = true;
  snprintf (tbuff, sizeof (tbuff), "%d%c%" PRId32, mqidx, MSG_ARGS_RS, idx);
  connSendMessage (uimusicq->conn, ROUTE_MAIN, MSG_MUSICQ_MOVE_UP, tbuff);
  logProcEnd ("");
}

void
uimusicqMoveDown (uimusicq_t *uimusicq, int mqidx, nlistidx_t idx)
{
  char        tbuff [40];

  uimusicq->changed = true;
  snprintf (tbuff, sizeof (tbuff), "%d%c%" PRId32, mqidx, MSG_ARGS_RS, idx);
  connSendMessage (uimusicq->conn, ROUTE_MAIN, MSG_MUSICQ_MOVE_DOWN, tbuff);
}

void
uimusicqTogglePause (uimusicq_t *uimusicq, int mqidx, nlistidx_t idx)
{
  char        tbuff [40];

  snprintf (tbuff, sizeof (tbuff), "%d%c%" PRId32, mqidx, MSG_ARGS_RS, idx);
  connSendMessage (uimusicq->conn, ROUTE_MAIN,
      MSG_MUSICQ_TOGGLE_PAUSE, tbuff);
}

void
uimusicqRemove (uimusicq_t *uimusicq, int mqidx, nlistidx_t idx)
{
  char        tbuff [40];

  uimusicq->changed = true;
  snprintf (tbuff, sizeof (tbuff), "%d%c%" PRId32, mqidx, MSG_ARGS_RS, idx);
  connSendMessage (uimusicq->conn, ROUTE_MAIN,
      MSG_MUSICQ_REMOVE, tbuff);
}

void
uimusicqSwap (uimusicq_t *uimusicq, int mqidx)
{
  char          tbuff [100];
  uimusicqui_t  *mqui;

  mqui = &uimusicq->ui [mqidx];
  if (mqui->prevSelection < 0 ||
      mqui->currSelection < 0 ||
      mqui->prevSelection >= mqui->rowcount ||
      mqui->currSelection >= mqui->rowcount ||
      mqui->prevSelection == mqui->currSelection) {
    return;
  }

  uimusicq->changed = true;
  snprintf (tbuff, sizeof (tbuff), "%d%c%" PRId32 "%c%" PRId32, mqidx,
      MSG_ARGS_RS, mqui->prevSelection,
      MSG_ARGS_RS, mqui->currSelection);
  connSendMessage (uimusicq->conn, ROUTE_MAIN, MSG_MUSICQ_SWAP, tbuff);
}

void
uimusicqTruncateQueue (uimusicq_t *uimusicq, int mqidx, nlistidx_t idx)
{
  char          tbuff [40];

  snprintf (tbuff, sizeof (tbuff), "%d%c%" PRId32, mqidx, MSG_ARGS_RS, idx);
  connSendMessage (uimusicq->conn, ROUTE_MAIN, MSG_MUSICQ_TRUNCATE, tbuff);
}

void
uimusicqPlay (uimusicq_t *uimusicq, int mqidx, dbidx_t dbidx)
{
  char          tbuff [80];
  char          tmp [40];

  snprintf (tmp, sizeof (tmp), "%d", mqidx);
  /* clear the playlist queue and music queue */
  connSendMessage (uimusicq->conn, ROUTE_MAIN, MSG_QUEUE_CLEAR, tmp);
  /* and insert the new song */
  snprintf (tbuff, sizeof (tbuff), "%d%c%d%c%" PRId32,
      mqidx, MSG_ARGS_RS, QUEUE_LOC_LAST, MSG_ARGS_RS, dbidx);
  connSendMessage (uimusicq->conn, ROUTE_MAIN, MSG_MUSICQ_INSERT, tbuff);
  /* any play the next song */
  connSendMessage (uimusicq->conn, ROUTE_MAIN, MSG_CMD_NEXTSONG_PLAY, NULL);
}
