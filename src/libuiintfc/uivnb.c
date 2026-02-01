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
  uiwcont_t   *nb;
  uiwcont_t   *vlist;
  uiwcont_t   *tablist [VNB_MAX_PAGECOUNT];
  uiwcont_t   *indlist [VNB_MAX_PAGECOUNT];
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
  uiwcont_t   *hbox;
  uiwcont_t   *sw;

  vnb = mdmalloc (sizeof (uivnb_t));

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (box, hbox);

  sw = uiCreateScrolledWindow (50);
  uiBoxPackStart (hbox, sw);
  vnb->vlist = uiCreateVertBox ();
  uiWindowPackInWindow (sw, vnb->vlist);

  vnb->nb = uiCreateNotebook ();
  uiBoxPackStartExpand (hbox, vnb->nb);

  for (int i = 0; i < VNB_MAX_PAGECOUNT; ++i) {
    vnb->tablist [i] = NULL;
    vnb->indlist [i] = NULL;
    vnb->tabcblist [i] = NULL;
    vnb->cbdata [i].pagenum = i;
    vnb->cbdata [i].vnb = vnb;
    vnb->idlist [i] = VNB_NO_ID;
  }

  vnb->pagecount = 0;
  vnb->selected = -1;
  vnb->textdir = sysvarsGetNum (SVL_LOCALE_TEXT_DIR);

  uiwcontFree (sw);
  uiwcontFree (hbox);

  return vnb;
}

void
uivnbFree (uivnb_t *vnb)
{
  if (vnb == NULL) {
    return;
  }

  for (int i = 0; i < VNB_MAX_PAGECOUNT; ++i) {
    uiwcontFree (vnb->tablist [i]);
    uiwcontFree (vnb->indlist [i]);
    callbackFree (vnb->tabcblist [i]);
  }

  uiwcontFree (vnb->vlist);
  uiwcontFree (vnb->nb);
  mdfree (vnb);
}

void
uivnbAppendPage (uivnb_t *vnb, uiwcont_t *uiwidget, const char *nbtxt, int id)
{
  uiwcont_t   *hbox;
  uiwcont_t   *button;
  uiwcont_t   *label;
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

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vnb->vlist, hbox);
  uiWidgetAddClass (hbox, NB_CLASS);
  uiWidgetAddClass (hbox, NB_VERT_CLASS);

  button = uiCreateButton (cb, nbtxt, NULL, NULL);
  uiButtonSetReliefNone (button);
  uiButtonAlignLeft (button);
  uiBoxPackStartExpand (hbox, button);
  uiWidgetAddClass (button, NB_CLASS);
  uiWidgetAddClass (button, NB_VERT_CLASS);

  label = uiCreateLabel ("");
  uiBoxPackStart (hbox, label);
  uiWidgetSetMarginStart (label, 0);
  uiWidgetAddClass (label, NB_CLASS);
  uiWidgetAddClass (label, NB_VERT_CLASS);

  vnb->pagecount += 1;

  vnb->tablist [pagenum] = button;
  vnb->indlist [pagenum] = label;
  vnb->idlist [pagenum] = id;
  if (pagenum == 0) {
    uivnbSetPage (vnb, pagenum);
  }

  uiwcontFree (hbox);
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
  uiWidgetAddClass (vnb->indlist [vnb->selected], NB_VERT_SEL_CLASS);

  if (prevsel >= 0) {
    uiWidgetRemoveClass (vnb->tablist [prevsel], NB_SEL_CLASS);
    uiWidgetRemoveClass (vnb->indlist [prevsel], NB_VERT_SEL_CLASS);
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
