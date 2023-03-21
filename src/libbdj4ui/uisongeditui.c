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

#include "bdj4intl.h"
#include "bdjopt.h"
#include "bdjvarsdf.h"
#include "callback.h"
#include "conn.h"
#include "dance.h"
#include "level.h"
#include "log.h"
#include "mdebug.h"
#include "nlist.h"
#include "pathbld.h"
#include "pathutil.h"
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

enum {
  SONGEDIT_CHK_NONE,
  SONGEDIT_CHK_NUM,
  SONGEDIT_CHK_DOUBLE,
  SONGEDIT_CHK_STR,
  SONGEDIT_CHK_LIST,
};

typedef struct se_internal se_internal_t;

typedef struct {
  int             tagkey;
  uichgind_t      *chgind;
  uiwcont_t      label;
  union {
    uientry_t     *entry;
    uispinbox_t   *spinbox;
    uiwcont_t     uiwidget;
    uidance_t     *uidance;
    uifavorite_t  *uifavorite;
    uigenre_t     *uigenre;
    uilevel_t     *uilevel;
    uirating_t    *uirating;
    uistatus_t    *uistatus;
  };
  uiwcont_t       display;
  callback_t      *callback;
  se_internal_t   *seint;           // need for scale changed.
  bool            lastchanged : 1;
  bool            changed : 1;
} uisongedititem_t;

enum {
  UISONGEDIT_CB_FIRST,
  UISONGEDIT_CB_PLAY,
  UISONGEDIT_CB_SAVE,
  UISONGEDIT_CB_COPY_TEXT,
  UISONGEDIT_CB_PREV,
  UISONGEDIT_CB_NEXT,
  UISONGEDIT_CB_KEYB,
  UISONGEDIT_CB_CHANGED,
  UISONGEDIT_CB_MAX,
};

enum {
  UISONGEDIT_BUTTON_FIRST,
  UISONGEDIT_BUTTON_PREV,
  UISONGEDIT_BUTTON_NEXT,
  UISONGEDIT_BUTTON_PLAY,
  UISONGEDIT_BUTTON_SAVE,
  UISONGEDIT_BUTTON_COPY_TEXT,
  UISONGEDIT_BUTTON_MAX,
};

enum {
  UISONGEDIT_MAIN_TIMER = 40,
};


