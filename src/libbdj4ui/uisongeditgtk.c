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
#include "conn.h"
#include "log.h"
#include "nlist.h"
#include "slist.h"
#include "tagdef.h"
#include "tmutil.h"
#include "uisong.h"
#include "uisongedit.h"
#include "ui.h"

typedef struct {
  int         tagkey;
  union {
    uientry_t   *entry;
    uispinbox_t *spinbox;
    UIWidget    uiwidget;
  };
  UIWidget    display;
  UICallback  callback;
} uisongedititem_t;

typedef struct {
  UIWidget            *parentwin;
  UIWidget            vbox;
  UIWidget            filedisp;
  UIWidget            sgentry;
  UIWidget            sgsbint;
  UIWidget            sgsbtime;
  UIWidget            sgscale;
  UIWidget            sgscaledisp;
  int                 itemalloccount;
  int                 itemcount;
  uisongedititem_t    *items;
} uisongeditgtk_t;

static void uisongeditAddDisplay (uisongedit_t *songedit, UIWidget *col, UIWidget *sg, int dispsel);
static void uisongeditAddItem (uisongedit_t *uisongedit, UIWidget *hbox, UIWidget *sg, int tagkey);
static void uisongeditAddEntry (uisongedit_t *uisongedit, UIWidget *hbox, int tagkey);
static void uisongeditAddSpinboxInt (uisongedit_t *uisongedit, UIWidget *hbox, int tagkey);
static void uisongeditAddLabel (uisongedit_t *uisongedit, UIWidget *hbox, int tagkey);
static void uisongeditAddSpinboxTime (uisongedit_t *uisongedit, UIWidget *hbox, int tagkey);
static void uisongeditAddScale (uisongedit_t *uisongedit, UIWidget *hbox, int tagkey);
static bool uisongeditScaleDisplayCallback (void *udata, double value);

void
uisongeditUIInit (uisongedit_t *uisongedit)
{
  uisongeditgtk_t  *uiw;

  uiw = malloc (sizeof (uisongeditgtk_t));
  uiw->parentwin = NULL;
  uiw->itemalloccount = 0;
  uiw->itemcount = 0;
  uiw->items = NULL;
  uiutilsUIWidgetInit (&uiw->vbox);
  uiCreateSizeGroupHoriz (&uiw->sgentry);
  uiCreateSizeGroupHoriz (&uiw->sgsbint);
  uiCreateSizeGroupHoriz (&uiw->sgsbtime);
  uiCreateSizeGroupHoriz (&uiw->sgscale);
  uiCreateSizeGroupHoriz (&uiw->sgscaledisp);

  uisongedit->uiWidgetData = uiw;
}

void
uisongeditUIFree (uisongedit_t *uisongedit)
{
  slist_t         *sellist;
  slistidx_t      dsiteridx;
  int             count;
  uisongeditgtk_t *uiw;
  char            *keystr;

  uiw = uisongedit->uiWidgetData;
  if (uiw != NULL) {
    for (int dispsel = DISP_SEL_SONGEDIT_A; dispsel <= DISP_SEL_SONGEDIT_C; ++dispsel) {
      sellist = dispselGetList (uisongedit->dispsel, dispsel);

      slistStartIterator (sellist, &dsiteridx);
      count = 0;
      while ((keystr = slistIterateKey (sellist, &dsiteridx)) != NULL) {
        int   tagkey;

        tagkey = slistGetNum (sellist, keystr);
        switch (tagdefs [tagkey].editType) {
          case ET_ENTRY: {
            uiEntryFree (uiw->items [count].entry);
            break;
          }
          case ET_COMBOBOX: {
            break;
          }
          case ET_SPINBOX_TEXT: {
            break;
          }
          case ET_SPINBOX: {
            break;
          }
          case ET_SPINBOX_TIME: {
            uiSpinboxTimeFree (uiw->items [count].spinbox);
            break;
          }
          case ET_NA:
          case ET_SCALE:
          case ET_LABEL: {
            break;
          }
        }
        ++count;
      }
    }

    if (uiw->items != NULL) {
      free (uiw->items);
    }
    free (uiw);
    uisongedit->uiWidgetData = NULL;
  }
}

