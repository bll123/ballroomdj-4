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

#include "audiosrc.h"
#include "audiotag.h"
#include "bdj4.h"
#include "bdj4intl.h"
#include "bdj4ui.h"
#include "bdjopt.h"
#include "dance.h"
#include "datafile.h"
#include "fileop.h"
#include "log.h"
#include "mdebug.h"
#include "musicdb.h"
#include "nlist.h"
#include "pathinfo.h"
#include "slist.h"
#include "song.h"
#include "songfav.h"
#include "tagdef.h"
#include "ui.h"
#include "callback.h"
#include "uidance.h"
#include "uiextreq.h"
#include "uiselectfile.h"

enum {
  UIEXTREQ_CB_DIALOG,
  UIEXTREQ_CB_DANCE,
  UIEXTREQ_CB_AUDIO_FILE,
  UIEXTREQ_CB_MAX,
};

typedef struct uiextreq {
  uiwcont_t       *parentwin;
  musicdb_t       *musicdb;
  uiwcont_t       *statusMsg;
  nlist_t         *options;
  uiwcont_t       *extreqDialog;
  uiwcont_t       *audioFileEntry;
  uiwcont_t       *audioFileDialogButton;
  uisfcb_t        audiofilesfcb;
  uiwcont_t       *artistEntry;
  uiwcont_t       *titleEntry;
  uidance_t       *uidance;
  callback_t      *callbacks [UIEXTREQ_CB_MAX];
  callback_t      *responsecb;
  song_t          *song;
  bool            isactive : 1;
} uiextreq_t;

/* external request */
static void   uiextreqCreateDialog (uiextreq_t *uiextreq);
static bool   uiextreqDanceSelectHandler (void *udata, long idx, int count);
static void   uiextreqInitDisplay (uiextreq_t *uiextreq, const char *fn);
static void   uiextreqClearSong (uiextreq_t *uiextreq);
static bool   uiextreqResponseHandler (void *udata, long responseid);
static void   uiextreqProcessAudioFile (uiextreq_t *uiextreq);
static int    uiextreqValidateAudioFile (uiwcont_t *entry, void *udata);
static int    uiextreqValidateArtist (uiwcont_t *entry, void *udata);
static int    uiextreqValidateTitle (uiwcont_t *entry, void *udata);

uiextreq_t *
uiextreqInit (uiwcont_t *windowp, musicdb_t *musicdb, nlist_t *opts)
{
  uiextreq_t  *uiextreq;

  uiextreq = mdmalloc (sizeof (uiextreq_t));
  uiextreq->extreqDialog = NULL;
  uiextreq->audioFileEntry = NULL;
  uiextreq->artistEntry = NULL;
  uiextreq->titleEntry = NULL;
  uiextreq->uidance = NULL;
  uiextreq->parentwin = windowp;
  uiextreq->statusMsg = NULL;
  uiextreq->options = opts;
  uiextreq->song = NULL;
  for (int i = 0; i < UIEXTREQ_CB_MAX; ++i) {
    uiextreq->callbacks [i] = NULL;
  }
  uiextreq->audioFileDialogButton = NULL;
  uiextreq->responsecb = NULL;
  uiextreq->isactive = false;
  uiextreq->musicdb = musicdb;

  selectInitCallback (&uiextreq->audiofilesfcb);
  uiextreq->audiofilesfcb.title = NULL;
  uiextreq->audiofilesfcb.defdir = nlistGetStr (uiextreq->options, REQ_EXT_DIR);
  uiextreq->audiofilesfcb.entry = uiextreq->audioFileEntry;
  uiextreq->audiofilesfcb.window = uiextreq->parentwin;

  return uiextreq;
}

void
uiextreqFree (uiextreq_t *uiextreq)
{
  if (uiextreq != NULL) {
    for (int i = 0; i < UIEXTREQ_CB_MAX; ++i) {
      callbackFree (uiextreq->callbacks [i]);
    }
    if (uiextreq->song != NULL) {
      songFree (uiextreq->song);
    }
    uiwcontFree (uiextreq->audioFileEntry);
    uiwcontFree (uiextreq->artistEntry);
    uiwcontFree (uiextreq->titleEntry);
    uiwcontFree (uiextreq->extreqDialog);
    uiwcontFree (uiextreq->audioFileDialogButton);
    uidanceFree (uiextreq->uidance);
    mdfree (uiextreq);
  }
}

