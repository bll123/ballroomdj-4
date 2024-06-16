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
#include "bdj4intl.h"
#include "bdj4ui.h"
#include "bdjopt.h"
#include "bdjstring.h"      // needed for snprintf macro
#include "bdjvarsdf.h"
#include "callback.h"
#include "conn.h"
#include "dance.h"
#include "level.h"
#include "log.h"
#include "mdebug.h"
#include "nlist.h"
#include "pathbld.h"
#include "pathdisp.h"
#include "slist.h"
#include "song.h"
#include "songutil.h"
#include "sysvars.h"
#include "tagdef.h"
#include "tmutil.h"
#include "ui.h"
#include "uidance.h"
#include "uifavorite.h"
#include "uigenre.h"
#include "uilevel.h"
#include "uirating.h"
#include "uistatus.h"
#include "uisong.h"
#include "uisongsel.h"
#include "uisongedit.h"
#include "validate.h"

enum {
  UISE_CHK_NONE,
  UISE_CHK_NUM,
  UISE_CHK_DOUBLE,
  UISE_CHK_STR,
  UISE_CHK_LIST,
};

enum {
  UISE_SZGRP_ENTRY,
  /* label_A through label_C must be sequential */
  UISE_SZGRP_LABEL_A,
  UISE_SZGRP_LABEL_B,
  UISE_SZGRP_LABEL_C,
  UISE_SZGRP_SCALE,
  UISE_SZGRP_SCALE_DISP,
  UISE_SZGRP_SPIN_NUM,
  UISE_SZGRP_SPIN_TEXT,
  UISE_SZGRP_SPIN_TIME,
  UISE_SZGRP_MAX,
};

enum {
  UISE_NOT_DISPLAYED = -1,
};

typedef struct se_internal se_internal_t;

typedef struct {
  int             tagkey;
  uiwcont_t       *chgind;
  uiwcont_t       *label;
  union {
    uiwcont_t     *uiwidgetp;
    uidance_t     *uidance;
    uifavorite_t  *uifavorite;
    uigenre_t     *uigenre;
    uilevel_t     *uilevel;
    uirating_t    *uirating;
    uistatus_t    *uistatus;
  };
  uiwcont_t       *display;
  callback_t      *callback;
  se_internal_t   *seint;           // need for scale changed.
  bool            lastchanged : 1;
  bool            changed : 1;
} uisongedititem_t;

enum {
  UISE_CB_FIRST,
  UISE_CB_PLAY,
  UISE_CB_SAVE,
  UISE_CB_COPY_TEXT,
  UISE_CB_PREV,
  UISE_CB_NEXT,
  UISE_CB_KEYB,
  UISE_CB_CHANGED,
  UISE_CB_VAL_TIME,     /* there are currently only two time fields */
  UISE_CB_VAL_TIME_B,
  UISE_CB_MAX,
};

enum {
  UISE_MAIN_TIMER = 40,
};

enum {
  UISE_W_BUTTON_FIRST,
  UISE_W_BUTTON_PREV,
  UISE_W_BUTTON_NEXT,
  UISE_W_BUTTON_PLAY,
  UISE_W_BUTTON_SAVE,
  UISE_W_BUTTON_COPY_TEXT,
  UISE_W_EDIT_ALL,
  UISE_W_PARENT_WIN,
  UISE_W_MAIN_VBOX,
  UISE_W_MUSICBRAINZ,
  UISE_W_MODIFIED,
  UISE_W_AUDIOID_IMG,
  UISE_W_FILE_DISP,
  UISE_W_KEY_HNDLR,
  UISE_W_MAX,
};

typedef struct se_internal {
  uisongedit_t        *uisongedit;
  uiwcont_t           *wcont [UISE_W_MAX];
  uiwcont_t           *szgrp [UISE_SZGRP_MAX];
  callback_t          *callbacks [UISE_CB_MAX];
  level_t             *levels;
  song_t              *song;
  dbidx_t             dbidx;
  int                 itemcount;
  uisongedititem_t    *items;
  int                 changed;
  int                 bpmidx;
  int                 songstartidx;
  int                 songendidx;
  int                 speedidx;
  int                 keywordidx;
  int                 lastspeed;
  int                 currdanceidx;
  bool                checkchanged : 1;
  bool                ineditallapply : 1;
} se_internal_t;

static void uisongeditCheckChanged (uisongedit_t *uisongedit);
static void uisongeditAddDisplay (uisongedit_t *songedit, uiwcont_t *col, uiwcont_t *sg, int dispsel);
static void uisongeditAddItem (uisongedit_t *uisongedit, uiwcont_t *hbox, uiwcont_t *sg, int tagkey);
static void uisongeditAddEntry (uisongedit_t *uisongedit, uiwcont_t *hbox, int tagkey);
static void uisongeditAddSpinboxInt (uisongedit_t *uisongedit, uiwcont_t *hbox, int tagkey);
static void uisongeditAddLabel (uisongedit_t *uisongedit, uiwcont_t *hbox, int tagkey);
static void uisongeditAddSecondaryLabel (uisongedit_t *uisongedit, uiwcont_t *hbox, int tagkey);
static void uisongeditAddSpinboxTime (uisongedit_t *uisongedit, uiwcont_t *hbox, int tagkey);
static void uisongeditAddScale (uisongedit_t *uisongedit, uiwcont_t *hbox, int tagkey);
static void uisongeditAddSwitch (uisongedit_t *uisongedit, uiwcont_t *hbox, int tagkey);
static bool uisongeditScaleDisplayCallback (void *udata, double value);
static bool uisongeditSwitchCallback (void *udata, int32_t value);
static bool uisongeditSaveCallback (void *udata);
static bool uisongeditSave (void *udata, nlist_t *chglist);
static int  uisongeditGetCheckValue (uisongedit_t *uisongedit, int tagkey);
static nlist_t * uisongeditGetChangedData (uisongedit_t *uisongedit);
static bool uisongeditFirstSelection (void *udata);
static bool uisongeditPreviousSelection (void *udata);
static bool uisongeditNextSelection (void *udata);
static bool uisongeditCopyPath (void *udata);
static bool uisongeditKeyEvent (void *udata);
static int uisongeditEntryChangedCallback (uiwcont_t *entry, const char *label, void *udata);
static bool uisongeditChangedCallback (void *udata);
static char * uisongeditGetBPMRangeDisplay (int danceidx);
static void uisongeditSetBPMRangeDisplay (se_internal_t *seint, int bpmdispidx, ilistidx_t danceidx);
static void uisongeditSetBPMIncrement (se_internal_t *seint, ilistidx_t danceidx);
static int32_t uisongeditValHMSPCallback (void *udata, const char *label, const char *txt);

void
uisongeditUIInit (uisongedit_t *uisongedit)
{
  se_internal_t  *seint;

  logProcBegin ();

  seint = mdmalloc (sizeof (se_internal_t));
  seint->uisongedit = uisongedit;
  seint->itemcount = 0;
  seint->items = NULL;
  seint->changed = 0;
  seint->levels = bdjvarsdfGet (BDJVDF_LEVELS);
  seint->bpmidx = UISE_NOT_DISPLAYED;
  seint->songstartidx = UISE_NOT_DISPLAYED;
  seint->songendidx = UISE_NOT_DISPLAYED;
  seint->speedidx = UISE_NOT_DISPLAYED;
  seint->keywordidx = UISE_NOT_DISPLAYED;
  seint->dbidx = -1;
  seint->lastspeed = -1;
  for (int i = 0; i < UISE_CB_MAX; ++i) {
    seint->callbacks [i] = NULL;
  }
  for (int i = 0; i < UISE_SZGRP_MAX; ++i) {
    seint->szgrp [i] = uiCreateSizeGroupHoriz ();
  }
  for (int i = 0; i < UISE_W_MAX; ++i) {
    seint->wcont [i] = NULL;
  }
  seint->checkchanged = false;
  seint->ineditallapply = false;

  uisongedit->seInternalData = seint;
  logProcEnd ("");
}

