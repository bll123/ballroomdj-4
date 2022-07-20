#include "config.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "conn.h"
#include "log.h"
#include "musicdb.h"
#include "playlist.h"
#include "tmutil.h"
#include "uimusicq.h"
#include "ui.h"

void
uimusicqQueueDanceProcess (uimusicq_t *uimusicq, long idx)
{
  int           ci;
  char          tbuff [20];


  logProcBegin (LOG_PROC, "uimusicqQueueDance");

  ci = uimusicq->musicqManageIdx;

  if (idx >= 0) {
    snprintf (tbuff, sizeof (tbuff), "%d%c%ld", ci, MSG_ARGS_RS, idx);
    connSendMessage (uimusicq->conn, ROUTE_MAIN, MSG_QUEUE_DANCE, tbuff);
  }
  logProcEnd (LOG_PROC, "uimusicqQueueDance", "");
}

void
uimusicqQueuePlaylistProcess (uimusicq_t *uimusicq, long idx)
{
  int           ci;
  char          tbuff [100];


  logProcBegin (LOG_PROC, "uimusicqQueuePlaylist");

  ci = uimusicq->musicqManageIdx;

  if (idx >= 0) {
    snprintf (tbuff, sizeof (tbuff), "%d%c%s", ci, MSG_ARGS_RS,
        uiDropDownGetString (uimusicq->ui [ci].playlistsel));
    connSendMessage (uimusicq->conn, ROUTE_MAIN, MSG_QUEUE_PLAYLIST, tbuff);
  }
  logProcEnd (LOG_PROC, "uimusicqQueuePlaylist", "");
}

void
uimusicqMoveTop (uimusicq_t *uimusicq, int mqidx, long idx)
{
  char        tbuff [40];

  snprintf (tbuff, sizeof (tbuff), "%d%c%ld", mqidx, MSG_ARGS_RS, idx);
  connSendMessage (uimusicq->conn, ROUTE_MAIN,
      MSG_MUSICQ_MOVE_TOP, tbuff);
}

void
uimusicqMoveUp (uimusicq_t *uimusicq, int mqidx, long idx)
{
  char        tbuff [40];

  snprintf (tbuff, sizeof (tbuff), "%d%c%ld", mqidx, MSG_ARGS_RS, idx);
  connSendMessage (uimusicq->conn, ROUTE_MAIN,
      MSG_MUSICQ_MOVE_UP, tbuff);
  uimusicq->repeatButton = UIMUSICQ_REPEAT_UP;
  mstimeset (&uimusicq->repeatTimer, UIMUSICQ_REPEAT_TIME);
  logProcEnd (LOG_PROC, "uimusicqMoveUp", "");
}

void
uimusicqMoveDown (uimusicq_t *uimusicq, int mqidx, long idx)
{
  char        tbuff [40];

  snprintf (tbuff, sizeof (tbuff), "%d%c%ld", mqidx, MSG_ARGS_RS, idx);
  connSendMessage (uimusicq->conn, ROUTE_MAIN,
      MSG_MUSICQ_MOVE_DOWN, tbuff);
  uimusicq->repeatButton = UIMUSICQ_REPEAT_DOWN;
  mstimeset (&uimusicq->repeatTimer, UIMUSICQ_REPEAT_TIME);
}

void
uimusicqTogglePause (uimusicq_t *uimusicq, int mqidx, long idx)
{
  char        tbuff [40];

  snprintf (tbuff, sizeof (tbuff), "%d%c%ld", mqidx, MSG_ARGS_RS, idx);
  connSendMessage (uimusicq->conn, ROUTE_MAIN,
      MSG_MUSICQ_TOGGLE_PAUSE, tbuff);
}

void
uimusicqRemove (uimusicq_t *uimusicq, int mqidx, long idx)
{
  char        tbuff [40];

  snprintf (tbuff, sizeof (tbuff), "%d%c%ld", mqidx, MSG_ARGS_RS, idx);
  connSendMessage (uimusicq->conn, ROUTE_MAIN,
      MSG_MUSICQ_REMOVE, tbuff);
}

bool
uimusicqStopRepeat (void *udata)
{
  uimusicq_t  *uimusicq = udata;

  logProcBegin (LOG_PROC, "uimusicqStopRepeat");
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: stop repeat");

  uimusicq->repeatButton = UIMUSICQ_REPEAT_NONE;
  logProcEnd (LOG_PROC, "uimusicqStopRepeat", "");
  return UICB_CONT;
}

void
uimusicqCreatePlaylistList (uimusicq_t *uimusicq)
{
  int               ci;
  slist_t           *plList;


  logProcBegin (LOG_PROC, "uimusicqCreatePlaylistList");

  ci = uimusicq->musicqManageIdx;

  plList = playlistGetPlaylistList (PL_LIST_NORMAL);
  uiDropDownSetList (uimusicq->ui [ci].playlistsel, plList, NULL);
  slistFree (plList);
  logProcEnd (LOG_PROC, "uimusicqCreatePlaylistList", "");
}

void
uimusicqClearQueue (uimusicq_t *uimusicq, int mqidx, long idx)
{
  char          tbuff [40];

  snprintf (tbuff, sizeof (tbuff), "%d%c%ld", mqidx, MSG_ARGS_RS, idx);
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

int
uimusicqMusicQueueDataParse (uimusicq_t *uimusicq, char *args)
{
  int               ci;
  char              *p;
  char              *tokstr;
  int               idx;
  musicqupdate_t    *musicqupdate;


  logProcBegin (LOG_PROC, "uimusicqMusicQueueDataParse");

  /* first, build ourselves a list to work with */
  uimusicqMusicQueueDataFree (uimusicq);
  uimusicq->uniqueList = nlistAlloc ("temp-musicq-unique", LIST_ORDERED, free);
  uimusicq->dispList = nlistAlloc ("temp-musicq-disp", LIST_ORDERED, NULL);

  p = strtok_r (args, MSG_ARGS_RS_STR, &tokstr);
  ci = atoi (p);
  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  /* queue duration */

  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  idx = 1;
  while (p != NULL) {
    musicqupdate = malloc (sizeof (musicqupdate_t));
    assert (musicqupdate != NULL);

    musicqupdate->dispidx = atoi (p);
    p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
    if (p != NULL) {
      musicqupdate->uniqueidx = atoi (p);
    }
    p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
    if (p != NULL) {
      musicqupdate->dbidx = atol (p);
    }
    p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
    if (p != NULL) {
      musicqupdate->pflag = atoi (p);
    }

    nlistSetData (uimusicq->uniqueList, musicqupdate->uniqueidx, musicqupdate);
    nlistSetData (uimusicq->dispList, idx, musicqupdate);

    p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
    ++idx;
  }

  logProcEnd (LOG_PROC, "uimusicqMusicQueueDataParse", "");
  return ci;
}

void
uimusicqMusicQueueDataFree (uimusicq_t *uimusicq)
{
  if (uimusicq->dispList != NULL) {
    nlistFree (uimusicq->dispList);
    uimusicq->dispList = NULL;
  }
  if (uimusicq->uniqueList != NULL) {
    nlistFree (uimusicq->uniqueList);
    uimusicq->uniqueList = NULL;
  }
}

void
uimusicqSetPeerFlag (uimusicq_t *uimusicq, bool val)
{
  uimusicq->ispeercall = val;
}

