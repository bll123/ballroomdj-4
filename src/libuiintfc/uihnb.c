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
  const char      *tag;
  uiwcont_t       *nb;
  uiwcont_t       *hlist;
  uiwcont_t       *boxlist [HNB_MAX_PAGECOUNT];
  uiwcont_t       *tablist [HNB_MAX_PAGECOUNT];
  callback_t      *tabcblist [HNB_MAX_PAGECOUNT];
  uihnbcb_t       cbdata [HNB_MAX_PAGECOUNT];
  int             idlist [HNB_MAX_PAGECOUNT];
  int             show [HNB_MAX_PAGECOUNT];
  int             buttononpage;
  int             pageiter;
  int             pagecount;
  int             selected;
  int             textdir;
} uihnb_t;

bool uihnbSetPageCallback (void *udata);

uihnb_t *
uihnbCreate (uiwcont_t *box, const char *tag)
{
  uihnb_t     *hnb;
  uiwcont_t   *vbox;

  hnb = mdmalloc (sizeof (uihnb_t));
  hnb->tag = tag;

  vbox = uiCreateVertBox ();
  uiBoxPackStartExpandChildren (box, vbox, WCONT_FREE);
  uiWidgetExpandHoriz (vbox);

  hnb->hlist = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hnb->hlist, WCONT_KEEP);

  hnb->nb = uiCreateNotebook ();
  uiWidgetSetClass (hnb->nb, NB_HORIZ_CLASS);
  uiBoxPackStartExpandChildren (vbox, hnb->nb, WCONT_KEEP);

  for (int i = 0; i < HNB_MAX_PAGECOUNT; ++i) {
    hnb->boxlist [i] = NULL;
    hnb->tablist [i] = NULL;
    hnb->tabcblist [i] = NULL;
    hnb->cbdata [i].pagenum = i;
    hnb->cbdata [i].hnb = hnb;
    hnb->idlist [i] = HNB_NO_ID;
    hnb->show [i] = HNB_SHOW;
  }

  hnb->buttononpage = 0;
  hnb->pagecount = 0;
  hnb->selected = -1;
  hnb->textdir = sysvarsGetNum (SVL_LOCALE_TEXT_DIR);

fprintf (stderr, "hnb: %s vbox pp\n", hnb->tag);
  uiBoxPostProcess (vbox);

  return hnb;
}

void
uihnbFree (uihnb_t *hnb)
{
  if (hnb == NULL) {
    return;
  }

  for (int i = 0; i < HNB_MAX_PAGECOUNT; ++i) {
    if (hnb->boxlist [i] == NULL) {
      break;
    }
    uiwcontFree (hnb->boxlist [i]);
    uiwcontFree (hnb->tablist [i]);
    callbackFree (hnb->tabcblist [i]);
  }

  uiwcontFree (hnb->hlist);
  uiwcontFree (hnb->nb);
  mdfree (hnb);
}

void
uihnbAppendPage (uihnb_t *hnb, uiwcont_t *uibox,
    const char *nbtxt, const char *imagenm, const char *altimagenm, int id)
{
  uiwcont_t       *hbox;
  uiwcont_t       *button;
  callback_t      *cb;
  int             pagenum;
  uibuttonstate_t state;

  if (hnb == NULL) {
    return;
  }

  if (hnb->pagecount >= HNB_MAX_PAGECOUNT) {
    return;
  }

  uiNotebookAppendPage (hnb->nb, uibox, NULL);

  pagenum = hnb->pagecount;
  hnb->boxlist [pagenum] = uibox;

  cb = callbackInit (uihnbSetPageCallback, &hnb->cbdata [pagenum], NULL);
  hnb->tabcblist [pagenum] = cb;

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (hnb->hlist, hbox, WCONT_FREE);
  uiWidgetSetClass (hbox, NB_CLASS);
  uiWidgetSetClass (hbox, NB_HORIZ_CLASS);

  button = uiCreateButton (cb, nbtxt, imagenm, NULL);
  uiButtonSetAltImage (button, altimagenm);
  uiWidgetAlignHorizCenter (button);
  uiButtonSetReliefNone (button);
  uiBoxPackStartExpandChildren (hbox, button, WCONT_KEEP);
  uiWidgetSetClass (button, NB_CLASS);
  uiWidgetSetClass (button, NB_HORIZ_CLASS);
  state = BUTTON_OFF;
  if (hnb->pagecount == 0) {
    state = BUTTON_ON;
  }
  uiButtonSetState (button, state);

  hnb->pagecount += 1;

  hnb->tablist [pagenum] = button;
  hnb->idlist [pagenum] = id;

fprintf (stderr, "hnb: %s hbox pp\n", hnb->tag);
  uiBoxPostProcess (hbox);
}

void
uihnbPostProcess (uihnb_t *hnb)
{
  if (hnb == NULL) {
    return;
  }

  uihnbSetPage (hnb, 0);
fprintf (stderr, "hnb: %s hlist pp\n", hnb->tag);
  uiBoxPostProcess (hnb->hlist);
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
  uiWidgetSetClass (hnb->tablist [hnb->selected], NB_SEL_CLASS);
  uiWidgetSetClass (hnb->tablist [hnb->selected], NB_HORIZ_SEL_CLASS);

  if (prevsel >= 0) {
    uiWidgetClearClass (hnb->tablist [prevsel], NB_SEL_CLASS);
    uiWidgetClearClass (hnb->tablist [prevsel], NB_HORIZ_SEL_CLASS);
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
  uiBoxPackEnd (hnb->hlist, uiwidget, WCONT_KEEP);
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
  hnb->show [pagenum] = show;
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
uihnbIterateID (uihnb_t *hnb, int *pagenum)
{
  int     id;

  if (hnb == NULL) {
    *pagenum = 0;
    return HNB_NO_ID;
  }

  if (hnb->pageiter >= hnb->pagecount) {
    hnb->pageiter = 0;
    *pagenum = 0;
    return HNB_NO_ID;
  }

  /* iterate over all pages, hidden or not */

  id = hnb->idlist [hnb->pageiter];
  *pagenum = hnb->pageiter;

  ++hnb->pageiter;

  return id;
}

void
uihnbSelect (uihnb_t *hnb, int pagenum)
{
  if (hnb == NULL) {
    return;
  }
  if (pagenum < 0 || pagenum >= hnb->pagecount) {
    return;
  }
  if (hnb->show [pagenum] == HNB_HIDE) {
    return;
  }

  uiButtonSetState (hnb->tablist [hnb->buttononpage], BUTTON_OFF);
  uiButtonSetState (hnb->tablist [pagenum], BUTTON_ON);
  hnb->buttononpage = pagenum;
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