void
uisongeditUIFree (uisongedit_t *uisongedit)
{
  se_internal_t *seint;

  logProcBegin ();
  if (uisongedit == NULL) {
    logProcEnd ("null");
    return;
  }

  seint = uisongedit->seInternalData;
  if (seint != NULL) {
    for (int count = 0; count < seint->itemcount; ++count) {
      int   tagkey;

      uiwcontFree (seint->items [count].chgind);

      tagkey = seint->items [count].tagkey;

      switch (tagdefs [tagkey].editType) {
        case ET_ENTRY: {
          uiwcontFree (seint->items [count].uiwidgetp);
          break;
        }
        case ET_COMBOBOX: {
          if (tagkey == TAG_DANCE) {
            uidanceFree (seint->items [count].uidance);
          }
          if (tagkey == TAG_GENRE) {
            uigenreFree (seint->items [count].uigenre);
          }
          break;
        }
        case ET_SPINBOX_TEXT: {
          if (tagkey == TAG_FAVORITE) {
            uifavoriteFree (seint->items [count].uifavorite);
          }
          if (tagkey == TAG_DANCELEVEL) {
            uilevelFree (seint->items [count].uilevel);
          }
          if (tagkey == TAG_DANCERATING) {
            uiratingFree (seint->items [count].uirating);
          }
          if (tagkey == TAG_STATUS) {
            uistatusFree (seint->items [count].uistatus);
          }
          break;
        }
        case ET_SWITCH:
        case ET_SCALE:
        case ET_LABEL:
        case ET_SPINBOX:
        case ET_SPINBOX_TIME: {
          uiwcontFree (seint->items [count].uiwidgetp);
          break;
        }
        case ET_NA: {
          break;
        }
      }
      callbackFree (seint->items [count].callback);
      uiwcontFree (seint->items [count].label);
      uiwcontFree (seint->items [count].display);
    }

    uiWidgetClearPersistent (seint->wcont [UISE_W_MUSICBRAINZ]);

    for (int i = 0; i < UISE_SZGRP_MAX; ++i) {
      uiwcontFree (seint->szgrp [i]);
    }
    for (int i = 0; i < UISE_W_MAX; ++i) {
      if (i == UISE_W_PARENT_WIN) {
        continue;
      }
      uiwcontFree (seint->wcont [i]);
    }
    for (int i = 0; i < UISE_CB_MAX; ++i) {
      callbackFree (seint->callbacks [i]);
    }

    dataFree (seint->items);
    mdfree (seint);
    uisongedit->seInternalData = NULL;
  }

  logProcEnd ("");
}

uiwcont_t *
uisongeditBuildUI (uisongsel_t *uisongsel, uisongedit_t *uisongedit,
    uiwcont_t *parentwin, uiwcont_t *statusMsg)
{
  se_internal_t     *seint;
  uiwcont_t         *hbox;
  uiwcont_t         *uiwidgetp;
  int               count;
  char              tbuff [MAXPATHLEN];

  logProcBegin ();
  logProcBegin ();

  uisongedit->statusMsg = statusMsg;
  uisongedit->uisongsel = uisongsel;
  seint = uisongedit->seInternalData;
  seint->wcont [UISE_W_PARENT_WIN] = parentwin;

  seint->wcont [UISE_W_MAIN_VBOX] = uiCreateVertBox ();
  uiWidgetExpandHoriz (seint->wcont [UISE_W_MAIN_VBOX]);

  seint->wcont [UISE_W_KEY_HNDLR] = uiEventAlloc ();
  seint->callbacks [UISE_CB_KEYB] = callbackInit (
      uisongeditKeyEvent, uisongedit, NULL);
  uiEventSetKeyCallback (seint->wcont [UISE_W_KEY_HNDLR],
      seint->wcont [UISE_W_MAIN_VBOX], seint->callbacks [UISE_CB_KEYB]);

  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiWidgetAlignHorizFill (hbox);
  uiBoxPackStart (seint->wcont [UISE_W_MAIN_VBOX], hbox);

  seint->callbacks [UISE_CB_FIRST] = callbackInit (
      uisongeditFirstSelection, uisongedit, "songedit: first");
  uiwidgetp = uiCreateButton (seint->callbacks [UISE_CB_FIRST],
      /* CONTEXT: song editor : first song */
      _("First"), NULL);
  uiBoxPackStart (hbox, uiwidgetp);
  seint->wcont [UISE_W_BUTTON_FIRST] = uiwidgetp;

  seint->callbacks [UISE_CB_PREV] = callbackInit (
      uisongeditPreviousSelection, uisongedit, "songedit: previous");
  uiwidgetp = uiCreateButton (seint->callbacks [UISE_CB_PREV],
      /* CONTEXT: song editor : previous song */
      _("Previous"), NULL);
  uiButtonSetRepeat (uiwidgetp, REPEAT_TIME);
  uiBoxPackStart (hbox, uiwidgetp);
  seint->wcont [UISE_W_BUTTON_PREV] = uiwidgetp;

  seint->callbacks [UISE_CB_NEXT] = callbackInit (
      uisongeditNextSelection, uisongedit, "songedit: next");
  uiwidgetp = uiCreateButton (seint->callbacks [UISE_CB_NEXT],
      /* CONTEXT: song editor : next song */
      _("Next"), NULL);
  uiButtonSetRepeat (uiwidgetp, REPEAT_TIME);
  uiBoxPackStart (hbox, uiwidgetp);
  seint->wcont [UISE_W_BUTTON_NEXT] = uiwidgetp;

  seint->callbacks [UISE_CB_PLAY] = callbackInit (
      uisongselPlayCallback, uisongsel, "songedit: play");
  uiwidgetp = uiCreateButton (seint->callbacks [UISE_CB_PLAY],
      /* CONTEXT: song editor : play song */
      _("Play"), NULL);
  uiBoxPackStart (hbox, uiwidgetp);
  seint->wcont [UISE_W_BUTTON_PLAY] = uiwidgetp;

  seint->callbacks [UISE_CB_SAVE] = callbackInit (
      uisongeditSaveCallback, uisongedit, "songedit: save");
  uiwidgetp = uiCreateButton (seint->callbacks [UISE_CB_SAVE],
      /* CONTEXT: song editor : save data */
      _("Save"), NULL);
  uiBoxPackEnd (hbox, uiwidgetp);
  seint->wcont [UISE_W_BUTTON_SAVE] = uiwidgetp;

  uiwidgetp = uiCreateLabel ("");
  uiBoxPackEnd (hbox, uiwidgetp);
  uiWidgetSetMarginEnd (uiwidgetp, 6);
  uiWidgetSetClass (uiwidgetp, DARKACCENT_CLASS);
  seint->wcont [UISE_W_EDIT_ALL] = uiwidgetp;

  uiwcontFree (hbox);

  /* begin line */

  /* audio-identification logo, modified indicator, */
  /* copy button, file label, filename */
  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiWidgetAlignHorizFill (hbox);
  uiBoxPackStart (seint->wcont [UISE_W_MAIN_VBOX], hbox);

  pathbldMakePath (tbuff, sizeof (tbuff), "musicbrainz-logo", BDJ4_IMG_SVG_EXT,
      PATHBLD_MP_DIR_IMG);
  seint->wcont [UISE_W_MUSICBRAINZ] = uiImageFromFile (tbuff);
  uiImageConvertToPixbuf (seint->wcont [UISE_W_MUSICBRAINZ]);
  uiWidgetMakePersistent (seint->wcont [UISE_W_MUSICBRAINZ]);

  seint->wcont [UISE_W_AUDIOID_IMG] = uiImageNew ();
  uiImageClear (seint->wcont [UISE_W_AUDIOID_IMG]);
  uiWidgetSetSizeRequest (seint->wcont [UISE_W_AUDIOID_IMG], 24, -1);
  uiWidgetSetMarginStart (seint->wcont [UISE_W_AUDIOID_IMG], 1);
  uiBoxPackStart (hbox, seint->wcont [UISE_W_AUDIOID_IMG]);

  uiwidgetp = uiCreateLabel (" ");
  uiBoxPackStart (hbox, uiwidgetp);
  uiWidgetSetClass (uiwidgetp, DARKACCENT_CLASS);
  seint->wcont [UISE_W_MODIFIED] = uiwidgetp;

  seint->callbacks [UISE_CB_COPY_TEXT] = callbackInit (
      uisongeditCopyPath, uisongedit, "songedit: copy-text");
  uiwidgetp = uiCreateButton (
      seint->callbacks [UISE_CB_COPY_TEXT],
      "", NULL);
  uiButtonSetImageIcon (uiwidgetp, "edit-copy");
  uiWidgetSetMarginStart (uiwidgetp, 1);
  uiBoxPackStart (hbox, uiwidgetp);
  seint->wcont [UISE_W_BUTTON_COPY_TEXT] = uiwidgetp;

  /* CONTEXT: song editor: label for displaying the audio file path */
  uiwidgetp = uiCreateColonLabel (_("File"));
  uiBoxPackStart (hbox, uiwidgetp);
  uiWidgetSetState (uiwidgetp, UIWIDGET_DISABLE);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiCreateLabel ("");
  uiLabelEllipsizeOn (uiwidgetp);
  uiBoxPackStart (hbox, uiwidgetp);
  uiWidgetSetClass (uiwidgetp, DARKACCENT_CLASS);
  uiLabelSetSelectable (uiwidgetp);
  seint->wcont [UISE_W_FILE_DISP] = uiwidgetp;

  uiwcontFree (hbox);

  /* begin line */
  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiWidgetAlignHorizFill (hbox);
  uiBoxPackStartExpand (seint->wcont [UISE_W_MAIN_VBOX], hbox);

  count = 0;
  for (int i = DISP_SEL_SONGEDIT_A; i <= DISP_SEL_SONGEDIT_C; ++i) {
    slist_t           *sellist;

    sellist = dispselGetList (uisongedit->dispsel, i);
    count += slistGetCount (sellist);
  }

  /* the items must all be alloc'd beforehand so that the callback */
  /* pointer is static */
  /* need to add 1 for the BPM display secondary display */
  seint->items = mdmalloc (sizeof (uisongedititem_t) * (count + 1));
  for (int i = 0; i < count + 1; ++i) {
    seint->items [i].tagkey = 0;
    seint->items [i].chgind = NULL;
    seint->items [i].label = NULL;
    seint->items [i].uiwidgetp = NULL;
    seint->items [i].display = NULL;
    seint->items [i].callback = NULL;
    seint->items [i].seint = seint;
    seint->items [i].lastchanged = false;
    seint->items [i].changed = false;
  }

  /* must be set before the items are instantiated */
  seint->callbacks [UISE_CB_CHANGED] = callbackInit (
      uisongeditChangedCallback, seint, NULL);

  for (int i = DISP_SEL_SONGEDIT_A; i <= DISP_SEL_SONGEDIT_C; ++i) {
    uiwcont_t   *col;
    slist_t     *sellist;

    sellist = dispselGetList (uisongedit->dispsel, i);
    count = slistGetCount (sellist);

    /* don't bother building the vertical box if the column has no items */
    /* otherwise it takes up space in the song editor display */
    if (count > 0) {
      col = uiCreateVertBox ();
      uiWidgetSetAllMargins (col, 4);
      uiWidgetExpandHoriz (col);
      uiWidgetExpandVert (col);
      uiBoxPackStartExpand (hbox, col);

      uisongeditAddDisplay (uisongedit, col,
          seint->szgrp [UISE_SZGRP_LABEL_A + i - DISP_SEL_SONGEDIT_A], i);
      uiwcontFree (col);
    }
  }

  uiwcontFree (hbox);

  logProcEnd ("");
  return seint->wcont [UISE_W_MAIN_VBOX];
}

