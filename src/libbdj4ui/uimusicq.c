#include "config.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "bdj4intl.h"
#include "conn.h"
#include "dispsel.h"
#include "fileop.h"
#include "filemanip.h"
#include "log.h"
#include "musicq.h"
#include "nlist.h"
#include "playlist.h"
#include "tagdef.h"
#include "uimusicq.h"
#include "ui.h"

static void   uimusicqProcessSongSelect (uimusicq_t *uimusicq, char * args);
static int    uimusicqMoveUpRepeat (void *udata);
static int    uimusicqMoveDownRepeat (void *udata);
static void   uimusicqSaveListCallback (uimusicq_t *uimusicq, dbidx_t dbidx);

uimusicq_t *
uimusicqInit (const char *tag, conn_t *conn, musicdb_t *musicdb,
    dispsel_t *dispsel, dispselsel_t dispselType)
{
  uimusicq_t    *uimusicq;


  logProcBegin (LOG_PROC, "uimusicqInit");

  uimusicq = malloc (sizeof (uimusicq_t));
  assert (uimusicq != NULL);

  uimusicq->tag = tag;
  uimusicq->conn = conn;
  uimusicq->dispsel = dispsel;
  uimusicq->musicdb = musicdb;
  uimusicq->dispselType = dispselType;
  uimusicq->uniqueList = NULL;
  uimusicq->dispList = NULL;
  uimusicq->workList = NULL;
  for (int i = 0; i < MUSICQ_MAX; ++i) {
    int     sz;

    uimusicq->ui [i].hasui = false;
    uimusicq->ui [i].repeatTimer = 0;
    uimusicq->ui [i].count = 0;
    uimusicq->ui [i].haveselloc = false;
    uiDropDownInit (&uimusicq->ui [i].playlistsel);
    sz = 20;
    if (uimusicq->dispselType == DISP_SEL_SONGLIST ||
        uimusicq->dispselType == DISP_SEL_EZSONGLIST) {
      sz = 15;
    }
    uimusicq->ui [i].slname = uiEntryInit (sz, 40);
  }
  uimusicq->musicqManageIdx = MUSICQ_A;
  uimusicq->musicqPlayIdx = MUSICQ_A;
  uimusicq->iteratecb = NULL;
  uimusicq->savelist = NULL;
  uimusicq->cbci = MUSICQ_A;
  uiutilsUIWidgetInit (&uimusicq->pausePixbuf);
  uimusicqUIInit (uimusicq);
  uimusicq->newselcbdbidx = NULL;

  logProcEnd (LOG_PROC, "uimusicqInit", "");
  return uimusicq;
}

void
uimusicqSetDatabase (uimusicq_t *uimusicq, musicdb_t *musicdb)
{
  uimusicq->musicdb = musicdb;
}

void
uimusicqFree (uimusicq_t *uimusicq)
{
  logProcBegin (LOG_PROC, "uimusicqFree");
  if (uimusicq != NULL) {
    uiWidgetClearPersistent (&uimusicq->pausePixbuf);
    for (int i = 0; i < MUSICQ_MAX; ++i) {
      uiDropDownFree (&uimusicq->ui [i].playlistsel);
      uiEntryFree (uimusicq->ui [i].slname);
    }
    uimusicqUIFree (uimusicq);
    free (uimusicq);
  }
  logProcEnd (LOG_PROC, "uimusicqFree", "");
}

void
uimusicqMainLoop (uimusicq_t *uimusicq)
{
//  int           ci;

//  ci = uimusicq->musicqManageIdx;

  return;
}