UIWidget *
uisongeditBuildUI (uisongedit_t *uisongedit, UIWidget *parentwin)
{
  uisongeditgtk_t   *uiw;
  UIWidget          hbox;
  UIWidget          col;
  UIWidget          sg;
  UIWidget          uiwidget;

  logProcBegin (LOG_PROC, "uisongeditBuildUI");

  uiw = uisongedit->uiWidgetData;
  uiw->parentwin = parentwin;

  uiCreateVertBox (&uiw->vbox);
  uiWidgetExpandHoriz (&uiw->vbox);

  uiCreateHorizBox (&hbox);
  uiWidgetExpandHoriz (&hbox);
  uiWidgetAlignHorizFill (&hbox);
  uiBoxPackStart (&uiw->vbox, &hbox);

  uiCreateButton (&uiwidget, NULL, _("First"), NULL);
  uiBoxPackStart (&hbox, &uiwidget);
  uiWidgetDisable (&uiwidget);

  uiCreateButton (&uiwidget, NULL, _("Next"), NULL);
  uiBoxPackStart (&hbox, &uiwidget);
  uiWidgetDisable (&uiwidget);

  uiCreateButton (&uiwidget, NULL, _("Previous"), NULL);
  uiBoxPackStart (&hbox, &uiwidget);
  uiWidgetDisable (&uiwidget);

  uiCreateButton (&uiwidget, NULL, _("Play"), NULL);
  uiBoxPackStart (&hbox, &uiwidget);
  uiWidgetDisable (&uiwidget);

  uiCreateButton (&uiwidget, NULL, _("Save"), NULL);
  uiBoxPackEnd (&hbox, &uiwidget);
  uiWidgetDisable (&uiwidget);

  uiCreateHorizBox (&hbox);
  uiWidgetExpandHoriz (&hbox);
  uiWidgetAlignHorizFill (&hbox);
  uiBoxPackStart (&uiw->vbox, &hbox);

  /* CONTEXT: the audio file */
  uiCreateColonLabel (&uiwidget, _("File"));
  uiBoxPackStart (&hbox, &uiwidget);

  uiCreateLabel (&uiwidget, "");
  uiLabelEllipsizeOn (&uiwidget);
  uiBoxPackStart (&hbox, &uiwidget);
  uiutilsUIWidgetCopy (&uiw->filedisp, &uiwidget);

  uiCreateHorizBox (&hbox);
  uiWidgetExpandHoriz (&hbox);
  uiWidgetAlignHorizFill (&hbox);
  uiBoxPackStartExpand (&uiw->vbox, &hbox);

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
uisongeditLoadData (uisongedit_t *uisongedit, song_t *song)
{
  uisongeditgtk_t *uiw;
  char            *data;
  long            val;
  double          dval;

  uiw = uisongedit->uiWidgetData;

  for (int count = 0; count < uiw->itemcount; ++count) {
    int tagkey = uiw->items [count].tagkey;

    switch (tagdefs [tagkey].editType) {
      case ET_ENTRY: {
        data = uisongGetDisplay (song, tagkey, &val, &dval);
        uiEntrySetValue (uiw->items [count].entry, "");
        if (data != NULL) {
          uiEntrySetValue (uiw->items [count].entry, data);
          free (data);
        }
        break;
      }
      case ET_SPINBOX: {
        data = uisongGetDisplay (song, tagkey, &val, &dval);
        if (data != NULL) {
          fprintf (stderr, "et_spinbox: mismatch type\n");
        }
        if (val < 0) { val = 0; }
        uiSpinboxSetValue (&uiw->items [count].uiwidget, val);
        break;
      }
      case ET_SPINBOX_TIME: {
        data = uisongGetDisplay (song, tagkey, &val, &dval);
        if (data != NULL) {
          fprintf (stderr, "et_spinbox_time: mismatch type\n");
        }
        if (val < 0) { val = 0; }
        uiSpinboxTimeSetValue (uiw->items [count].spinbox, val);
        break;
      }
      case ET_SCALE: {
        data = uisongGetDisplay (song, tagkey, &val, &dval);
        if (data != NULL) {
          fprintf (stderr, "et_scale: mismatch type\n");
        }
        if (tagkey == TAG_SPEEDADJUSTMENT && dval == 0.0) {
          dval = 100.0;
        }
        uiScaleSetValue (&uiw->items [count].uiwidget, dval);
        uisongeditScaleDisplayCallback (&uiw->items [count], dval);
        break;
      }
      case ET_LABEL: {
        data = uisongGetDisplay (song, tagkey, &val, &dval);
        if (data != NULL) {
          uiLabelSetText (&uiw->items [count].uiwidget, data);
          free (data);
        }
        break;
      }
      default: {
        break;
      }
    }
  }
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

    if (uiw->itemcount >= uiw->itemalloccount) {
      uiw->itemalloccount += 10;
      uiw->items = realloc (uiw->items,
          sizeof (uisongedititem_t) * uiw->itemalloccount);
    }

    uisongeditAddItem (uisongedit, &hbox, sg, tagkey);

    uiw->items [uiw->itemcount].tagkey = tagkey;

    ++uiw->itemcount;
  }
}