void
uisongeditLoadData (uisongedit_t *uisongedit, song_t *song,
    dbidx_t dbidx, int editallflag)
{
  se_internal_t   *seint;
  char            *tval;
  const char      *data;
  long            val;
  double          dval;
  int             bpmdispidx = UISE_NOT_DISPLAYED;

  logProcBegin ();
  seint = uisongedit->seInternalData;
  seint->song = song;
  seint->dbidx = dbidx;
  seint->changed = 0;

  tval = uisongGetDisplay (song, TAG_URI, &val, &dval);
  uiLabelSetText (seint->wcont [UISE_W_FILE_DISP], tval);
  dataFree (tval);
  tval = NULL;

  uiImageClear (seint->wcont [UISE_W_AUDIOID_IMG]);
  data = songGetStr (song, TAG_RECORDING_ID);
  if (data != NULL && *data) {
    uiImageSetFromPixbuf (seint->wcont [UISE_W_AUDIOID_IMG], seint->wcont [UISE_W_MUSICBRAINZ]);
  }

  val = songGetNum (song, TAG_ADJUSTFLAGS);
  uiLabelSetText (seint->wcont [UISE_W_MODIFIED], " ");
  if (val != SONG_ADJUST_NONE) {
    uiLabelSetText (seint->wcont [UISE_W_MODIFIED], "*");
  }

  for (int count = 0; count < seint->itemcount; ++count) {
    int tagkey = seint->items [count].tagkey;

    if (editallflag == UISONGEDIT_EDITALL) {
      /* if the editallflag is used, only re-load those items that are */
      /* not set for edit-all */
      if (tagdefs [tagkey].allEdit) {
        continue;
      }
    }

    if (tagkey == TAG_BPM_DISPLAY) {
      bpmdispidx = count;
      continue;
    }

    if (tagkey == TAG_DURATION) {
      tval = songDisplayString (song, tagkey, SONG_LONG_DURATION);
    } else {
      tval = uisongGetDisplay (song, tagkey, &val, &dval);
    }
    if (! seint->ineditallapply) {
      seint->items [count].changed = false;
      seint->items [count].lastchanged = false;
    }

    switch (tagdefs [tagkey].editType) {
      case ET_ENTRY: {
        uiEntrySetValue (seint->items [count].uiwidgetp, "");
        if (tval != NULL) {
          uiEntrySetValue (seint->items [count].uiwidgetp, tval);
        }
        break;
      }
      case ET_COMBOBOX: {
        if (tagkey == TAG_DANCE) {
          if (val < 0) { val = -1; } // empty value
          uidanceSetValue (seint->items [count].uidance, val);
          seint->currdanceidx = val;
        }
        if (tagkey == TAG_GENRE) {
          if (val < 0) { val = 0; }
          uigenreSetValue (seint->items [count].uigenre, val);
        }
        break;
      }
      case ET_SPINBOX_TEXT: {
        if (tagkey == TAG_FAVORITE) {
          if (val < 0) { val = 0; }
          uifavoriteSetValue (seint->items [count].uifavorite, val);
        }
        if (tagkey == TAG_DANCELEVEL) {
          if (val < 0) { val = levelGetDefaultKey (seint->levels); }
          uilevelSetValue (seint->items [count].uilevel, val);
        }
        if (tagkey == TAG_DANCERATING) {
          if (val < 0) { val = 0; }
          uiratingSetValue (seint->items [count].uirating, val);
        }
        if (tagkey == TAG_STATUS) {
          if (val < 0) { val = 0; }
          uistatusSetValue (seint->items [count].uistatus, val);
        }
        break;
      }
      case ET_SPINBOX: {
        if (tval != NULL) {
          fprintf (stderr, "et_spinbox: mismatch type\n");
        }
        if (val < 0) { val = 0; }
        uiSpinboxSetValue (seint->items [count].uiwidgetp, val);
        break;
      }
      case ET_SPINBOX_TIME: {
        if (val < 0) { val = 0; }
        uiSpinboxTimeSetValue (seint->items [count].uiwidgetp, val);
        break;
      }
      case ET_SCALE: {
        /* volume adjust percentage may be a negative value */
        if (dval == LIST_DOUBLE_INVALID) { dval = 0.0; }
        if (isnan (dval)) { dval = 0.0; }
        if (tagkey == TAG_SPEEDADJUSTMENT) {
          /* speed adjustment uses a scale, but has a numeric stored */
          dval = (double) val;
          if (dval <= 0.0) { dval = 100.0; }
          seint->lastspeed = val;
        }
        uiScaleSetValue (seint->items [count].uiwidgetp, dval);
        uisongeditScaleDisplayCallback (&seint->items [count], dval);
        break;
      }
      case ET_SWITCH: {
        uiSwitchSetValue (seint->items [count].uiwidgetp, val);
        break;
      }
      case ET_LABEL: {
        if (tval != NULL) {
          uiLabelSetText (seint->items [count].uiwidgetp, tval);
        }
        break;
      }
      default: {
        break;
      }
    }

    dataFree (tval);
    tval = NULL;
  }

  if (seint->bpmidx != UISE_NOT_DISPLAYED) {
    int     speed;

    val = songGetNum (song, TAG_BPM);
    if (val < 0) { val = 0; }
    speed = songGetNum (seint->song, TAG_SPEEDADJUSTMENT);
    val = songutilAdjustBPM (val, speed);
    val = danceConvertMPMtoBPM (seint->currdanceidx, val);
    uiSpinboxSetValue (seint->items [seint->bpmidx].uiwidgetp, val);
  }

  uisongeditSetBPMRangeDisplay (seint, bpmdispidx, seint->currdanceidx);
  uisongeditSetBPMIncrement (seint, seint->currdanceidx);

  logProcEnd ("");
}

void
uisongeditUIMainLoop (uisongedit_t *uisongedit)
{
  se_internal_t   *seint;

  seint = uisongedit->seInternalData;

  uiButtonCheckRepeat (seint->wcont [UISE_W_BUTTON_NEXT]);
  uiButtonCheckRepeat (seint->wcont [UISE_W_BUTTON_PREV]);
  uisongeditCheckChanged (uisongedit);
  return;
}

void
uisongeditSetBPMValue (uisongedit_t *uisongedit, int val)
{
  se_internal_t *seint;

  logProcBegin ();
  seint = uisongedit->seInternalData;

  if (seint->bpmidx < 0) {
    logProcEnd ("bad-bpmidx");
    return;
  }

  /* the bpm value received from the bpm counter is always mpm */
  val = danceConvertMPMtoBPM (seint->currdanceidx, val);

  uiSpinboxSetValue (seint->items [seint->bpmidx].uiwidgetp, val);
  logProcEnd ("");
}

void
uisongeditSetPlayButtonState (uisongedit_t *uisongedit, int active)
{
  se_internal_t *seint;

  logProcBegin ();
  seint = uisongedit->seInternalData;

  /* if the player is active, disable the button */
  if (active) {
    uiWidgetSetState (seint->wcont [UISE_W_BUTTON_PLAY], UIWIDGET_DISABLE);
  } else {
    uiWidgetSetState (seint->wcont [UISE_W_BUTTON_PLAY], UIWIDGET_ENABLE);
  }
  logProcEnd ("");
}

