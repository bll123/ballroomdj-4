/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "audioadjust.h"
#include "bdj4intl.h"
#include "conn.h"
#include "dispsel.h"
#include "fileop.h"
#include "filemanip.h"
#include "log.h"
#include "m3u.h"
#include "mdebug.h"
#include "mp3exp.h"
#include "musicq.h"
#include "nlist.h"
#include "pathutil.h"
#include "song.h"
#include "songlist.h"
#include "sysvars.h"
#include "tagdef.h"
#include "ui.h"
#include "callback.h"
#include "uimusicq.h"

static void   uimusicqSaveListCallback (uimusicq_t *uimusicq, dbidx_t dbidx);

uimusicq_t *
uimusicqInit (const char *tag, conn_t *conn, musicdb_t *musicdb,
    dispsel_t *dispsel, dispselsel_t dispselType)
{
  uimusicq_t    *uimusicq;


  logProcBegin (LOG_PROC, "uimusicqInit");

  uimusicq = mdmalloc (sizeof (uimusicq_t));
  assert (uimusicq != NULL);

  uimusicq->tag = tag;
  uimusicq->conn = conn;
  uimusicq->dispsel = dispsel;
  uimusicq->musicdb = musicdb;
  uimusicq->statusMsg = NULL;

  for (int i = 0; i < MUSICQ_MAX; ++i) {
    int     sz;

    uimusicq->ui [i].dispselType = dispselType;
    if (i == MUSICQ_HISTORY) {
      /* override the setting from the player ui */
      uimusicq->ui [i].dispselType = DISP_SEL_HISTORY;
    }
    uimusicq->ui [i].hasui = false;
    uimusicq->ui [i].count = 0;
    uimusicq->ui [i].haveselloc = false;
    uimusicq->ui [i].lastLocation = -1;
    uimusicq->ui [i].prevSelection = -1;
    uimusicq->ui [i].currSelection = -1;
    uimusicq->ui [i].selchgbypass = false;
    uimusicq->ui [i].playlistsel = uiDropDownInit ();
    sz = 20;
    if (uimusicq->ui [i].dispselType == DISP_SEL_SONGLIST) {
      sz = 25;
    }
    if (uimusicq->ui [i].dispselType == DISP_SEL_EZSONGLIST) {
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
  uimusicq->editcb = NULL;
  uimusicq->songsavecb = NULL;
  uimusicq->queuecb = NULL;
  uimusicq->clearqueuecb = NULL;
  uimusicq->queueplcb = NULL;
  uimusicq->queuedancecb = NULL;
  uimusicq->peercount = 0;
  uimusicq->backupcreated = false;
  uimusicq->changed = false;
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
    uiWidgetClearPersistent (&uimusicq->pausePixbuf);
    for (int i = 0; i < MUSICQ_MAX; ++i) {
      uiDropDownFree (uimusicq->ui [i].playlistsel);
      uiEntryFree (uimusicq->ui [i].slname);
    }
    callbackFree (uimusicq->queueplcb);
    callbackFree (uimusicq->queuedancecb);
    uimusicqUIFree (uimusicq);
    mdfree (uimusicq);
  }
  logProcEnd (LOG_PROC, "uimusicqFree", "");
}

void
uimusicqMainLoop (uimusicq_t *uimusicq)
{
  uimusicqUIMainLoop (uimusicq);
  return;
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
uimusicqSetSelectionCallback (uimusicq_t *uimusicq, callback_t *uicbdbidx)
{
  if (uimusicq == NULL) {
    return;
  }
  uimusicq->newselcb = uicbdbidx;
}

void
uimusicqSetSongSaveCallback (uimusicq_t *uimusicq, callback_t *uicb)
{
  if (uimusicq == NULL) {
    return;
  }
  uimusicq->songsavecb = uicb;
}

void
uimusicqSetClearQueueCallback (uimusicq_t *uimusicq, callback_t *uicb)
{
  if (uimusicq == NULL) {
    return;
  }
  uimusicq->clearqueuecb = uicb;
}

void
uimusicqSetSonglistName (uimusicq_t *uimusicq, const char *nm)
{
  int   ci;

  ci = uimusicq->musicqManageIdx;
  uiEntrySetValue (uimusicq->ui [ci].slname, nm);
}

/* the caller takes ownership of the returned value */
char *
uimusicqGetSonglistName (uimusicq_t *uimusicq)
{
  int         ci;
  const char  *val;
  char        *tval;

  ci = uimusicq->musicqManageIdx;
  val = uiEntryGetValue (uimusicq->ui [ci].slname);
  while (*val == ' ') {
    ++val;
  }
  tval = mdstrdup (val);
  stringTrimChar (tval, ' ');
  return tval;
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

  logProcBegin (LOG_PROC, "uimusicqSave");

  if (! uimusicq->changed) {
    logProcEnd (LOG_PROC, "uimusicqSave", "not-changed");
    return;
  }

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

  songlistSave (songlist, SONGLIST_UPDATE_TIMESTAMP);
  songlistFree (songlist);
  nlistFree (uimusicq->savelist);
  uimusicq->savelist = NULL;
  uimusicq->changed = false;
  logProcEnd (LOG_PROC, "uimusicqSave", "");
}

void
uimusicqSetEditCallback (uimusicq_t *uimusicq, callback_t *uicb)
{
  if (uimusicq == NULL) {
    return;
  }

  uimusicq->editcb = uicb;
}

void
uimusicqExportM3U (uimusicq_t *uimusicq, const char *fname, const char *slname)
{
  uimusicq->savelist = nlistAlloc ("m3u-export", LIST_UNORDERED, NULL);
  uimusicqIterate (uimusicq, uimusicqSaveListCallback, MUSICQ_SL);

  m3uExport (uimusicq->musicdb, uimusicq->savelist, fname, slname);

  nlistFree (uimusicq->savelist);
  uimusicq->savelist = NULL;
}

void
uimusicqProcessSongSelect (uimusicq_t *uimusicq, mp_songselect_t *songselect)
{
  uimusicq->ui [songselect->mqidx].haveselloc = true;
  uimusicq->ui [songselect->mqidx].selectLocation = songselect->loc;
}

void
uimusicqSetQueueCallback (uimusicq_t *uimusicq, callback_t *uicb)
{
  if (uimusicq == NULL) {
    return;
  }
  uimusicq->queuecb = uicb;
}

/* internal routines */

static void
uimusicqSaveListCallback (uimusicq_t *uimusicq, dbidx_t dbidx)
{
  nlistSetStr (uimusicq->savelist, dbidx, NULL);
}

