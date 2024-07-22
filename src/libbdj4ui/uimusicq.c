/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "audioadjust.h"
#include "bdj4intl.h"
#include "conn.h"
#include "dispsel.h"
#include "fileop.h"
#include "filemanip.h"
#include "log.h"
#include "expimp.h"
#include "mdebug.h"
#include "mp3exp.h"
#include "musicq.h"
#include "nlist.h"
#include "pathbld.h"
#include "pathutil.h"
#include "playlist.h"
#include "song.h"
#include "songlist.h"
#include "songlistutil.h"
#include "sysvars.h"
#include "tagdef.h"
#include "ui.h"
#include "uiexppl.h"
#include "callback.h"
#include "uimusicq.h"

uimusicq_t *
uimusicqInit (const char *tag, conn_t *conn, musicdb_t *musicdb,
    dispsel_t *dispsel, dispselsel_t dispselType)
{
  uimusicq_t    *uimusicq;
  char          tbuff [MAXPATHLEN];

  logProcBegin ();

  uimusicq = mdmalloc (sizeof (uimusicq_t));

  uimusicq->tag = tag;
  uimusicq->conn = conn;
  uimusicq->dispsel = dispsel;
  uimusicq->musicdb = musicdb;
  uimusicq->statusMsg = NULL;
  for (int i = 0; i < UIMUSICQ_CB_MAX; ++i) {
    uimusicq->callbacks [i] = NULL;
  }
  for (int i = 0; i < UIMUSICQ_USER_CB_MAX; ++i) {
    uimusicq->usercb [i] = NULL;
  }
  uimusicq->musicqManageIdx = MUSICQ_PB_A;
  uimusicq->musicqPlayIdx = MUSICQ_PB_A;
  uimusicq->cbci = MUSICQ_PB_A;
  uimusicq->pausePixbuf = NULL;
  uimusicq->backupcreated = false;
  uimusicq->changed = false;

  /* want a copy of the pixbuf for this image */
  pathbldMakePath (tbuff, sizeof (tbuff), "button_pause", ".svg",
      PATHBLD_MP_DREL_IMG | PATHBLD_MP_USEIDX);
  uimusicq->pausePixbuf = uiImageFromFile (tbuff);
  uiImageConvertToPixbuf (uimusicq->pausePixbuf);
  uiWidgetMakePersistent (uimusicq->pausePixbuf);

  uimusicqUIInit (uimusicq);

  for (int i = 0; i < MUSICQ_MAX; ++i) {
    uimusicq->ui [i].newflag = true;
    uimusicq->ui [i].mainbox = NULL;
    uimusicq->ui [i].dispselType = dispselType;
    if (i == MUSICQ_HISTORY) {
      /* override the setting from the player ui */
      uimusicq->ui [i].dispselType = DISP_SEL_HISTORY;
    }
    uimusicq->ui [i].hasui = false;
    uimusicq->ui [i].rowcount = 0;
    uimusicq->ui [i].haveselloc = false;
    uimusicq->ui [i].lastLocation = -1;
    uimusicq->ui [i].prevSelection = -1;
    uimusicq->ui [i].currSelection = -1;
    uimusicq->ui [i].playlistsel = NULL;
    uimusicq->ui [i].slname = NULL;
  }

  logProcEnd ("");
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
  logProcBegin ();
  if (uimusicq == NULL) {
    return;
  }

  for (int i = 0; i < UIMUSICQ_CB_MAX; ++i) {
    callbackFree (uimusicq->callbacks [i]);
  }
  for (int i = 0; i < MUSICQ_MAX; ++i) {
    uiwcontFree (uimusicq->ui [i].mainbox);
    uiplaylistFree (uimusicq->ui [i].playlistsel);
    uiwcontFree (uimusicq->ui [i].slname);
  }
  uimusicqUIFree (uimusicq);

  uiWidgetClearPersistent (uimusicq->pausePixbuf);
  uiwcontFree (uimusicq->pausePixbuf);

  mdfree (uimusicq);
  logProcEnd ("");
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
  uimusicq->usercb [UIMUSICQ_USER_CB_NEW_SEL] = uicbdbidx;
}

void
uimusicqSetSongSaveCallback (uimusicq_t *uimusicq, callback_t *uicb)
{
  if (uimusicq == NULL) {
    return;
  }
  uimusicq->usercb [UIMUSICQ_USER_CB_SONG_SAVE] = uicb;
}

void
uimusicqSetClearQueueCallback (uimusicq_t *uimusicq, callback_t *uicb)
{
  if (uimusicq == NULL) {
    return;
  }
  uimusicq->usercb [UIMUSICQ_USER_CB_CLEAR_QUEUE] = uicb;
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

nlistidx_t
uimusicqGetCount (uimusicq_t *uimusicq)
{
  int           ci;

  if (uimusicq == NULL) {
    return 0;
  }

  ci = uimusicq->musicqManageIdx;
  return uimusicq->ui [ci].rowcount;
}

void
uimusicqSetEditCallback (uimusicq_t *uimusicq, callback_t *uicb)
{
  if (uimusicq == NULL) {
    return;
  }

  uimusicq->usercb [UIMUSICQ_USER_CB_EDIT] = uicb;
}

void
uimusicqExport (uimusicq_t *uimusicq, mp_musicqupdate_t *musicqupdate,
    const char *fname, const char *slname, int exptype)
{
  nlist_t   *dbidxlist;

  dbidxlist = uimusicqGetDBIdxList (musicqupdate);
  switch (exptype) {
    case EI_TYPE_M3U: {
      m3uExport (uimusicq->musicdb, dbidxlist, fname, slname);
      break;
    }
    case EI_TYPE_XSPF: {
      xspfExport (uimusicq->musicdb, dbidxlist, fname, slname);
      break;
    }
    case EI_TYPE_JSPF: {
      jspfExport (uimusicq->musicdb, dbidxlist, fname, slname);
      break;
    }
  }
  nlistFree (dbidxlist);
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
  uimusicq->usercb [UIMUSICQ_USER_CB_QUEUE] = uicb;
}

void
uimusicqSave (musicdb_t *musicdb, mp_musicqupdate_t *musicqupdate, const char *name)
{
  nlist_t             *dbidxlist;

  logProcBegin ();

  dbidxlist = uimusicqGetDBIdxList (musicqupdate);
  songlistutilCreateFromList (musicdb, name, dbidxlist);
  nlistFree (dbidxlist);
  playlistCheckAndCreate (name, PLTYPE_SONGLIST);

  logProcEnd ("");
}

nlist_t *
uimusicqGetDBIdxList (mp_musicqupdate_t *musicqupdate)
{
  nlist_t             *dbidxlist;
  nlistidx_t          mqiter;
  nlistidx_t          key;

  dbidxlist = nlistAlloc ("dbidxlist", LIST_UNORDERED, NULL);
  if (musicqupdate == NULL) {
    return dbidxlist;
  }

  nlistStartIterator (musicqupdate->dispList, &mqiter);
  while ((key = nlistIterateKey (musicqupdate->dispList, &mqiter)) >= 0) {
    mp_musicqupditem_t  *musicqupditem;

    musicqupditem = nlistGetData (musicqupdate->dispList, key);
    nlistSetNum (dbidxlist, musicqupditem->dbidx, 0);
  }

  return dbidxlist;
}
