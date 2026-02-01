/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
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
#include "uihnb.h"
#include "uigeneral.h"
#include "uiwcont.h"

enum {
  HNB_MAX_PAGECOUNT = 40,
};

typedef struct {
  uihnb_t     *hnb;
  int         pagenum;
} uihnbcb_t;

typedef struct uihnb {
  uiwcont_t   *nb;
  uiwcont_t   *hlist;
  uiwcont_t   *tablist [HNB_MAX_PAGECOUNT];
  callback_t  *tabcblist [HNB_MAX_PAGECOUNT];
  uihnbcb_t   cbdata [HNB_MAX_PAGECOUNT];
  int         idlist [HNB_MAX_PAGECOUNT];
  int         pageiter;
  int         pagecount;
  int         selected;
  int         textdir;
} uihnb_t;

bool uihnbSetPageCallback (void *udata);

uihnb_t *
uihnbCreate (uiwcont_t *box)
{
  uihnb_t     *hnb;
  uiwcont_t   *vbox;

  hnb = mdmalloc (sizeof (uihnb_t));

  vbox = uiCreateVertBox ();
  uiBoxPackStartExpand (box, vbox);
  uiWidgetExpandHoriz (vbox);

  hnb->hlist = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hnb->hlist);

  hnb->nb = uiCreateNotebook ();
  uiWidgetAddClass (hnb->nb, NB_HORIZ_CLASS);
  uiNotebookHideTabs (hnb->nb);
  uiBoxPackStartExpand (vbox, hnb->nb);

  for (int i = 0; i < HNB_MAX_PAGECOUNT; ++i) {
    hnb->tablist [i] = NULL;
    hnb->tabcblist [i] = NULL;
    hnb->cbdata [i].pagenum = i;
    hnb->cbdata [i].hnb = hnb;
    hnb->idlist [i] = HNB_NO_ID;
  }

  hnb->pagecount = 0;
  hnb->selected = -1;
  hnb->textdir = sysvarsGetNum (SVL_LOCALE_TEXT_DIR);

  uiwcontFree (vbox);

  return hnb;
}

void
uihnbFree (uihnb_t *hnb)
{
  if (hnb == NULL) {
    return;
  }

  for (int i = 0; i < HNB_MAX_PAGECOUNT; ++i) {
    uiwcontFree (hnb->tablist [i]);
    callbackFree (hnb->tabcblist [i]);
  }

  uiwcontFree (hnb->hlist);
  uiwcontFree (hnb->nb);
  mdfree (hnb);
}

void
uihnbAppendPage (uihnb_t *hnb, uiwcont_t *uiwidget,
    const char *nbtxt, const char *imagenm, int id)
{
  uiwcont_t   *hbox;
  uiwcont_t   *button;
  callback_t  *cb;
  int         pagenum;

  if (hnb == NULL) {
    return;
  }

  if (hnb->pagecount >= HNB_MAX_PAGECOUNT) {
    return;
  }

  uiNotebookAppendPage (hnb->nb, uiwidget, NULL);

  pagenum = hnb->pagecount;
  cb = callbackInit (uihnbSetPageCallback, &hnb->cbdata [pagenum], NULL);
  hnb->tabcblist [pagenum] = cb;

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (hnb->hlist, hbox);
  uiWidgetAddClass (hbox, NB_CLASS);
  uiWidgetAddClass (hbox, NB_HORIZ_CLASS);

  button = uiCreateButton (cb, nbtxt, imagenm, NULL);
  uiWidgetAlignHorizCenter (button);
  uiButtonSetReliefNone (button);
  uiBoxPackStartExpand (hbox, button);
  uiWidgetAddClass (button, NB_CLASS);
  uiWidgetAddClass (button, NB_HORIZ_CLASS);

  hnb->pagecount += 1;

  hnb->tablist [pagenum] = button;
  hnb->idlist [pagenum] = id;
  if (pagenum == 0) {
    uihnbSetPage (hnb, pagenum);
  }

  uiwcontFree (hbox);
}

void
uihnbSetPage (uihnb_t *hnb, int pagenum)
{
  int     prevsel;

  if (hnb == NULL) {
    return;
  }
  if (pagenum < 0 || pagenum > hnb->pagecount) {
    return;
  }

  if (pagenum == hnb->selected) {
    return;
  }

  prevsel = hnb->selected;
  hnb->selected = pagenum;
  uiWidgetAddClass (hnb->tablist [hnb->selected], NB_SEL_CLASS);
  uiWidgetAddClass (hnb->tablist [hnb->selected], NB_HORIZ_SEL_CLASS);

  if (prevsel >= 0) {
    uiWidgetRemoveClass (hnb->tablist [prevsel], NB_SEL_CLASS);
    uiWidgetRemoveClass (hnb->tablist [prevsel], NB_HORIZ_SEL_CLASS);
  }

  /* after hnb->selected has been set */
  uiNotebookSetPage (hnb->nb, pagenum);
}

void
uihnbSetCallback (uihnb_t *hnb, callback_t *uicb)
{
  if (hnb == NULL) {
    return;
  }

  uiNotebookSetCallback (hnb->nb, uicb);
}

int
uihnbGetID (uihnb_t *hnb)
{
  if (hnb == NULL) {
    return HNB_NO_ID;
  }

  return hnb->idlist [hnb->selected];
}

int
uihnbGetIDByPage (uihnb_t *hnb, int pagenum)
{
  if (hnb == NULL) {
    return HNB_NO_ID;
  }

  if (pagenum < 0 || pagenum >= hnb->pagecount) {
    return HNB_NO_ID;
  }

  return hnb->idlist [pagenum];
}

int
uihnbGetPage (uihnb_t *hnb, int id)
{
  if (hnb == NULL) {
    return 0;
  }

  for (int i = 0; i < HNB_MAX_PAGECOUNT; ++i) {
    if (hnb->idlist [i] == id) {
      return i;
    }
  }

  return 0;
}

void
uihnbSetActionWidget (uihnb_t *hnb, uiwcont_t *uiwidget)
{
  if (hnb == NULL || hnb->hlist == NULL || uiwidget == NULL) {
    return;
  }
  uiBoxPackEnd (hnb->hlist, uiwidget);
}

void
uihnbHideShowPage (uihnb_t *hnb, int pagenum, bool show)
{
  if (hnb == NULL) {
    return;
  }

  if (pagenum < 0 || pagenum >= hnb->pagecount) {
    return;
  }

  if (show == HNB_SHOW) {
    uiWidgetShow (hnb->tablist [pagenum]);
  }
  if (show == HNB_HIDE) {
    uiWidgetHide (hnb->tablist [pagenum]);
  }
}

void
uihnbStartIDIterator (uihnb_t *hnb)
{
  if (hnb == NULL) {
    return;
  }

  hnb->pageiter = 0;
}

int
uihnbIterateID (uihnb_t *hnb)
{
  int     id;

  if (hnb == NULL) {
    return HNB_NO_ID;
  }

  if (hnb->pageiter >= hnb->pagecount) {
    hnb->pageiter = 0;
    return HNB_NO_ID;
  }

  id = hnb->idlist [hnb->pageiter];
  ++hnb->pageiter;
  return id;
}

/* internal routines */

bool
uihnbSetPageCallback (void *udata)
{
  uihnbcb_t   *cbdata = udata;

  if (cbdata == NULL) {
    return UICB_STOP;
  }

  uihnbSetPage (cbdata->hnb, cbdata->pagenum);
  return UICB_CONT;
}