/* also sets the edit-all display label */
/* also enables/disables the save button */
void
uisongeditEditAllSetFields (uisongedit_t *uisongedit, int editflag)
{
  se_internal_t *seint;
  int           newstate;

  seint = uisongedit->seInternalData;

  newstate = UIWIDGET_ENABLE;
  if (editflag == UISONGEDIT_EDITALL_ON) {
    newstate = UIWIDGET_DISABLE;
  }

  if (editflag == UISONGEDIT_EDITALL_ON) {
    /* CONTEXT: song editor: display indicator that the song editor is in edit all mode */
    uiLabelSetText (seint->wcont [UISE_W_EDIT_ALL], _("Edit All"));
  } else {
    uiLabelSetText (seint->wcont [UISE_W_EDIT_ALL], "");
  }

  uiWidgetSetState (seint->wcont [UISE_W_BUTTON_SAVE], newstate);

  for (int count = 0; count < seint->itemcount; ++count) {
    int   tagkey;

    tagkey = seint->items [count].tagkey;

    if (tagdefs [tagkey].editType != ET_LABEL &&
        ! tagdefs [tagkey].isEditable) {
      continue;
    }

    if (tagdefs [tagkey].allEdit) {
      /* fields with the edit-all flag set are left enabled */
      continue;
    }

    if (tagdefs [tagkey].editType != ET_LABEL &&
        tagdefs [tagkey].editType != ET_NA) {
      uiWidgetSetState (seint->items [count].label, newstate);
    }

    switch (tagdefs [tagkey].editType) {
      case ET_ENTRY: {
        uiEntrySetState (seint->items [count].uiwidgetp, newstate);
        break;
      }
      case ET_COMBOBOX: {
        if (tagkey == TAG_DANCE) {
          uidanceSetState (seint->items [count].uidance, newstate);
        }
        if (tagkey == TAG_GENRE) {
          uigenreSetState (seint->items [count].uigenre, newstate);
        }
        break;
      }
      case ET_SPINBOX_TIME: {
        uiSpinboxSetState (seint->items [count].uiwidgetp, newstate);
        break;
      }
      case ET_SPINBOX_TEXT: {
        if (tagkey == TAG_FAVORITE) {
          uifavoriteSetState (seint->items [count].uifavorite, newstate);
        }
        if (tagkey == TAG_DANCELEVEL) {
          uilevelSetState (seint->items [count].uilevel, newstate);
        }
        if (tagkey == TAG_DANCERATING) {
          uiratingSetState (seint->items [count].uirating, newstate);
        }
        if (tagkey == TAG_STATUS) {
          uistatusSetState (seint->items [count].uistatus, newstate);
        }
        break;
      }
      case ET_SWITCH:
      case ET_SCALE:
      case ET_SPINBOX: {
        uiWidgetSetState (seint->items [count].uiwidgetp, newstate);
        break;
      }
      case ET_LABEL: {
        break;
      }
      case ET_NA: {
        break;
      }
    } /* switch on item type */
  } /* for each displayed item */
}

void
uisongeditClearChanged (uisongedit_t *uisongedit, int editallflag)
{
  se_internal_t *seint =  NULL;

  logProcBegin ();
  seint = uisongedit->seInternalData;
  for (int count = 0; count < seint->itemcount; ++count) {
    int tagkey = seint->items [count].tagkey;

    if (seint->items [count].chgind == NULL) {
      continue;
    }
    if (editallflag == UISONGEDIT_EDITALL) {
      if (tagdefs [tagkey].allEdit) {
        continue;
      }
    }
    uichgindMarkNormal (seint->items [count].chgind);
    seint->items [count].changed = false;
    seint->items [count].lastchanged = false;
  }
  seint->changed = 0;
  logProcEnd ("");
}

bool
uisongeditEditAllApply (uisongedit_t *uisongedit)
{
  se_internal_t   *seint =  NULL;
  nlist_t         *chglist;
  dbidx_t         lastdbidx;

  logProcBegin ();
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: song edit: edit-all apply");
  seint = uisongedit->seInternalData;

  chglist = uisongeditGetChangedData (uisongedit);

  seint->ineditallapply = true;
  lastdbidx = -1;
  while (seint->dbidx != lastdbidx) {
    uisongeditSave (uisongedit, chglist);
    lastdbidx = seint->dbidx;
    uisongselNextSelection (uisongedit->uisongsel);
  }
  nlistFree (chglist);
  uisongeditClearChanged (uisongedit, UISONGEDIT_ALL);
  seint->ineditallapply = false;

  logProcEnd ("");
  return UICB_CONT;
}

/* internal routines */

static void
uisongeditCheckChanged (uisongedit_t *uisongedit)
{
  se_internal_t   *seint;
  double          dval;
  double          ndval = LIST_DOUBLE_INVALID;
  long            val = LIST_VALUE_INVALID;
  long            nval = LIST_VALUE_INVALID;
  char            *songdata = NULL;
  const char      *ndata = NULL;
  int             bpmdispidx = UISE_NOT_DISPLAYED;
  bool            spdchanged = false;
  int             speed = 100;
  int             danceidx = UISE_NOT_DISPLAYED;

  seint = uisongedit->seInternalData;

  if (seint->checkchanged) {
    /* check for any changed items.*/
    /* this works well for the user, as it is able to determine if a */
    /* value has reverted back to the original value */
    for (int count = 0; count < seint->itemcount; ++count) {
      int   chkvalue;
      int   tagkey = seint->items [count].tagkey;

      if (tagkey == TAG_BPM_DISPLAY) {
        bpmdispidx = count;
      }

      songdata = uisongGetDisplay (seint->song, tagkey, &val, &dval);
      chkvalue = uisongeditGetCheckValue (uisongedit, tagkey);

      switch (tagdefs [tagkey].editType) {
        case ET_ENTRY: {
          ndata = uiEntryGetValue (seint->items [count].uiwidgetp);
          break;
        }
        case ET_COMBOBOX: {
          if (tagkey == TAG_DANCE) {
            if (val < 0) { val = -1; }
            nval = uidanceGetValue (seint->items [count].uidance);
            danceidx = nval;
            seint->currdanceidx = danceidx;
          }
          if (tagkey == TAG_GENRE) {
            nval = uigenreGetValue (seint->items [count].uigenre);
          }
          break;
        }
        case ET_SPINBOX_TEXT: {
          if (tagkey == TAG_FAVORITE) {
            nval = uifavoriteGetValue (seint->items [count].uifavorite);
          }
          if (tagkey == TAG_DANCELEVEL) {
            nval = uilevelGetValue (seint->items [count].uilevel);
          }
          if (tagkey == TAG_DANCERATING) {
            nval = uiratingGetValue (seint->items [count].uirating);
          }
          if (tagkey == TAG_STATUS) {
            nval = uistatusGetValue (seint->items [count].uistatus);
          }
          break;
        }
        case ET_SPINBOX: {
          if (val < 0) { val = 0; }
          nval = uiSpinboxGetValue (seint->items [count].uiwidgetp);
          if (nval < 0) { nval = 0; }
          if (count == seint->bpmidx && nval > 0) {
            nval = songutilNormalizeBPM (nval, seint->lastspeed);
            nval = danceConvertBPMtoMPM (seint->currdanceidx, nval, DANCE_NO_FORCE);
          }
          break;
        }
        case ET_SPINBOX_TIME: {
          if (val < 0) { val = 0; }
          nval = uiSpinboxTimeGetValue (seint->items [count].uiwidgetp);
          break;
        }
        case ET_SCALE: {
          if (isnan (dval)) { dval = 0.0; }
          if (tagkey == TAG_SPEEDADJUSTMENT) {
            dval = (double) val;
            if (dval < 0.0) { dval = 0.0; }
            /* for the purposes of check-changed processing, check */
            /* speed adjustment as a double */
            chkvalue = UISE_CHK_DOUBLE;
          }
          ndval = uiScaleGetValue (seint->items [count].uiwidgetp);
          if (ndval == LIST_DOUBLE_INVALID) { ndval = 0.0; }
          if (dval == LIST_DOUBLE_INVALID) { dval = 0.0; }
          if (isnan (dval)) { dval = 0.0; }
          break;
        }
        case ET_SWITCH: {
          nval = uiSwitchGetValue (seint->items [count].uiwidgetp);
        }
        default: {
          break;
        }
      }

      if (chkvalue == UISE_CHK_STR ||
          chkvalue == UISE_CHK_LIST) {
        if ((ndata != NULL && songdata == NULL) ||
            strcmp (songdata, ndata) != 0) {
          seint->items [count].changed = true;
        } else {
          if (seint->items [count].changed) {
            seint->items [count].changed = false;
          }
        }
      }

      if (chkvalue == UISE_CHK_NUM) {
        int   rcdisc, rctotdisc, rctrk, rctottrk;

        rcdisc = (tagkey == TAG_DISCNUMBER && val == 0.0 && nval == 1.0);
        rctotdisc = (tagkey == TAG_DISCTOTAL && val == 0.0 && nval == 1.0);
        rctrk = (tagkey == TAG_TRACKNUMBER && val == 0.0 && nval == 1.0);
        rctottrk = (tagkey == TAG_TRACKTOTAL && val == 0.0 && nval == 1.0);
        if (tagkey == TAG_FAVORITE || tagkey == TAG_GENRE) {
          if (val < 0) { val = 0; }
        }
        if (! rcdisc && ! rctrk && ! rctotdisc && ! rctottrk && nval != val) {
          seint->items [count].changed = true;
        } else {
          if (seint->items [count].changed) {
            seint->items [count].changed = false;
          }
        }
      }

      if (chkvalue == UISE_CHK_DOUBLE) {
        int   rc;
        int   waschanged = false;

        /* for the speed adjustment, 0.0 (no setting) is changed to 100.0 */
        rc = (tagkey == TAG_SPEEDADJUSTMENT && ndval == 100.0 && dval == 0.0);
        if (! rc && fabs (ndval - dval) > 0.009 ) {
          seint->items [count].changed = true;
          waschanged = true;
        } else {
          if (seint->items [count].changed) {
            waschanged = true;
            seint->items [count].changed = false;
          }
        }

        if (tagkey == TAG_SPEEDADJUSTMENT && waschanged) {
          speed = (int) ndval;
          spdchanged = true;
        }
      }

      if (seint->items [count].changed != seint->items [count].lastchanged) {
        if (seint->items [count].changed) {
          uichgindMarkChanged (seint->items [count].chgind);
          seint->changed += 1;
        } else {
          uichgindMarkNormal (seint->items [count].chgind);
          seint->changed -= 1;
        }
        seint->items [count].lastchanged = seint->items [count].changed;
      }

      dataFree (songdata);
      songdata = NULL;
    }

    if (spdchanged) {
      if (seint->songstartidx != UISE_NOT_DISPLAYED) {
        nval = uiSpinboxTimeGetValue (seint->items [seint->songstartidx].uiwidgetp);
        if (nval > 0) {
          nval = songutilNormalizePosition (nval, seint->lastspeed);
          nval = songutilAdjustPosReal (nval, speed);
          uiSpinboxTimeSetValue (seint->items [seint->songstartidx].uiwidgetp, nval);
        }
      }
      if (seint->songendidx != UISE_NOT_DISPLAYED) {
        nval = uiSpinboxTimeGetValue (seint->items [seint->songendidx].uiwidgetp);
        if (nval > 0) {
          nval = songutilNormalizePosition (nval, seint->lastspeed);
          nval = songutilAdjustPosReal (nval, speed);
          uiSpinboxTimeSetValue (seint->items [seint->songendidx].uiwidgetp, nval);
        }
      }
      if (seint->bpmidx != UISE_NOT_DISPLAYED) {
        nval = uiSpinboxGetValue (seint->items [seint->bpmidx].uiwidgetp);
        if (nval > 0) {
          nval = songutilNormalizeBPM (nval, seint->lastspeed);
          nval = songutilAdjustBPM (nval, speed);
          uiSpinboxSetValue (seint->items [seint->bpmidx].uiwidgetp, nval);
        }
      }
      seint->lastspeed = speed;
    }

    /* always re-create the BPM display */
    uisongeditSetBPMRangeDisplay (seint, bpmdispidx, danceidx);
    uisongeditSetBPMIncrement (seint, danceidx);

    seint->checkchanged = false;
  } /* check changed is true */
}