typedef struct se_internal {
  uiwcont_t          editalldisp;
  uiwcont_t          *parentwin;
  uiwcont_t          vbox;
  uiwcont_t          musicbrainzPixbuf;
  uiwcont_t          modified;
  uiwcont_t          audioidImg;
  uiwcont_t          filedisp;
  uiwcont_t           *sgentry;
  uiwcont_t           *sgsbint;
  uiwcont_t           *sgsbtext;
  uiwcont_t           *sgsbtime;
  uiwcont_t           *sgscale;
  uiwcont_t           *sgscaledisp;
  callback_t          *callbacks [UISONGEDIT_CB_MAX];
  uibutton_t          *buttons [UISONGEDIT_BUTTON_MAX];
  level_t             *levels;
  song_t              *song;
  dbidx_t             dbidx;
  int                 itemcount;
  uisongedititem_t    *items;
  int                 changed;
  uikey_t             *uikey;
  int                 bpmidx;
  int                 songstartidx;
  int                 songendidx;
  int                 speedidx;
  int                 lastspeed;
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
static bool uisongeditScaleDisplayCallback (void *udata, double value);
static bool uisongeditSaveCallback (void *udata);
static bool uisongeditSave (void *udata, nlist_t *chglist);
static int  uisongeditGetCheckValue (uisongedit_t *uisongedit, int tagkey);
static nlist_t * uisongeditGetChangedData (uisongedit_t *uisongedit);
static bool uisongeditFirstSelection (void *udata);
static bool uisongeditPreviousSelection (void *udata);
static bool uisongeditNextSelection (void *udata);
static bool uisongeditCopyPath (void *udata);
static bool uisongeditKeyEvent (void *udata);
static bool uisongeditApplyAdjCallback (void *udata);
static int uisongeditEntryChangedCallback (uientry_t *entry, void *udata);
static bool uisongeditChangedCallback (void *udata);
static char * uisongeditGetBPMRangeDisplay (int danceidx);

void
uisongeditUIInit (uisongedit_t *uisongedit)
{
  se_internal_t  *seint;

  logProcBegin (LOG_PROC, "uisongeditUIInit");

  seint = mdmalloc (sizeof (se_internal_t));
  seint->parentwin = NULL;
  seint->itemcount = 0;
  seint->items = NULL;
  seint->changed = 0;
  seint->uikey = NULL;
  seint->levels = bdjvarsdfGet (BDJVDF_LEVELS);
  seint->bpmidx = -1;
  seint->songstartidx = -1;
  seint->songendidx = -1;
  seint->speedidx = -1;
  seint->dbidx = -1;
  seint->lastspeed = -1;
  for (int i = 0; i < UISONGEDIT_BUTTON_MAX; ++i) {
    seint->buttons [i] = NULL;
  }
  for (int i = 0; i < UISONGEDIT_CB_MAX; ++i) {
    seint->callbacks [i] = NULL;
  }
  seint->checkchanged = false;
  seint->ineditallapply = false;

  uiwcontInit (&seint->vbox);
  uiwcontInit (&seint->audioidImg);
  uiwcontInit (&seint->modified);

  seint->sgentry = uiCreateSizeGroupHoriz ();
  seint->sgsbint = uiCreateSizeGroupHoriz ();
  seint->sgsbtext = uiCreateSizeGroupHoriz ();
  seint->sgsbtime = uiCreateSizeGroupHoriz ();
  seint->sgscale = uiCreateSizeGroupHoriz ();
  seint->sgscaledisp = uiCreateSizeGroupHoriz ();

  uisongedit->seInternalData = seint;
  logProcEnd (LOG_PROC, "uisongeditUIInit", "");
}

void
uisongeditUIFree (uisongedit_t *uisongedit)
{
  se_internal_t *seint;

  logProcBegin (LOG_PROC, "uisongeditUIFree");
  if (uisongedit == NULL) {
    logProcEnd (LOG_PROC, "uisongeditUIFree", "null");
    return;
  }

  seint = uisongedit->seInternalData;
  if (seint != NULL) {
    for (int count = 0; count < seint->itemcount; ++count) {
      int   tagkey;

      uichgindFree (seint->items [count].chgind);

      tagkey = seint->items [count].tagkey;

      switch (tagdefs [tagkey].editType) {
        case ET_ENTRY: {
          uiEntryFree (seint->items [count].entry);
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
        case ET_SPINBOX: {
          break;
        }
        case ET_SPINBOX_TIME: {
          if (seint->items [count].spinbox != NULL) {
            uiSpinboxFree (seint->items [count].spinbox);
          }
          break;
        }
        case ET_NA:
        case ET_SCALE:
        case ET_LABEL: {
          break;
        }
      }
      callbackFree (seint->items [count].callback);
    }

    uiwcontFree (seint->sgentry);
    uiwcontFree (seint->sgsbint);
    uiwcontFree (seint->sgsbtext);
    uiwcontFree (seint->sgsbtime);
    uiwcontFree (seint->sgscale);
    uiwcontFree (seint->sgscaledisp);

    for (int i = 0; i < UISONGEDIT_BUTTON_MAX; ++i) {
      uiButtonFree (seint->buttons [i]);
    }
    dataFree (seint->items);
    uiKeyFree (seint->uikey);
    for (int i = 0; i < UISONGEDIT_CB_MAX; ++i) {
      callbackFree (seint->callbacks [i]);
    }
    mdfree (seint);
    uisongedit->seInternalData = NULL;
  }

  logProcEnd (LOG_PROC, "uisongeditUIFree", "");
}

uiwcont_t *
uisongeditBuildUI (uisongsel_t *uisongsel, uisongedit_t *uisongedit,
    uiwcont_t *parentwin, uiwcont_t *statusMsg)
{
  se_internal_t     *seint;
  uiwcont_t         hbox;
  uiwcont_t         col;
  uiwcont_t         *szgrp;
  uiwcont_t         uiwidget;
  uibutton_t        *uibutton;
  uiwcont_t         *uiwidgetp;
  int               count;
  char              tbuff [MAXPATHLEN];

  logProcBegin (LOG_PROC, "uisongeditBuildUI");
  logProcBegin (LOG_PROC, "uisongeditBuildUI");

  uisongedit->statusMsg = statusMsg;
  uisongedit->uisongsel = uisongsel;
  seint = uisongedit->seInternalData;
  seint->parentwin = parentwin;

  uiCreateVertBox (&seint->vbox);
  uiWidgetExpandHoriz (&seint->vbox);

  seint->uikey = uiKeyAlloc ();
  seint->callbacks [UISONGEDIT_CB_KEYB] = callbackInit (
      uisongeditKeyEvent, uisongedit, NULL);
  uiKeySetKeyCallback (seint->uikey, &seint->vbox,
      seint->callbacks [UISONGEDIT_CB_KEYB]);

  uiCreateHorizBox (&hbox);
  uiWidgetExpandHoriz (&hbox);
  uiWidgetAlignHorizFill (&hbox);
  uiBoxPackStart (&seint->vbox, &hbox);

  seint->callbacks [UISONGEDIT_CB_FIRST] = callbackInit (
      uisongeditFirstSelection, uisongedit, "songedit: first");
  uibutton = uiCreateButton (seint->callbacks [UISONGEDIT_CB_FIRST],
      /* CONTEXT: song editor : first song */
      _("First"), NULL);
  seint->buttons [UISONGEDIT_BUTTON_FIRST] = uibutton;
  uiwidgetp = uiButtonGetWidgetContainer (uibutton);
  uiBoxPackStart (&hbox, uiwidgetp);

  seint->callbacks [UISONGEDIT_CB_PREV] = callbackInit (
      uisongeditPreviousSelection, uisongedit, "songedit: previous");
  uibutton = uiCreateButton (seint->callbacks [UISONGEDIT_CB_PREV],
      /* CONTEXT: song editor : previous song */
      _("Previous"), NULL);
  seint->buttons [UISONGEDIT_BUTTON_PREV] = uibutton;
  uiButtonSetRepeat (uibutton, UISONGEDIT_REPEAT_TIME);
  uiwidgetp = uiButtonGetWidgetContainer (uibutton);
  uiBoxPackStart (&hbox, uiwidgetp);

  seint->callbacks [UISONGEDIT_CB_NEXT] = callbackInit (
      uisongeditNextSelection, uisongedit, "songedit: next");
  uibutton = uiCreateButton (seint->callbacks [UISONGEDIT_CB_NEXT],
      /* CONTEXT: song editor : next song */
      _("Next"), NULL);
  seint->buttons [UISONGEDIT_BUTTON_NEXT] = uibutton;
  uiButtonSetRepeat (uibutton, UISONGEDIT_REPEAT_TIME);
  uiwidgetp = uiButtonGetWidgetContainer (uibutton);
  uiBoxPackStart (&hbox, uiwidgetp);

  seint->callbacks [UISONGEDIT_CB_PLAY] = callbackInit (
      uisongselPlayCallback, uisongsel, "songedit: play");
  uibutton = uiCreateButton (seint->callbacks [UISONGEDIT_CB_PLAY],
      /* CONTEXT: song editor : play song */
      _("Play"), NULL);
  seint->buttons [UISONGEDIT_BUTTON_PLAY] = uibutton;
  uiwidgetp = uiButtonGetWidgetContainer (uibutton);
  uiBoxPackStart (&hbox, uiwidgetp);

  seint->callbacks [UISONGEDIT_CB_SAVE] = callbackInit (
      uisongeditSaveCallback, uisongedit, "songedit: save");
  uibutton = uiCreateButton (seint->callbacks [UISONGEDIT_CB_SAVE],
      /* CONTEXT: song editor : save data */
      _("Save"), NULL);
  seint->buttons [UISONGEDIT_BUTTON_SAVE] = uibutton;
  uiwidgetp = uiButtonGetWidgetContainer (uibutton);
  uiBoxPackEnd (&hbox, uiwidgetp);

  uiCreateLabel (&uiwidget, "");
  uiBoxPackEnd (&hbox, &uiwidget);
  uiWidgetSetMarginEnd (&uiwidget, 6);
  uiWidgetSetClass (&uiwidget, DARKACCENT_CLASS);
  uiwcontCopy (&seint->editalldisp, &uiwidget);

  /* audio-identification logo, modified indicator, */
  /* copy button, file label, filename */
  uiCreateHorizBox (&hbox);
  uiWidgetExpandHoriz (&hbox);
  uiWidgetAlignHorizFill (&hbox);
  uiBoxPackStart (&seint->vbox, &hbox);

  pathbldMakePath (tbuff, sizeof (tbuff), "musicbrainz-logo", BDJ4_IMG_SVG_EXT,
      PATHBLD_MP_DIR_IMG);
  uiImageFromFile (&seint->musicbrainzPixbuf, tbuff);
  uiImageConvertToPixbuf (&seint->musicbrainzPixbuf);
  uiWidgetMakePersistent (&seint->musicbrainzPixbuf);

  uiImageNew (&seint->audioidImg);
  uiImageClear (&seint->audioidImg);
  uiWidgetSetSizeRequest (&seint->audioidImg, 24, -1);
  uiWidgetSetMarginStart (&seint->audioidImg, 1);
  uiBoxPackStart (&hbox, &seint->audioidImg);

  uiCreateLabel (&uiwidget, " ");
  uiBoxPackStart (&hbox, &uiwidget);
  uiwcontCopy (&seint->modified, &uiwidget);
  uiWidgetSetClass (&seint->modified, DARKACCENT_CLASS);

  seint->callbacks [UISONGEDIT_CB_COPY_TEXT] = callbackInit (
      uisongeditCopyPath, uisongedit, "songedit: copy-text");
  uibutton = uiCreateButton (
      seint->callbacks [UISONGEDIT_CB_COPY_TEXT],
      "", NULL);
  seint->buttons [UISONGEDIT_BUTTON_COPY_TEXT] = uibutton;
  uiwidgetp = uiButtonGetWidgetContainer (uibutton);
  uiButtonSetImageIcon (uibutton, "edit-copy");
  uiWidgetSetMarginStart (uiwidgetp, 1);
  uiBoxPackStart (&hbox, uiwidgetp);

  /* CONTEXT: song editor: label for displaying the audio file path */
  uiCreateColonLabel (&uiwidget, _("File"));
  uiBoxPackStart (&hbox, &uiwidget);
  uiWidgetSetState (&uiwidget, UIWIDGET_DISABLE);

  uiCreateLabel (&uiwidget, "");
  uiLabelEllipsizeOn (&uiwidget);
  uiBoxPackStart (&hbox, &uiwidget);
  uiwcontCopy (&seint->filedisp, &uiwidget);
  uiWidgetSetClass (&seint->filedisp, DARKACCENT_CLASS);
  uiLabelSetSelectable (&seint->filedisp);

  uiCreateHorizBox (&hbox);
  uiWidgetExpandHoriz (&hbox);
  uiWidgetAlignHorizFill (&hbox);
  uiBoxPackStartExpand (&seint->vbox, &hbox);

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
    uiwcontInit (&seint->items [i].uiwidget);
    seint->items [i].entry = NULL;
    seint->items [i].chgind = NULL;
    seint->items [i].lastchanged = false;
    seint->items [i].changed = false;
    seint->items [i].callback = NULL;
    seint->items [i].seint = seint;
  }

  /* must be set before the items are instantiated */
  seint->callbacks [UISONGEDIT_CB_CHANGED] = callbackInit (
      uisongeditChangedCallback, seint, NULL);

  szgrp = uiCreateSizeGroupHoriz ();

  for (int i = DISP_SEL_SONGEDIT_A; i <= DISP_SEL_SONGEDIT_C; ++i) {
    uiCreateVertBox (&col);
    uiWidgetSetAllMargins (&col, 4);
    uiWidgetExpandHoriz (&col);
    uiWidgetExpandVert (&col);
    uiBoxPackStartExpand (&hbox, &col);

    uisongeditAddDisplay (uisongedit, &col, szgrp, i);
  }

  uiwcontFree (szgrp);

  logProcEnd (LOG_PROC, "uisongeditBuildUI", "");
  return &seint->vbox;
}

void
uisongeditLoadData (uisongedit_t *uisongedit, song_t *song,
    dbidx_t dbidx, int editallflag)
{
  se_internal_t *seint;
  char            *data;
  long            val;
  double          dval;

  logProcBegin (LOG_PROC, "uisongeditLoadData");
  seint = uisongedit->seInternalData;
  seint->song = song;
  seint->dbidx = dbidx;
  seint->changed = 0;

  data = uisongGetDisplay (song, TAG_FILE, &val, &dval);
  uiLabelSetText (&seint->filedisp, data);
  dataFree (data);
  data = NULL;

  uiImageClear (&seint->audioidImg);
  data = songGetStr (song, TAG_RECORDING_ID);
  if (data != NULL && *data) {
    uiImageSetFromPixbuf (&seint->audioidImg, &seint->musicbrainzPixbuf);
  }

  val = songGetNum (song, TAG_ADJUSTFLAGS);
  uiLabelSetText (&seint->modified, " ");
  if (val != SONG_ADJUST_NONE) {
    uiLabelSetText (&seint->modified, "*");
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
      data = uisongeditGetBPMRangeDisplay (songGetNum (song, TAG_DANCE));
      uiLabelSetText (&seint->items [count].uiwidget, data);
      dataFree (data);
      continue;
    }
    data = uisongGetDisplay (song, tagkey, &val, &dval);
    if (! seint->ineditallapply) {
      seint->items [count].changed = false;
      seint->items [count].lastchanged = false;
    }

    switch (tagdefs [tagkey].editType) {
      case ET_ENTRY: {
        uiEntrySetValue (seint->items [count].entry, "");
        if (data != NULL) {
          uiEntrySetValue (seint->items [count].entry, data);
        }
        break;
      }
      case ET_COMBOBOX: {
        if (tagkey == TAG_DANCE) {
          if (val < 0) { val = -1; } // empty value
          uidanceSetValue (seint->items [count].uidance, val);
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
        if (data != NULL) {
          fprintf (stderr, "et_spinbox: mismatch type\n");
        }
        if (val < 0) { val = 0; }
        uiSpinboxSetValue (&seint->items [count].uiwidget, val);
        break;
      }
      case ET_SPINBOX_TIME: {
        if (val < 0) { val = 0; }
        uiSpinboxTimeSetValue (seint->items [count].spinbox, val);
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
        if (data != NULL) {
          fprintf (stderr, "et_scale: mismatch type\n");
        }
        uiScaleSetValue (&seint->items [count].uiwidget, dval);
        uisongeditScaleDisplayCallback (&seint->items [count], dval);
        break;
      }
      case ET_LABEL: {
        if (data != NULL) {
          uiLabelSetText (&seint->items [count].uiwidget, data);
        }
        break;
      }
      default: {
        break;
      }
    }

    dataFree (data);
    data = NULL;
  }

  logProcEnd (LOG_PROC, "uisongeditLoadData", "");
}

void
uisongeditUIMainLoop (uisongedit_t *uisongedit)
{
  se_internal_t   *seint;

  seint = uisongedit->seInternalData;

  uiButtonCheckRepeat (seint->buttons [UISONGEDIT_BUTTON_NEXT]);
  uiButtonCheckRepeat (seint->buttons [UISONGEDIT_BUTTON_PREV]);
  uisongeditCheckChanged (uisongedit);
  return;
}

void
uisongeditSetBPMValue (uisongedit_t *uisongedit, const char *args)
{
  se_internal_t *seint;
  int             val;

  logProcBegin (LOG_PROC, "uisongeditSetBPMValue");
  seint = uisongedit->seInternalData;

  if (seint->bpmidx < 0) {
    logProcEnd (LOG_PROC, "uisongeditSetBPMValue", "bad-bpmidx");
    return;
  }

  val = atoi (args);
  uiSpinboxSetValue (&seint->items [seint->bpmidx].uiwidget, val);
  logProcEnd (LOG_PROC, "uisongeditSetBPMValue", "");
}

void
uisongeditSetPlayButtonState (uisongedit_t *uisongedit, int active)
{
  se_internal_t *seint;

  logProcBegin (LOG_PROC, "uisongeditSetPlayButtonState");
  seint = uisongedit->seInternalData;

  /* if the player is active, disable the button */
  if (active) {
    uiButtonSetState (seint->buttons [UISONGEDIT_BUTTON_PLAY], UIWIDGET_DISABLE);
  } else {
    uiButtonSetState (seint->buttons [UISONGEDIT_BUTTON_PLAY], UIWIDGET_ENABLE);
  }
  logProcEnd (LOG_PROC, "uisongeditSetPlayButtonState", "");
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
    uiLabelSetText (&seint->editalldisp, _("Edit All"));
  } else {
    uiLabelSetText (&seint->editalldisp, "");
  }

  uiButtonSetState (seint->buttons [UISONGEDIT_BUTTON_SAVE], newstate);

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
      uiWidgetSetState (&seint->items [count].label, newstate);
    }

    switch (tagdefs [tagkey].editType) {
      case ET_ENTRY: {
        uiEntrySetState (seint->items [count].entry, newstate);
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
        uiSpinboxSetState (seint->items [count].spinbox, newstate);
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
      case ET_SPINBOX:
      case ET_SCALE: {
        uiWidgetSetState (&seint->items [count].uiwidget, newstate);
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

  logProcBegin (LOG_PROC, "uisongeditClearChanged");
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
  logProcEnd (LOG_PROC, "uisongeditClearChanged", "");
}

bool
uisongeditEditAllApply (uisongedit_t *uisongedit)
{
  se_internal_t   *seint =  NULL;
  nlist_t         *chglist;
  dbidx_t         lastdbidx;

  logProcBegin (LOG_PROC, "uisongeditEditAllApply");
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

  logProcEnd (LOG_PROC, "uisongeditEditAllApply", "");
  return UICB_CONT;
}

/* internal routines */

static void
uisongeditCheckChanged (uisongedit_t *uisongedit)
{
  se_internal_t   *seint;
  double          dval;
  double          ndval = LIST_DOUBLE_INVALID;
  long            val;
  long            nval = LIST_VALUE_INVALID;
  char            *songdata = NULL;
  const char      *ndata = NULL;
  int             bpmdispidx = -1;
  bool            spdchanged = false;
  int             speed = 100;
  int             danceidx = -1;

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
          ndata = uiEntryGetValue (seint->items [count].entry);
          break;
        }
        case ET_COMBOBOX: {
          if (tagkey == TAG_DANCE) {
            if (val < 0) { val = -1; }
            nval = uidanceGetValue (seint->items [count].uidance);
            danceidx = nval;
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
          nval = uiSpinboxGetValue (&seint->items [count].uiwidget);
          break;
        }
        case ET_SPINBOX_TIME: {
          if (val < 0) { val = 0; }
          nval = uiSpinboxTimeGetValue (seint->items [count].spinbox);
          break;
        }
        case ET_SCALE: {
          if (isnan (dval)) { dval = 0.0; }
          if (tagkey == TAG_SPEEDADJUSTMENT) {
            dval = (double) val;
            if (dval < 0.0) { dval = 0.0; }
            /* for the purposes of check-changed processing, check */
            /* speed adjustment as a double */
            chkvalue = SONGEDIT_CHK_DOUBLE;
          }
          ndval = uiScaleGetValue (&seint->items [count].uiwidget);
          if (ndval == LIST_DOUBLE_INVALID) { ndval = 0.0; }
          if (dval == LIST_DOUBLE_INVALID) { dval = 0.0; }
          if (isnan (dval)) { dval = 0.0; }
          break;
        }
        default: {
          break;
        }
      }

      if (chkvalue == SONGEDIT_CHK_STR ||
          chkvalue == SONGEDIT_CHK_LIST) {
        if ((ndata != NULL && songdata == NULL) ||
            strcmp (songdata, ndata) != 0) {
          seint->items [count].changed = true;
        } else {
          if (seint->items [count].changed) {
            seint->items [count].changed = false;
          }
        }
      }

      if (chkvalue == SONGEDIT_CHK_NUM) {
        int   rcdisc, rctrk;

        rcdisc = (tagkey == TAG_DISCNUMBER && val == 0.0 && nval == 1.0);
        rctrk = (tagkey == TAG_TRACKNUMBER && val == 0.0 && nval == 1.0);
        if (tagkey == TAG_FAVORITE) {
          if (val < 0) { val = 0; }
        }
        if (! rcdisc && ! rctrk && nval != val) {
          seint->items [count].changed = true;
        } else {
          if (seint->items [count].changed) {
            seint->items [count].changed = false;
          }
        }
      }

      if (chkvalue == SONGEDIT_CHK_DOUBLE) {
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
    }

    if (spdchanged) {
      if (seint->songstartidx != -1) {
        nval = uiSpinboxTimeGetValue (seint->items [seint->songstartidx].spinbox);
        if (nval > 0) {
          nval = songutilNormalizePosition (nval, seint->lastspeed);
          nval = songutilAdjustPosReal (nval, speed);
          uiSpinboxTimeSetValue (seint->items [seint->songstartidx].spinbox, nval);
        }
      }
      if (seint->songendidx != -1) {
        nval = uiSpinboxTimeGetValue (seint->items [seint->songendidx].spinbox);
        if (nval > 0) {
          nval = songutilNormalizePosition (nval, seint->lastspeed);
          nval = songutilAdjustPosReal (nval, speed);
          uiSpinboxTimeSetValue (seint->items [seint->songendidx].spinbox, nval);
        }
      }
      if (seint->bpmidx != -1) {
        nval = uiSpinboxGetValue (&seint->items [seint->bpmidx].uiwidget);
        if (nval > 0) {
          nval = songutilNormalizeBPM (nval, seint->lastspeed);
          nval = songutilAdjustBPM (nval, speed);
          uiSpinboxSetValue (&seint->items [seint->bpmidx].uiwidget, nval);
        }
      }
      seint->lastspeed = speed;
    }

    /* always re-create the BPM display */
    if (bpmdispidx != -1 && danceidx != -1) {
      char    *data;

      data = uisongeditGetBPMRangeDisplay (danceidx);
      uiLabelSetText (&seint->items [bpmdispidx].uiwidget, data);
      dataFree (data);
    }

    seint->checkchanged = false;
  } /* check changed is true */
}