void
uiextreqSetResponseCallback (uiextreq_t *uiextreq, callback_t *uicb)
{
  if (uiextreq == NULL) {
    return;
  }
  uiextreq->responsecb = uicb;
}

bool
uiextreqDialog (uiextreq_t *uiextreq, const char *fn)
{
  int         x, y;

  if (uiextreq == NULL) {
    return UICB_STOP;
  }

  logProcBegin (LOG_PROC, "uiextreqDialog");
  uiextreqCreateDialog (uiextreq);
  uiextreqInitDisplay (uiextreq, fn);
  uiDialogShow (uiextreq->extreqDialog);
  uiextreq->isactive = true;

  x = nlistGetNum (uiextreq->options, REQ_EXT_POSITION_X);
  y = nlistGetNum (uiextreq->options, REQ_EXT_POSITION_Y);
  uiWindowMove (uiextreq->extreqDialog, x, y, -1);
  logProcEnd (LOG_PROC, "uiextreqDialog", "");
  return UICB_CONT;
}

/* the caller must free the song */
song_t *
uiextreqGetSong (uiextreq_t *uiextreq)
{
  song_t    *song;

  if (uiextreq == NULL) {
    return NULL;
  }

  song = uiextreq->song;
  uiextreq->song = NULL;
  return song;
}

/* delayed entry validation for the audio file needs to be run */
void
uiextreqProcess (uiextreq_t *uiextreq)
{
  if (! uiextreq->isactive) {
    return;
  }

  uiEntryValidate (uiextreq->audioFileEntry, false);
}

/* internal routines */

static void
uiextreqCreateDialog (uiextreq_t *uiextreq)
{
  uiwcont_t     *vbox;
  uiwcont_t     *hbox;
  uiwcont_t     *uiwidgetp;
  uiwcont_t     *szgrp;  // labels
  uiwcont_t     *szgrpEntry; // title, artist

  logProcBegin (LOG_PROC, "uiextreqCreateDialog");

  if (uiextreq == NULL) {
    return;
  }

  if (uiextreq->extreqDialog != NULL) {
    return;
  }

  szgrp = uiCreateSizeGroupHoriz ();
  szgrpEntry = uiCreateSizeGroupHoriz ();

  uiextreq->callbacks [UIEXTREQ_CB_DIALOG] = callbackInitLong (
      uiextreqResponseHandler, uiextreq);
  uiextreq->extreqDialog = uiCreateDialog (uiextreq->parentwin,
      uiextreq->callbacks [UIEXTREQ_CB_DIALOG],
      /* CONTEXT: external request dialog: title for the dialog */
      _("External Request"),
      /* CONTEXT: external request dialog: closes the dialog */
      _("Close"),
      RESPONSE_CLOSE,
      /* CONTEXT: (verb) external request dialog: queue the selected file */
      _("Queue"),
      RESPONSE_APPLY,
      NULL
      );

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);
  uiWidgetExpandHoriz (vbox);
  uiWidgetExpandVert (vbox);
  uiDialogPackInDialog (uiextreq->extreqDialog, vbox);

  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  uiwidgetp = uiCreateColonLabel (
      /* CONTEXT: external request: enter the audio file location */
      _("Audio File"));
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (szgrp, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiEntryInit (50, MAXPATHLEN);
  uiEntrySetValue (uiwidgetp, "");
  uiWidgetAlignHorizFill (uiwidgetp);
  uiWidgetExpandHoriz (uiwidgetp);
  uiBoxPackStartExpand (hbox, uiwidgetp);
  uiextreq->audioFileEntry = uiwidgetp;

  uiextreq->callbacks [UIEXTREQ_CB_AUDIO_FILE] = callbackInit (
      selectAudioFileCallback, &uiextreq->audiofilesfcb, NULL);
  uiwidgetp = uiCreateButton (
      uiextreq->callbacks [UIEXTREQ_CB_AUDIO_FILE],
      "", NULL);
  uiButtonSetImageIcon (uiwidgetp, "folder");
  uiWidgetSetMarginStart (uiwidgetp, 0);
  uiBoxPackStart (hbox, uiwidgetp);
  uiextreq->audioFileDialogButton = uiwidgetp;

  /* artist display */
  uiwcontFree (hbox);
  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);

  uiwidgetp = uiCreateColonLabel (tagdefs [TAG_ARTIST].displayname);
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (szgrp, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiEntryInit (40, MAXPATHLEN);
  uiEntrySetValue (uiwidgetp, "");
  uiWidgetAlignHorizFill (uiwidgetp);
  uiBoxPackStart (hbox, uiwidgetp);
  uiextreq->artistEntry = uiwidgetp;

  /* title display */
  uiwcontFree (hbox);
  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);

  uiwidgetp = uiCreateColonLabel (tagdefs [TAG_TITLE].displayname);
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (szgrp, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiEntryInit (40, MAXPATHLEN);
  uiEntrySetValue (uiwidgetp, "");
  uiWidgetAlignHorizFill (uiwidgetp);
  uiBoxPackStart (hbox, uiwidgetp);
  uiextreq->titleEntry = uiwidgetp;

  /* dance : always available */
  uiwcontFree (hbox);
  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);

  uiwidgetp = uiCreateColonLabel (tagdefs [TAG_DANCE].displayname);
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (szgrp, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiextreq->callbacks [UIEXTREQ_CB_DANCE] = callbackInitLongInt (
      uiextreqDanceSelectHandler, uiextreq);
  uiextreq->uidance = uidanceDropDownCreate (hbox, uiextreq->extreqDialog,
      /* CONTEXT: external request: dance drop-down */
      UIDANCE_EMPTY_DANCE, _("Select Dance"), UIDANCE_PACK_START, 1);
  uidanceSetCallback (uiextreq->uidance, uiextreq->callbacks [UIEXTREQ_CB_DANCE]);

  uiwcontFree (hbox);
  uiwcontFree (vbox);
  uiwcontFree (szgrp);
  uiwcontFree (szgrpEntry);

  uiEntrySetValidate (uiextreq->audioFileEntry, uiextreqValidateAudioFile,
      uiextreq, UIENTRY_DELAYED);
  uiEntrySetValidate (uiextreq->artistEntry, uiextreqValidateArtist,
      uiextreq, UIENTRY_IMMEDIATE);
  uiEntrySetValidate (uiextreq->titleEntry, uiextreqValidateTitle,
      uiextreq, UIENTRY_IMMEDIATE);

  logProcEnd (LOG_PROC, "uiextreqCreateDialog", "");
}