static void
uisongeditAddItem (uisongedit_t *uisongedit, UIWidget *hbox, UIWidget *sg, int tagkey)
{
  UIWidget        uiwidget;
  uisongeditgtk_t *uiw;

  uiw = uisongedit->uiWidgetData;

  uiCreateColonLabel (&uiwidget, tagdefs [tagkey].displayname);
  uiBoxPackStart (hbox, &uiwidget);
  uiSizeGroupAdd (sg, &uiwidget);

  switch (tagdefs [tagkey].editType) {
    case ET_ENTRY: {
      uisongeditAddEntry (uisongedit, hbox, tagkey);
      break;
    }
    case ET_COMBOBOX: {
      break;
    }
    case ET_SPINBOX_TEXT: {
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
}

static void
uisongeditAddEntry (uisongedit_t *uisongedit, UIWidget *hbox, int tagkey)
{
  uientry_t       *entryp;
  UIWidget        *uiwidgetp;
  uisongeditgtk_t *uiw;

  uiw = uisongedit->uiWidgetData;
  entryp = uiEntryInit (tagdefs [tagkey].editWidth, 100);
  uiw->items [uiw->itemcount].entry = entryp;
  uiEntryCreate (entryp);
  uiwidgetp = uiEntryGetUIWidget (entryp);
  uiWidgetAlignHorizFill (uiwidgetp);
  uiSizeGroupAdd (&uiw->sgentry, uiwidgetp);
  uiBoxPackStartExpand (hbox, uiwidgetp);
}

static void
uisongeditAddSpinboxInt (uisongedit_t *uisongedit, UIWidget *hbox, int tagkey)
{
  UIWidget        *uiwidgetp;
  uisongeditgtk_t *uiw;

  uiw = uisongedit->uiWidgetData;
  uiwidgetp = &uiw->items [uiw->itemcount].uiwidget;
  uiSpinboxIntCreate (uiwidgetp);
  uiSizeGroupAdd (&uiw->sgsbint, uiwidgetp);
  uiBoxPackStart (hbox, uiwidgetp);
}

static void
uisongeditAddLabel (uisongedit_t *uisongedit, UIWidget *hbox, int tagkey)
{
  UIWidget        *uiwidgetp;
  uisongeditgtk_t *uiw;

  uiw = uisongedit->uiWidgetData;
  uiwidgetp = &uiw->items [uiw->itemcount].uiwidget;
  uiCreateLabel (uiwidgetp, "");
  uiLabelEllipsizeOn (uiwidgetp);
  uiSizeGroupAdd (&uiw->sgentry, uiwidgetp);
  uiBoxPackStartExpand (hbox, uiwidgetp);
}

static void
uisongeditAddSpinboxTime (uisongedit_t *uisongedit, UIWidget *hbox, int tagkey)
{
  uispinbox_t     *sbp;
  uisongeditgtk_t *uiw;
  UIWidget        *uiwidgetp;

  uiw = uisongedit->uiWidgetData;
  sbp = uiSpinboxTimeInit ();
  uiw->items [uiw->itemcount].spinbox = sbp;
  uiSpinboxTimeCreate (sbp, uisongedit, NULL);
  uiSpinboxTimeSetValue (sbp, 0);
  uiwidgetp = uiSpinboxGetUIWidget (sbp);
  uiSizeGroupAdd (&uiw->sgsbtime, uiwidgetp);
  uiBoxPackStart (hbox, uiwidgetp);
}

static void
uisongeditAddScale (uisongedit_t *uisongedit, UIWidget *hbox, int tagkey)
{
  uisongeditgtk_t *uiw;
  UIWidget        *uiwidgetp;
  double          lower, upper;
  int             digits;

  uiw = uisongedit->uiWidgetData;
  uiwidgetp = &uiw->items [uiw->itemcount].uiwidget;
  lower = 70.0;
  upper = 130.0;
  digits = 0;
  if (tagkey == TAG_VOLUMEADJUSTPERC) {
    lower = -50.0;
    upper = 50.0;
    digits = 1;
  }
  uiCreateScale (uiwidgetp, lower, upper, 1.0, 5.0, 0.0, digits);
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
}

static bool
uisongeditScaleDisplayCallback (void *udata, double value)
{
  uisongedititem_t  *item = udata;
  char              tbuff [40];
  int               digits;

  digits = uiScaleGetDigits (&item->uiwidget);
  snprintf (tbuff, sizeof (tbuff), "%.*f%%", digits, value);
  uiLabelSetText (&item->display, tbuff);
  return UICB_CONT;
}
