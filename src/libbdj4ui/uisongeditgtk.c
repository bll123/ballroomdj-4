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
#include "callback.h"
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

typedef struct uisongeditgtk se_internal_t;

typedef struct {
  int         tagkey;
  union {
    uientry_t     *entry;
    uispinbox_t   *spinbox;
    UIWidget      uiwidget;
    uidance_t     *uidance;
    uifavorite_t  *uifavorite;
    uigenre_t     *uigenre;
    uilevel_t     *uilevel;
    uirating_t    *uirating;
    uistatus_t    *uistatus;
  };
  uichgind_t      *chgind;
  UIWidget        display;
  callback_t      *callback;
  se_internal_t   *uiw;           // need for scale changed.
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


typedef struct uisongeditgtk {
  UIWidget            *parentwin;
  UIWidget            vbox;
  UIWidget            musicbrainzPixbuf;
  UIWidget            modified;
  UIWidget            audioidImg;
  UIWidget            filedisp;
  UIWidget            sgentry;
  UIWidget            sgsbint;
  UIWidget            sgsbtext;
  UIWidget            sgsbtime;
  UIWidget            sgscale;
  UIWidget            sgscaledisp;
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
} se_internal_t;

static void uisongeditAddDisplay (uisongedit_t *songedit, UIWidget *col, UIWidget *sg, int dispsel);
static void uisongeditAddItem (uisongedit_t *uisongedit, UIWidget *hbox, UIWidget *sg, int tagkey);
static void uisongeditAddEntry (uisongedit_t *uisongedit, UIWidget *hbox, int tagkey);
static void uisongeditAddSpinboxInt (uisongedit_t *uisongedit, UIWidget *hbox, int tagkey);
static void uisongeditAddLabel (uisongedit_t *uisongedit, UIWidget *hbox, int tagkey);
static void uisongeditAddSecondaryLabel (uisongedit_t *uisongedit, UIWidget *hbox, int tagkey);
static void uisongeditAddSpinboxTime (uisongedit_t *uisongedit, UIWidget *hbox, int tagkey);
static void uisongeditAddScale (uisongedit_t *uisongedit, UIWidget *hbox, int tagkey);
static bool uisongeditScaleDisplayCallback (void *udata, double value);
static void uisongeditClearChanged (uisongedit_t *uisongedit);
static bool uisongeditSaveCallback (void *udata);
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
  se_internal_t  *uiw;

  logProcBegin (LOG_PROC, "uisongeditUIInit");

  uiw = mdmalloc (sizeof (se_internal_t));
  uiw->parentwin = NULL;
  uiw->itemcount = 0;
  uiw->items = NULL;
  uiw->changed = 0;
  uiw->uikey = NULL;
  uiw->levels = bdjvarsdfGet (BDJVDF_LEVELS);
  uiw->bpmidx = -1;
  uiw->songstartidx = -1;
  uiw->songendidx = -1;
  uiw->speedidx = -1;
  uiw->dbidx = -1;
  uiw->lastspeed = -1;
  for (int i = 0; i < UISONGEDIT_BUTTON_MAX; ++i) {
    uiw->buttons [i] = NULL;
  }
  for (int i = 0; i < UISONGEDIT_CB_MAX; ++i) {
    uiw->callbacks [i] = NULL;
  }
  uiw->checkchanged = false;

  uiutilsUIWidgetInit (&uiw->vbox);
  uiutilsUIWidgetInit (&uiw->audioidImg);
  uiutilsUIWidgetInit (&uiw->modified);

  uiCreateSizeGroupHoriz (&uiw->sgentry);
  uiCreateSizeGroupHoriz (&uiw->sgsbint);
  uiCreateSizeGroupHoriz (&uiw->sgsbtext);
  uiCreateSizeGroupHoriz (&uiw->sgsbtime);
  uiCreateSizeGroupHoriz (&uiw->sgscale);
  uiCreateSizeGroupHoriz (&uiw->sgscaledisp);

  uisongedit->uiWidgetData = uiw;
  logProcEnd (LOG_PROC, "uisongeditUIInit", "");
}

void
uisongeditUIFree (uisongedit_t *uisongedit)
{
  se_internal_t *uiw;

  logProcBegin (LOG_PROC, "uisongeditUIFree");
  if (uisongedit == NULL) {
    logProcEnd (LOG_PROC, "uisongeditUIFree", "null");
    return;
  }

  uiw = uisongedit->uiWidgetData;
  if (uiw != NULL) {
    for (int count = 0; count < uiw->itemcount; ++count) {
      int   tagkey;

      uichgindFree (uiw->items [count].chgind);

      tagkey = uiw->items [count].tagkey;

      switch (tagdefs [tagkey].editType) {
        case ET_ENTRY: {
          uiEntryFree (uiw->items [count].entry);
          break;
        }
        case ET_COMBOBOX: {
          if (tagkey == TAG_DANCE) {
            uidanceFree (uiw->items [count].uidance);
          }
          if (tagkey == TAG_GENRE) {
            uigenreFree (uiw->items [count].uigenre);
          }
          break;
        }
        case ET_SPINBOX_TEXT: {
          if (tagkey == TAG_FAVORITE) {
            uifavoriteFree (uiw->items [count].uifavorite);
          }
          if (tagkey == TAG_DANCELEVEL) {
            uilevelFree (uiw->items [count].uilevel);
          }
          if (tagkey == TAG_DANCERATING) {
            uiratingFree (uiw->items [count].uirating);
          }
          if (tagkey == TAG_STATUS) {
            uistatusFree (uiw->items [count].uistatus);
          }
          break;
        }
        case ET_SPINBOX: {
          break;
        }
        case ET_SPINBOX_TIME: {
          if (uiw->items [count].spinbox != NULL) {
            uiSpinboxFree (uiw->items [count].spinbox);
          }
          break;
        }
        case ET_NA:
        case ET_SCALE:
        case ET_LABEL: {
          break;
        }
      }
      callbackFree (uiw->items [count].callback);
    }

    for (int i = 0; i < UISONGEDIT_BUTTON_MAX; ++i) {
      uiButtonFree (uiw->buttons [i]);
    }
    dataFree (uiw->items);
    uiKeyFree (uiw->uikey);
    for (int i = 0; i < UISONGEDIT_CB_MAX; ++i) {
      callbackFree (uiw->callbacks [i]);
    }
    mdfree (uiw);
    uisongedit->uiWidgetData = NULL;
  }

  logProcEnd (LOG_PROC, "uisongeditUIFree", "");
}

