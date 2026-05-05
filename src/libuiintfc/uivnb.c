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
#include "uivnb.h"

enum {
  VNB_MAX_PAGECOUNT = 40,
};

typedef struct {
  uivnb_t     *vnb;
  int         pagenum;
} uivnbcb_t;

typedef struct uivnb {
  const char  *tag;
  uiwcont_t   *nb;
  uiwcont_t   *vgrid;
  uiwcont_t   *boxlist [VNB_MAX_PAGECOUNT];
  uiwcont_t   *tablist [VNB_MAX_PAGECOUNT];
  uiwcont_t   *indlist [VNB_MAX_PAGECOUNT];
  callback_t  *tabcblist [VNB_MAX_PAGECOUNT];
  uivnbcb_t   cbdata [VNB_MAX_PAGECOUNT];
  int         idlist [VNB_MAX_PAGECOUNT];
  int         pagecount;
  int         selected;
  int         textdir;
  bool        processed;
} uivnb_t;

bool uivnbSetPageCallback (void *udata);

uivnb_t *
uivnbCreate (uiwcont_t *box, const char *tag)
{
  uivnb_t     *vnb;
  uiwcont_t   *hbox;
  uiwcont_t   *sw;

  vnb = mdmalloc (sizeof (uivnb_t));
  vnb->tag = tag;

  hbox = uiCreateHorizBox ();
  uiBoxPackStartExpandChildren (box, hbox, WCONT_FREE);

  sw = uiCreateScrolledWindow (50);
  uiBoxPackStartExpandChildren (hbox, sw, WCONT_FREE);

  vnb->vgrid = uiCreateGrid ();
  uiWidgetSetClass (vnb->vgrid, VERTNB_BG_CLASS);
  uiWindowPackInWindow (sw, vnb->vgrid);

  vnb->nb = uiCreateNotebook ();
  uiWidgetSetMarginTop (vnb->nb, 0);
  uiBoxPackStartExpandChildren (hbox, vnb->nb, WCONT_KEEP);

  for (int i = 0; i < VNB_MAX_PAGECOUNT; ++i) {
    vnb->boxlist [i] = NULL;
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
  vnb->processed = false;

  uiBoxPostProcess (hbox);

  return vnb;
}

void
uivnbFree (uivnb_t *vnb)
{
  if (vnb == NULL) {
    return;
  }

  for (int i = 0; i < VNB_MAX_PAGECOUNT; ++i) {
    if (vnb->boxlist [i] == NULL) {
      break;
    }
    logMsg (LOG_DBG, LOG_UI, "vnb: free box/tab %d", i);
    uiwcontFree (vnb->boxlist [i]);
    callbackFree (vnb->tabcblist [i]);
  }

  uiwcontFree (vnb->vgrid);
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
  vnb->boxlist [pagenum] = uiwidget;

  cb = callbackInit (uivnbSetPageCallback, &vnb->cbdata [pagenum], NULL);
  vnb->tabcblist [pagenum] = cb;

  hbox = uiCreateHorizBox ();
  uiGridNewRow (vnb->vgrid);
  uiGridPack (vnb->vgrid, hbox, WCONT_FREE);
  uiWidgetExpandHoriz (hbox);
  uiWidgetAlignHorizFill (hbox);
  uiWidgetSetClass (hbox, NB_CLASS);
  uiWidgetSetClass (hbox, NB_VERT_CLASS);

  button = uiCreateButton (cb, nbtxt, NULL, NULL);
  uiButtonSetReliefNone (button);
  uiButtonAlignLeft (button);
  uiBoxPackStartExpandChildren (hbox, button, WCONT_FREE);
  uiWidgetSetClass (button, NB_CLASS);
  uiWidgetSetClass (button, NB_VERT_CLASS);

  label = uiCreateLabel ("");
  uiBoxPackStart (hbox, label, WCONT_FREE);
  uiWidgetSetMarginStart (label, 0);
  uiWidgetSetClass (label, NB_CLASS);
  uiWidgetSetClass (label, NB_VERT_CLASS);

  vnb->pagecount += 1;

  vnb->tablist [pagenum] = button;
  vnb->indlist [pagenum] = label;
  vnb->idlist [pagenum] = id;

  uiBoxPostProcess (hbox);
}

void
uivnbPostProcess (uivnb_t *vnb)
{
  if (vnb == NULL) {
    return;
  }

  uiGridPostProcess (vnb->vgrid);
  uivnbSetPage (vnb, 0);
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
  uiWidgetSetClass (vnb->tablist [vnb->selected], NB_SEL_CLASS);
  uiWidgetSetClass (vnb->indlist [vnb->selected], NB_VERT_SEL_CLASS);

  if (prevsel >= 0) {
    uiWidgetClearClass (vnb->tablist [prevsel], NB_SEL_CLASS);
    uiWidgetClearClass (vnb->indlist [prevsel], NB_VERT_SEL_CLASS);
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
