#include "config.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "conn.h"
#include "dispsel.h"
#include "log.h"
#include "musicq.h"
#include "playlist.h"
#include "tagdef.h"
#include "uimusicq.h"
#include "uiutils.h"

static void   uimusicqProcessSongSelect (uimusicq_t *uimusicq, char * args);
static int    uimusicqMoveUpRepeat (void *udata);
static int    uimusicqMoveDownRepeat (void *udata);

uimusicq_t *
uimusicqInit (conn_t *conn, musicdb_t *musicdb,
    dispsel_t *dispsel,
    int uimusicqflags, dispselsel_t dispselType)
{
  uimusicq_t    *uimusicq;


  logProcBegin (LOG_PROC, "uimusicqInit");

  uimusicq = malloc (sizeof (uimusicq_t));
  assert (uimusicq != NULL);

  uimusicq->conn = conn;
  uimusicq->dispsel = dispsel;
  uimusicq->musicdb = musicdb;
  uimusicq->dispselType = dispselType;
  uimusicq->uimusicqflags = uimusicqflags;
  uimusicq->uniqueList = NULL;
  uimusicq->dispList = NULL;
  uimusicq->workList = NULL;
  for (int i = 0; i < MUSICQ_MAX; ++i) {
    uimusicq->ui [i].active = false;
    uimusicq->ui [i].repeatTimer = 0;
    uiutilsDropDownInit (&uimusicq->ui [i].playlistsel);
    uiutilsDropDownInit (&uimusicq->ui [i].dancesel);
    uimusicq->ui [i].selPathStr = NULL;
    uimusicq->ui [i].musicqTree = NULL;
    mstimeset (&uimusicq->ui [i].rowChangeTimer, 3600000);
  }
  uimusicq->musicqManageIdx = MUSICQ_A;
  uimusicq->musicqPlayIdx = MUSICQ_A;

  logProcEnd (LOG_PROC, "uimusicqInit", "");
  return uimusicq;
}

void
uimusicqFree (uimusicq_t *uimusicq)
{
  logProcBegin (LOG_PROC, "uimusicqFree");
  if (uimusicq != NULL) {
    for (int i = 0; i < MUSICQ_MAX; ++i) {
      if (uimusicq->ui [i].selPathStr != NULL) {
        free (uimusicq->ui [i].selPathStr);
      }
      uiutilsDropDownFree (&uimusicq->ui [i].playlistsel);
      uiutilsDropDownFree (&uimusicq->ui [i].dancesel);
    }
    free (uimusicq);
  }
  logProcEnd (LOG_PROC, "uimusicqFree", "");
}

void
uimusicqMainLoop (uimusicq_t *uimusicq)
{
  int           ci;

  ci = uimusicq->musicqManageIdx;

  /* macos loses the selection after it is moved.  reset it. */
  /* fortunately, it seems to only need to be reset once. */
  if (mstimeCheck (&uimusicq->ui [ci].rowChangeTimer)) {
    if (uimusicq->ui [ci].selPathStr != NULL) {
      uimusicqSetSelection (uimusicq, uimusicq->ui [ci].selPathStr);
    }
    mstimeset (&uimusicq->ui [ci].rowChangeTimer, 3600000);
  }

  return;
}