static void
uisongeditAddDisplay (uisongedit_t *uisongedit, uiwcont_t *col, uiwcont_t *sg, int dispsel)
{
  slist_t         *sellist;
  const char      *keystr;
  slistidx_t      dsiteridx;
  se_internal_t   *seint;

  logProcBegin ();
  seint = uisongedit->seInternalData;
  sellist = dispselGetList (uisongedit->dispsel, dispsel);

  slistStartIterator (sellist, &dsiteridx);
  while ((keystr = slistIterateKey (sellist, &dsiteridx)) != NULL) {
    uiwcont_t   *hbox;
    int         tagkey;

    tagkey = slistGetNum (sellist, keystr);

    if (tagkey >= TAG_KEY_MAX) {
      continue;
    }
    if (tagdefs [tagkey].editType != ET_LABEL &&
        ! tagdefs [tagkey].isEditable) {
      continue;
    }

    hbox = uiCreateHorizBox ();
    uiWidgetExpandHoriz (hbox);
    uiBoxPackStart (col, hbox);
    uisongeditAddItem (uisongedit, hbox, sg, tagkey);
    seint->items [seint->itemcount].tagkey = tagkey;
    if (tagkey == TAG_BPM) {
      ++seint->itemcount;
      uisongeditAddItem (uisongedit, hbox, sg, TAG_BPM_DISPLAY);
      seint->items [seint->itemcount].tagkey = TAG_BPM_DISPLAY;
    }
    ++seint->itemcount;
    uiwcontFree (hbox);
  }

  logProcEnd ("");
}

static void
uisongeditAddItem (uisongedit_t *uisongedit, uiwcont_t *hbox, uiwcont_t *sg, int tagkey)
{
  uiwcont_t      *uiwidgetp;
  se_internal_t *seint;

  logProcBegin ();
  seint = uisongedit->seInternalData;

  if (! tagdefs [tagkey].secondaryDisplay) {
    seint->items [seint->itemcount].chgind = uiCreateChangeIndicator (hbox);
    uichgindMarkNormal (seint->items [seint->itemcount].chgind);
    seint->items [seint->itemcount].changed = false;
    seint->items [seint->itemcount].lastchanged = false;

    uiwidgetp = uiCreateColonLabel (tagdefs [tagkey].displayname);
    uiWidgetSetMarginEnd (uiwidgetp, 4);
    uiBoxPackStart (hbox, uiwidgetp);
    uiSizeGroupAdd (sg, uiwidgetp);
    seint->items [seint->itemcount].label = uiwidgetp;
  }

  switch (tagdefs [tagkey].editType) {
    case ET_ENTRY: {
      uisongeditAddEntry (uisongedit, hbox, tagkey);
      if (tagkey == TAG_KEYWORD) {
        seint->keywordidx = seint->itemcount;
      }
      break;
    }
    case ET_COMBOBOX: {
      if (tagkey == TAG_DANCE) {
        seint->items [seint->itemcount].uidance =
            uidanceDropDownCreate (hbox, seint->wcont [UISE_W_PARENT_WIN],
            UIDANCE_EMPTY_DANCE, "", UIDANCE_PACK_START, 1);
        uidanceSetCallback (seint->items [seint->itemcount].uidance,
            seint->callbacks [UISE_CB_CHANGED]);
      }
      if (tagkey == TAG_GENRE) {
        seint->items [seint->itemcount].uigenre =
            uigenreDropDownCreate (hbox, seint->wcont [UISE_W_PARENT_WIN], false);
        uigenreSetCallback (seint->items [seint->itemcount].uigenre,
            seint->callbacks [UISE_CB_CHANGED]);
      }
      break;
    }
    case ET_SPINBOX_TEXT: {
      if (tagkey == TAG_FAVORITE) {
        seint->items [seint->itemcount].uifavorite =
            uifavoriteSpinboxCreate (hbox);
        uifavoriteSetChangedCallback (seint->items [seint->itemcount].uifavorite,
            seint->callbacks [UISE_CB_CHANGED]);
      }
      if (tagkey == TAG_DANCELEVEL) {
        seint->items [seint->itemcount].uilevel =
            uilevelSpinboxCreate (hbox, false);
        uilevelSizeGroupAdd (seint->items [seint->itemcount].uilevel, seint->szgrp [UISE_SZGRP_SPIN_TEXT]);
        uilevelSetChangedCallback (seint->items [seint->itemcount].uilevel,
            seint->callbacks [UISE_CB_CHANGED]);
      }
      if (tagkey == TAG_DANCERATING) {
        seint->items [seint->itemcount].uirating =
            uiratingSpinboxCreate (hbox, UIRATING_NORM);
        uiratingSizeGroupAdd (seint->items [seint->itemcount].uirating, seint->szgrp [UISE_SZGRP_SPIN_TEXT]);
        uiratingSetChangedCallback (seint->items [seint->itemcount].uirating,
            seint->callbacks [UISE_CB_CHANGED]);
      }
      if (tagkey == TAG_STATUS) {
        seint->items [seint->itemcount].uistatus =
            uistatusSpinboxCreate (hbox, false);
        uistatusSizeGroupAdd (seint->items [seint->itemcount].uistatus, seint->szgrp [UISE_SZGRP_SPIN_TEXT]);
        uistatusSetChangedCallback (seint->items [seint->itemcount].uistatus,
            seint->callbacks [UISE_CB_CHANGED]);
      }
      break;
    }
    case ET_SPINBOX: {
      if (tagkey == TAG_BPM) {
        seint->bpmidx = seint->itemcount;
      }
      uisongeditAddSpinboxInt (uisongedit, hbox, tagkey);
      break;
    }
    case ET_SPINBOX_TIME: {
      if (tagkey == TAG_SONGSTART) {
        seint->songstartidx = seint->itemcount;
      }
      if (tagkey == TAG_SONGEND) {
        seint->songendidx = seint->itemcount;
      }
      uisongeditAddSpinboxTime (uisongedit, hbox, tagkey);
      break;
    }
    case ET_SCALE: {
      if (tagkey == TAG_SPEEDADJUSTMENT) {
        seint->speedidx = seint->itemcount;
      }
      uisongeditAddScale (uisongedit, hbox, tagkey);
      break;
    }
    case ET_LABEL: {
      if (tagdefs [tagkey].secondaryDisplay) {
        uisongeditAddSecondaryLabel (uisongedit, hbox, tagkey);
      } else {
        uisongeditAddLabel (uisongedit, hbox, tagkey);
      }
      break;
    }
    case ET_SWITCH: {
      uisongeditAddSwitch (uisongedit, hbox, tagkey);
      break;
    }
    case ET_NA: {
      break;
    }
  }
  logProcEnd ("");
}