/* count is not used */
static bool
uiextreqDanceSelectHandler (void *udata, long idx, int count)
{
  uiextreq_t  *uiextreq = udata;

  if (uiextreq->song != NULL) {
    songSetNum (uiextreq->song, TAG_DANCE, idx);
  }
  return UICB_CONT;
}

static void
uiextreqInitDisplay (uiextreq_t *uiextreq, const char *fn)
{
  if (uiextreq == NULL) {
    return;
  }

  uiextreqClearSong (uiextreq);
  /* the validation routine will process the entry */
  if (fn != NULL) {
    uiEntrySetValue (uiextreq->audioFileEntry, fn);
  } else {
    uiEntrySetValue (uiextreq->audioFileEntry, "");
  }
  uiEntrySetValue (uiextreq->artistEntry, "");
  uiEntrySetValue (uiextreq->titleEntry, "");
  uidanceSetValue (uiextreq->uidance, -1);
}

static void
uiextreqClearSong (uiextreq_t *uiextreq)
{
  if (uiextreq == NULL) {
    return;
  }

  if (uiextreq->song != NULL) {
    songFree (uiextreq->song);
    uiextreq->song = NULL;
  }
}

static bool
uiextreqResponseHandler (void *udata, long responseid)
{
  uiextreq_t  *uiextreq = udata;
  int         x, y, ws;

  uiWindowGetPosition (uiextreq->extreqDialog, &x, &y, &ws);
  nlistSetNum (uiextreq->options, REQ_EXT_POSITION_X, x);
  nlistSetNum (uiextreq->options, REQ_EXT_POSITION_Y, y);

  switch (responseid) {
    case RESPONSE_DELETE_WIN: {
      logMsg (LOG_DBG, LOG_ACTIONS, "= action: extreq: del window");
      uiwcontFree (uiextreq->extreqDialog);
      uiextreq->extreqDialog = NULL;
      uiextreqClearSong (uiextreq);
      break;
    }
    case RESPONSE_CLOSE: {
      logMsg (LOG_DBG, LOG_ACTIONS, "= action: extreq: close window");
      uiWidgetHide (uiextreq->extreqDialog);
      uiextreqClearSong (uiextreq);
      break;
    }
    case RESPONSE_APPLY: {
      logMsg (LOG_DBG, LOG_ACTIONS, "= action: extreq: apply");
      uiWidgetHide (uiextreq->extreqDialog);
      if (uiextreq->responsecb != NULL) {
        callbackHandler (uiextreq->responsecb);
      }
      break;
    }
  }

  uiextreq->isactive = false;
  return UICB_CONT;
}