static void
uisongeditAddDisplay (uisongedit_t *uisongedit, uiwcont_t *col, uiwcont_t *sg, int dispsel)
{
  slist_t         *sellist;
  uiwcont_t      hbox;
  char            *keystr;
  slistidx_t      dsiteridx;
  se_internal_t *seint;

  logProcBegin (LOG_PROC, "uisongeditAddDisplay");
  seint = uisongedit->seInternalData;
  sellist = dispselGetList (uisongedit->dispsel, dispsel);

  slistStartIterator (sellist, &dsiteridx);
  while ((keystr = slistIterateKey (sellist, &dsiteridx)) != NULL) {
    int   tagkey;

    tagkey = slistGetNum (sellist, keystr);

    if (tagkey >= TAG_KEY_MAX) {
      continue;
    }
    if (tagdefs [tagkey].editType != ET_LABEL &&
        ! tagdefs [tagkey].isEditable) {
      continue;
    }

    uiCreateHorizBox (&hbox);
    uiWidgetExpandHoriz (&hbox);
    uiBoxPackStart (col, &hbox);
    uisongeditAddItem (uisongedit, &hbox, sg, tagkey);
    seint->items [seint->itemcount].tagkey = tagkey;
    if (tagkey == TAG_BPM) {
      ++seint->itemcount;
      uisongeditAddItem (uisongedit, &hbox, sg, TAG_BPM_DISPLAY);
      seint->items [seint->itemcount].tagkey = TAG_BPM_DISPLAY;
    }
    ++seint->itemcount;
  }
  logProcEnd (LOG_PROC, "uisongeditAddDisplay", "");
}