static void
uisongeditAddEntry (uisongedit_t *uisongedit, uiwcont_t *hbox, int tagkey)
{
  uiwcont_t     *entryp;
  se_internal_t *seint;

  logProcBegin ();
  seint = uisongedit->seInternalData;
  entryp = uiEntryInit (20, 250);
  seint->items [seint->itemcount].uiwidgetp = entryp;
  /* set the validate callback to set the changed flag */
  uiEntrySetValidate (entryp, tagdefs [tagkey].displayname,
      uisongeditEntryChangedCallback, seint, UIENTRY_IMMEDIATE);

  uiWidgetAlignHorizFill (entryp);
  uiSizeGroupAdd (seint->szgrp [UISE_SZGRP_ENTRY], entryp);
  uiBoxPackStartExpand (hbox, entryp);
  logProcEnd ("");
}

static void
uisongeditAddSpinboxInt (uisongedit_t *uisongedit, uiwcont_t *hbox, int tagkey)
{
  uiwcont_t      *uiwidgetp;
  se_internal_t *seint;

  logProcBegin ();
  seint = uisongedit->seInternalData;
  uiwidgetp = uiSpinboxIntCreate ();
  seint->items [seint->itemcount].uiwidgetp = uiwidgetp;
  if (tagkey == TAG_BPM) {
    uiSpinboxSet (uiwidgetp, 0.0, 400.0);
  }
  if (tagkey == TAG_TRACKNUMBER || tagkey == TAG_DISCNUMBER ||
      tagkey == TAG_TRACKTOTAL || tagkey == TAG_DISCTOTAL) {
    uiSpinboxSet (uiwidgetp, 1.0, 300.0);
  }
  uiSpinboxSetValueChangedCallback (uiwidgetp,
      seint->callbacks [UISE_CB_CHANGED]);
  uiSizeGroupAdd (seint->szgrp [UISE_SZGRP_SPIN_NUM], uiwidgetp);
  uiBoxPackStart (hbox, uiwidgetp);
  logProcEnd ("");
}

static void
uisongeditAddLabel (uisongedit_t *uisongedit, uiwcont_t *hbox, int tagkey)
{
  uiwcont_t       *uiwidgetp;
  se_internal_t   *seint;

  logProcBegin ();
  seint = uisongedit->seInternalData;
  uiwidgetp = uiCreateLabel ("");
  uiSizeGroupAdd (seint->szgrp [UISE_SZGRP_ENTRY], uiwidgetp);
  uiBoxPackStartExpand (hbox, uiwidgetp);
  seint->items [seint->itemcount].uiwidgetp = uiwidgetp;
  logProcEnd ("");
}

static void
uisongeditAddSecondaryLabel (uisongedit_t *uisongedit, uiwcont_t *hbox, int tagkey)
{
  uiwcont_t       *uiwidgetp;
  se_internal_t   *seint;

  logProcBegin ();
  seint = uisongedit->seInternalData;
  uiwidgetp = uiCreateLabel ("");
  uiBoxPackStart (hbox, uiwidgetp);
  seint->items [seint->itemcount].uiwidgetp = uiwidgetp;
  logProcEnd ("");
}

static void
uisongeditAddSpinboxTime (uisongedit_t *uisongedit, uiwcont_t *hbox, int tagkey)
{
  uiwcont_t       *sbp;
  se_internal_t   *seint;
  int             cbidx;

  logProcBegin ();
  seint = uisongedit->seInternalData;

  /* there are currently only two time fields */
  cbidx = UISE_CB_VAL_TIME;
  while (seint->callbacks [cbidx] != NULL) {
    ++cbidx;
  }
  seint->callbacks [cbidx] = callbackInitSS (
      uisongeditValHMSPCallback, &seint->items [seint->itemcount]);

  sbp = uiSpinboxTimeCreate (SB_TIME_PRECISE, uisongedit,
      tagdefs [tagkey].displayname,
      seint->callbacks [cbidx]);
  seint->items [seint->itemcount].uiwidgetp = sbp;
  uiSpinboxSetRange (sbp, 0.0, 1200000.0);
  uiSpinboxTimeSetValue (sbp, 0);
  uiSpinboxTimeSetValueChangedCallback (sbp,
      seint->callbacks [UISE_CB_CHANGED]);

  uiSizeGroupAdd (seint->szgrp [UISE_SZGRP_SPIN_TIME], sbp);
  uiBoxPackStart (hbox, sbp);
  logProcEnd ("");
}

static void
uisongeditAddScale (uisongedit_t *uisongedit, uiwcont_t *hbox, int tagkey)
{
  se_internal_t   *seint;
  uiwcont_t       *uiwidgetp;
  double          lower, upper;
  double          inca, incb;
  int             digits;

  logProcBegin ();
  seint = uisongedit->seInternalData;
  if (tagkey == TAG_SPEEDADJUSTMENT) {
    lower = SPD_LOWER;
    upper = SPD_UPPER;
    digits = SPD_DIGITS;
    inca = SPD_INCA;
    incb = SPD_INCB;
  }
  if (tagkey == TAG_VOLUMEADJUSTPERC) {
    lower = VOL_ADJ_LOWER;
    upper = VOL_ADJ_UPPER;
    digits = VOL_ADJ_DIGITS;
    inca = VOL_ADJ_INCA;
    incb = VOL_ADJ_INCB;
  }
  uiwidgetp = uiCreateScale (lower, upper, inca, incb, 0.0, digits);
  seint->items [seint->itemcount].uiwidgetp = uiwidgetp;
  seint->items [seint->itemcount].callback = callbackInitD (
      uisongeditScaleDisplayCallback, &seint->items [seint->itemcount]);
  uiScaleSetCallback (uiwidgetp, seint->items [seint->itemcount].callback);

  uiSizeGroupAdd (seint->szgrp [UISE_SZGRP_SCALE], uiwidgetp);
  uiBoxPackStart (hbox, uiwidgetp);

  uiwidgetp = uiCreateLabel ("100%");
  uiLabelAlignEnd (uiwidgetp);
  uiSizeGroupAdd (seint->szgrp [UISE_SZGRP_SCALE_DISP], uiwidgetp);
  uiBoxPackStart (hbox, uiwidgetp);
  seint->items [seint->itemcount].display = uiwidgetp;
  logProcEnd ("");
}

static void
uisongeditAddSwitch (uisongedit_t *uisongedit, uiwcont_t *hbox, int tagkey)
{
  se_internal_t   *seint;
  uiwcont_t       *uiwidgetp;

  logProcBegin ();

  seint = uisongedit->seInternalData;
  uiwidgetp = uiCreateSwitch (0);
  seint->items [seint->itemcount].uiwidgetp = uiwidgetp;
  uiBoxPackStart (hbox, uiwidgetp);
  seint->items [seint->itemcount].callback = callbackInitI (
      uisongeditSwitchCallback, &seint->items [seint->itemcount]);
  uiSwitchSetCallback (uiwidgetp, seint->items [seint->itemcount].callback);

  logProcEnd ("");
}

/* also sets the changed flag */
static bool
uisongeditScaleDisplayCallback (void *udata, double value)
{
  uisongedititem_t  *item = udata;
  se_internal_t     *seint;
  char              tbuff [40];
  int               digits;

  logProcBegin ();
  seint = item->seint;
  seint->checkchanged = true;
  digits = uiScaleGetDigits (item->uiwidgetp);
  value = uiScaleEnforceMax (item->uiwidgetp, value);
  snprintf (tbuff, sizeof (tbuff), "%4.*f%%", digits, value);
  uiLabelSetText (item->display, tbuff);
  logProcEnd ("");
  return UICB_CONT;
}

static bool
uisongeditSwitchCallback (void *udata, int32_t value)
{
  uisongedititem_t  *item = udata;
  se_internal_t     *seint;

  seint = item->seint;
  seint->checkchanged = true;
  return UICB_CONT;
}

static bool
uisongeditSaveCallback (void *udata)
{
  uisongedit_t    *uisongedit = udata;

  return uisongeditSave (uisongedit, NULL);
}

