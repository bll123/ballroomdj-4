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
#include "level.h"
#include "log.h"
#include "nlist.h"
#include "pathbld.h"
#include "slist.h"
#include "song.h"
#include "songutil.h"
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
  uichgind_t  *chgind;
  UIWidget    display;
  UICallback  callback;
  bool        lastchanged : 1;
  bool        changed : 1;
} uisongedititem_t;

enum {
  UISONGEDIT_CB_FIRST,
  UISONGEDIT_CB_NEXT,
  UISONGEDIT_CB_PREVIOUS,
  UISONGEDIT_CB_PLAY,
  UISONGEDIT_CB_SAVE,
  UISONGEDIT_CB_MAX,
};

enum {
  UISONGEDIT_MAIN_TIMER = 40,
};

typedef struct {
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
  UICallback          callbacks [UISONGEDIT_CB_MAX];
  UIWidget            playbutton;
  level_t             *levels;
  song_t              *song;
  dbidx_t             dbidx;
  int                 itemcount;
  uisongedititem_t    *items;
  int                 changed;
  mstime_t            mainlooptimer;
  int                 bpmidx;
} uisongeditgtk_t;

static void uisongeditAddDisplay (uisongedit_t *songedit, UIWidget *col, UIWidget *sg, int dispsel);
static void uisongeditAddItem (uisongedit_t *uisongedit, UIWidget *hbox, UIWidget *sg, int tagkey);
static void uisongeditAddEntry (uisongedit_t *uisongedit, UIWidget *hbox, int tagkey);
static void uisongeditAddSpinboxInt (uisongedit_t *uisongedit, UIWidget *hbox, int tagkey);
static void uisongeditAddLabel (uisongedit_t *uisongedit, UIWidget *hbox, int tagkey);
static void uisongeditAddSpinboxTime (uisongedit_t *uisongedit, UIWidget *hbox, int tagkey);
static void uisongeditAddScale (uisongedit_t *uisongedit, UIWidget *hbox, int tagkey);
static bool uisongeditScaleDisplayCallback (void *udata, double value);
static void uisongeditClearChanged (uisongedit_t *uisongedit);
static bool uisongeditSaveCallback (void *udata);
static bool uisongeditFirstSelection (void *udata);
static bool uisongeditPreviousSelection (void *udata);
static bool uisongeditNextSelection (void *udata);

void
uisongeditUIInit (uisongedit_t *uisongedit)
{
  uisongeditgtk_t  *uiw;

  logProcBegin (LOG_PROC, "uisongeditUIInit");
  uiw = malloc (sizeof (uisongeditgtk_t));
  uiw->parentwin = NULL;
  uiw->itemcount = 0;
  uiw->items = NULL;
  uiw->changed = 0;
  uiw->levels = bdjvarsdfGet (BDJVDF_LEVELS);
  mstimeset (&uiw->mainlooptimer, UISONGEDIT_MAIN_TIMER);
  uiw->bpmidx = -1;
  uiw->dbidx = -1;

  uiutilsUIWidgetInit (&uiw->vbox);
  uiutilsUIWidgetInit (&uiw->playbutton);
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
  uisongeditgtk_t *uiw;

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
          if (uiw->items [count].entry != NULL) {
            uiEntryFree (uiw->items [count].entry);
          }
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
            uiSpinboxTimeFree (uiw->items [count].spinbox);
          }
          break;
        }
        case ET_NA:
        case ET_SCALE:
        case ET_LABEL: {
          break;
        }
      }
    }

    if (uiw->items != NULL) {
      free (uiw->items);
    }
    free (uiw);
    uisongedit->uiWidgetData = NULL;
  }
  logProcEnd (LOG_PROC, "uisongeditUIFree", "");
}

