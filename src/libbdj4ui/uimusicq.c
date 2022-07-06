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
#include "songlist.h"
#include "tagdef.h"
#include "uimusicq.h"
#include "ui.h"

static void   uimusicqProcessSongSelect (uimusicq_t *uimusicq, char * args);
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
  uimusicq->repeatButton = UIMUSICQ_REPEAT_NONE;
  uimusicq->dispsel = dispsel;
  uimusicq->musicdb = musicdb;
  uimusicq->dispselType = dispselType;
  uimusicq->uniqueList = NULL;
  uimusicq->dispList = NULL;
  uimusicq->workList = NULL;
  uimusicq->statusMsg = NULL;

  for (int i = 0; i < MUSICQ_MAX; ++i) {
    int     sz;

    uimusicq->ui [i].hasui = false;
    uimusicq->ui [i].count = 0;
    uimusicq->ui [i].haveselloc = false;
    uimusicq->ui [i].playlistsel = uiDropDownInit ();
    sz = 20;
    if (uimusicq->dispselType == DISP_SEL_SONGLIST ||
        uimusicq->dispselType == DISP_SEL_EZSONGLIST) {
      sz = 15;
    }
    uimusicq->ui [i].slname = uiEntryInit (sz, 40);
  }
  uimusicq->musicqManageIdx = MUSICQ_PB_A;
  uimusicq->musicqPlayIdx = MUSICQ_PB_A;
  uimusicq->iteratecb = NULL;
  uimusicq->savelist = NULL;
  uimusicq->cbci = MUSICQ_PB_A;
  uiutilsUIWidgetInit (&uimusicq->pausePixbuf);
  uimusicqUIInit (uimusicq);
  uimusicq->newselcb = NULL;
  uimusicq->peercount = 0;
  uimusicq->ispeercall = false;
  for (int i = 0; i < UIMUSICQ_PEER_MAX; ++i) {
    uimusicq->peers [i] = NULL;
  }

  logProcEnd (LOG_PROC, "uimusicqInit", "");
  return uimusicq;
}

void
uimusicqSetPeer (uimusicq_t *uimusicq, uimusicq_t *peer)
{
  if (uimusicq->peercount >= UIMUSICQ_PEER_MAX) {
    return;
  }
  uimusicq->peers [uimusicq->peercount] = peer;
  ++uimusicq->peercount;
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
    uimusicqMusicQueueDataFree (uimusicq);
    uiWidgetClearPersistent (&uimusicq->pausePixbuf);
    for (int i = 0; i < MUSICQ_MAX; ++i) {
      uiDropDownFree (uimusicq->ui [i].playlistsel);
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
  uimusicqUIMainLoop (uimusicq);
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
  uimusicq->newselcb = uicbdbidx;
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
  uiEntryPeerBuffer (targetq->ui [MUSICQ_SL].slname,
      sourceq->ui [MUSICQ_SL].slname);
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
  uimusicqIterate (uimusicq, uimusicqSaveListCallback, MUSICQ_SL);

  songlist = songlistAlloc (fname);

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

void
uimusicqSetEditCallback (uimusicq_t *uimusicq, UICallback *uicb)
{
  if (uimusicq == NULL) {
    return;
  }

  uimusicq->editcb = uicb;
}

/* internal routines */

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

