/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>
#include <math.h>

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
#include "slist.h"
#include "song.h"
#include "songfav.h"
#include "tagdef.h"
#include "ui.h"
#include "callback.h"
#include "uidance.h"
#include "uireqext.h"

enum {
  UIREQEXT_CB_DIALOG,
  UIREQEXT_CB_DANCE,
  UIREQEXT_CB_AUDIO_FILE,
  UIREQEXT_CB_MAX,
};

typedef struct uireqext {
  uiwcont_t      *parentwin;
  uiwcont_t      *statusMsg;
  nlist_t         *options;
  uiwcont_t       *reqextDialog;
  uientry_t       *audioFileEntry;
  uientry_t       *artistEntry;
  uientry_t       *titleEntry;
  uidance_t       *uidance;
  uibutton_t      *audioFileDialogButton;
  callback_t      *callbacks [UIREQEXT_CB_MAX];
  callback_t      *responsecb;
  song_t          *song;
  char            *songEntryText;
  bool            isactive : 1;
} uireqext_t;

/* request external */
static void   uireqextCreateDialog (uireqext_t *uireqext);
static bool   uireqextAudioFileDialog (void *udata);
static bool   uireqextDanceSelectHandler (void *udata, long idx, int count);
static void   uireqextInitDisplay (uireqext_t *uireqext);
static void   uireqextClearSong (uireqext_t *uireqext);
static bool   uireqextResponseHandler (void *udata, long responseid);
static void   uireqextProcessAudioFile (uireqext_t *uireqext);
static int    uireqextValidateAudioFile (uientry_t *entry, void *udata);
static int    uireqextValidateArtist (uientry_t *entry, void *udata);
static int    uireqextValidateTitle (uientry_t *entry, void *udata);

uireqext_t *
uireqextInit (uiwcont_t *windowp, nlist_t *opts)
{
  uireqext_t  *uireqext;

  uireqext = mdmalloc (sizeof (uireqext_t));
  uireqext->reqextDialog = NULL;
  uireqext->audioFileEntry = uiEntryInit (50, MAXPATHLEN);
  uireqext->artistEntry = uiEntryInit (40, MAXPATHLEN);
  uireqext->titleEntry = uiEntryInit (40, MAXPATHLEN);
  uireqext->uidance = NULL;
  uireqext->parentwin = windowp;
  uireqext->statusMsg = NULL;
  uireqext->options = opts;
  uireqext->song = NULL;
  uireqext->songEntryText = NULL;
  for (int i = 0; i < UIREQEXT_CB_MAX; ++i) {
    uireqext->callbacks [i] = NULL;
  }
  uireqext->audioFileDialogButton = NULL;
  uireqext->responsecb = NULL;
  uireqext->isactive = false;

  return uireqext;
}

void
uireqextFree (uireqext_t *uireqext)
{
  if (uireqext != NULL) {
    for (int i = 0; i < UIREQEXT_CB_MAX; ++i) {
      callbackFree (uireqext->callbacks [i]);
    }
    if (uireqext->song != NULL) {
      songFree (uireqext->song);
    }
    dataFree (uireqext->songEntryText);
    uiEntryFree (uireqext->audioFileEntry);
    uiEntryFree (uireqext->artistEntry);
    uiEntryFree (uireqext->titleEntry);
    uiDialogDestroy (uireqext->reqextDialog);
    uiwcontFree (uireqext->reqextDialog);
    uiButtonFree (uireqext->audioFileDialogButton);
    uidanceFree (uireqext->uidance);
    mdfree (uireqext);
  }
}

void
uireqextSetResponseCallback (uireqext_t *uireqext, callback_t *uicb)
{
  if (uireqext == NULL) {
    return;
  }
  uireqext->responsecb = uicb;
}

bool
uireqextDialog (uireqext_t *uireqext)
{
  int         x, y;

  if (uireqext == NULL) {
    return UICB_STOP;
  }

  logProcBegin (LOG_PROC, "uireqextDialog");
  uireqextCreateDialog (uireqext);
  uireqextInitDisplay (uireqext);
  uiDialogShow (uireqext->reqextDialog);
  uireqext->isactive = true;

  x = nlistGetNum (uireqext->options, REQ_EXT_POSITION_X);
  y = nlistGetNum (uireqext->options, REQ_EXT_POSITION_Y);
  uiWindowMove (uireqext->reqextDialog, x, y, -1);
  logProcEnd (LOG_PROC, "uireqextDialog", "");
  return UICB_CONT;
}