static void
uisongeditAddItem (uisongedit_t *uisongedit, uiwcont_t *hbox, uiwcont_t *sg, int tagkey)
{
  uiwcont_t      uiwidget;
  se_internal_t *seint;

  logProcBegin (LOG_PROC, "uisongeditAddItem");
  seint = uisongedit->seInternalData;

  if (! tagdefs [tagkey].secondaryDisplay) {
    seint->items [seint->itemcount].chgind = uiCreateChangeIndicator (hbox);
    uichgindMarkNormal (seint->items [seint->itemcount].chgind);
    seint->items [seint->itemcount].changed = false;
    seint->items [seint->itemcount].lastchanged = false;

    uiCreateColonLabel (&uiwidget, tagdefs [tagkey].displayname);
    uiBoxPackStart (hbox, &uiwidget);
    uiSizeGroupAdd (sg, &uiwidget);
    uiwcontCopy (&seint->items [seint->itemcount].label, &uiwidget);
  }

  switch (tagdefs [tagkey].editType) {
    case ET_ENTRY: {
      uisongeditAddEntry (uisongedit, hbox, tagkey);
      break;
    }
    case ET_COMBOBOX: {
      if (tagkey == TAG_DANCE) {
        seint->items [seint->itemcount].uidance =
            uidanceDropDownCreate (hbox, seint->parentwin,
            UIDANCE_EMPTY_DANCE, "", UIDANCE_PACK_START, 1);
        uidanceSetCallback (seint->items [seint->itemcount].uidance,
            seint->callbacks [UISONGEDIT_CB_CHANGED]);
      }
      if (tagkey == TAG_GENRE) {
        seint->items [seint->itemcount].uigenre =
            uigenreDropDownCreate (hbox, seint->parentwin, false);
        uigenreSetCallback (seint->items [seint->itemcount].uigenre,
            seint->callbacks [UISONGEDIT_CB_CHANGED]);
      }
      break;
    }
    case ET_SPINBOX_TEXT: {
      if (tagkey == TAG_FAVORITE) {
        seint->items [seint->itemcount].uifavorite =
            uifavoriteSpinboxCreate (hbox);
        uifavoriteSetChangedCallback (seint->items [seint->itemcount].uifavorite,
            seint->callbacks [UISONGEDIT_CB_CHANGED]);
      }
      if (tagkey == TAG_DANCELEVEL) {
        seint->items [seint->itemcount].uilevel =
            uilevelSpinboxCreate (hbox, FALSE);
        uilevelSizeGroupAdd (seint->items [seint->itemcount].uilevel, seint->sgsbtext);
        uilevelSetChangedCallback (seint->items [seint->itemcount].uilevel,
            seint->callbacks [UISONGEDIT_CB_CHANGED]);
      }
      if (tagkey == TAG_DANCERATING) {
        seint->items [seint->itemcount].uirating =
            uiratingSpinboxCreate (hbox, FALSE);
        uiratingSizeGroupAdd (seint->items [seint->itemcount].uirating, seint->sgsbtext);
        uiratingSetChangedCallback (seint->items [seint->itemcount].uirating,
            seint->callbacks [UISONGEDIT_CB_CHANGED]);
      }
      if (tagkey == TAG_STATUS) {
        seint->items [seint->itemcount].uistatus =
            uistatusSpinboxCreate (hbox, FALSE);
        uistatusSizeGroupAdd (seint->items [seint->itemcount].uistatus, seint->sgsbtext);
        uistatusSetChangedCallback (seint->items [seint->itemcount].uistatus,
            seint->callbacks [UISONGEDIT_CB_CHANGED]);
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
    case ET_NA: {
      break;
    }
  }
  logProcEnd (LOG_PROC, "uisongeditAddItem", "");
}

static void
uisongeditAddEntry (uisongedit_t *uisongedit, uiwcont_t *hbox, int tagkey)
{
  uientry_t       *entryp;
  uiwcont_t      *uiwidgetp;
  se_internal_t *seint;

  logProcBegin (LOG_PROC, "uisongeditAddEntry");
  seint = uisongedit->seInternalData;
  entryp = uiEntryInit (20, 100);
  seint->items [seint->itemcount].entry = entryp;
  uiEntryCreate (entryp);
  /* set the validate callback to set the changed flag */
  uiEntrySetValidate (entryp,
      uisongeditEntryChangedCallback, seint, UIENTRY_IMMEDIATE);

  uiwidgetp = uiEntryGetWidgetContainer (entryp);
  uiWidgetAlignHorizFill (uiwidgetp);
  uiSizeGroupAdd (seint->sgentry, uiwidgetp);
  uiBoxPackStartExpand (hbox, uiwidgetp);
  logProcEnd (LOG_PROC, "uisongeditAddEntry", "");
}

static void
uisongeditAddSpinboxInt (uisongedit_t *uisongedit, uiwcont_t *hbox, int tagkey)
{
  uiwcont_t      *uiwidgetp;
  se_internal_t *seint;

  logProcBegin (LOG_PROC, "uisongeditAddSpinboxInt");
  seint = uisongedit->seInternalData;
  uiwidgetp = &seint->items [seint->itemcount].uiwidget;
  uiSpinboxIntCreate (uiwidgetp);
  if (tagkey == TAG_BPM) {
    uiSpinboxSet (uiwidgetp, 0.0, 400.0);
  }
  if (tagkey == TAG_TRACKNUMBER || tagkey == TAG_DISCNUMBER) {
    uiSpinboxSet (uiwidgetp, 1.0, 300.0);
  }
  uiSpinboxSetValueChangedCallback (uiwidgetp,
      seint->callbacks [UISONGEDIT_CB_CHANGED]);
  uiSizeGroupAdd (seint->sgsbint, uiwidgetp);
  uiBoxPackStart (hbox, uiwidgetp);
  logProcEnd (LOG_PROC, "uisongeditAddSpinboxInt", "");
}

static void
uisongeditAddLabel (uisongedit_t *uisongedit, uiwcont_t *hbox, int tagkey)
{
  uiwcont_t      *uiwidgetp;
  se_internal_t *seint;

  logProcBegin (LOG_PROC, "uisongeditAddLabel");
  seint = uisongedit->seInternalData;
  uiwidgetp = &seint->items [seint->itemcount].uiwidget;
  uiCreateLabel (uiwidgetp, "");
  uiSizeGroupAdd (seint->sgentry, uiwidgetp);
  uiBoxPackStartExpand (hbox, uiwidgetp);
  logProcEnd (LOG_PROC, "uisongeditAddLabel", "");
}

static void
uisongeditAddSecondaryLabel (uisongedit_t *uisongedit, uiwcont_t *hbox, int tagkey)
{
  uiwcont_t      *uiwidgetp;
  se_internal_t *seint;

  logProcBegin (LOG_PROC, "uisongeditAddSecondaryLabel");
  seint = uisongedit->seInternalData;
  uiwidgetp = &seint->items [seint->itemcount].uiwidget;
  uiCreateLabel (uiwidgetp, "");
  uiBoxPackStart (hbox, uiwidgetp);
  logProcEnd (LOG_PROC, "uisongeditAddSecondaryLabel", "");
}

static void
uisongeditAddSpinboxTime (uisongedit_t *uisongedit, uiwcont_t *hbox, int tagkey)
{
  uispinbox_t     *sbp;
  se_internal_t *seint;
  uiwcont_t      *uiwidgetp;

  logProcBegin (LOG_PROC, "uisongeditAddSpinboxTime");
  seint = uisongedit->seInternalData;
  sbp = uiSpinboxTimeInit (SB_TIME_PRECISE);
  seint->items [seint->itemcount].spinbox = sbp;
  uiSpinboxTimeCreate (sbp, uisongedit, NULL);
  uiSpinboxSetRange (sbp, 0.0, 1200000.0);
  uiSpinboxTimeSetValue (sbp, 0);
  uiSpinboxTimeSetValueChangedCallback (sbp,
      seint->callbacks [UISONGEDIT_CB_CHANGED]);

  uiwidgetp = uiSpinboxGetWidgetContainer (sbp);
  uiSizeGroupAdd (seint->sgsbtime, uiwidgetp);
  uiBoxPackStart (hbox, uiwidgetp);
  logProcEnd (LOG_PROC, "uisongeditAddSpinboxTime", "");
}

static void
uisongeditAddScale (uisongedit_t *uisongedit, uiwcont_t *hbox, int tagkey)
{
  se_internal_t *seint;
  uiwcont_t      *uiwidgetp;
  double          lower, upper;
  double          inca, incb;
  int             digits;

  logProcBegin (LOG_PROC, "uisongeditAddScale");
  seint = uisongedit->seInternalData;
  uiwidgetp = &seint->items [seint->itemcount].uiwidget;
  if (tagkey == TAG_SPEEDADJUSTMENT) {
    lower = 70.0;
    upper = 130.0;
    digits = 0;
    inca = 1.0;
    incb = 5.0;
  }
  if (tagkey == TAG_VOLUMEADJUSTPERC) {
    lower = -50.0;
    upper = 50.0;
    digits = 1;
    inca = 0.1;
    incb = 5.0;
  }
  uiCreateScale (uiwidgetp, lower, upper, inca, incb, 0.0, digits);
  seint->items [seint->itemcount].callback = callbackInitDouble (
      uisongeditScaleDisplayCallback, &seint->items [seint->itemcount]);
  uiScaleSetCallback (uiwidgetp, seint->items [seint->itemcount].callback);

  uiSizeGroupAdd (seint->sgscale, uiwidgetp);
  uiBoxPackStart (hbox, uiwidgetp);

  uiwidgetp = &seint->items [seint->itemcount].display;
  uiCreateLabel (uiwidgetp, "100%");
  uiLabelAlignEnd (uiwidgetp);
  uiSizeGroupAdd (seint->sgscaledisp, uiwidgetp);
  uiBoxPackStart (hbox, uiwidgetp);
  logProcEnd (LOG_PROC, "uisongeditAddScale", "");
}

/* also sets the changed flag */
static bool
uisongeditScaleDisplayCallback (void *udata, double value)
{
  uisongedititem_t  *item = udata;
  se_internal_t   *seint;
  char              tbuff [40];
  int               digits;

  logProcBegin (LOG_PROC, "uisongeditScaleDisplayCallback");
  seint = item->seint;
  seint->checkchanged = true;
  digits = uiScaleGetDigits (&item->uiwidget);
  value = uiScaleEnforceMax (&item->uiwidget, value);
  snprintf (tbuff, sizeof (tbuff), "%4.*f%%", digits, value);
  uiLabelSetText (&item->display, tbuff);
  logProcEnd (LOG_PROC, "uisongeditScaleDisplayCallback", "");
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
  long            nval;
  char            tbuff [200];
  bool            valid = false;

  logProcBegin (LOG_PROC, "uisongeditSaveCallback");
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: song edit: save");
  seint = uisongedit->seInternalData;

  valid = true;
  uiLabelSetText (uisongedit->statusMsg, "");

  /* do some validations */
  /* this is not necessary with an edit-all-apply */
  /* as the validation checks currently are only for non-edit-all fields */
  if (chglist == NULL) {
    long      songstart;
    long      songend;
    long      dur;
    int       speed;
    double    ndval;

    ndval = uiScaleGetValue (&seint->items [seint->speedidx].uiwidget);
    speed = round (ndval);
    songstart = uiSpinboxTimeGetValue (seint->items [seint->songstartidx].spinbox);
    songend = uiSpinboxTimeGetValue (seint->items [seint->songendidx].spinbox);
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

    if (chkvalue == SONGEDIT_CHK_STR) {
      songSetStr (seint->song, tagkey, nlistGetStr (chglist, tagkey));
    }

    if (chkvalue == SONGEDIT_CHK_NUM) {
      if (tagkey == TAG_BPM) {
        if (nval == 0) {
          nval = LIST_VALUE_INVALID;
        }
      }
      songSetNum (seint->song, tagkey, nlistGetNum (chglist, tagkey));
    }

    if (chkvalue == SONGEDIT_CHK_DOUBLE) {
      songSetDouble (seint->song, tagkey, nlistGetDouble (chglist, tagkey));
    }

    if (chkvalue == SONGEDIT_CHK_LIST) {
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
      if (seint->songstartidx != -1 && seint->items [seint->songstartidx].changed) {
        val = songGetNum (seint->song, TAG_SONGSTART);
        if (val > 0) {
          val = songutilNormalizePosition (val, speed);
          songSetNum (seint->song, TAG_SONGSTART, val);
        }
      }
      if (seint->songendidx != -1 && seint->items [seint->songendidx].changed) {
        val = songGetNum (seint->song, TAG_SONGEND);
        if (val > 0) {
          val = songutilNormalizePosition (val, speed);
          songSetNum (seint->song, TAG_SONGEND, val);
        }
      }
      if (seint->bpmidx != -1 && seint->items [seint->bpmidx].changed) {
        val = songGetNum (seint->song, TAG_BPM);
        if (val > 0) {
          val = songutilNormalizeBPM (val, speed);
          songSetNum (seint->song, TAG_BPM, val);
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
    callbackHandlerLong (uisongedit->savecb, seint->dbidx);
  }

  logProcEnd (LOG_PROC, "uisongeditSaveCallback", "");
  return UICB_CONT;
}

static int
uisongeditGetCheckValue (uisongedit_t *uisongedit, int tagkey)
{
  int   chkvalue = SONGEDIT_CHK_NONE;

  switch (tagdefs [tagkey].editType) {
    case ET_ENTRY: {
      chkvalue = SONGEDIT_CHK_STR;
      if (tagkey == TAG_TAGS) {
        chkvalue = SONGEDIT_CHK_LIST;
      }
      break;
    }
    case ET_COMBOBOX: {
      if (tagkey == TAG_DANCE) {
        chkvalue = SONGEDIT_CHK_NUM;
      }
      if (tagkey == TAG_GENRE) {
        chkvalue = SONGEDIT_CHK_NUM;
      }
      break;
    }
    case ET_SPINBOX_TEXT: {
      if (tagkey == TAG_FAVORITE) {
        chkvalue = SONGEDIT_CHK_NUM;
      }
      if (tagkey == TAG_DANCELEVEL) {
        chkvalue = SONGEDIT_CHK_NUM;
      }
      if (tagkey == TAG_DANCERATING) {
        chkvalue = SONGEDIT_CHK_NUM;
      }
      if (tagkey == TAG_STATUS) {
        chkvalue = SONGEDIT_CHK_NUM;
      }
      break;
    }
    case ET_SPINBOX: {
      chkvalue = SONGEDIT_CHK_NUM;
      break;
    }
    case ET_SPINBOX_TIME: {
      chkvalue = SONGEDIT_CHK_NUM;
      break;
    }
    case ET_SCALE: {
      chkvalue = SONGEDIT_CHK_DOUBLE;
      if (tagkey == TAG_SPEEDADJUSTMENT) {
        chkvalue = SONGEDIT_CHK_NUM;
      }
      break;
    }
    default: {
      chkvalue = SONGEDIT_CHK_NONE;
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
        ndata = uiEntryGetValue (seint->items [count].entry);
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
        nval = uiSpinboxGetValue (&seint->items [count].uiwidget);
        break;
      }
      case ET_SPINBOX_TIME: {
        nval = uiSpinboxTimeGetValue (seint->items [count].spinbox);
        break;
      }
      case ET_SCALE: {
        ndval = uiScaleGetValue (&seint->items [count].uiwidget);
        if (tagkey == TAG_SPEEDADJUSTMENT) {
          nval = round (ndval);
        }
        break;
      }
      default: {
        break;
      }
    }

    if (chkvalue == SONGEDIT_CHK_STR ||
        chkvalue == SONGEDIT_CHK_LIST) {
      nlistSetStr (chglist, tagkey, ndata);
    }

    if (chkvalue == SONGEDIT_CHK_NUM) {
      if (tagkey == TAG_BPM) {
        if (nval == 0) {
          nval = LIST_VALUE_INVALID;
        }
      }
      nlistSetNum (chglist, tagkey, nval);
    }

    if (chkvalue == SONGEDIT_CHK_DOUBLE) {
      nlistSetDouble (chglist, tagkey, ndval);
    }
  }

  return chglist;
}

static bool
uisongeditFirstSelection (void *udata)
{
  uisongedit_t  *uisongedit = udata;

  logProcBegin (LOG_PROC, "uisongeditFirstSelection");
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: song edit: first");
  uisongselFirstSelection (uisongedit->uisongsel);
  uisongeditClearChanged (uisongedit, UISONGEDIT_ALL);
  logProcEnd (LOG_PROC, "uisongeditFirstSelection", "");
  return UICB_CONT;
}

static bool
uisongeditPreviousSelection (void *udata)
{
  uisongedit_t  *uisongedit = udata;

  logProcBegin (LOG_PROC, "uisongeditPreviousSelection");
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: song edit: previous");
  uisongselPreviousSelection (uisongedit->uisongsel);
  uisongeditClearChanged (uisongedit, UISONGEDIT_ALL);
  logProcEnd (LOG_PROC, "uisongeditPreviousSelection", "");
  return UICB_CONT;
}

static bool
uisongeditNextSelection (void *udata)
{
  uisongedit_t  *uisongedit = udata;

  logProcBegin (LOG_PROC, "uisongeditNextSelection");
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: song edit: next");
  uisongselNextSelection (uisongedit->uisongsel);
  uisongeditClearChanged (uisongedit, UISONGEDIT_ALL);
  logProcEnd (LOG_PROC, "uisongeditNextSelection", "");
  return UICB_CONT;
}

static bool
uisongeditCopyPath (void *udata)
{
  uisongedit_t  *uisongedit = udata;
  se_internal_t *seint;
  const char    *txt;
  char          *ffn;
  char          tbuff [MAXPATHLEN];

  seint = uisongedit->seInternalData;

  txt = uiLabelGetText (&seint->filedisp);
  ffn = songutilFullFileName (txt);
  strlcpy (tbuff, ffn, sizeof (tbuff));
  mdfree (ffn);
  if (isWindows ()) {
    pathWinPath (tbuff, sizeof (tbuff));
  }
  uiClipboardSet (tbuff);

  return UICB_CONT;
}

static bool
uisongeditKeyEvent (void *udata)
{
  uisongedit_t    *uisongedit = udata;
  se_internal_t *seint;

  seint = uisongedit->seInternalData;

  if (uiKeyIsPressEvent (seint->uikey) &&
      uiKeyIsAudioPlayKey (seint->uikey)) {
    uisongselPlayCallback (uisongedit->uisongsel);
  }

  if (uiKeyIsPressEvent (seint->uikey)) {
    if (uiKeyIsControlPressed (seint->uikey)) {
      if (uiKeyIsShiftPressed (seint->uikey)) {
        if (uiKeyIsKey (seint->uikey, 'A')) {
          uisongeditApplyAdjCallback (uisongedit);
          return UICB_STOP;
        }
      }
      if (uiKeyIsKey (seint->uikey, 'S')) {
        uisongeditSaveCallback (uisongedit);
        return UICB_STOP;
      }
      if (uiKeyIsKey (seint->uikey, 'N')) {
        uisongeditNextSelection (uisongedit);
        return UICB_STOP;
      }
      if (uiKeyIsKey (seint->uikey, 'P')) {
        uisongeditPreviousSelection (uisongedit);
        return UICB_STOP;
      }
    }
  }

  return UICB_CONT;
}

static bool
uisongeditApplyAdjCallback (void *udata)
{
  uisongedit_t    *uisongedit = udata;

  logProcBegin (LOG_PROC, "uisongeditApplyAdjCallback");
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: song edit: apply-adj (kb)");

  if (uisongedit->applyadjcb != NULL) {
    callbackHandlerLong (uisongedit->applyadjcb, SONG_ADJUST_NONE);
  }

  logProcEnd (LOG_PROC, "uisongeditApplyAdjCallback", "");
  return UICB_CONT;
}

static int
uisongeditEntryChangedCallback (uientry_t *entry, void *udata)
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
  int     lowbpm, highbpm;
  dance_t *dances;
  char    tbuff [100];
  char    *str;

  dances = bdjvarsdfGet (BDJVDF_DANCES);
  lowbpm = danceGetNum (dances, danceidx, DANCE_LOW_BPM);
  highbpm = danceGetNum (dances, danceidx, DANCE_HIGH_BPM);
  *tbuff = '\0';
  if (lowbpm != 0 && highbpm != 0) {
    if (lowbpm == highbpm) {
      snprintf (tbuff, sizeof (tbuff), " (%d)", lowbpm);
    } else {
      snprintf (tbuff, sizeof (tbuff), " (%d - %d)", lowbpm, highbpm);
    }
  }
  str = strdup (tbuff);
  return str;
}