static bool
uisongeditSave (void *udata, nlist_t *chglist)
{
  uisongedit_t    *uisongedit = udata;
  se_internal_t   *seint =  NULL;
  char            tbuff [200];
  bool            valid = false;

  logProcBegin ();
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: song edit: save");
  seint = uisongedit->seInternalData;

  valid = true;
  uiLabelSetText (uisongedit->statusMsg, "");

  /* do some validations */
  /* this is not necessary with an edit-all-apply */
  /* as the validation checks currently are only for non-edit-all fields */
  if (chglist == NULL) {
    long        songstart;
    long        songend;
    long        dur;
    int         speed;
    double      ndval;
    const char  *tval;

    speed = songGetDouble (seint->song, TAG_SPEEDADJUSTMENT);
    if (seint->speedidx != UISE_NOT_DISPLAYED) {
      ndval = uiScaleGetValue (seint->items [seint->speedidx].uiwidgetp);
      speed = round (ndval);
    }
    songstart = songGetNum (seint->song, TAG_SONGSTART);
    if (seint->songstartidx != UISE_NOT_DISPLAYED) {
      songstart = uiSpinboxTimeGetValue (seint->items [seint->songstartidx].uiwidgetp);
    }
    songend = songGetNum (seint->song, TAG_SONGEND);
    if (seint->songendidx != UISE_NOT_DISPLAYED) {
      songend = uiSpinboxTimeGetValue (seint->items [seint->songendidx].uiwidgetp);
    }
    /* for the validation checks, the song-start and song-end must be */
    /* normalized. */
    if (speed > 0 && speed != 100) {
      if (songstart > 0) {
        songstart = songutilNormalizePosition (songstart, speed);
      }
      if (songend > 0) {
        songend = songutilNormalizePosition (songend, speed);
      }
    }

    dur = songGetNum (seint->song, TAG_DURATION);
    if (songstart > 0 && songstart > dur) {
      if (uisongedit->statusMsg != NULL) {
        /* CONTEXT: song editor: status msg: (song start may not exceed the song duration) */
        snprintf (tbuff, sizeof (tbuff), _("%s may not exceed the song duration"),
            tagdefs [TAG_SONGSTART].displayname);
        uiLabelSetText (uisongedit->statusMsg, tbuff);
        logMsg (LOG_DBG, LOG_IMPORTANT, "song-start>dur");
        valid = false;
      }
    }
    if (songend > 0 && songend > dur) {
      if (uisongedit->statusMsg != NULL) {
        /* CONTEXT: song editor: status msg: (song end may not exceed the song duration) */
        snprintf (tbuff, sizeof (tbuff), _("%s may not exceed the song duration"),
            tagdefs [TAG_SONGEND].displayname);
        uiLabelSetText (uisongedit->statusMsg, tbuff);
        logMsg (LOG_DBG, LOG_IMPORTANT, "song-end>dur");
        valid = false;
      }
    }
    if (songstart > 0 && songend > 0 && songstart >= songend) {
      if (uisongedit->statusMsg != NULL) {
        /* CONTEXT: song editor: status msg: (song end must be greater than song start) */
        snprintf (tbuff, sizeof (tbuff), _("%1$s must be greater than %2$s"),
            tagdefs [TAG_SONGEND].displayname,
            tagdefs [TAG_SONGSTART].displayname);
        uiLabelSetText (uisongedit->statusMsg, tbuff);
        logMsg (LOG_DBG, LOG_IMPORTANT, "song-end-<-song-start");
        valid = false;
      }
    }

    if (seint->keywordidx != UISE_NOT_DISPLAYED) {
      bool        val;

      tval = uiEntryGetValue (seint->items [seint->keywordidx].uiwidgetp);
      val = validate (tbuff, sizeof (tbuff), tagdefs [TAG_KEYWORD].displayname,
          tval, VAL_NO_SPACES);
      if (val == false) {
        uiLabelSetText (uisongedit->statusMsg, tbuff);
        logMsg (LOG_DBG, LOG_IMPORTANT, "keyword-has-spaces");
        valid = false;
      }
    }
  }

  if (! valid) {
    /* don't change anything if the validation checks fail */
    return UICB_CONT;
  }

  if (! seint->ineditallapply) {
    chglist = uisongeditGetChangedData (uisongedit);
  }

  for (int count = 0; count < seint->itemcount; ++count) {
    int   chkvalue;
    int   tagkey = seint->items [count].tagkey;

    if (! seint->items [count].changed) {
      continue;
    }

    chkvalue = uisongeditGetCheckValue (uisongedit, tagkey);

    if (chkvalue == UISE_CHK_STR) {
      songSetStr (seint->song, tagkey, nlistGetStr (chglist, tagkey));
    }

    if (chkvalue == UISE_CHK_NUM) {
      long  nval;

      nval = nlistGetNum (chglist, tagkey);

      if (tagkey == TAG_BPM) {
        int     speed;

        if (nval <= 0) {
          nval = LIST_VALUE_INVALID;
        }
        speed = songGetNum (seint->song, TAG_SPEEDADJUSTMENT);
        if (nval > 0) {
          nval = songutilNormalizeBPM (nval, speed);
          nval = danceConvertBPMtoMPM (seint->currdanceidx, nval, DANCE_NO_FORCE);
        }
      }
      songSetNum (seint->song, tagkey, nval);
    }

    if (chkvalue == UISE_CHK_DOUBLE) {
      songSetDouble (seint->song, tagkey, nlistGetDouble (chglist, tagkey));
    }

    if (chkvalue == UISE_CHK_LIST) {
      songSetList (seint->song, tagkey, nlistGetStr (chglist, tagkey));
    }
  }

  /* song start, song end and bpm must be adjusted if they changed */
  /* on a speed change, the song start, song end and bpm will be modified */
  /* and the changed flag will be set. */
  if (uisongedit->savecb != NULL) {
    int     speed;
    ssize_t val;

    speed = songGetNum (seint->song, TAG_SPEEDADJUSTMENT);
    if (speed > 0 && speed != 100) {
      if (seint->songstartidx != UISE_NOT_DISPLAYED &&
          seint->items [seint->songstartidx].changed) {
        val = songGetNum (seint->song, TAG_SONGSTART);
        if (val > 0) {
          val = songutilNormalizePosition (val, speed);
          songSetNum (seint->song, TAG_SONGSTART, val);
        }
      }
      if (seint->songendidx != UISE_NOT_DISPLAYED &&
          seint->items [seint->songendidx].changed) {
        val = songGetNum (seint->song, TAG_SONGEND);
        if (val > 0) {
          val = songutilNormalizePosition (val, speed);
          songSetNum (seint->song, TAG_SONGEND, val);
        }
      }
    }
  }

  if (! seint->ineditallapply) {
    nlistFree (chglist);
    /* reset the changed flags and indicators */
    uisongeditClearChanged (uisongedit, UISONGEDIT_ALL);
  }

  if (uisongedit->savecb != NULL) {
    /* the callback re-loads the song editor */
    callbackHandlerI (uisongedit->savecb, seint->dbidx);
  }

  logProcEnd ("");
  return UICB_CONT;
}

static int
uisongeditGetCheckValue (uisongedit_t *uisongedit, int tagkey)
{
  int   chkvalue = UISE_CHK_NONE;

  switch (tagdefs [tagkey].editType) {
    case ET_ENTRY: {
      chkvalue = UISE_CHK_STR;
      if (tagkey == TAG_TAGS) {
        chkvalue = UISE_CHK_LIST;
      }
      break;
    }
    case ET_COMBOBOX: {
      if (tagkey == TAG_DANCE) {
        chkvalue = UISE_CHK_NUM;
      }
      if (tagkey == TAG_GENRE) {
        chkvalue = UISE_CHK_NUM;
      }
      break;
    }
    case ET_SPINBOX_TEXT: {
      if (tagkey == TAG_FAVORITE) {
        chkvalue = UISE_CHK_NUM;
      }
      if (tagkey == TAG_DANCELEVEL) {
        chkvalue = UISE_CHK_NUM;
      }
      if (tagkey == TAG_DANCERATING) {
        chkvalue = UISE_CHK_NUM;
      }
      if (tagkey == TAG_STATUS) {
        chkvalue = UISE_CHK_NUM;
      }
      break;
    }
    case ET_SPINBOX: {
      chkvalue = UISE_CHK_NUM;
      break;
    }
    case ET_SPINBOX_TIME: {
      chkvalue = UISE_CHK_NUM;
      break;
    }
    case ET_SCALE: {
      chkvalue = UISE_CHK_DOUBLE;
      if (tagkey == TAG_SPEEDADJUSTMENT) {
        chkvalue = UISE_CHK_NUM;
      }
      break;
    }
    case ET_SWITCH: {
      chkvalue = UISE_CHK_NUM;
      break;
    }
    default: {
      chkvalue = UISE_CHK_NONE;
      break;
    }
  }

  return chkvalue;
}