song_t *
uireqextGetSong (uireqext_t *uireqext)
{
  song_t    *song;

  if (uireqext == NULL) {
    return NULL;
  }

  song = uireqext->song;
  /* it is the caller's responsibility to free the song */
  uireqext->song = NULL;
  return song;
}

char *
uireqextGetSongEntryText (uireqext_t *uireqext)
{
  if (uireqext == NULL) {
    return NULL;
  }

  return uireqext->songEntryText;
}

/* delayed entry validation for the audio file needs to be run */
void
uireqextProcess (uireqext_t *uireqext)
{
  if (! uireqext->isactive) {
    return;
  }

  uiEntryValidate (uireqext->audioFileEntry, false);
}

/* internal routines */

static void
uireqextCreateDialog (uireqext_t *uireqext)
{
  uiwcont_t     vbox;
  uiwcont_t     hbox;
  uiwcont_t     uiwidget;
  uibutton_t    *uibutton;
  uiwcont_t     *uiwidgetp;
  uiwcont_t     *szgrp;  // labels
  uiwcont_t     *szgrpEntry; // title, artist

  logProcBegin (LOG_PROC, "uireqextCreateDialog");

  if (uireqext == NULL) {
    return;
  }

  if (uireqext->reqextDialog != NULL) {
    return;
  }

  szgrp = uiCreateSizeGroupHoriz ();
  szgrpEntry = uiCreateSizeGroupHoriz ();

  uireqext->callbacks [UIREQEXT_CB_DIALOG] = callbackInitLong (
      uireqextResponseHandler, uireqext);
  uireqext->reqextDialog = uiCreateDialog (uireqext->parentwin,
      uireqext->callbacks [UIREQEXT_CB_DIALOG],
      /* CONTEXT: request external dialog: title for the dialog */
      _("Select Audio File"),
      /* CONTEXT: request external dialog: closes the dialog */
      _("Close"),
      RESPONSE_CLOSE,
      /* CONTEXT: (verb) request external dialog: queue the selected file */
      _("Queue"),
      RESPONSE_APPLY,
      NULL
      );

  uiCreateVertBox (&vbox);
  uiWidgetSetAllMargins (&vbox, 10);
  uiWidgetSetMarginTop (&vbox, 20);
  uiWidgetExpandHoriz (&vbox);
  uiWidgetExpandVert (&vbox);
  uiDialogPackInDialog (uireqext->reqextDialog, &vbox);

  uiCreateHorizBox (&hbox);
  uiWidgetExpandHoriz (&hbox);
  uiBoxPackStart (&vbox, &hbox);

  uiCreateColonLabel (&uiwidget,
      /* CONTEXT: request external: enter the audio file location */
      _("Audio File"));
  uiBoxPackStart (&hbox, &uiwidget);
  uiSizeGroupAdd (szgrp, &uiwidget);

  uiEntryCreate (uireqext->audioFileEntry);
  uiEntrySetValue (uireqext->audioFileEntry, "");
  uiwidgetp = uiEntryGetWidgetContainer (uireqext->audioFileEntry);
  uiWidgetAlignHorizFill (uiwidgetp);
  uiWidgetExpandHoriz (uiwidgetp);
  uiBoxPackStartExpand (&hbox, uiwidgetp);
  uiEntrySetValidate (uireqext->audioFileEntry, uireqextValidateAudioFile,
      uireqext, UIENTRY_DELAYED);

  uireqext->callbacks [UIREQEXT_CB_AUDIO_FILE] = callbackInit (
      uireqextAudioFileDialog, uireqext, NULL);
  uibutton = uiCreateButton (
      uireqext->callbacks [UIREQEXT_CB_AUDIO_FILE],
      "", NULL);
  uireqext->audioFileDialogButton = uibutton;
  uiwidgetp = uiButtonGetWidgetContainer (uibutton);
  uiButtonSetImageIcon (uibutton, "folder");
  uiWidgetSetMarginStart (uiwidgetp, 0);
  uiBoxPackStart (&hbox, uiwidgetp);

  /* artist display */
  uiCreateHorizBox (&hbox);
  uiBoxPackStart (&vbox, &hbox);

  uiCreateColonLabel (&uiwidget, tagdefs [TAG_ARTIST].displayname);
  uiBoxPackStart (&hbox, &uiwidget);
  uiSizeGroupAdd (szgrp, &uiwidget);

  uiEntryCreate (uireqext->artistEntry);
  uiEntrySetValue (uireqext->artistEntry, "");
  uiwidgetp = uiEntryGetWidgetContainer (uireqext->artistEntry);
  uiWidgetAlignHorizFill (uiwidgetp);
  uiBoxPackStart (&hbox, uiwidgetp);
  uiEntrySetValidate (uireqext->artistEntry, uireqextValidateArtist,
      uireqext, UIENTRY_IMMEDIATE);

  /* title display */
  uiCreateHorizBox (&hbox);
  uiBoxPackStart (&vbox, &hbox);

  uiCreateColonLabel (&uiwidget, tagdefs [TAG_TITLE].displayname);
  uiBoxPackStart (&hbox, &uiwidget);
  uiSizeGroupAdd (szgrp, &uiwidget);

  uiEntryCreate (uireqext->titleEntry);
  uiEntrySetValue (uireqext->titleEntry, "");
  uiwidgetp = uiEntryGetWidgetContainer (uireqext->titleEntry);
  uiWidgetAlignHorizFill (uiwidgetp);
  uiBoxPackStart (&hbox, uiwidgetp);
  uiEntrySetValidate (uireqext->titleEntry, uireqextValidateTitle,
      uireqext, UIENTRY_IMMEDIATE);

  /* dance : always available */
  uiCreateHorizBox (&hbox);
  uiBoxPackStart (&vbox, &hbox);

  uiCreateColonLabel (&uiwidget, tagdefs [TAG_DANCE].displayname);
  uiBoxPackStart (&hbox, &uiwidget);
  uiSizeGroupAdd (szgrp, &uiwidget);

  uireqext->callbacks [UIREQEXT_CB_DANCE] = callbackInitLongInt (
      uireqextDanceSelectHandler, uireqext);
  uireqext->uidance = uidanceDropDownCreate (&hbox, uireqext->reqextDialog,
      /* CONTEXT: request external: dance drop-down */
      UIDANCE_EMPTY_DANCE, _("Select Dance"), UIDANCE_PACK_START, 1);
  uidanceSetCallback (uireqext->uidance, uireqext->callbacks [UIREQEXT_CB_DANCE]);

  uiwcontFree (szgrp);
  uiwcontFree (szgrpEntry);

  logProcEnd (LOG_PROC, "uireqextCreateDialog", "");
}

