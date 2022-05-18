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
#include "slist.h"
#include "tagdef.h"
#include "uisongedit.h"
#include "ui.h"

typedef struct {
  UIWidget            *parentwin;
  UIWidget            vbox;
} uisongeditgtk_t;

static void uisongeditAddDisplay (uisongedit_t *songedit, UIWidget *col, int dispsel);
static void uisongeditAddItem (UIWidget *hbox, UIWidget *sg, int tagkey);

void
uisongeditUIInit (uisongedit_t *uisongedit)
{
  uisongeditgtk_t  *uiw;

  uiw = malloc (sizeof (uisongeditgtk_t));
  uiw->parentwin = NULL;
  uiutilsUIWidgetInit (&uiw->vbox);
  uisongedit->uiWidgetData = uiw;
}

void
uisongeditUIFree (uisongedit_t *uisongedit)
{
  if (uisongedit->uiWidgetData != NULL) {
    free (uisongedit->uiWidgetData);
    uisongedit->uiWidgetData = NULL;
  }
}

UIWidget *
uisongeditBuildUI (uisongedit_t *uisongedit, UIWidget *parentwin)
{
  uisongeditgtk_t   *uiw;
  UIWidget          hbox;
  UIWidget          lcol;
  UIWidget          rcol;

  logProcBegin (LOG_PROC, "uisongeditBuildUI");

  uiw = uisongedit->uiWidgetData;
  uiw->parentwin = parentwin;

  uiCreateVertBox (&uiw->vbox);
  uiWidgetExpandHoriz (&uiw->vbox);

  uiCreateHorizBox (&hbox);
  uiWidgetExpandHoriz (&hbox);
  uiWidgetAlignHorizFill (&hbox);
  uiBoxPackStart (&uiw->vbox, &hbox);

  uiCreateVertBox (&lcol);
  uiWidgetAlignHorizStart (&lcol);
  uiBoxPackStart (&hbox, &lcol);

  uisongeditAddDisplay (uisongedit, &lcol, DISP_SEL_SONGEDIT_A);

  uiCreateVertBox (&rcol);
  uiWidgetAlignHorizStart (&rcol);
  uiWidgetExpandHoriz (&rcol);
  uiBoxPackStart (&hbox, &rcol);

  uisongeditAddDisplay (uisongedit, &rcol, DISP_SEL_SONGEDIT_B);

  logProcEnd (LOG_PROC, "uisongeditBuildUI", "");
  return &uiw->vbox;
}

/* internal routines */

static void
uisongeditAddDisplay (uisongedit_t *uisongedit, UIWidget *col, int dispsel)
{
  slist_t       *sellist;
  UIWidget      hbox;
  char          *keystr;
  slistidx_t    dsiteridx;
  UIWidget      sg;

  sellist = dispselGetList (uisongedit->dispsel, dispsel);
  uiCreateSizeGroupHoriz (&sg);

  slistStartIterator (sellist, &dsiteridx);
  while ((keystr = slistIterateKey (sellist, &dsiteridx)) != NULL) {
    int   tagkey;

    tagkey = slistGetNum (sellist, keystr);

    uiCreateHorizBox (&hbox);
    uiBoxPackStart (col, &hbox);
    uisongeditAddItem (&hbox, &sg, tagkey);
  }
}

static void
uisongeditAddItem (UIWidget *hbox, UIWidget *sg, int tagkey)
{
  UIWidget  uiwidget;

  if (tagkey >= TAG_KEY_MAX) {
    return;
  }
  if (! tagdefs [tagkey].isEditable) {
    return;
  }

  uiCreateColonLabel (&uiwidget, tagdefs [tagkey].displayname);
  uiBoxPackStart (hbox, &uiwidget);
  uiSizeGroupAdd (sg, &uiwidget);

  switch (tagkey) {
    /* entry box */
    case TAG_ALBUM:
    case TAG_ALBUMARTIST:
    case TAG_ARTIST:
    case TAG_COMPOSER:
    case TAG_CONDUCTOR:
    case TAG_KEYWORD:
    case TAG_MQDISPLAY:
    case TAG_NOTES:
    case TAG_TAGS:
    case TAG_TITLE: {
      break;
    }
    /* dropdown */
    case TAG_DANCE:
    case TAG_GENRE: {
      break;
    }
    /* spinbox-text */
    case TAG_DANCELEVEL:
    case TAG_DANCERATING:
    case TAG_STATUS: {
      break;
    }
    /* numeric entry box */
    case TAG_BPM: {
      break;
    }
    /* numeric spin box */
    case TAG_DISCNUMBER:
    case TAG_TRACKNUMBER: {
      break;
    }
    /* time spin box */
    case TAG_SONGEND:
    case TAG_SONGSTART: {
      break;
    }
    /* favorite spin box */
    case TAG_FAVORITE: {
      break;
    }
    /* scale */
    case TAG_SPEEDADJUSTMENT:
    case TAG_VOLUMEADJUSTPERC: {
      break;
    }
    default: {
      /* unknown */
      break;
    }
  }
}