UIWidget *
uisongeditBuildUI (uisongsel_t *uisongsel, uisongedit_t *uisongedit,
    UIWidget *parentwin, UIWidget *statusMsg)
{
  se_internal_t   *uiw;
  UIWidget          hbox;
  UIWidget          col;
  UIWidget          sg;
  UIWidget          uiwidget;
  uibutton_t        *uibutton;
  UIWidget          *uiwidgetp;
  int               count;
  char              tbuff [MAXPATHLEN];

  logProcBegin (LOG_PROC, "uisongeditBuildUI");
  logProcBegin (LOG_PROC, "uisongeditBuildUI");

  uisongedit->statusMsg = statusMsg;
  uisongedit->uisongsel = uisongsel;
  uiw = uisongedit->uiWidgetData;
  uiw->parentwin = parentwin;

  uiCreateVertBox (&uiw->vbox);
  uiWidgetExpandHoriz (&uiw->vbox);

  uiw->uikey = uiKeyAlloc ();
  uiw->callbacks [UISONGEDIT_CB_KEYB] = callbackInit (
      uisongeditKeyEvent, uisongedit, NULL);
  uiKeySetKeyCallback (uiw->uikey, &uiw->vbox,
      uiw->callbacks [UISONGEDIT_CB_KEYB]);

  uiCreateHorizBox (&hbox);
  uiWidgetExpandHoriz (&hbox);
  uiWidgetAlignHorizFill (&hbox);
  uiBoxPackStart (&uiw->vbox, &hbox);

  uiw->callbacks [UISONGEDIT_CB_FIRST] = callbackInit (
      uisongeditFirstSelection, uisongedit, "songedit: first");
  uibutton = uiCreateButton (uiw->callbacks [UISONGEDIT_CB_FIRST],
      /* CONTEXT: song editor : first song */
      _("First"), NULL);
  uiw->buttons [UISONGEDIT_BUTTON_FIRST] = uibutton;
  uiwidgetp = uiButtonGetUIWidget (uibutton);
  uiBoxPackStart (&hbox, uiwidgetp);

  uiw->callbacks [UISONGEDIT_CB_PREV] = callbackInit (
      uisongeditPreviousSelection, uisongedit, "songedit: previous");
  uibutton = uiCreateButton (uiw->callbacks [UISONGEDIT_CB_PREV],
      /* CONTEXT: song editor : previous song */
      _("Previous"), NULL);
  uiw->buttons [UISONGEDIT_BUTTON_PREV] = uibutton;
  uiButtonSetRepeat (uibutton, UISONGEDIT_REPEAT_TIME);
  uiwidgetp = uiButtonGetUIWidget (uibutton);
  uiBoxPackStart (&hbox, uiwidgetp);

  uiw->callbacks [UISONGEDIT_CB_NEXT] = callbackInit (
      uisongeditNextSelection, uisongedit, "songedit: next");
  uibutton = uiCreateButton (uiw->callbacks [UISONGEDIT_CB_NEXT],
      /* CONTEXT: song editor : next song */
      _("Next"), NULL);
  uiw->buttons [UISONGEDIT_BUTTON_NEXT] = uibutton;
  uiButtonSetRepeat (uibutton, UISONGEDIT_REPEAT_TIME);
  uiwidgetp = uiButtonGetUIWidget (uibutton);
  uiBoxPackStart (&hbox, uiwidgetp);

  uiw->callbacks [UISONGEDIT_CB_PLAY] = callbackInit (
      uisongselPlayCallback, uisongsel, "songedit: play");
  uibutton = uiCreateButton (uiw->callbacks [UISONGEDIT_CB_PLAY],
      /* CONTEXT: song editor : play song */
      _("Play"), NULL);
  uiw->buttons [UISONGEDIT_BUTTON_PLAY] = uibutton;
  uiwidgetp = uiButtonGetUIWidget (uibutton);
  uiBoxPackStart (&hbox, uiwidgetp);

  uiw->callbacks [UISONGEDIT_CB_SAVE] = callbackInit (
      uisongeditSaveCallback, uisongedit, "songedit: save");
  uibutton = uiCreateButton (uiw->callbacks [UISONGEDIT_CB_SAVE],
      /* CONTEXT: song editor : save data */
      _("Save"), NULL);
  uiw->buttons [UISONGEDIT_BUTTON_SAVE] = uibutton;
  uiwidgetp = uiButtonGetUIWidget (uibutton);
  uiBoxPackEnd (&hbox, uiwidgetp);

  /* audio-identification logo, modified indicator, */
  /* copy button, file label, filename */
  uiCreateHorizBox (&hbox);
  uiWidgetExpandHoriz (&hbox);
  uiWidgetAlignHorizFill (&hbox);
  uiBoxPackStart (&uiw->vbox, &hbox);

  pathbldMakePath (tbuff, sizeof (tbuff), "musicbrainz-logo", BDJ4_IMG_SVG_EXT,
      PATHBLD_MP_DIR_IMG);
  uiImageFromFile (&uiw->musicbrainzPixbuf, tbuff);
  uiImageGetPixbuf (&uiw->musicbrainzPixbuf);
  uiWidgetMakePersistent (&uiw->musicbrainzPixbuf);

  uiImageNew (&uiw->audioidImg);
  uiImageClear (&uiw->audioidImg);
  uiWidgetSetSizeRequest (&uiw->audioidImg, 24, -1);
  uiWidgetSetMarginStart (&uiw->audioidImg, 1);
  uiBoxPackStart (&hbox, &uiw->audioidImg);

  uiCreateLabel (&uiwidget, " ");
  uiBoxPackStart (&hbox, &uiwidget);
  uiutilsUIWidgetCopy (&uiw->modified, &uiwidget);
  uiWidgetSetClass (&uiw->modified, DARKACCENT_CLASS);

  uiw->callbacks [UISONGEDIT_CB_COPY_TEXT] = callbackInit (
      uisongeditCopyPath, uisongedit, "songedit: copy-text");
  uibutton = uiCreateButton (
      uiw->callbacks [UISONGEDIT_CB_COPY_TEXT],
      "", NULL);
  uiw->buttons [UISONGEDIT_BUTTON_COPY_TEXT] = uibutton;
  uiwidgetp = uiButtonGetUIWidget (uibutton);
  uiButtonSetImageIcon (uibutton, "edit-copy");
  uiWidgetSetMarginStart (uiwidgetp, 1);
  uiBoxPackStart (&hbox, uiwidgetp);

  /* CONTEXT: song editor: label for displaying the audio file path */
  uiCreateColonLabel (&uiwidget, _("File"));
  uiBoxPackStart (&hbox, &uiwidget);
  uiWidgetDisable (&uiwidget);

  uiCreateLabel (&uiwidget, "");
  uiLabelEllipsizeOn (&uiwidget);
  uiBoxPackStart (&hbox, &uiwidget);
  uiutilsUIWidgetCopy (&uiw->filedisp, &uiwidget);
  uiWidgetSetClass (&uiw->filedisp, DARKACCENT_CLASS);
  uiLabelSetSelectable (&uiw->filedisp);

  uiCreateHorizBox (&hbox);
  uiWidgetExpandHoriz (&hbox);
  uiWidgetAlignHorizFill (&hbox);
  uiBoxPackStartExpand (&uiw->vbox, &hbox);

  count = 0;
  for (int i = DISP_SEL_SONGEDIT_A; i <= DISP_SEL_SONGEDIT_C; ++i) {
    slist_t           *sellist;

    sellist = dispselGetList (uisongedit->dispsel, i);
    count += slistGetCount (sellist);
  }

  /* the items must all be alloc'd beforehand so that the callback */
  /* pointer is static */
  /* need to add 1 for the BPM display secondary display */
  uiw->items = mdmalloc (sizeof (uisongedititem_t) * (count + 1));
  for (int i = 0; i < count + 1; ++i) {
    uiw->items [i].tagkey = 0;
    uiutilsUIWidgetInit (&uiw->items [i].uiwidget);
    uiw->items [i].entry = NULL;
    uiw->items [i].chgind = NULL;
    uiw->items [i].lastchanged = false;
    uiw->items [i].changed = false;
    uiw->items [i].callback = NULL;
    uiw->items [i].uiw = uiw;
  }

  /* must be set before the items are instantiated */
  uiw->callbacks [UISONGEDIT_CB_CHANGED] = callbackInit (
      uisongeditChangedCallback, uiw, NULL);

  for (int i = DISP_SEL_SONGEDIT_A; i <= DISP_SEL_SONGEDIT_C; ++i) {
    uiCreateVertBox (&col);
    uiWidgetSetAllMargins (&col, 4);
    uiWidgetExpandHoriz (&col);
    uiWidgetExpandVert (&col);
    uiBoxPackStartExpand (&hbox, &col);

    uiCreateSizeGroupHoriz (&sg);
    uisongeditAddDisplay (uisongedit, &col, &sg, i);
  }

  logProcEnd (LOG_PROC, "uisongeditBuildUI", "");
  return &uiw->vbox;
}

