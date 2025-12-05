/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 *
 * vertical notebook
 *    GTK: the scrollable up/down arrows do not work correctly for RTL
 *        languages, and the arrows also do selection in addition to
 *        scrolling.
 *    This will be useful for MacOS, if a UI is ever written.
 *
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "callback.h"
#include "log.h"
#include "mdebug.h"
#include "oslocale.h"
#include "sysvars.h"
#include "tmutil.h"
#include "ui.h"
#include "uiclass.h"
#include "uivnb.h"
#include "uigeneral.h"
#include "uiwcont.h"

enum {
  VNB_MAX_PAGECOUNT = 40,
};

typedef struct {
  uivnb_t     *vnb;
  int         pagenum;
} uivnbcb_t;

typedef struct uivnb {
  uiwcont_t   *hbox;
  uiwcont_t   *sw;
  uiwcont_t   *vlist;
  uiwcont_t   *nb;
  uiwcont_t   *tablist [VNB_MAX_PAGECOUNT];
  callback_t  *tabcblist [VNB_MAX_PAGECOUNT];
  uivnbcb_t   cbdata [VNB_MAX_PAGECOUNT];
  int         idlist [VNB_MAX_PAGECOUNT];
  int         pagecount;
  int         selected;
  int         textdir;
} uivnb_t;

bool uivnbSetPageCallback (void *udata);

uivnb_t *
uivnbCreate (uiwcont_t *box)
{
  uivnb_t     *vnb;

  vnb = mdmalloc (sizeof (uivnb_t));

  vnb->hbox = uiCreateHorizBox ();

  vnb->sw = uiCreateScrolledWindow (50);
  uiBoxPackStart (vnb->hbox, vnb->sw);
  vnb->vlist = uiCreateVertBox ();
  uiWindowPackInWindow (vnb->sw, vnb->vlist);

  vnb->nb = uiCreateNotebook ();
  uiNotebookHideTabs (vnb->nb);
  uiBoxPackStartExpand (vnb->hbox, vnb->nb);

  uiBoxPackStart (box, vnb->hbox);

  for (int i = 0; i < VNB_MAX_PAGECOUNT; ++i) {
    vnb->tablist [i] = NULL;
    vnb->tabcblist [i] = NULL;
    vnb->cbdata [i].pagenum = i;
    vnb->cbdata [i].vnb = vnb;
    vnb->idlist [i] = VNB_NO_ID;
  }

  vnb->pagecount = 0;
  vnb->selected = -1;
  vnb->textdir = sysvarsGetNum (SVL_LOCALE_TEXT_DIR);

  return vnb;
}

void
uivnbFree (uivnb_t *vnb)
{
  for (int i = 0; i < VNB_MAX_PAGECOUNT; ++i) {
    uiwcontFree (vnb->tablist [i]);
    callbackFree (vnb->tabcblist [i]);
  }

  uiwcontFree (vnb->nb);
  uiwcontFree (vnb->vlist);
  uiwcontFree (vnb->sw);
  uiwcontFree (vnb->hbox);
}

void
uivnbAppendPage (uivnb_t *vnb, uiwcont_t *uiwidget, const char *label, int id)
{
  uiwcont_t   *button;
  char        tbuff [40];
  callback_t  *cb;
  int         pagenum;

  if (vnb == NULL) {
    return;
  }

  if (vnb->pagecount >= VNB_MAX_PAGECOUNT) {
    return;
  }

  uiNotebookAppendPage (vnb->nb, uiwidget, NULL);

  pagenum = vnb->pagecount;
  cb = callbackInit (uivnbSetPageCallback, &vnb->cbdata [pagenum], NULL);
  vnb->tabcblist [pagenum] = cb;

  snprintf (tbuff, sizeof (tbuff), "vnb-%d", vnb->pagecount);
  button = uiCreateButton (tbuff, cb, label, NULL);
  uiButtonAlignLeft (button);
  uiButtonSetReliefNone (button);
  uiBoxPackStart (vnb->vlist, button);
  uiWidgetExpandHoriz (button);
  uiWidgetAddClass (button, LEFT_NB_CLASS);

  vnb->pagecount += 1;

  vnb->tablist [pagenum] = button;
  vnb->idlist [pagenum] = id;
  if (pagenum == 0) {
    uivnbSetPage (vnb, pagenum);
  }
}

void
uivnbSetPage (uivnb_t *vnb, int pagenum)
{
  int     prevsel;

  if (vnb == NULL) {
    return;
  }
  if (pagenum < 0 || pagenum > vnb->pagecount) {
    return;
  }

  if (pagenum == vnb->selected) {
    return;
  }

  prevsel = vnb->selected;
  vnb->selected = pagenum;
  uiWidgetAddClass (vnb->tablist [vnb->selected], NB_SEL_CLASS);
  if (vnb->textdir == TEXT_DIR_RTL) {
    uiWidgetAddClass (vnb->tablist [vnb->selected], NB_SEL_RTL_CLASS);
  } else {
    uiWidgetAddClass (vnb->tablist [vnb->selected], NB_SEL_LTR_CLASS);
  }

  if (prevsel >= 0) {
    uiWidgetRemoveClass (vnb->tablist [prevsel], NB_SEL_CLASS);
    if (vnb->textdir == TEXT_DIR_RTL) {
      uiWidgetRemoveClass (vnb->tablist [prevsel], NB_SEL_RTL_CLASS);
    } else {
      uiWidgetRemoveClass (vnb->tablist [prevsel], NB_SEL_LTR_CLASS);
    }
  }

  /* after vnb->selected has been set */
  uiNotebookSetPage (vnb->nb, pagenum);
}

void
uivnbSetCallback (uivnb_t *vnb, callback_t *uicb)
{
  if (vnb == NULL) {
    return;
  }

  uiNotebookSetCallback (vnb->nb, uicb);
}

int
uivnbGetID (uivnb_t *vnb)
{
  if (vnb == NULL) {
    return VNB_NO_ID;
  }

  return vnb->idlist [vnb->selected];
}

int
uivnbGetPage (uivnb_t *vnb, int id)
{
  if (vnb == NULL) {
    return 0;
  }

  for (int i = 0; i < VNB_MAX_PAGECOUNT; ++i) {
    if (vnb->idlist [i] == id) {
      return i;
    }
  }

  return 0;
}

/* internal routines */

bool
uivnbSetPageCallback (void *udata)
{
  uivnbcb_t   *cbdata = udata;

  if (cbdata == NULL) {
    return UICB_STOP;
  }

  uivnbSetPage (cbdata->vnb, cbdata->pagenum);
  return UICB_CONT;
}