int
uimusicqProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
    bdjmsgmsg_t msg, char *args, void *udata)
{
  uimusicq_t    *uimusicq = udata;
  bool          dbgdisp = false;
  char          *targs = NULL;

  if (args != NULL) {
    targs = strdup (args);
  }

  switch (route) {
    case ROUTE_NONE:
    case ROUTE_MANAGEUI:
    case ROUTE_PLAYERUI: {
      switch (msg) {
        case MSG_MUSIC_QUEUE_DATA: {
          uimusicqProcessMusicQueueData (uimusicq, targs);
          // dbgdisp = true;
          break;
        }
        case MSG_SONG_SELECT: {
          uimusicqProcessSongSelect (uimusicq, targs);
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
    logMsg (LOG_DBG, LOG_MSGS, "uimusicq (%d): rcvd: from:%d/%s route:%d/%s msg:%d/%s args:%s",
        uimusicq->dispselType, routefrom, msgRouteDebugText (routefrom),
        route, msgRouteDebugText (route), msg, msgDebugText (msg), args);
  }
  if (args != NULL) {
    free (targs);
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
uimusicqSetSelectionCallback (uimusicq_t *uimusicq, UICallback *uicbdbidx)
{
  if (uimusicq == NULL) {
    return;
  }
  uimusicq->newselcbdbidx = uicbdbidx;
}

bool
uimusicqMoveTop (void *udata)
{
  uimusicq_t        *uimusicq = udata;
  int               ci;
  ssize_t           idx;
  char              tbuff [20];


  logProcBegin (LOG_PROC, "uimusicqMoveTopProcess");


  ci = uimusicq->musicqManageIdx;

  idx = uimusicqGetSelection (uimusicq);
  if (idx < 0) {
    logProcEnd (LOG_PROC, "uimusicqMoveTopProcess", "bad-idx");
    return UICB_STOP;
  }

  uimusicqMusicQueueSetSelected (uimusicq, ci, UIMUSICQ_SEL_TOP);

  snprintf (tbuff, sizeof (tbuff), "%d%c%zu", ci, MSG_ARGS_RS, idx);
  connSendMessage (uimusicq->conn, ROUTE_MAIN,
      MSG_MUSICQ_MOVE_TOP, tbuff);
  logProcEnd (LOG_PROC, "uimusicqMoveTopProcess", "");
  return UICB_CONT;
}

bool
uimusicqMoveUp (void *udata)
{
  uimusicq_t        *uimusicq = udata;
  int               ci;
  ssize_t           idx;
  char              tbuff [20];


  logProcBegin (LOG_PROC, "uimusicqMoveUp");

  ci = uimusicq->musicqManageIdx;

  idx = uimusicqGetSelection (uimusicq);
  if (idx < 0) {
    logProcEnd (LOG_PROC, "uimusicqMoveUp", "bad-idx");
    return UICB_STOP;
  }

  uimusicqMusicQueueSetSelected (uimusicq, ci, UIMUSICQ_SEL_PREV);
  snprintf (tbuff, sizeof (tbuff), "%d%c%zu", ci, MSG_ARGS_RS, idx);
  connSendMessage (uimusicq->conn, ROUTE_MAIN,
      MSG_MUSICQ_MOVE_UP, tbuff);
  if (uimusicq->ui [ci].repeatTimer == 0) {
    uimusicq->ui [ci].repeatTimer = g_timeout_add (
        UIMUSICQ_REPEAT_TIME, uimusicqMoveUpRepeat, uimusicq);
  }
  logProcEnd (LOG_PROC, "uimusicqMoveUp", "");
  return UICB_CONT;
}

bool
uimusicqMoveDown (void *udata)
{
  uimusicq_t        *uimusicq = udata;
  int               ci;
  ssize_t           idx;
  char              tbuff [20];


  logProcBegin (LOG_PROC, "uimusicqMoveDown");

  ci = uimusicq->musicqManageIdx;

  idx = uimusicqGetSelection (uimusicq);
  if (idx < 0) {
    logProcEnd (LOG_PROC, "uimusicqMoveDown", "bad-idx");
    return UICB_STOP;
  }

  uimusicqMusicQueueSetSelected (uimusicq, ci, UIMUSICQ_SEL_NEXT);
  snprintf (tbuff, sizeof (tbuff), "%d%c%zu", ci, MSG_ARGS_RS, idx);
  connSendMessage (uimusicq->conn, ROUTE_MAIN,
      MSG_MUSICQ_MOVE_DOWN, tbuff);

  if (uimusicq->ui [ci].repeatTimer == 0) {
    uimusicq->ui [ci].repeatTimer = g_timeout_add (
        UIMUSICQ_REPEAT_TIME, uimusicqMoveDownRepeat, uimusicq);
  }
  logProcEnd (LOG_PROC, "uimusicqMoveDown", "");
  return UICB_CONT;
}

bool
uimusicqTogglePause (void *udata)
{
  uimusicq_t        *uimusicq = udata;
  int           ci;
  ssize_t       idx;
  char          tbuff [20];


  logProcBegin (LOG_PROC, "uimusicqTogglePause");

  idx = uimusicqGetSelection (uimusicq);
  if (idx < 0) {
    logProcEnd (LOG_PROC, "uimusicqTogglePause", "bad-idx");
    return UICB_STOP;
  }

  ci = uimusicq->musicqManageIdx;

  snprintf (tbuff, sizeof (tbuff), "%d%c%zu", ci, MSG_ARGS_RS, idx);
  connSendMessage (uimusicq->conn, ROUTE_MAIN,
      MSG_MUSICQ_TOGGLE_PAUSE, tbuff);
  logProcEnd (LOG_PROC, "uimusicqTogglePause", "");
  return UICB_CONT;
}

bool
uimusicqRemove (void *udata)
{
  uimusicq_t    *uimusicq = udata;
  int           ci;
  ssize_t       idx;
  char          tbuff [20];


  logProcBegin (LOG_PROC, "uimusicqRemove");

  idx = uimusicqGetSelection (uimusicq);
  if (idx < 0) {
    logProcEnd (LOG_PROC, "uimusicqRemove", "bad-idx");
    return UICB_STOP;
  }

  ci = uimusicq->musicqManageIdx;

  snprintf (tbuff, sizeof (tbuff), "%d%c%zu", ci, MSG_ARGS_RS, idx);
  connSendMessage (uimusicq->conn, ROUTE_MAIN,
      MSG_MUSICQ_REMOVE, tbuff);
  logProcEnd (LOG_PROC, "uimusicqRemove", "");
  return UICB_CONT;
}

bool
uimusicqStopRepeat (void *udata)
{
  uimusicq_t  *uimusicq = udata;
  int         ci;

  logProcBegin (LOG_PROC, "uimusicqStopRepeat");

  ci = uimusicq->musicqManageIdx;
  if (uimusicq->ui [ci].repeatTimer != 0) {
    g_source_remove (uimusicq->ui [ci].repeatTimer);
  }
  uimusicq->ui [ci].repeatTimer = 0;
  logProcEnd (LOG_PROC, "uimusicqStopRepeat", "");
  return UICB_CONT;
}

bool
uimusicqClearQueue (void *udata)
{
  uimusicq_t        *uimusicq = udata;
  int           ci;
  int           idx;
  char          tbuff [20];


  logProcBegin (LOG_PROC, "uimusicqClearQueue");

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
  logProcEnd (LOG_PROC, "uimusicqClearQueue", "");
  return UICB_CONT;
}

void
uimusicqQueueDanceProcess (uimusicq_t *uimusicq, ssize_t idx)
{
  int           ci;
  char          tbuff [20];


  logProcBegin (LOG_PROC, "uimusicqQueueDance");

  ci = uimusicq->musicqManageIdx;

  if (idx >= 0) {
    snprintf (tbuff, sizeof (tbuff), "%d%c%zu", ci, MSG_ARGS_RS, idx);
    connSendMessage (uimusicq->conn, ROUTE_MAIN, MSG_QUEUE_DANCE, tbuff);
  }
  logProcEnd (LOG_PROC, "uimusicqQueueDance", "");
}

void
uimusicqQueuePlaylistProcess (uimusicq_t *uimusicq, ssize_t idx)
{
  int           ci;
  char          tbuff [100];


  logProcBegin (LOG_PROC, "uimusicqQueuePlaylist");

  ci = uimusicq->musicqManageIdx;

  if (idx >= 0) {
    snprintf (tbuff, sizeof (tbuff), "%d%c%s", ci, MSG_ARGS_RS,
        uimusicq->ui [ci].playlistsel.strSelection);
    connSendMessage (uimusicq->conn, ROUTE_MAIN, MSG_QUEUE_PLAYLIST, tbuff);
  }
  logProcEnd (LOG_PROC, "uimusicqQueuePlaylist", "");
}

void
uimusicqCreatePlaylistList (uimusicq_t *uimusicq)
{
  int               ci;
  slist_t           *plList;


  logProcBegin (LOG_PROC, "uimusicqCreatePlaylistList");

  ci = uimusicq->musicqManageIdx;

  plList = playlistGetPlaylistList (PL_LIST_NORMAL);
  uiDropDownSetList (&uimusicq->ui [ci].playlistsel, plList, NULL);
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

void
uimusicqSetSonglistName (uimusicq_t *uimusicq, const char *nm)
{
  int   ci;

  ci = uimusicq->musicqManageIdx;
  uiEntrySetValue (uimusicq->ui [ci].slname, nm);
}

const char *
uimusicqGetSonglistName (uimusicq_t *uimusicq)
{
  int         ci;
  const char  *val;

  ci = uimusicq->musicqManageIdx;
  val = uiEntryGetValue (uimusicq->ui [ci].slname);
  return val;
}

void
uimusicqPeerSonglistName (uimusicq_t *targetq, uimusicq_t *sourceq)
{
  uiEntryPeerBuffer (targetq->ui [MUSICQ_A].slname,
      sourceq->ui [MUSICQ_A].slname);
}

long
uimusicqGetCount (uimusicq_t *uimusicq)
{
  int           ci;

  if (uimusicq == NULL) {
    return 0;
  }

  ci = uimusicq->musicqManageIdx;
  return uimusicq->ui [ci].count;
}

void
uimusicqSave (uimusicq_t *uimusicq, const char *fname)
{
  char        tbuff [MAXPATHLEN];
  nlistidx_t  iteridx;
  dbidx_t     dbidx;
  songlist_t  *songlist;
  song_t      *song;
  ilistidx_t  key;

  snprintf (tbuff, sizeof (tbuff), "save-%s", fname);
  uimusicq->savelist = nlistAlloc (tbuff, LIST_UNORDERED, NULL);
  uimusicqIterate (uimusicq, uimusicqSaveListCallback, MUSICQ_A);

  songlist = songlistCreate (fname);

  nlistStartIterator (uimusicq->savelist, &iteridx);
  key = 0;
  while ((dbidx = nlistIterateKey (uimusicq->savelist, &iteridx)) >= 0) {
    song = dbGetByIdx (uimusicq->musicdb, dbidx);

    songlistSetStr (songlist, key, SONGLIST_FILE, songGetStr (song, TAG_FILE));
    songlistSetStr (songlist, key, SONGLIST_TITLE, songGetStr (song, TAG_TITLE));
    songlistSetNum (songlist, key, SONGLIST_DANCE, songGetNum (song, TAG_DANCE));
    ++key;
  }

  songlistSave (songlist);
  songlistFree (songlist);
  nlistFree (uimusicq->savelist);
  uimusicq->savelist = NULL;
}

/* internal routines */

static int
uimusicqMoveUpRepeat (gpointer udata)
{
  uimusicq_t        *uimusicq = udata;

  uimusicqMoveUp (uimusicq);
  return TRUE;
}

static int
uimusicqMoveDownRepeat (gpointer udata)
{
  uimusicq_t        *uimusicq = udata;

  uimusicqMoveDown (uimusicq);
  return TRUE;
}

static void
uimusicqProcessSongSelect (uimusicq_t *uimusicq, char * args)
{
  char    *p;
  char    *tokstr;
  int     mqidx;

  p = strtok_r (args, MSG_ARGS_RS_STR, &tokstr);
  mqidx = atoi (p);
  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  uimusicq->ui [mqidx].haveselloc = true;
  uimusicq->ui [mqidx].selectLocation = atol (p);
}


static void
uimusicqSaveListCallback (uimusicq_t *uimusicq, dbidx_t dbidx)
{
  nlistSetNum (uimusicq->savelist, dbidx, 0);
}