void
uisongeditLoadData (uisongedit_t *uisongedit, song_t *song, dbidx_t dbidx)
{
  se_internal_t *uiw;
  char            *data;
  long            val;
  double          dval;

  logProcBegin (LOG_PROC, "uisongeditLoadData");
  uiw = uisongedit->uiWidgetData;
  uiw->song = song;
  uiw->dbidx = dbidx;
  uiw->changed = 0;

  data = uisongGetDisplay (song, TAG_FILE, &val, &dval);
  uiLabelSetText (&uiw->filedisp, data);
  dataFree (data);
  data = NULL;

  uiImageClear (&uiw->audioidImg);
  data = songGetStr (song, TAG_RECORDING_ID);
  if (data != NULL && *data) {
    uiImageSetFromPixbuf (&uiw->audioidImg, &uiw->musicbrainzPixbuf);
  }

  val = songGetNum (song, TAG_ADJUSTFLAGS);
  uiLabelSetText (&uiw->modified, " ");
  if (val != SONG_ADJUST_NONE) {
    uiLabelSetText (&uiw->modified, "*");
  }

  for (int count = 0; count < uiw->itemcount; ++count) {
    int tagkey = uiw->items [count].tagkey;

    if (tagkey == TAG_BPM_DISPLAY) {
      data = uisongeditGetBPMRangeDisplay (songGetNum (song, TAG_DANCE));
      uiLabelSetText (&uiw->items [count].uiwidget, data);
      dataFree (data);
      continue;
    }
    data = uisongGetDisplay (song, tagkey, &val, &dval);
    uiw->items [count].changed = false;
    uiw->items [count].lastchanged = false;

    switch (tagdefs [tagkey].editType) {
      case ET_ENTRY: {
        uiEntrySetValue (uiw->items [count].entry, "");
        if (data != NULL) {
          uiEntrySetValue (uiw->items [count].entry, data);
        }
        break;
      }
      case ET_COMBOBOX: {
        if (tagkey == TAG_DANCE) {
          if (val < 0) { val = -1; } // empty value
          uidanceSetValue (uiw->items [count].uidance, val);
        }
        if (tagkey == TAG_GENRE) {
          if (val < 0) { val = 0; }
          uigenreSetValue (uiw->items [count].uigenre, val);
        }
        break;
      }
      case ET_SPINBOX_TEXT: {
        if (tagkey == TAG_FAVORITE) {
          if (val < 0) { val = 0; }
          uifavoriteSetValue (uiw->items [count].uifavorite, val);
        }
        if (tagkey == TAG_DANCELEVEL) {
          if (val < 0) { val = levelGetDefaultKey (uiw->levels); }
          uilevelSetValue (uiw->items [count].uilevel, val);
        }
        if (tagkey == TAG_DANCERATING) {
          if (val < 0) { val = 0; }
          uiratingSetValue (uiw->items [count].uirating, val);
        }
        if (tagkey == TAG_STATUS) {
          if (val < 0) { val = 0; }
          uistatusSetValue (uiw->items [count].uistatus, val);
        }
        break;
      }
      case ET_SPINBOX: {
        if (data != NULL) {
          fprintf (stderr, "et_spinbox: mismatch type\n");
        }
        if (val < 0) { val = 0; }
        uiSpinboxSetValue (&uiw->items [count].uiwidget, val);
        break;
      }
      case ET_SPINBOX_TIME: {
        if (val < 0) { val = 0; }
        uiSpinboxTimeSetValue (uiw->items [count].spinbox, val);
        break;
      }
      case ET_SCALE: {
        /* volume adjust percentage may be a negative value */
        if (dval == LIST_DOUBLE_INVALID) { dval = 0.0; }
        if (tagkey == TAG_SPEEDADJUSTMENT) {
          /* speed adjustment uses a scale, but has a numeric stored */
          dval = (double) val;
          if (dval < 0.0) { dval = 100.0; }
          if (dval == 0.0) { dval = 100.0; }
          uiw->lastspeed = val;
        }
        if (data != NULL) {
          fprintf (stderr, "et_scale: mismatch type\n");
        }
        uiScaleSetValue (&uiw->items [count].uiwidget, dval);
        uisongeditScaleDisplayCallback (&uiw->items [count], dval);
        break;
      }
      case ET_LABEL: {
        if (data != NULL) {
          uiLabelSetText (&uiw->items [count].uiwidget, data);
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
  se_internal_t *uiw;
  double          dval, ndval;
  long            val, nval;
  char            *songdata;
  const char      *ndata;
  int             chkvalue;
  int             bpmdispidx = -1;
  bool            spdchanged = false;
  int             speed = 100;
  int             danceidx = -1;

  uiw = uisongedit->uiWidgetData;

  uiButtonCheckRepeat (uiw->buttons [UISONGEDIT_BUTTON_NEXT]);
  uiButtonCheckRepeat (uiw->buttons [UISONGEDIT_BUTTON_PREV]);

  if (uiw->checkchanged) {
    /* check for any changed items.*/
    /* this works well for the user, as it is able to determine if a */
    /* value has reverted back to the original value */
    for (int count = 0; count < uiw->itemcount; ++count) {
      int tagkey = uiw->items [count].tagkey;

      if (tagkey == TAG_BPM_DISPLAY) {
        bpmdispidx = count;
      }

      songdata = uisongGetDisplay (uiw->song, tagkey, &val, &dval);
      chkvalue = SONGEDIT_CHK_NONE;

      switch (tagdefs [tagkey].editType) {
        case ET_ENTRY: {
          ndata = uiEntryGetValue (uiw->items [count].entry);
          chkvalue = SONGEDIT_CHK_STR;
          break;
        }
        case ET_COMBOBOX: {
          if (tagkey == TAG_DANCE) {
            if (val < 0) { val = -1; }
            nval = uidanceGetValue (uiw->items [count].uidance);
            danceidx = nval;
            chkvalue = SONGEDIT_CHK_NUM;
          }
          if (tagkey == TAG_GENRE) {
            nval = uigenreGetValue (uiw->items [count].uigenre);
            chkvalue = SONGEDIT_CHK_NUM;
          }
          break;
        }
        case ET_SPINBOX_TEXT: {
          if (tagkey == TAG_FAVORITE) {
            nval = uifavoriteGetValue (uiw->items [count].uifavorite);
            chkvalue = SONGEDIT_CHK_NUM;
          }
          if (tagkey == TAG_DANCELEVEL) {
            nval = uilevelGetValue (uiw->items [count].uilevel);
            chkvalue = SONGEDIT_CHK_NUM;
          }
          if (tagkey == TAG_DANCERATING) {
            nval = uiratingGetValue (uiw->items [count].uirating);
            chkvalue = SONGEDIT_CHK_NUM;
          }
          if (tagkey == TAG_STATUS) {
            nval = uistatusGetValue (uiw->items [count].uistatus);
            chkvalue = SONGEDIT_CHK_NUM;
          }
          break;
        }
        case ET_SPINBOX: {
          if (val < 0) { val = 0; }
          nval = uiSpinboxGetValue (&uiw->items [count].uiwidget);
          chkvalue = SONGEDIT_CHK_NUM;
          break;
        }
        case ET_SPINBOX_TIME: {
          if (val < 0) { val = 0; }
          nval = uiSpinboxTimeGetValue (uiw->items [count].spinbox);
          chkvalue = SONGEDIT_CHK_NUM;
          break;
        }
        case ET_SCALE: {
          if (tagkey == TAG_SPEEDADJUSTMENT) {
            dval = (double) val;
            if (dval < 0.0) { dval = 0.0; }
          }
          ndval = uiScaleGetValue (&uiw->items [count].uiwidget);
          if (ndval == LIST_DOUBLE_INVALID) { ndval = 0.0; }
          if (dval == LIST_DOUBLE_INVALID) { dval = 0.0; }
          chkvalue = SONGEDIT_CHK_DOUBLE;
          break;
        }
        default: {
          chkvalue = SONGEDIT_CHK_NONE;
          break;
        }
      }

      if (chkvalue == SONGEDIT_CHK_STR) {
        if ((ndata != NULL && songdata == NULL) ||
            strcmp (songdata, ndata) != 0) {
          uiw->items [count].changed = true;
        } else {
          if (uiw->items [count].changed) {
            uiw->items [count].changed = false;
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
          uiw->items [count].changed = true;
        } else {
          if (uiw->items [count].changed) {
            uiw->items [count].changed = false;
          }
        }
      }

      if (chkvalue == SONGEDIT_CHK_DOUBLE) {
        int   rc;
        int   waschanged = false;

        /* for the speed adjustment, 0.0 (no setting) is changed to 100.0 */
        rc = (tagkey == TAG_SPEEDADJUSTMENT && ndval == 100.0 && dval == 0.0);
        if (! rc && fabs (ndval - dval) > 0.009 ) {
          uiw->items [count].changed = true;
          waschanged = true;
        } else {
          if (uiw->items [count].changed) {
            waschanged = true;
            uiw->items [count].changed = false;
          }
        }

        if (tagkey == TAG_SPEEDADJUSTMENT && waschanged) {
          speed = (int) ndval;
          spdchanged = true;
        }
      }

      if (uiw->items [count].changed != uiw->items [count].lastchanged) {
        if (uiw->items [count].changed) {
          uichgindMarkChanged (uiw->items [count].chgind);
          uiw->changed += 1;
        } else {
          uichgindMarkNormal (uiw->items [count].chgind);
          uiw->changed -= 1;
        }
        uiw->items [count].lastchanged = uiw->items [count].changed;
      }

      dataFree (songdata);
    }

    if (spdchanged) {
      if (uiw->songstartidx != -1) {
        nval = uiSpinboxTimeGetValue (uiw->items [uiw->songstartidx].spinbox);
        if (nval > 0) {
          nval = songutilNormalizePosition (nval, uiw->lastspeed);
          nval = songutilAdjustPosReal (nval, speed);
          uiSpinboxTimeSetValue (uiw->items [uiw->songstartidx].spinbox, nval);
        }
      }
      if (uiw->songendidx != -1) {
        nval = uiSpinboxTimeGetValue (uiw->items [uiw->songendidx].spinbox);
        if (nval > 0) {
          nval = songutilNormalizePosition (nval, uiw->lastspeed);
          nval = songutilAdjustPosReal (nval, speed);
          uiSpinboxTimeSetValue (uiw->items [uiw->songendidx].spinbox, nval);
        }
      }
      if (uiw->bpmidx != -1) {
        nval = uiSpinboxGetValue (&uiw->items [uiw->bpmidx].uiwidget);
        if (nval > 0) {
          nval = songutilNormalizeBPM (nval, uiw->lastspeed);
          nval = songutilAdjustBPM (nval, speed);
          uiSpinboxSetValue (&uiw->items [uiw->bpmidx].uiwidget, nval);
        }
      }
      uiw->lastspeed = speed;
    }

    /* always re-create the BPM display */
    if (bpmdispidx != -1 && danceidx != -1) {
      char    *data;

      data = uisongeditGetBPMRangeDisplay (danceidx);
      uiLabelSetText (&uiw->items [bpmdispidx].uiwidget, data);
      dataFree (data);
    }

    uiw->checkchanged = false;
  } /* check changed is true */

  return;
}

void
uisongeditSetBPMValue (uisongedit_t *uisongedit, const char *args)
{
  se_internal_t *uiw;
  int             val;

  logProcBegin (LOG_PROC, "uisongeditSetBPMValue");
  uiw = uisongedit->uiWidgetData;

  if (uiw->bpmidx < 0) {
    logProcEnd (LOG_PROC, "uisongeditSetBPMValue", "bad-bpmidx");
    return;
  }

  val = atoi (args);
  uiSpinboxSetValue (&uiw->items [uiw->bpmidx].uiwidget, val);
  logProcEnd (LOG_PROC, "uisongeditSetBPMValue", "");
}

void
uisongeditSetPlayButtonState (uisongedit_t *uisongedit, int active)
{
  se_internal_t *uiw;
  UIWidget        *uiwidgetp;

  logProcBegin (LOG_PROC, "uisongeditSetPlayButtonState");
  uiw = uisongedit->uiWidgetData;

  uiwidgetp = uiButtonGetUIWidget (uiw->buttons [UISONGEDIT_BUTTON_PLAY]);
  /* if the player is active, disable the button */
  if (active) {
    uiWidgetDisable (uiwidgetp);
  } else {
    uiWidgetEnable (uiwidgetp);
  }
  logProcEnd (LOG_PROC, "uisongeditSetPlayButtonState", "");
}

/* internal routines */

static void
uisongeditAddDisplay (uisongedit_t *uisongedit, UIWidget *col, UIWidget *sg, int dispsel)
{
  slist_t         *sellist;
  UIWidget        hbox;
  char            *keystr;
  slistidx_t      dsiteridx;
  se_internal_t *uiw;

  logProcBegin (LOG_PROC, "uisongeditAddDisplay");
  uiw = uisongedit->uiWidgetData;
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
    uiw->items [uiw->itemcount].tagkey = tagkey;
    if (tagkey == TAG_BPM) {
      ++uiw->itemcount;
      uisongeditAddItem (uisongedit, &hbox, sg, TAG_BPM_DISPLAY);
      uiw->items [uiw->itemcount].tagkey = TAG_BPM_DISPLAY;
    }
    ++uiw->itemcount;
  }
  logProcEnd (LOG_PROC, "uisongeditAddDisplay", "");
}

static void
uisongeditAddItem (uisongedit_t *uisongedit, UIWidget *hbox, UIWidget *sg, int tagkey)
{
  UIWidget        uiwidget;
  se_internal_t *uiw;

  logProcBegin (LOG_PROC, "uisongeditAddItem");
  uiw = uisongedit->uiWidgetData;

  if (! tagdefs [tagkey].secondaryDisplay) {
    uiw->items [uiw->itemcount].chgind = uiCreateChangeIndicator (hbox);
    uichgindMarkNormal (uiw->items [uiw->itemcount].chgind);
    uiw->items [uiw->itemcount].changed = false;
    uiw->items [uiw->itemcount].lastchanged = false;

    uiCreateColonLabel (&uiwidget, tagdefs [tagkey].displayname);
    uiBoxPackStart (hbox, &uiwidget);
    uiSizeGroupAdd (sg, &uiwidget);
  }

  switch (tagdefs [tagkey].editType) {
    case ET_ENTRY: {
      uisongeditAddEntry (uisongedit, hbox, tagkey);
      break;
    }
    case ET_COMBOBOX: {
      if (tagkey == TAG_DANCE) {
        uiw->items [uiw->itemcount].uidance =
            uidanceDropDownCreate (hbox, uiw->parentwin,
            UIDANCE_EMPTY_DANCE, "", UIDANCE_PACK_START, 1);
        uidanceSetCallback (uiw->items [uiw->itemcount].uidance,
            uiw->callbacks [UISONGEDIT_CB_CHANGED]);
      }
      if (tagkey == TAG_GENRE) {
        uiw->items [uiw->itemcount].uigenre =
            uigenreDropDownCreate (hbox, uiw->parentwin, false);
        uigenreSetCallback (uiw->items [uiw->itemcount].uigenre,
            uiw->callbacks [UISONGEDIT_CB_CHANGED]);
      }
      break;
    }
    case ET_SPINBOX_TEXT: {
      if (tagkey == TAG_FAVORITE) {
        uiw->items [uiw->itemcount].uifavorite =
            uifavoriteSpinboxCreate (hbox);
        uifavoriteSetChangedCallback (uiw->items [uiw->itemcount].uifavorite,
            uiw->callbacks [UISONGEDIT_CB_CHANGED]);
      }
      if (tagkey == TAG_DANCELEVEL) {
        uiw->items [uiw->itemcount].uilevel =
            uilevelSpinboxCreate (hbox, FALSE);
        uilevelSizeGroupAdd (uiw->items [uiw->itemcount].uilevel, &uiw->sgsbtext);
        uilevelSetChangedCallback (uiw->items [uiw->itemcount].uilevel,
            uiw->callbacks [UISONGEDIT_CB_CHANGED]);
      }
      if (tagkey == TAG_DANCERATING) {
        uiw->items [uiw->itemcount].uirating =
            uiratingSpinboxCreate (hbox, FALSE);
        uiratingSizeGroupAdd (uiw->items [uiw->itemcount].uirating, &uiw->sgsbtext);
        uiratingSetChangedCallback (uiw->items [uiw->itemcount].uirating,
            uiw->callbacks [UISONGEDIT_CB_CHANGED]);
      }
      if (tagkey == TAG_STATUS) {
        uiw->items [uiw->itemcount].uistatus =
            uistatusSpinboxCreate (hbox, FALSE);
        uistatusSizeGroupAdd (uiw->items [uiw->itemcount].uistatus, &uiw->sgsbtext);
        uistatusSetChangedCallback (uiw->items [uiw->itemcount].uistatus,
            uiw->callbacks [UISONGEDIT_CB_CHANGED]);
      }
      break;
    }
    case ET_SPINBOX: {
      if (tagkey == TAG_BPM) {
        uiw->bpmidx = uiw->itemcount;
      }
      uisongeditAddSpinboxInt (uisongedit, hbox, tagkey);
      break;
    }
    case ET_SPINBOX_TIME: {
      if (tagkey == TAG_SONGSTART) {
        uiw->songstartidx = uiw->itemcount;
      }
      if (tagkey == TAG_SONGEND) {
        uiw->songendidx = uiw->itemcount;
      }
      uisongeditAddSpinboxTime (uisongedit, hbox, tagkey);
      break;
    }
    case ET_SCALE: {
      if (tagkey == TAG_SPEEDADJUSTMENT) {
        uiw->speedidx = uiw->itemcount;
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
uisongeditAddEntry (uisongedit_t *uisongedit, UIWidget *hbox, int tagkey)
{
  uientry_t       *entryp;
  UIWidget        *uiwidgetp;
  se_internal_t *uiw;

  logProcBegin (LOG_PROC, "uisongeditAddEntry");
  uiw = uisongedit->uiWidgetData;
  entryp = uiEntryInit (20, 100);
  uiw->items [uiw->itemcount].entry = entryp;
  uiEntryCreate (entryp);
  /* set the validate callback to set the changed flag */
  uiEntrySetValidate (entryp,
      uisongeditEntryChangedCallback, uiw, UIENTRY_IMMEDIATE);

  uiwidgetp = uiEntryGetUIWidget (entryp);
  uiWidgetAlignHorizFill (uiwidgetp);
  uiSizeGroupAdd (&uiw->sgentry, uiwidgetp);
  uiBoxPackStartExpand (hbox, uiwidgetp);
  logProcEnd (LOG_PROC, "uisongeditAddEntry", "");
}

static void
uisongeditAddSpinboxInt (uisongedit_t *uisongedit, UIWidget *hbox, int tagkey)
{
  UIWidget        *uiwidgetp;
  se_internal_t *uiw;

  logProcBegin (LOG_PROC, "uisongeditAddSpinboxInt");
  uiw = uisongedit->uiWidgetData;
  uiwidgetp = &uiw->items [uiw->itemcount].uiwidget;
  uiSpinboxIntCreate (uiwidgetp);
  if (tagkey == TAG_BPM) {
    uiSpinboxSet (uiwidgetp, 0.0, 400.0);
  }
  if (tagkey == TAG_TRACKNUMBER || tagkey == TAG_DISCNUMBER) {
    uiSpinboxSet (uiwidgetp, 1.0, 300.0);
  }
  uiSpinboxSetValueChangedCallback (uiwidgetp,
      uiw->callbacks [UISONGEDIT_CB_CHANGED]);
  uiSizeGroupAdd (&uiw->sgsbint, uiwidgetp);
  uiBoxPackStart (hbox, uiwidgetp);
  logProcEnd (LOG_PROC, "uisongeditAddSpinboxInt", "");
}

static void
uisongeditAddLabel (uisongedit_t *uisongedit, UIWidget *hbox, int tagkey)
{
  UIWidget        *uiwidgetp;
  se_internal_t *uiw;

  logProcBegin (LOG_PROC, "uisongeditAddLabel");
  uiw = uisongedit->uiWidgetData;
  uiwidgetp = &uiw->items [uiw->itemcount].uiwidget;
  uiCreateLabel (uiwidgetp, "");
  uiSizeGroupAdd (&uiw->sgentry, uiwidgetp);
  uiBoxPackStartExpand (hbox, uiwidgetp);
  logProcEnd (LOG_PROC, "uisongeditAddLabel", "");
}

static void
uisongeditAddSecondaryLabel (uisongedit_t *uisongedit, UIWidget *hbox, int tagkey)
{
  UIWidget        *uiwidgetp;
  se_internal_t *uiw;

  logProcBegin (LOG_PROC, "uisongeditAddSecondaryLabel");
  uiw = uisongedit->uiWidgetData;
  uiwidgetp = &uiw->items [uiw->itemcount].uiwidget;
  uiCreateLabel (uiwidgetp, "");
  uiBoxPackStart (hbox, uiwidgetp);
  logProcEnd (LOG_PROC, "uisongeditAddSecondaryLabel", "");
}

static void
uisongeditAddSpinboxTime (uisongedit_t *uisongedit, UIWidget *hbox, int tagkey)
{
  uispinbox_t     *sbp;
  se_internal_t *uiw;
  UIWidget        *uiwidgetp;

  logProcBegin (LOG_PROC, "uisongeditAddSpinboxTime");
  uiw = uisongedit->uiWidgetData;
  sbp = uiSpinboxTimeInit (SB_TIME_PRECISE);
  uiw->items [uiw->itemcount].spinbox = sbp;
  uiSpinboxTimeCreate (sbp, uisongedit, NULL);
  uiSpinboxSetRange (sbp, 0.0, 1200000.0);
  uiSpinboxTimeSetValue (sbp, 0);
  uiSpinboxTimeSetValueChangedCallback (sbp,
      uiw->callbacks [UISONGEDIT_CB_CHANGED]);

  uiwidgetp = uiSpinboxGetUIWidget (sbp);
  uiSizeGroupAdd (&uiw->sgsbtime, uiwidgetp);
  uiBoxPackStart (hbox, uiwidgetp);
  logProcEnd (LOG_PROC, "uisongeditAddSpinboxTime", "");
}

static void
uisongeditAddScale (uisongedit_t *uisongedit, UIWidget *hbox, int tagkey)
{
  se_internal_t *uiw;
  UIWidget        *uiwidgetp;
  double          lower, upper;
  double          inca, incb;
  int             digits;

  logProcBegin (LOG_PROC, "uisongeditAddScale");
  uiw = uisongedit->uiWidgetData;
  uiwidgetp = &uiw->items [uiw->itemcount].uiwidget;
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
  uiw->items [uiw->itemcount].callback = callbackInitDouble (
      uisongeditScaleDisplayCallback, &uiw->items [uiw->itemcount]);
  uiScaleSetCallback (uiwidgetp, uiw->items [uiw->itemcount].callback);

  uiSizeGroupAdd (&uiw->sgscale, uiwidgetp);
  uiBoxPackStart (hbox, uiwidgetp);

  uiwidgetp = &uiw->items [uiw->itemcount].display;
  uiCreateLabel (uiwidgetp, "100%");
  uiLabelAlignEnd (uiwidgetp);
  uiSizeGroupAdd (&uiw->sgscaledisp, uiwidgetp);
  uiBoxPackStart (hbox, uiwidgetp);
  logProcEnd (LOG_PROC, "uisongeditAddScale", "");
}

/* also sets the changed flag */
static bool
uisongeditScaleDisplayCallback (void *udata, double value)
{
  uisongedititem_t  *item = udata;
  se_internal_t   *uiw;
  char              tbuff [40];
  int               digits;

  logProcBegin (LOG_PROC, "uisongeditScaleDisplayCallback");
  uiw = item->uiw;
  uiw->checkchanged = true;
  digits = uiScaleGetDigits (&item->uiwidget);
  value = uiScaleEnforceMax (&item->uiwidget, value);
  snprintf (tbuff, sizeof (tbuff), "%4.*f%%", digits, value);
  uiLabelSetText (&item->display, tbuff);
  logProcEnd (LOG_PROC, "uisongeditScaleDisplayCallback", "");
  return UICB_CONT;
}

static void
uisongeditClearChanged (uisongedit_t *uisongedit)
{
  se_internal_t *uiw = NULL;

  logProcBegin (LOG_PROC, "uisongeditClearChanged");
  uiw = uisongedit->uiWidgetData;
  for (int count = 0; count < uiw->itemcount; ++count) {
    if (uiw->items [count].chgind == NULL) {
      continue;
    }
    uichgindMarkNormal (uiw->items [count].chgind);
    uiw->items [count].changed = false;
    uiw->items [count].lastchanged = false;
  }
  uiw->changed = 0;
  logProcEnd (LOG_PROC, "uisongeditClearChanged", "");
}

static bool
uisongeditSaveCallback (void *udata)
{
  uisongedit_t    *uisongedit = udata;
  se_internal_t *uiw = NULL;
  double          ndval;
  long            nval;
  const char      *ndata = NULL;
  int             chkvalue;
  char            tbuff [200];
  bool            valid = false;

  logProcBegin (LOG_PROC, "uisongeditSaveCallback");
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: song edit: save");
  uiw = uisongedit->uiWidgetData;

  valid = true;
  uiLabelSetText (uisongedit->statusMsg, "");

  /* do some validations */
  {
    long      songstart;
    long      songend;
    long      dur;
    int       speed;

    ndval = uiScaleGetValue (&uiw->items [uiw->speedidx].uiwidget);
    speed = round (ndval);
    songstart = uiSpinboxTimeGetValue (uiw->items [uiw->songstartidx].spinbox);
    songend = uiSpinboxTimeGetValue (uiw->items [uiw->songendidx].spinbox);
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
    dur = songGetNum (uiw->song, TAG_DURATION);
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

  for (int count = 0; count < uiw->itemcount; ++count) {
    int tagkey = uiw->items [count].tagkey;

    if (! uiw->items [count].changed) {
      continue;
    }

    chkvalue = SONGEDIT_CHK_NONE;

    switch (tagdefs [tagkey].editType) {
      case ET_ENTRY: {
        ndata = uiEntryGetValue (uiw->items [count].entry);
        chkvalue = SONGEDIT_CHK_STR;
        if (tagkey == TAG_TAGS) {
          chkvalue = SONGEDIT_CHK_LIST;
        }
        break;
      }
      case ET_COMBOBOX: {
        if (tagkey == TAG_DANCE) {
          nval = uidanceGetValue (uiw->items [count].uidance);
          chkvalue = SONGEDIT_CHK_NUM;
        }
        if (tagkey == TAG_GENRE) {
          nval = uigenreGetValue (uiw->items [count].uigenre);
          chkvalue = SONGEDIT_CHK_NUM;
        }
        break;
      }
      case ET_SPINBOX_TEXT: {
        if (tagkey == TAG_FAVORITE) {
          nval = uifavoriteGetValue (uiw->items [count].uifavorite);
          chkvalue = SONGEDIT_CHK_NUM;
        }
        if (tagkey == TAG_DANCELEVEL) {
          nval = uilevelGetValue (uiw->items [count].uilevel);
          chkvalue = SONGEDIT_CHK_NUM;
        }
        if (tagkey == TAG_DANCERATING) {
          nval = uiratingGetValue (uiw->items [count].uirating);
          chkvalue = SONGEDIT_CHK_NUM;
        }
        if (tagkey == TAG_STATUS) {
          nval = uistatusGetValue (uiw->items [count].uistatus);
          chkvalue = SONGEDIT_CHK_NUM;
        }
        break;
      }
      case ET_SPINBOX: {
        nval = uiSpinboxGetValue (&uiw->items [count].uiwidget);
        chkvalue = SONGEDIT_CHK_NUM;
        break;
      }
      case ET_SPINBOX_TIME: {
        nval = uiSpinboxTimeGetValue (uiw->items [count].spinbox);
        chkvalue = SONGEDIT_CHK_NUM;
        break;
      }
      case ET_SCALE: {
        ndval = uiScaleGetValue (&uiw->items [count].uiwidget);
        chkvalue = SONGEDIT_CHK_DOUBLE;
        if (tagkey == TAG_SPEEDADJUSTMENT) {
          nval = round (ndval);
          chkvalue = SONGEDIT_CHK_NUM;
        }
        break;
      }
      default: {
        chkvalue = SONGEDIT_CHK_NONE;
        break;
      }
    }

    if (chkvalue == SONGEDIT_CHK_STR) {
      songSetStr (uiw->song, tagkey, ndata);
    }

    if (chkvalue == SONGEDIT_CHK_NUM) {
      if (tagkey == TAG_BPM) {
        if (nval == 0) {
          nval = LIST_VALUE_INVALID;
        }
      }
      songSetNum (uiw->song, tagkey, nval);
    }

    if (chkvalue == SONGEDIT_CHK_DOUBLE) {
      songSetDouble (uiw->song, tagkey, ndval);
    }

    if (chkvalue == SONGEDIT_CHK_LIST) {
      songSetList (uiw->song, tagkey, ndata);
    }
  }

  /* song start, song end and bpm must be adjusted if they changed */
  /* on a speed change, the song start, song end and bpm will be modified */
  /* and the changed flag will be set. */
  if (uisongedit->savecb != NULL) {
    int     speed;
    ssize_t val;

    speed = songGetNum (uiw->song, TAG_SPEEDADJUSTMENT);
    if (speed > 0 && speed != 100) {
      if (uiw->songstartidx != -1 && uiw->items [uiw->songstartidx].changed) {
        val = songGetNum (uiw->song, TAG_SONGSTART);
        if (val > 0) {
          val = songutilNormalizePosition (val, speed);
          songSetNum (uiw->song, TAG_SONGSTART, val);
        }
      }
      if (uiw->songendidx != -1 && uiw->items [uiw->songendidx].changed) {
        val = songGetNum (uiw->song, TAG_SONGEND);
        if (val > 0) {
          val = songutilNormalizePosition (val, speed);
          songSetNum (uiw->song, TAG_SONGEND, val);
        }
      }
      if (uiw->bpmidx != -1 && uiw->items [uiw->bpmidx].changed) {
        val = songGetNum (uiw->song, TAG_BPM);
        if (val > 0) {
          val = songutilNormalizeBPM (val, speed);
          songSetNum (uiw->song, TAG_BPM, val);
        }
      }
    }
  }

  /* reset the changed flags and indicators */
  for (int count = 0; count < uiw->itemcount; ++count) {
    if (! uiw->items [count].changed) {
      continue;
    }
    if (uiw->items [count].chgind == NULL) {
      continue;
    }

    uichgindMarkNormal (uiw->items [count].chgind);
    uiw->items [count].changed = false;
    uiw->items [count].lastchanged = false;
  }

  if (uisongedit->savecb != NULL) {
    /* the callback re-loads the song editor */
    callbackHandlerLong (uisongedit->savecb, uiw->dbidx);
  }

  logProcEnd (LOG_PROC, "uisongeditSaveCallback", "");
  return UICB_CONT;
}

static bool
uisongeditFirstSelection (void *udata)
{
  uisongedit_t  *uisongedit = udata;

  logProcBegin (LOG_PROC, "uisongeditFirstSelection");
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: song edit: first");
  uisongselFirstSelection (uisongedit->uisongsel);
  uisongeditClearChanged (uisongedit);
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
  uisongeditClearChanged (uisongedit);
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
  uisongeditClearChanged (uisongedit);
  logProcEnd (LOG_PROC, "uisongeditNextSelection", "");
  return UICB_CONT;
}

static bool
uisongeditCopyPath (void *udata)
{
  uisongedit_t  *uisongedit = udata;
  se_internal_t *uiw;
  const char    *txt;
  char          *ffn;
  char          tbuff [MAXPATHLEN];

  uiw = uisongedit->uiWidgetData;

  txt = uiLabelGetText (&uiw->filedisp);
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
  se_internal_t *uiw;

  uiw = uisongedit->uiWidgetData;

  if (uiKeyIsPressEvent (uiw->uikey) &&
      uiKeyIsAudioPlayKey (uiw->uikey)) {
    uisongselPlayCallback (uisongedit->uisongsel);
  }

  if (uiKeyIsPressEvent (uiw->uikey)) {
    if (uiKeyIsControlPressed (uiw->uikey)) {
      if (uiKeyIsShiftPressed (uiw->uikey)) {
        if (uiKeyIsKey (uiw->uikey, 'A')) {
          uisongeditApplyAdjCallback (uisongedit);
          return UICB_STOP;
        }
      }
      if (uiKeyIsKey (uiw->uikey, 'S')) {
        uisongeditSaveCallback (uisongedit);
        return UICB_STOP;
      }
      if (uiKeyIsKey (uiw->uikey, 'N')) {
        uisongeditNextSelection (uisongedit);
        return UICB_STOP;
      }
      if (uiKeyIsKey (uiw->uikey, 'P')) {
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
  se_internal_t *uiw = udata;

  uiw->checkchanged = true;
  return 0;
}

static bool
uisongeditChangedCallback (void *udata)
{
  se_internal_t *uiw = udata;

  uiw->checkchanged = true;
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
