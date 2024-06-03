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

enum {
  UIEXTREQ_W_WINDOW,
  UIEXTREQ_W_DIALOG,
  UIEXTREQ_W_AUDIO_FILE,
  UIEXTREQ_W_AUDIO_FILE_CHOOSER,
  UIEXTREQ_W_ARTIST,
  UIEXTREQ_W_TITLE,
  UIEXTREQ_W_MQ_DISP,
  UIEXTREQ_W_MAX,
};

typedef struct uiextreq {
  uiwcont_t       *wcont [UIEXTREQ_W_MAX];
  musicdb_t       *musicdb;
  nlist_t         *options;
  uisfcb_t        audiofilesfcb;
  uidance_t       *uidance;
  callback_t      *callbacks [UIEXTREQ_CB_MAX];
  callback_t      *responsecb;
  song_t          *song;
  bool            isactive : 1;
} uiextreq_t;

/* external request */
static void   uiextreqCreateDialog (uiextreq_t *uiextreq);
static int    uiextreqDanceSelectHandler (void *udata, long idx, int count);
static void   uiextreqInitDisplay (uiextreq_t *uiextreq, const char *fn);
static void   uiextreqClearSong (uiextreq_t *uiextreq);
static bool   uiextreqResponseHandler (void *udata, long responseid);
static void   uiextreqProcessAudioFile (uiextreq_t *uiextreq);
static int    uiextreqValidateAudioFile (uiwcont_t *entry, void *udata);
static int    uiextreqValidateArtist (uiwcont_t *entry, void *udata);
static int    uiextreqValidateTitle (uiwcont_t *entry, void *udata);
static int    uiextreqValidateMQDisplay (uiwcont_t *entry, void *udata);

uiextreq_t *
uiextreqInit (uiwcont_t *windowp, musicdb_t *musicdb, nlist_t *opts)
{
  uiextreq_t  *uiextreq;

  uiextreq = mdmalloc (sizeof (uiextreq_t));
  for (int i = 0; i < UIEXTREQ_W_MAX; ++i) {
    uiextreq->wcont [i] = NULL;
  }
  uiextreq->uidance = NULL;
  uiextreq->wcont [UIEXTREQ_W_WINDOW] = windowp;
  uiextreq->options = opts;
  uiextreq->song = NULL;
  for (int i = 0; i < UIEXTREQ_CB_MAX; ++i) {
    uiextreq->callbacks [i] = NULL;
  }
  uiextreq->responsecb = NULL;
  uiextreq->isactive = false;
  uiextreq->musicdb = musicdb;

  selectInitCallback (&uiextreq->audiofilesfcb);

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
    for (int i = 0; i < UIEXTREQ_W_MAX; ++i) {
      if (i == UIEXTREQ_W_WINDOW) {
        /* not owner of window */
        continue;
      }
      uiwcontFree (uiextreq->wcont [i]);
    }
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

  logProcBegin ();
  uiextreqCreateDialog (uiextreq);
  uiextreqInitDisplay (uiextreq, fn);
  uiDialogShow (uiextreq->wcont [UIEXTREQ_W_DIALOG]);
  uiextreq->isactive = true;

  x = nlistGetNum (uiextreq->options, REQ_EXT_POSITION_X);
  y = nlistGetNum (uiextreq->options, REQ_EXT_POSITION_Y);
  uiWindowMove (uiextreq->wcont [UIEXTREQ_W_DIALOG], x, y, -1);
  logProcEnd ("");
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

  uiEntryValidate (uiextreq->wcont [UIEXTREQ_W_AUDIO_FILE], false);
  uiEntryValidate (uiextreq->wcont [UIEXTREQ_W_ARTIST], false);
  uiEntryValidate (uiextreq->wcont [UIEXTREQ_W_TITLE], false);
  uiEntryValidate (uiextreq->wcont [UIEXTREQ_W_MQ_DISP], false);
}

/* internal routines */