static bool
uireqextAudioFileDialog (void *udata)
{
  uireqext_t  *uireqext = udata;
  char        *fn = NULL;
  uiselect_t  *selectdata;

  if (uireqext == NULL) {
    return UICB_STOP;
  }

  selectdata = uiDialogCreateSelect (uireqext->parentwin,
      /* CONTEXT: request external: file selection dialog: window title */
      _("Select File"),
      bdjoptGetStr (OPT_M_DIR_MUSIC),
      NULL,
      /* CONTEXT: request external: file selection dialog: audio file filter */
      _("Audio Files"), "audio/*");
  fn = uiSelectFileDialog (selectdata);
  if (fn != NULL) {
    uiEntrySetValue (uireqext->audioFileEntry, fn);
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "selected loc: %s", fn);
    uireqextProcessAudioFile (uireqext);
    mdfree (fn);   // allocated by gtk
  }
  mdfree (selectdata);

  return UICB_CONT;
}

/* count is not used */
static bool
uireqextDanceSelectHandler (void *udata, long idx, int count)
{
  uireqext_t  *uireqext = udata;

  if (uireqext->song != NULL) {
    songSetNum (uireqext->song, TAG_DANCE, idx);
  }
  return UICB_CONT;
}

static void
uireqextInitDisplay (uireqext_t *uireqext)
{
  if (uireqext == NULL) {
    return;
  }

  uireqextClearSong (uireqext);
  uiEntrySetValue (uireqext->audioFileEntry, "");
  uiEntrySetValue (uireqext->artistEntry, "");
  uiEntrySetValue (uireqext->titleEntry, "");
  uidanceSetValue (uireqext->uidance, -1);
}

static void
uireqextClearSong (uireqext_t *uireqext)
{
  if (uireqext == NULL) {
    return;
  }

  if (uireqext->song != NULL) {
    songFree (uireqext->song);
    uireqext->song = NULL;
  }
}