UIWidget *
uisongeditBuildUI (uisongsel_t *uisongsel, uisongedit_t *uisongedit,
    UIWidget *parentwin, UIWidget *statusMsg)
{
  uisongeditgtk_t   *uiw;
  UIWidget          hbox;
  UIWidget          col;
  UIWidget          sg;
  UIWidget          uiwidget;
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

  uiCreateHorizBox (&hbox);
  uiWidgetExpandHoriz (&hbox);
  uiWidgetAlignHorizFill (&hbox);
  uiBoxPackStart (&uiw->vbox, &hbox);

  uiutilsUICallbackInit (&uiw->callbacks [UISONGEDIT_CB_FIRST],
      uisongeditFirstSelection, uisongedit);
  uiCreateButton (&uiwidget, &uiw->callbacks [UISONGEDIT_CB_FIRST],
      /* CONTEXT: song editor : first song */
      _("First"), NULL);
  uiBoxPackStart (&hbox, &uiwidget);

  uiutilsUICallbackInit (&uiw->callbacks [UISONGEDIT_CB_PREVIOUS],
      uisongeditPreviousSelection, uisongedit);
  uiCreateButton (&uiwidget, &uiw->callbacks [UISONGEDIT_CB_PREVIOUS],
      /* CONTEXT: song editor : previous song */
      _("Previous"), NULL);
  uiBoxPackStart (&hbox, &uiwidget);

  uiutilsUICallbackInit (&uiw->callbacks [UISONGEDIT_CB_NEXT],
      uisongeditNextSelection, uisongedit);
  uiCreateButton (&uiwidget, &uiw->callbacks [UISONGEDIT_CB_NEXT],
      /* CONTEXT: song editor : next song */
      _("Next"), NULL);
  uiBoxPackStart (&hbox, &uiwidget);

  uiutilsUICallbackInit (&uiw->callbacks [UISONGEDIT_CB_PLAY],
      uisongselPlayCallback, uisongsel);
  uiCreateButton (&uiwidget, &uiw->callbacks [UISONGEDIT_CB_PLAY],
      /* CONTEXT: song editor : play song */
      _("Play"), NULL);
  uiBoxPackStart (&hbox, &uiwidget);
  uiutilsUIWidgetCopy (&uiw->playbutton, &uiwidget);

  uiutilsUICallbackInit (&uiw->callbacks [UISONGEDIT_CB_SAVE],
      uisongeditSaveCallback, uisongedit);
  uiCreateButton (&uiwidget, &uiw->callbacks [UISONGEDIT_CB_SAVE],
      /* CONTEXT: song editor : save data */
      _("Save"), NULL);
  uiBoxPackEnd (&hbox, &uiwidget);

  uiCreateHorizBox (&hbox);
  uiWidgetExpandHoriz (&hbox);
  uiWidgetAlignHorizFill (&hbox);
  uiBoxPackStart (&uiw->vbox, &hbox);

  /* CONTEXT: song editor: label for displaying the audio file path */
  uiCreateColonLabel (&uiwidget, _("File"));
  uiBoxPackStart (&hbox, &uiwidget);
  uiWidgetDisable (&uiwidget);

  uiCreateLabel (&uiwidget, "");
  uiLabelEllipsizeOn (&uiwidget);
  uiBoxPackStart (&hbox, &uiwidget);
  uiutilsUIWidgetCopy (&uiw->filedisp, &uiwidget);
  uiLabelDarkenColor (&uiw->filedisp, bdjoptGetStr (OPT_P_UI_ACCENT_COL));
  uiLabelSetSelectable (&uiw->filedisp);

  pathbldMakePath (tbuff, sizeof (tbuff), "musicbrainz-logo", ".svg",
      PATHBLD_MP_IMGDIR);
  uiImageFromFile (&uiw->musicbrainzPixbuf, tbuff);
  uiImageGetPixbuf (&uiw->musicbrainzPixbuf);
  uiWidgetMakePersistent (&uiw->musicbrainzPixbuf);

  uiImageNew (&uiw->audioidImg);
  uiImageClear (&uiw->audioidImg);
  uiWidgetSetSizeRequest (&uiw->audioidImg, 24, -1);
  uiWidgetSetMarginStart (&uiw->audioidImg, uiBaseMarginSz);
  uiBoxPackEnd (&hbox, &uiw->audioidImg);

  uiCreateLabel (&uiwidget, "");
  uiBoxPackEnd (&hbox, &uiwidget);
  uiutilsUIWidgetCopy (&uiw->modified, &uiwidget);
  uiLabelDarkenColor (&uiw->modified, bdjoptGetStr (OPT_P_UI_ACCENT_COL));

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
  uiw->items = malloc (sizeof (uisongedititem_t) * count);
  for (int i = 0; i < count; ++i) {
    uiw->items [i].tagkey = 0;
    uiutilsUIWidgetInit (&uiw->items [i].uiwidget);
    uiw->items [i].entry = NULL;
    uiw->items [i].chgind = NULL;
    uiutilsUICallbackInit (&uiw->items [i].callback, NULL, NULL);
    uiw->items [i].lastchanged = false;
    uiw->items [i].changed = false;
  }

  for (int i = DISP_SEL_SONGEDIT_A; i <= DISP_SEL_SONGEDIT_C; ++i) {
    uiCreateVertBox (&col);
    uiWidgetSetAllMargins (&col, uiBaseMarginSz * 4);
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
  uisongeditgtk_t *uiw;
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
  if (data != NULL) {
    free (data);
    data = NULL;
  }

  uiImageClear (&uiw->audioidImg);
  data = songGetStr (song, TAG_RECORDING_ID);
  if (data != NULL && *data) {
    uiImageSetFromPixbuf (&uiw->audioidImg, &uiw->musicbrainzPixbuf);
  }
  val = songGetNum (song, TAG_ADJUSTFLAGS);

  uiLabelSetText (&uiw->modified, "");
  if (val != SONG_ADJUST_NONE) {
    uiLabelSetText (&uiw->modified, "*");
  }

  for (int count = 0; count < uiw->itemcount; ++count) {
    int tagkey = uiw->items [count].tagkey;

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
        if (dval < 0.0) { dval = 0.0; }
        if (tagkey == TAG_SPEEDADJUSTMENT) {
          if (dval == 0.0) { dval = 100.0; }
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

    if (data != NULL) {
      free (data);
      data = NULL;
    }
  }
  logProcEnd (LOG_PROC, "uisongeditLoadData", "");
}

void
uisongeditUIMainLoop (uisongedit_t *uisongedit)
{
  uisongeditgtk_t *uiw;
  double          dval, ndval;
  long            val, nval;
  char            *songdata;
  const char      *ndata;
  int             chkvalue;

  uiw = uisongedit->uiWidgetData;

  if (! mstimeCheck (&uiw->mainlooptimer)) {
    /* preserve some efficiency.  the changes don't need to be checked */
    /* too often */
    return;
  }
  mstimeset (&uiw->mainlooptimer, UISONGEDIT_MAIN_TIMER);

  /* look for changed items */
  /* this is not very efficient, but it works well for the user, */
  /* as it is able to determine if a value has reverted back to */
  /* the original value */
  for (int count = 0; count < uiw->itemcount; ++count) {
    int tagkey = uiw->items [count].tagkey;

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
        if (dval < 0.0) { dval = 0.0; }
        ndval = uiScaleGetValue (&uiw->items [count].uiwidget);
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

      /* for the speed adjustment, 0.0 (no setting) is changed to 100.0 */
      rc = (tagkey == TAG_SPEEDADJUSTMENT && ndval == 100.0 && dval == 0.0);
      if (! rc && ndval != dval) {
        uiw->items [count].changed = true;
      } else {
        if (uiw->items [count].changed) {
          uiw->items [count].changed = false;
        }
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

    if (songdata != NULL) {
      free (songdata);
    }
  }

  return;
}

void
uisongeditSetBPMValue (uisongedit_t *uisongedit, const char *args)
{
  uisongeditgtk_t *uiw;
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
  uisongeditgtk_t *uiw;

  logProcBegin (LOG_PROC, "uisongeditSetPlayButtonState");
  uiw = uisongedit->uiWidgetData;

  /* if the player is active, disable the button */
  if (active) {
    uiWidgetDisable (&uiw->playbutton);
  } else {
    uiWidgetEnable (&uiw->playbutton);
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
  uisongeditgtk_t *uiw;

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
    ++uiw->itemcount;
  }
  logProcEnd (LOG_PROC, "uisongeditAddDisplay", "");
}

static void
uisongeditAddItem (uisongedit_t *uisongedit, UIWidget *hbox, UIWidget *sg, int tagkey)
{
  UIWidget        uiwidget;
  uisongeditgtk_t *uiw;

  logProcBegin (LOG_PROC, "uisongeditAddItem");
  uiw = uisongedit->uiWidgetData;

  uiw->items [uiw->itemcount].chgind = uiCreateChangeIndicator (hbox);
  uiw->items [uiw->itemcount].changed = false;
  uiw->items [uiw->itemcount].lastchanged = false;

  uiCreateColonLabel (&uiwidget, tagdefs [tagkey].displayname);
  uiBoxPackStart (hbox, &uiwidget);
  uiSizeGroupAdd (sg, &uiwidget);

  switch (tagdefs [tagkey].editType) {
    case ET_ENTRY: {
      uisongeditAddEntry (uisongedit, hbox, tagkey);
      break;
    }
    case ET_COMBOBOX: {
      if (tagkey == TAG_DANCE) {
        uiw->items [uiw->itemcount].uidance =
            uidanceDropDownCreate (hbox, uiw->parentwin,
            UIDANCE_EMPTY_DANCE, "", UIDANCE_PACK_START);
      }
      if (tagkey == TAG_GENRE) {
        uiw->items [uiw->itemcount].uigenre =
            uigenreDropDownCreate (hbox, uiw->parentwin, false);
      }
      break;
    }
    case ET_SPINBOX_TEXT: {
      if (tagkey == TAG_FAVORITE) {
        uiw->items [uiw->itemcount].uifavorite =
            uifavoriteSpinboxCreate (hbox);
      }
      if (tagkey == TAG_DANCELEVEL) {
        uiw->items [uiw->itemcount].uilevel =
            uilevelSpinboxCreate (hbox, FALSE);
        uilevelSizeGroupAdd (uiw->items [uiw->itemcount].uilevel, &uiw->sgsbtext);
      }
      if (tagkey == TAG_DANCERATING) {
        uiw->items [uiw->itemcount].uirating =
            uiratingSpinboxCreate (hbox, FALSE);
        uiratingSizeGroupAdd (uiw->items [uiw->itemcount].uirating, &uiw->sgsbtext);
      }
      if (tagkey == TAG_STATUS) {
        uiw->items [uiw->itemcount].uistatus =
            uistatusSpinboxCreate (hbox, FALSE);
        uistatusSizeGroupAdd (uiw->items [uiw->itemcount].uistatus, &uiw->sgsbtext);
      }
      break;
    }
    case ET_SPINBOX: {
      uisongeditAddSpinboxInt (uisongedit, hbox, tagkey);
      break;
    }
    case ET_SPINBOX_TIME: {
      uisongeditAddSpinboxTime (uisongedit, hbox, tagkey);
      break;
    }
    case ET_SCALE: {
      uisongeditAddScale (uisongedit, hbox, tagkey);
      break;
    }
    case ET_LABEL: {
      uisongeditAddLabel (uisongedit, hbox, tagkey);
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
  uisongeditgtk_t *uiw;

  logProcBegin (LOG_PROC, "uisongeditAddEntry");
  uiw = uisongedit->uiWidgetData;
  entryp = uiEntryInit (20, 100);
  uiw->items [uiw->itemcount].entry = entryp;
  uiEntryCreate (entryp);
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
  uisongeditgtk_t *uiw;

  logProcBegin (LOG_PROC, "uisongeditAddSpinboxInt");
  uiw = uisongedit->uiWidgetData;
  uiwidgetp = &uiw->items [uiw->itemcount].uiwidget;
  uiSpinboxIntCreate (uiwidgetp);
  if (tagkey == TAG_BPM) {
    uiSpinboxSet (uiwidgetp, 0.0, 400.0);
    uiw->bpmidx = uiw->itemcount;
  }
  if (tagkey == TAG_TRACKNUMBER || tagkey == TAG_DISCNUMBER) {
    uiSpinboxSet (uiwidgetp, 1.0, 300.0);
  }
  uiSizeGroupAdd (&uiw->sgsbint, uiwidgetp);
  uiBoxPackStart (hbox, uiwidgetp);
  logProcEnd (LOG_PROC, "uisongeditAddSpinboxInt", "");
}

static void
uisongeditAddLabel (uisongedit_t *uisongedit, UIWidget *hbox, int tagkey)
{
  UIWidget        *uiwidgetp;
  uisongeditgtk_t *uiw;

  logProcBegin (LOG_PROC, "uisongeditAddLabel");
  uiw = uisongedit->uiWidgetData;
  uiwidgetp = &uiw->items [uiw->itemcount].uiwidget;
  uiCreateLabel (uiwidgetp, "");
  uiLabelEllipsizeOn (uiwidgetp);
  uiSizeGroupAdd (&uiw->sgentry, uiwidgetp);
  uiBoxPackStartExpand (hbox, uiwidgetp);
  logProcEnd (LOG_PROC, "uisongeditAddLabel", "");
}

static void
uisongeditAddSpinboxTime (uisongedit_t *uisongedit, UIWidget *hbox, int tagkey)
{
  uispinbox_t     *sbp;
  uisongeditgtk_t *uiw;
  UIWidget        *uiwidgetp;

  logProcBegin (LOG_PROC, "uisongeditAddSpinboxTime");
  uiw = uisongedit->uiWidgetData;
  sbp = uiSpinboxTimeInit (SB_TIME_PRECISE);
  uiw->items [uiw->itemcount].spinbox = sbp;
  uiSpinboxTimeCreate (sbp, uisongedit, NULL);
  uiSpinboxSetRange (sbp, 0, 1200000);
  uiSpinboxTimeSetValue (sbp, 0);
  uiwidgetp = uiSpinboxGetUIWidget (sbp);
  uiSizeGroupAdd (&uiw->sgsbtime, uiwidgetp);
  uiBoxPackStart (hbox, uiwidgetp);
  logProcEnd (LOG_PROC, "uisongeditAddSpinboxTime", "");
}

static void
uisongeditAddScale (uisongedit_t *uisongedit, UIWidget *hbox, int tagkey)
{
  uisongeditgtk_t *uiw;
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
  uiutilsUICallbackDoubleInit (&uiw->items [uiw->itemcount].callback,
      uisongeditScaleDisplayCallback, &uiw->items [uiw->itemcount]);
  uiScaleSetCallback (uiwidgetp, &uiw->items [uiw->itemcount].callback);
  uiSizeGroupAdd (&uiw->sgscale, uiwidgetp);
  uiBoxPackStart (hbox, uiwidgetp);

  uiwidgetp = &uiw->items [uiw->itemcount].display;
  uiCreateLabel (uiwidgetp, "100%");
  uiLabelAlignEnd (uiwidgetp);
  uiSizeGroupAdd (&uiw->sgscaledisp, uiwidgetp);
  uiBoxPackStart (hbox, uiwidgetp);
  logProcEnd (LOG_PROC, "uisongeditAddScale", "");
}

static bool
uisongeditScaleDisplayCallback (void *udata, double value)
{
  uisongedititem_t  *item = udata;
  char              tbuff [40];
  int               digits;

  logProcBegin (LOG_PROC, "uisongeditScaleDisplayCallback");
  digits = uiScaleGetDigits (&item->uiwidget);
  snprintf (tbuff, sizeof (tbuff), "%3.*f%%", digits, value);
  uiLabelSetText (&item->display, tbuff);
  logProcEnd (LOG_PROC, "uisongeditScaleDisplayCallback", "");
  return UICB_CONT;
}

static void
uisongeditClearChanged (uisongedit_t *uisongedit)
{
  uisongeditgtk_t *uiw = NULL;

  logProcBegin (LOG_PROC, "uisongeditClearChanged");
  uiw = uisongedit->uiWidgetData;
  for (int count = 0; count < uiw->itemcount; ++count) {
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
  uisongeditgtk_t *uiw = NULL;
  double          ndval;
  long            nval;
  const char      *ndata = NULL;
  int             chkvalue;
  char            tbuff [200];
  bool            valid;

  logProcBegin (LOG_PROC, "uisongeditSaveCallback");
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: song edit: save");
  uiw = uisongedit->uiWidgetData;

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
      songSetNum (uiw->song, tagkey, nval);
    }

    if (chkvalue == SONGEDIT_CHK_DOUBLE) {
      songSetDouble (uiw->song, tagkey, ndval);
    }

    if (chkvalue == SONGEDIT_CHK_LIST) {
      songSetList (uiw->song, tagkey, ndata);
    }

    uichgindMarkNormal (uiw->items [count].chgind);
    uiw->items [count].changed = false;
    uiw->items [count].lastchanged = false;
  }

  valid = true;
  uiLabelSetText (uisongedit->statusMsg, "");

  /* do some validations */
  {
    long      songstart;
    long      songend;

    songstart = songGetNum (uiw->song, TAG_SONGSTART);
    songend = songGetNum (uiw->song, TAG_SONGEND);
    if (songstart > 0 && songend > 0 && songstart >= songend) {
      if (uisongedit->statusMsg != NULL) {
        /* CONTEXT: song editor: status msg: (song end must be greater than song start) */
        snprintf (tbuff, sizeof (tbuff), _("%1$s must be greater than %2$s"),
            tagdefs [TAG_SONGEND].displayname,
            tagdefs [TAG_SONGSTART].displayname);
        uiLabelSetText (uisongedit->statusMsg, tbuff);
        logProcEnd (LOG_PROC, "uisongeditSaveCallback", "song-end-<-song-start");
        valid = false;
      }
    }
  }

  if (valid && uisongedit->savecb != NULL) {
    uiutilsCallbackLongHandler (uisongedit->savecb, uiw->dbidx);
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