static nlist_t *
uisongeditGetChangedData (uisongedit_t *uisongedit)
{
  se_internal_t   *seint =  NULL;
  nlist_t         *chglist = NULL;

  seint = uisongedit->seInternalData;
  chglist = nlistAlloc ("se-chg-list", LIST_ORDERED, NULL);

  for (int count = 0; count < seint->itemcount; ++count) {
    const char  *ndata = NULL;
    long        nval = LIST_VALUE_INVALID;
    double      ndval = LIST_DOUBLE_INVALID;
    int         chkvalue;
    int         tagkey = seint->items [count].tagkey;

    if (! seint->items [count].changed) {
      continue;
    }

    chkvalue = uisongeditGetCheckValue (uisongedit, tagkey);

    switch (tagdefs [tagkey].editType) {
      case ET_ENTRY: {
        ndata = uiEntryGetValue (seint->items [count].uiwidgetp);
        break;
      }
      case ET_COMBOBOX: {
        if (tagkey == TAG_DANCE) {
          nval = uidanceGetValue (seint->items [count].uidance);
        }
        if (tagkey == TAG_GENRE) {
          nval = uigenreGetValue (seint->items [count].uigenre);
        }
        break;
      }
      case ET_SPINBOX_TEXT: {
        if (tagkey == TAG_FAVORITE) {
          nval = uifavoriteGetValue (seint->items [count].uifavorite);
        }
        if (tagkey == TAG_DANCELEVEL) {
          nval = uilevelGetValue (seint->items [count].uilevel);
        }
        if (tagkey == TAG_DANCERATING) {
          nval = uiratingGetValue (seint->items [count].uirating);
        }
        if (tagkey == TAG_STATUS) {
          nval = uistatusGetValue (seint->items [count].uistatus);
        }
        break;
      }
      case ET_SPINBOX: {
        nval = uiSpinboxGetValue (seint->items [count].uiwidgetp);
        break;
      }
      case ET_SPINBOX_TIME: {
        nval = uiSpinboxTimeGetValue (seint->items [count].uiwidgetp);
        break;
      }
      case ET_SCALE: {
        ndval = uiScaleGetValue (seint->items [count].uiwidgetp);
        if (tagkey == TAG_SPEEDADJUSTMENT) {
          nval = round (ndval);
        }
        break;
      }
      case ET_SWITCH: {
        nval = uiSwitchGetValue (seint->items [count].uiwidgetp);
        break;
      }
      default: {
        break;
      }
    }

    if (chkvalue == UISE_CHK_STR ||
        chkvalue == UISE_CHK_LIST) {
      nlistSetStr (chglist, tagkey, ndata);
    }

    if (chkvalue == UISE_CHK_NUM) {
      if (tagkey == TAG_BPM) {
        if (nval <= 0) {
          nval = LIST_VALUE_INVALID;
        }
      }
      nlistSetNum (chglist, tagkey, nval);
    }

    if (chkvalue == UISE_CHK_DOUBLE) {
      nlistSetDouble (chglist, tagkey, ndval);
    }
  }

  return chglist;
}

static bool
uisongeditFirstSelection (void *udata)
{
  uisongedit_t  *uisongedit = udata;

  logProcBegin ();
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: song edit: first");
  uisongselFirstSelection (uisongedit->uisongsel);
  uisongeditClearChanged (uisongedit, UISONGEDIT_ALL);
  logProcEnd ("");
  return UICB_CONT;
}

static bool
uisongeditPreviousSelection (void *udata)
{
  uisongedit_t  *uisongedit = udata;

  logProcBegin ();
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: song edit: previous");
  uisongselPreviousSelection (uisongedit->uisongsel);
  uisongeditClearChanged (uisongedit, UISONGEDIT_ALL);
  logProcEnd ("");
  return UICB_CONT;
}

static bool
uisongeditNextSelection (void *udata)
{
  uisongedit_t  *uisongedit = udata;

  logProcBegin ();
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: song edit: next");
  uisongselNextSelection (uisongedit->uisongsel);
  uisongeditClearChanged (uisongedit, UISONGEDIT_ALL);
  logProcEnd ("");
  return UICB_CONT;
}

static bool
uisongeditCopyPath (void *udata)
{
  uisongedit_t  *uisongedit = udata;
  se_internal_t *seint;
  const char    *txt;
  char          ffn [MAXPATHLEN];

  seint = uisongedit->seInternalData;

  txt = uiLabelGetText (seint->wcont [UISE_W_FILE_DISP]);
  audiosrcFullPath (txt, ffn, sizeof (ffn), 0, NULL);
  pathDisplayPath (ffn, sizeof (ffn));
  uiClipboardSet (ffn);

  return UICB_CONT;
}

static bool
uisongeditKeyEvent (void *udata)
{
  uisongedit_t    *uisongedit = udata;
  se_internal_t *seint;

  seint = uisongedit->seInternalData;

  if (uiEventIsKeyPressEvent (seint->wcont [UISE_W_KEY_HNDLR]) &&
      uiEventIsAudioPlayKey (seint->wcont [UISE_W_KEY_HNDLR])) {
    uisongselPlayCallback (uisongedit->uisongsel);
  }

  if (uiEventIsKeyPressEvent (seint->wcont [UISE_W_KEY_HNDLR])) {
    if (uiEventIsControlPressed (seint->wcont [UISE_W_KEY_HNDLR])) {
      if (uiEventIsKey (seint->wcont [UISE_W_KEY_HNDLR], 'S')) {
        uisongeditSaveCallback (uisongedit);
        return UICB_STOP;
      }
      if (uiEventIsKey (seint->wcont [UISE_W_KEY_HNDLR], 'N')) {
        uisongeditNextSelection (uisongedit);
        return UICB_STOP;
      }
      if (uiEventIsKey (seint->wcont [UISE_W_KEY_HNDLR], 'P')) {
        uisongeditPreviousSelection (uisongedit);
        return UICB_STOP;
      }
    }
  }

  return UICB_CONT;
}

static int
uisongeditEntryChangedCallback (uiwcont_t *entry, const char *label, void *udata)
{
  se_internal_t *seint =  udata;

  seint->checkchanged = true;
  return 0;
}

static bool
uisongeditChangedCallback (void *udata)
{
  se_internal_t *seint =  udata;

  seint->checkchanged = true;
  return UICB_CONT;
}

static char *
uisongeditGetBPMRangeDisplay (int danceidx)
{
  int     lowmpm, highmpm;
  dance_t *dances;
  char    tbuff [100];
  char    *str;

  if (danceidx < 0) {
    return "";
  }

  dances = bdjvarsdfGet (BDJVDF_DANCES);

  lowmpm = danceGetNum (dances, danceidx, DANCE_MPM_LOW);
  lowmpm = danceConvertMPMtoBPM (danceidx, lowmpm);

  highmpm = danceGetNum (dances, danceidx, DANCE_MPM_HIGH);
  highmpm = danceConvertMPMtoBPM (danceidx, highmpm);

  *tbuff = '\0';
  if (lowmpm > 0 && highmpm > 0) {
    if (lowmpm == highmpm) {
      snprintf (tbuff, sizeof (tbuff), " (%d)", lowmpm);
    } else {
      snprintf (tbuff, sizeof (tbuff), " (%d - %d)", lowmpm, highmpm);
    }
  }
  str = mdstrdup (tbuff);
  return str;
}

static void
uisongeditSetBPMRangeDisplay (se_internal_t *seint,
    int bpmdispidx, ilistidx_t danceidx)
{
  if (bpmdispidx != UISE_NOT_DISPLAYED &&
      danceidx >= 0) {
    char    *data;

    data = uisongeditGetBPMRangeDisplay (danceidx);
    uiLabelSetText (seint->items [bpmdispidx].uiwidgetp, data);
    dataFree (data);
  }
}

static void
uisongeditSetBPMIncrement (se_internal_t *seint, ilistidx_t danceidx)
{
  if (seint->bpmidx != UISE_NOT_DISPLAYED &&
      danceidx >= 0 &&
      bdjoptGetNum (OPT_G_BPM) == BPM_BPM) {
    int   timesig;

    /* set the bpm increment values */
    timesig = danceGetTimeSignature (danceidx);
    uiSpinboxSetIncrement (seint->items [seint->bpmidx].uiwidgetp,
        (double) danceTimesigValues [timesig],
        (double) danceTimesigValues [timesig] * 5.0);
  }
}

static int32_t
uisongeditValHMSPCallback (void *udata, const char *label, const char *txt)
{
  uisongedititem_t *item = udata;
  uisongedit_t  *uisongedit;
  se_internal_t *seint;
  char          tbuff [200];
  int32_t       value;
  bool          val;

  logProcBegin ();

  seint = item->seint;
  uisongedit = seint->uisongedit;

  uiLabelSetText (uisongedit->statusMsg, "");
  val = validate (tbuff, sizeof (tbuff), label, txt, VAL_HMS_PRECISE);
  if (val == false) {
    int32_t oval;

    oval = uiSpinboxTimeGetValue (item->uiwidgetp);
    uiLabelSetText (uisongedit->statusMsg, tbuff);
    return oval;
  }

  value = tmutilStrToMS (txt);
  logProcEnd ("");
  return value;
}