static void
uiextreqProcessAudioFile (uiextreq_t *uiextreq)
{
  const char  *ffn;

  if (uiextreq == NULL) {
    return;
  }

  uiextreqClearSong (uiextreq);
  ffn = uiEntryGetValue (uiextreq->audioFileEntry);
  if (*ffn) {
    if (fileopFileExists (ffn)) {
      const char      *tfn;
      slist_t         *tagdata;
      int             rewrite;
      song_t          *dbsong;

      tfn = audiosrcRelativePath (ffn, 0);
      dbsong = dbGetByName (uiextreq->musicdb, tfn);
      if (dbsong != NULL) {
        tagdata = songTagList (dbsong);
        if (slistGetCount (tagdata) == 0) {
          slistFree (tagdata);
          return;
        }
      } else {
        tagdata = audiotagParseData (ffn, &rewrite);
        if (slistGetCount (tagdata) == 0) {
          slistFree (tagdata);
          return;
        }
      }

      /* set favorite to the imported symbol */
      /* do this in the tag-data so that the song-entry text will have */
      /* the change */
      slistSetStr (tagdata, tagdefs [TAG_FAVORITE].tag, "imported");

      uiextreq->song = songAlloc ();
      /* populate the song from the tag data */
      songFromTagList (uiextreq->song, tagdata);
      songSetStr (uiextreq->song, TAG_URI, ffn);
      songSetNum (uiextreq->song, TAG_DB_FLAGS, MUSICDB_TEMP);

      slistFree (tagdata);

      /* update the display */
      uiEntrySetValue (uiextreq->artistEntry,
          songGetStr (uiextreq->song, TAG_ARTIST));
      uiEntrySetValue (uiextreq->titleEntry,
          songGetStr (uiextreq->song, TAG_TITLE));
      uidanceSetValue (uiextreq->uidance,
          songGetNum (uiextreq->song, TAG_DANCE));
    }
  }
}

static int
uiextreqValidateAudioFile (uiwcont_t *entry, void *udata)
{
  uiextreq_t  *uiextreq = udata;
  int         rc;

  rc = uiEntryValidateFile (entry, NULL);
  if (rc == UIENTRY_OK) {
    pathinfo_t    *pi;
    const char    *fn;
    char          tmp [MAXPATHLEN];

    fn = uiEntryGetValue (uiextreq->audioFileEntry);
    pi = pathInfo (fn);
    pathInfoGetDir (pi, tmp, sizeof (tmp));
    nlistSetStr (uiextreq->options, REQ_EXT_DIR, tmp);
    pathInfoFree (pi);
    uiextreq->audiofilesfcb.defdir = nlistGetStr (uiextreq->options, REQ_EXT_DIR);
    uiextreqProcessAudioFile (uiextreq);
  }

  return rc;
}

static int
uiextreqValidateArtist (uiwcont_t *entry, void *udata)
{
  uiextreq_t  *uiextreq = udata;
  const char  *str;

  uiLabelSetText (uiextreq->statusMsg, "");
  str = uiEntryGetValue (entry);
  if (uiextreq->song != NULL) {
    songSetStr (uiextreq->song, TAG_ARTIST, str);
  }
  return UIENTRY_OK;
}

static int
uiextreqValidateTitle (uiwcont_t *entry, void *udata)
{
  uiextreq_t  *uiextreq = udata;
  const char  *str;

  uiLabelSetText (uiextreq->statusMsg, "");
  str = uiEntryGetValue (entry);
  if (uiextreq->song != NULL) {
    songSetStr (uiextreq->song, TAG_TITLE, str);
  }
  return UIENTRY_OK;
}