int
uimusicqProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
    bdjmsgmsg_t msg, char *args, void *udata)
{
  uimusicq_t    *uimusicq = udata;


  switch (route) {
    case ROUTE_NONE:
    case ROUTE_MANAGEUI:
    case ROUTE_PLAYERUI: {
      switch (msg) {
        case MSG_MUSIC_QUEUE_DATA: {
          uimusicqProcessMusicQueueData (uimusicq, args);
          break;
        }
        case MSG_SONG_SELECT: {
          uimusicqProcessSongSelect (uimusicq, args);
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
uimusicqSetPlayIdx (uimusicq_t *uimusicq, int playIdx)
{
  uimusicq->musicqPlayIdx = playIdx;
}

void
uimusicqSetManageIdx (uimusicq_t *uimusicq, int manageIdx)
{
  uimusicq->musicqManageIdx = manageIdx;
}

void
uimusicqMoveTopProcess (uimusicq_t *uimusicq)
{
  int               ci;
  ssize_t           idx;
  char              tbuff [20];


  logProcBegin (LOG_PROC, "uimusicqMoveTopProcess");


  ci = uimusicq->musicqManageIdx;

  idx = uimusicqGetSelection (uimusicq);
  if (idx < 0) {
    logProcEnd (LOG_PROC, "uimusicqMoveTopProcess", "bad-idx");
    return;
  }

  uimusicqMusicQueueSetSelected (uimusicq, ci, UIMUSICQ_SEL_TOP);

  snprintf (tbuff, sizeof (tbuff), "%d%c%zu", ci, MSG_ARGS_RS, idx);
  connSendMessage (uimusicq->conn, ROUTE_MAIN,
      MSG_MUSICQ_MOVE_TOP, tbuff);
  logProcEnd (LOG_PROC, "uimusicqMoveTopProcess", "");
}

void
uimusicqMoveUpProcess (uimusicq_t *uimusicq)
{
  int               ci;
  ssize_t           idx;
  char              tbuff [20];


  logProcBegin (LOG_PROC, "uimusicqMoveUpProcess");

  ci = uimusicq->musicqManageIdx;

  idx = uimusicqGetSelection (uimusicq);
  if (idx < 0) {
    logProcEnd (LOG_PROC, "uimusicqMoveUpProcess", "bad-idx");
    return;
  }

  uimusicqMusicQueueSetSelected (uimusicq, ci, UIMUSICQ_SEL_PREV);
  snprintf (tbuff, sizeof (tbuff), "%d%c%zu", ci, MSG_ARGS_RS, idx);
  connSendMessage (uimusicq->conn, ROUTE_MAIN,
      MSG_MUSICQ_MOVE_UP, tbuff);
  if (uimusicq->ui [ci].repeatTimer == 0) {
    uimusicq->ui [ci].repeatTimer = g_timeout_add (
        UIMUSICQ_REPEAT_TIME, uimusicqMoveUpRepeat, uimusicq);
  }
  logProcEnd (LOG_PROC, "uimusicqMoveUpProcess", "");
}

void
uimusicqMoveDownProcess (uimusicq_t *uimusicq)
{
  int               ci;
  ssize_t           idx;
  char              tbuff [20];


  logProcBegin (LOG_PROC, "uimusicqMoveDownProcess");

  ci = uimusicq->musicqManageIdx;

  idx = uimusicqGetSelection (uimusicq);
  if (idx < 0) {
    logProcEnd (LOG_PROC, "uimusicqMoveDownProcess", "bad-idx");
    return;
  }

  uimusicqMusicQueueSetSelected (uimusicq, ci, UIMUSICQ_SEL_NEXT);
  snprintf (tbuff, sizeof (tbuff), "%d%c%zu", ci, MSG_ARGS_RS, idx);
  connSendMessage (uimusicq->conn, ROUTE_MAIN,
      MSG_MUSICQ_MOVE_DOWN, tbuff);

  if (uimusicq->ui [ci].repeatTimer == 0) {
    uimusicq->ui [ci].repeatTimer = g_timeout_add (
        UIMUSICQ_REPEAT_TIME, uimusicqMoveDownRepeat, uimusicq);
  }
  logProcEnd (LOG_PROC, "uimusicqMoveDownProcess", "");
}

void
uimusicqTogglePauseProcess (uimusicq_t *uimusicq)
{
  int           ci;
  ssize_t       idx;
  char          tbuff [20];


  logProcBegin (LOG_PROC, "uimusicqTogglePauseProcess");

  idx = uimusicqGetSelection (uimusicq);
  if (idx < 0) {
    logProcEnd (LOG_PROC, "uimusicqTogglePauseProcess", "bad-idx");
    return;
  }

  ci = uimusicq->musicqManageIdx;

  snprintf (tbuff, sizeof (tbuff), "%d%c%zu", ci, MSG_ARGS_RS, idx);
  connSendMessage (uimusicq->conn, ROUTE_MAIN,
      MSG_MUSICQ_TOGGLE_PAUSE, tbuff);
  logProcEnd (LOG_PROC, "uimusicqTogglePauseProcess", "");
}

void
uimusicqRemoveProcess (uimusicq_t *uimusicq)
{
  int           ci;
  ssize_t       idx;
  char          tbuff [20];


  logProcBegin (LOG_PROC, "uimusicqRemoveProcess");

  idx = uimusicqGetSelection (uimusicq);
  if (idx < 0) {
    logProcEnd (LOG_PROC, "uimusicqRemoveProcess", "bad-idx");
    return;
  }

  ci = uimusicq->musicqManageIdx;

  snprintf (tbuff, sizeof (tbuff), "%d%c%zu", ci, MSG_ARGS_RS, idx);
  connSendMessage (uimusicq->conn, ROUTE_MAIN,
      MSG_MUSICQ_REMOVE, tbuff);
  logProcEnd (LOG_PROC, "uimusicqRemoveProcess", "");
}

void
uimusicqStopRepeat (uimusicq_t *uimusicq)
{
  int         ci;

  logProcBegin (LOG_PROC, "uimusicqStopRepeat");

  ci = uimusicq->musicqManageIdx;
  if (uimusicq->ui [ci].repeatTimer != 0) {
    g_source_remove (uimusicq->ui [ci].repeatTimer);
  }
  uimusicq->ui [ci].repeatTimer = 0;
  logProcEnd (LOG_PROC, "uimusicqStopRepeat", "");
}

void
uimusicqClearQueueProcess (uimusicq_t *uimusicq)
{
  int           ci;
  int           idx;
  char          tbuff [20];


  logProcBegin (LOG_PROC, "uimusicqClearQueueProcess");

  ci = uimusicq->musicqManageIdx;

  idx = uimusicqGetSelection (uimusicq);
  if (idx < 0) {
    /* if nothing is selected, clear everything */
    idx = 0;
    if (uimusicq->musicqManageIdx == uimusicq->musicqPlayIdx) {
      idx = 1;
    }
  }

  snprintf (tbuff, sizeof (tbuff), "%d%c%d", ci, MSG_ARGS_RS, idx);
  connSendMessage (uimusicq->conn, ROUTE_MAIN, MSG_QUEUE_CLEAR, tbuff);
  logProcEnd (LOG_PROC, "uimusicqClearQueueProcess", "");
}

void
uimusicqQueueDanceProcess (uimusicq_t *uimusicq, ssize_t idx)
{
  int           ci;
  char          tbuff [20];


  logProcBegin (LOG_PROC, "uimusicqQueueDanceProcess");

  ci = uimusicq->musicqManageIdx;

  if (idx >= 0) {
    snprintf (tbuff, sizeof (tbuff), "%d%c%zu", ci, MSG_ARGS_RS, idx);
    connSendMessage (uimusicq->conn, ROUTE_MAIN, MSG_QUEUE_DANCE, tbuff);
  }
  logProcEnd (LOG_PROC, "uimusicqQueueDanceProcess", "");
}

void
uimusicqQueuePlaylistProcess (uimusicq_t *uimusicq, ssize_t idx)
{
  int           ci;
  char          tbuff [100];


  logProcBegin (LOG_PROC, "uimusicqQueuePlaylistProcess");

  ci = uimusicq->musicqManageIdx;

  if (idx >= 0) {
    snprintf (tbuff, sizeof (tbuff), "%d%c%s", ci, MSG_ARGS_RS,
        uimusicq->ui [ci].playlistsel.strSelection);
    connSendMessage (uimusicq->conn, ROUTE_MAIN, MSG_QUEUE_PLAYLIST, tbuff);
  }
  logProcEnd (LOG_PROC, "uimusicqQueuePlaylistProcess", "");
}

void
uimusicqCreatePlaylistList (uimusicq_t *uimusicq)
{
  int               ci;
  slist_t           *plList;


  logProcBegin (LOG_PROC, "uimusicqCreatePlaylistList");

  ci = uimusicq->musicqManageIdx;

  plList = playlistGetPlaylistList ();
  uiutilsDropDownSetList (&uimusicq->ui [ci].playlistsel, plList, NULL);
  slistFree (plList);
  logProcEnd (LOG_PROC, "uimusicqCreatePlaylistList", "");
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
  uimusicq->uniqueList = nlistAlloc ("temp-musicq-unique", LIST_ORDERED, free);
  uimusicq->dispList = nlistAlloc ("temp-musicq-disp", LIST_ORDERED, NULL);

  p = strtok_r (args, MSG_ARGS_RS_STR, &tokstr);
  ci = atoi (p);

  /* index 0 is the currently playing song */
  idx = 1;
  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  while (p != NULL) {
    musicqupdate = malloc (sizeof (musicqupdate_t));
    assert (musicqupdate != NULL);

    musicqupdate->dispidx = atoi (p);
    p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
    musicqupdate->uniqueidx = atoi (p);
    p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
    musicqupdate->dbidx = atol (p);
    p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
    musicqupdate->pflag = atoi (p);
    musicqupdate->idx = idx;

    nlistSetData (uimusicq->uniqueList, musicqupdate->uniqueidx, musicqupdate);
    nlistSetData (uimusicq->dispList, musicqupdate->idx, musicqupdate);

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

/* internal routines */

static int
uimusicqMoveUpRepeat (gpointer udata)
{
  uimusicq_t        *uimusicq = udata;

  uimusicqMoveUpProcess (uimusicq);
  return TRUE;
}

static int
uimusicqMoveDownRepeat (gpointer udata)
{
  uimusicq_t        *uimusicq = udata;

  uimusicqMoveDownProcess (uimusicq);
  return TRUE;
}

static void
uimusicqProcessSongSelect (uimusicq_t *uimusicq, char * args)
{
  uimusicqSetSelection (uimusicq, args);
}