static bool
uireqextResponseHandler (void *udata, long responseid)
{
  uireqext_t  *uireqext = udata;
  int         x, y, ws;

  uiWindowGetPosition (uireqext->reqextDialog, &x, &y, &ws);
  nlistSetNum (uireqext->options, REQ_EXT_POSITION_X, x);
  nlistSetNum (uireqext->options, REQ_EXT_POSITION_Y, y);

  switch (responseid) {
    case RESPONSE_DELETE_WIN: {
      logMsg (LOG_DBG, LOG_ACTIONS, "= action: reqext: del window");
      uiwcontFree (uireqext->reqextDialog);
      uireqext->reqextDialog = NULL;
      uireqextClearSong (uireqext);
      break;
    }
    case RESPONSE_CLOSE: {
      logMsg (LOG_DBG, LOG_ACTIONS, "= action: reqext: close window");
      uiWidgetHide (uireqext->reqextDialog);
      uireqextClearSong (uireqext);
      break;
    }
    case RESPONSE_APPLY: {
      logMsg (LOG_DBG, LOG_ACTIONS, "= action: reqext: apply");
      uiWidgetHide (uireqext->reqextDialog);
      if (uireqext->responsecb != NULL) {
        callbackHandler (uireqext->responsecb);
      }
      break;
    }
  }

  uireqext->isactive = false;
  return UICB_CONT;
}

static void
uireqextProcessAudioFile (uireqext_t *uireqext)
{
  const char  *ffn;
  char        tbuff [3096];

  if (uireqext == NULL) {
    return;
  }

  uireqextClearSong (uireqext);
  ffn = uiEntryGetValue (uireqext->audioFileEntry);
  if (*ffn) {
    if (fileopFileExists (ffn)) {
      char            *data;
      slist_t         *tagdata;
      int             rewrite;

      data = audiotagReadTags (ffn);
      if (data == NULL) {
        return;
      }

      tagdata = audiotagParseData (ffn, data, &rewrite);
      mdfree (data);
      if (slistGetCount (tagdata) == 0) {
        slistFree (tagdata);
        return;
      }

      /* set favorite to the imported symbol */
      /* do this in the tag-data so that the song-entry text will have */
      /* the change */
      slistSetStr (tagdata, tagdefs [TAG_FAVORITE].tag, "imported");

      dbCreateSongEntryFromTags (tbuff, sizeof (tbuff), tagdata,
          ffn, MUSICDB_ENTRY_NEW);
      slistFree (tagdata);
      dataFree (uireqext->songEntryText);
      uireqext->songEntryText = mdstrdup (tbuff);

      uireqext->song = songAlloc ();
      /* populate the song from the tag data */
      songParse (uireqext->song, tbuff, 0);
      songSetNum (uireqext->song, TAG_TEMPORARY, true);

      /* update the display */
      uiEntrySetValue (uireqext->artistEntry,
          songGetStr (uireqext->song, TAG_ARTIST));
      uiEntrySetValue (uireqext->titleEntry,
          songGetStr (uireqext->song, TAG_TITLE));
      uidanceSetValue (uireqext->uidance,
          songGetNum (uireqext->song, TAG_DANCE));
    }
  }
}

static int
uireqextValidateAudioFile (uientry_t *entry, void *udata)
{
  uireqext_t  *uireqext = udata;
  int         rc;

  rc = uiEntryValidateFile (entry, NULL);
  if (rc == UIENTRY_OK) {
    uireqextProcessAudioFile (uireqext);
  }

  return rc;
}

static int
uireqextValidateArtist (uientry_t *entry, void *udata)
{
  uireqext_t  *uireqext = udata;
  const char  *str;

  if (uireqext->statusMsg != NULL) {
    uiLabelSetText (uireqext->statusMsg, "");
  }
  str = uiEntryGetValue (entry);
  if (uireqext->song != NULL) {
    songSetStr (uireqext->song, TAG_ARTIST, str);
  }
  return UIENTRY_OK;
}

static int
uireqextValidateTitle (uientry_t *entry, void *udata)
{
  uireqext_t  *uireqext = udata;
  const char  *str;

  if (uireqext->statusMsg != NULL) {
    uiLabelSetText (uireqext->statusMsg, "");
  }
  str = uiEntryGetValue (entry);
  if (uireqext->song != NULL) {
    songSetStr (uireqext->song, TAG_TITLE, str);
  }
  return UIENTRY_OK;
}