static void
uiextreqCreateDialog (uiextreq_t *uiextreq)
{
  uiwcont_t     *vbox;
  uiwcont_t     *hbox;
  uiwcont_t     *uiwidgetp;
  uiwcont_t     *szgrp;  // labels
  uiwcont_t     *szgrpEntry; // title, artist, mq-display
  const char    *tstr;

  logProcBegin ();

  if (uiextreq == NULL) {
    return;
  }

  if (uiextreq->wcont [UIEXTREQ_W_DIALOG] != NULL) {
    return;
  }

  szgrp = uiCreateSizeGroupHoriz ();
  szgrpEntry = uiCreateSizeGroupHoriz ();

  uiextreq->callbacks [UIEXTREQ_CB_DIALOG] = callbackInitLong (
      uiextreqResponseHandler, uiextreq);
  uiextreq->wcont [UIEXTREQ_W_DIALOG] = uiCreateDialog (uiextreq->wcont [UIEXTREQ_W_WINDOW],
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
  uiDialogPackInDialog (uiextreq->wcont [UIEXTREQ_W_DIALOG], vbox);

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
  uiextreq->wcont [UIEXTREQ_W_AUDIO_FILE] = uiwidgetp;

  uiextreq->audiofilesfcb.title = NULL;
  tstr = nlistGetStr (uiextreq->options, EXT_REQ_DIR);
  uiextreq->audiofilesfcb.defdir = tstr;
  uiextreq->audiofilesfcb.entry = uiextreq->wcont [UIEXTREQ_W_AUDIO_FILE];
  uiextreq->audiofilesfcb.window = uiextreq->wcont [UIEXTREQ_W_WINDOW];

  uiextreq->callbacks [UIEXTREQ_CB_AUDIO_FILE] = callbackInit (
      selectAudioFileCallback, &uiextreq->audiofilesfcb, NULL);
  uiwidgetp = uiCreateButton (
      uiextreq->callbacks [UIEXTREQ_CB_AUDIO_FILE],
      "", NULL);
  uiButtonSetImageIcon (uiwidgetp, "folder");
  uiWidgetSetMarginStart (uiwidgetp, 0);
  uiBoxPackStart (hbox, uiwidgetp);
  uiextreq->wcont [UIEXTREQ_W_AUDIO_FILE_CHOOSER] = uiwidgetp;

  uiwcontFree (hbox);

  /* artist display */
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
  uiextreq->wcont [UIEXTREQ_W_ARTIST] = uiwidgetp;

  uiwcontFree (hbox);

  /* title display */
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
  uiextreq->wcont [UIEXTREQ_W_TITLE] = uiwidgetp;

  uiwcontFree (hbox);

  /* dance : always available */
  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);

  uiwidgetp = uiCreateColonLabel (tagdefs [TAG_DANCE].displayname);
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (szgrp, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiextreq->callbacks [UIEXTREQ_CB_DANCE] = callbackInitLongInt (
      uiextreqDanceSelectHandler, uiextreq);
  uiextreq->uidance = uidanceDropDownCreate (hbox, uiextreq->wcont [UIEXTREQ_W_DIALOG],
      /* CONTEXT: external request: dance drop-down */
      UIDANCE_EMPTY_DANCE, _("Select Dance"), UIDANCE_PACK_START, 1);
  uidanceSetCallback (uiextreq->uidance, uiextreq->callbacks [UIEXTREQ_CB_DANCE]);

  uiwcontFree (hbox);

  /* marquee display */
  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);

  uiwidgetp = uiCreateColonLabel (tagdefs [TAG_MQDISPLAY].displayname);
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (szgrp, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiEntryInit (40, MAXPATHLEN);
  uiEntrySetValue (uiwidgetp, "");
  uiWidgetAlignHorizFill (uiwidgetp);
  uiBoxPackStart (hbox, uiwidgetp);
  uiextreq->wcont [UIEXTREQ_W_MQ_DISP] = uiwidgetp;

  uiwcontFree (hbox);

  uiwcontFree (vbox);
  uiwcontFree (szgrp);
  uiwcontFree (szgrpEntry);

  uiEntrySetValidate (uiextreq->wcont [UIEXTREQ_W_AUDIO_FILE],
      uiextreqValidateAudioFile, uiextreq, UIENTRY_DELAYED);
  uiEntrySetValidate (uiextreq->wcont [UIEXTREQ_W_ARTIST],
      uiextreqValidateArtist, uiextreq, UIENTRY_IMMEDIATE);
  uiEntrySetValidate (uiextreq->wcont [UIEXTREQ_W_TITLE],
      uiextreqValidateTitle, uiextreq, UIENTRY_IMMEDIATE);
  uiEntrySetValidate (uiextreq->wcont [UIEXTREQ_W_MQ_DISP],
      uiextreqValidateMQDisplay, uiextreq, UIENTRY_IMMEDIATE);

  logProcEnd ("");
}

/* count is not used */
static int
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
    uiEntrySetValue (uiextreq->wcont [UIEXTREQ_W_AUDIO_FILE], fn);
  } else {
    uiEntrySetValue (uiextreq->wcont [UIEXTREQ_W_AUDIO_FILE], "");
  }
  uiEntrySetValue (uiextreq->wcont [UIEXTREQ_W_ARTIST], "");
  uiEntrySetValue (uiextreq->wcont [UIEXTREQ_W_TITLE], "");
  uiEntrySetValue (uiextreq->wcont [UIEXTREQ_W_MQ_DISP], "");
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

  uiWindowGetPosition (uiextreq->wcont [UIEXTREQ_W_DIALOG], &x, &y, &ws);
  nlistSetNum (uiextreq->options, REQ_EXT_POSITION_X, x);
  nlistSetNum (uiextreq->options, REQ_EXT_POSITION_Y, y);

  switch (responseid) {
    case RESPONSE_DELETE_WIN: {
      logMsg (LOG_DBG, LOG_ACTIONS, "= action: extreq: del window");
      uiwcontFree (uiextreq->wcont [UIEXTREQ_W_DIALOG]);
      uiextreq->wcont [UIEXTREQ_W_DIALOG] = NULL;
      uiextreqClearSong (uiextreq);
      break;
    }
    case RESPONSE_CLOSE: {
      logMsg (LOG_DBG, LOG_ACTIONS, "= action: extreq: close window");
      uiWidgetHide (uiextreq->wcont [UIEXTREQ_W_DIALOG]);
      uiextreqClearSong (uiextreq);
      break;
    }
    case RESPONSE_APPLY: {
      logMsg (LOG_DBG, LOG_ACTIONS, "= action: extreq: apply");
      uiWidgetHide (uiextreq->wcont [UIEXTREQ_W_DIALOG]);
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
  ffn = uiEntryGetValue (uiextreq->wcont [UIEXTREQ_W_AUDIO_FILE]);
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
      uiEntrySetValue (uiextreq->wcont [UIEXTREQ_W_ARTIST],
          songGetStr (uiextreq->song, TAG_ARTIST));
      uiEntrySetValue (uiextreq->wcont [UIEXTREQ_W_TITLE],
          songGetStr (uiextreq->song, TAG_TITLE));
      uiEntrySetValue (uiextreq->wcont [UIEXTREQ_W_MQ_DISP],
          songGetStr (uiextreq->song, TAG_MQDISPLAY));
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

    fn = uiEntryGetValue (uiextreq->wcont [UIEXTREQ_W_AUDIO_FILE]);
    if (*fn) {
      pi = pathInfo (fn);
      pathInfoGetDir (pi, tmp, sizeof (tmp));
      nlistSetStr (uiextreq->options, EXT_REQ_DIR, tmp);
      pathInfoFree (pi);
    }
    uiextreq->audiofilesfcb.defdir = nlistGetStr (uiextreq->options, EXT_REQ_DIR);
    uiextreqProcessAudioFile (uiextreq);
  }

  return rc;
}

static int
uiextreqValidateArtist (uiwcont_t *entry, void *udata)
{
  uiextreq_t  *uiextreq = udata;
  const char  *str;

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

  str = uiEntryGetValue (entry);
  if (uiextreq->song != NULL) {
    songSetStr (uiextreq->song, TAG_TITLE, str);
  }
  return UIENTRY_OK;
}

static int
uiextreqValidateMQDisplay (uiwcont_t *entry, void *udata)
{
  uiextreq_t  *uiextreq = udata;
  const char  *str;

  str = uiEntryGetValue (entry);
  if (uiextreq->song != NULL) {
    songSetStr (uiextreq->song, TAG_MQDISPLAY, str);
  }
  return UIENTRY_OK;
}

