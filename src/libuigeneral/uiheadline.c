/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include "bdj4.h"
#include "bdjopt.h"
#include "bdjstring.h"      // needed for snprintf macro
#include "mdebug.h"
#include "tmutil.h"
#include "ui.h"
#include "uiheadline.h"

enum {
  PROFILE_BOX_SZ = 26,
};

typedef struct uihdrline {
  uiwcont_t   *hbox;
  uiwcont_t   *cbox;
} uihdrline_t;

uihdrline_t *
uiutilsHeaderLineSetup (uiwcont_t *boxp)
{
  uihdrline_t *hdrline;
  uiwcont_t   *hbox;
  uiwcont_t   *cbox;
  uiwcont_t   *filler;

  if (boxp == NULL) {
    return NULL;
  }

  hdrline = mdmalloc (sizeof (uihdrline_t));
  hdrline->hbox = NULL;
  hdrline->cbox = NULL;

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (boxp, hbox, WCONT_KEEP);
  uiWidgetExpandHoriz (hbox);

  cbox = uiCreateHorizBox ();
  uiWidgetSetSizeRequest (cbox, PROFILE_BOX_SZ, PROFILE_BOX_SZ);
  uiBoxPackEnd (hbox, cbox, WCONT_KEEP);
  uiWidgetAlignHorizCenter (cbox);
  uiWidgetAlignVertCenter (cbox);
  uiWidgetSetMarginStart (cbox, 4);
  uiBoxPostProcess (cbox);

  hdrline->cbox = cbox;
  hdrline->hbox = hbox;

  uiutilsHeaderLineSetColor (hdrline, NULL);

  /* put a blank in so that the header line does not change size */
  // ### not working, vert height still changes
  filler = uiCreateLabel (" ");
  uiBoxPackEnd (hbox, filler, WCONT_FREE);

  return hdrline;
}

uiwcont_t *
uiutilsHeaderLineAddMenubar (uihdrline_t *hdrline)
{
  uiwcont_t     *menubar;

  if (hdrline == NULL) {
    return NULL;
  }

  menubar = uiCreateMenubar ();
  uiBoxPackStart (hdrline->hbox, menubar, WCONT_KEEP);
  return menubar;
}

uiwcont_t *
uiutilsHeaderLineAddLabel (uihdrline_t *hdrline, const char *class)
{
  uiwcont_t     *msg;

  if (hdrline == NULL) {
    return NULL;
  }

  msg = uiCreateLabel ("");
  uiBoxPackEnd (hdrline->hbox, msg, WCONT_KEEP);
  uiWidgetSetClass (msg, class);
  return msg;
}

void
uiutilsHeaderLinePostProcess (uihdrline_t *hdrline)
{
  if (hdrline == NULL) {
    return;
  }

  uiWidgetShowAll (hdrline->hbox);
  uiBoxPostProcess (hdrline->hbox);
}

void
uiutilsHeaderLineFree (uihdrline_t *hdrline)
{
  if (hdrline == NULL) {
    return;
  }

  uiwcontFree (hdrline->hbox);
  uiwcontFree (hdrline->cbox);
  mdfree (hdrline);
}

void
uiutilsHeaderLineSetColor (uihdrline_t *hdrline, const char *oldcolor)
{
  char        classnm [100];
  char        bclassnm [100];
  const char  *tcolor = NULL;

  if (hdrline == NULL) {
    return;
  }

  tcolor = bdjoptGetStr (OPT_P_UI_PROFILE_COL);
  if (tcolor == NULL || ! *tcolor) {
    tcolor = BDJ4_DFLT_DARK_ACCENT_COLOR;
  }

  if (oldcolor != NULL) {
    snprintf (classnm, sizeof (classnm), "profcol%s", oldcolor + 1);
    uiWidgetClearClass (hdrline->cbox, classnm);
  }

  snprintf (classnm, sizeof (classnm), "profcol%s", tcolor + 1);
  /* macos will crash if just box.profcol%s is used! */
  snprintf (bclassnm, sizeof (bclassnm), "box.horizontal.profcol%s", tcolor + 1);
  /* the ui library has code to prevent duplicates */
  uiAddBGColorClass (bclassnm, bdjoptGetStr (OPT_P_UI_PROFILE_COL));
  uiWidgetSetClass (hdrline->cbox, classnm);
}
