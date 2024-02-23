/*
 * Copyright 2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <unistd.h>
#include <math.h>

#include "bdj4.h"
#include "bdj4arg.h"
#include "bdjopt.h"
#include "callback.h"
#include "localeutil.h"
#include "mdebug.h"
#include "osuiutils.h"
#include "pathbld.h"
#include "tmutil.h"
#include "ui.h"
#include "uiutils.h"
#include "uiwcont.h"
#include "sysvars.h"

enum {
  UITEST_W_WINDOW,
  UITEST_W_MAX,
};

enum {
  UITEST_CB_CLOSE,
  UITEST_CB_MAX,
};

typedef struct {
  uiwcont_t     *wcont [UITEST_W_MAX];
  callback_t    *callbacks [UITEST_CB_MAX];
  bool          stop : 1;
} uitest_t;

static void uitestMainLoop (uitest_t *uitest);
static void uitestBuildUI (uitest_t *uitest);
static bool uitestCloseWin (void *udata);
static void uitestCleanup (uitest_t *uitest);

int
main (int argc, char *argv[])
{
  uitest_t    uitest;
  const char  *targ;
  bdj4arg_t   *bdj4arg;

  for (int i = 0; i < UITEST_W_MAX; ++i) {
    uitest.wcont [i] = NULL;
  }
  for (int i = 0; i < UITEST_CB_MAX; ++i) {
    uitest.callbacks [i] = NULL;
  }
  uitest.stop = false;

  bdj4arg = bdj4argInit (argc, argv);
  targ = bdj4argGet (bdj4arg, 0, argv [0]);
  sysvarsInit (targ);
  localeInit ();
  bdjoptInit ();

  uiUIInitialize (sysvarsGetNum (SVL_LOCALE_DIR));
  uiSetUICSS (uiutilsGetCurrentFont (),
      bdjoptGetStr (OPT_P_UI_ACCENT_COL),
      bdjoptGetStr (OPT_P_UI_ERROR_COL));

  uitestBuildUI (&uitest);
  osuiFinalize ();

  uitestMainLoop (&uitest);

  bdj4argCleanup (bdj4arg);
  uitestCleanup (&uitest);
#if BDJ4_MEM_DEBUG
  mdebugReport ();
  mdebugCleanup ();
#endif
  return 0;
}

static void
uitestMainLoop (uitest_t *uitest)
{
  while (uitest->stop == false) {
    uiUIProcessEvents ();
    mssleep (50);
  }
}

static void
uitestBuildUI (uitest_t *uitest)
{
  uiwcont_t   *vbox;
  uiwcont_t   *hbox;
  uiwcont_t   *uiwidgetp;
  char        imgbuff [MAXPATHLEN];

  uitest->callbacks [UITEST_CB_CLOSE] = callbackInit (
      uitestCloseWin, uitest, NULL);

  pathbldMakePath (imgbuff, sizeof (imgbuff),
      "bdj4_icon", BDJ4_IMG_SVG_EXT, PATHBLD_MP_DIR_IMG);
  uitest->wcont [UITEST_W_WINDOW] = uiCreateMainWindow (
      uitest->callbacks [UITEST_CB_CLOSE], "uitest", imgbuff);

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);
  uiWindowPackInWindow (uitest->wcont [UITEST_W_WINDOW], vbox);
  uiWidgetExpandHoriz (vbox);
  uiWidgetExpandVert (vbox);

  /* line 1 */

  hbox = uiCreateHorizBox ();
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  uiwidgetp = uiCreateLabel ("pack-start");
  uiBoxPackStart (hbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiCreateLabel ("a");
  uiBoxPackStart (hbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiCreateLabel ("b");
  uiBoxPackStart (hbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwcontFree (hbox);

  /* line 2 */

  hbox = uiCreateHorizBox ();
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  uiwidgetp = uiCreateLabel ("pack-end");
  uiBoxPackEnd (hbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiCreateLabel ("a");
  uiBoxPackEnd (hbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiCreateLabel ("b");
  uiBoxPackEnd (hbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwcontFree (hbox);

  /* line 3 */

  hbox = uiCreateHorizBox ();
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  uiwidgetp = uiCreateLabel ("pack-start-expand");
  uiBoxPackStartExpand (hbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiCreateLabel ("a");
  uiBoxPackStartExpand (hbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiCreateLabel ("b");
  uiBoxPackStartExpand (hbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwcontFree (hbox);

  /* line 3 */

  hbox = uiCreateHorizBox ();
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  uiwidgetp = uiCreateLabel ("pack-end-expand");
  uiBoxPackEndExpand (hbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiCreateLabel ("a");
  uiBoxPackEndExpand (hbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiCreateLabel ("b");
  uiBoxPackEndExpand (hbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwcontFree (hbox);

  /* */

  uiwcontFree (vbox);

  uiWidgetShowAll (uitest->wcont [UITEST_W_WINDOW]);
}

static bool
uitestCloseWin (void *udata)
{
  uitest_t   *uitest = udata;

  uitest->stop = true;
  return UICB_STOP;
}

static void
uitestCleanup (uitest_t *uitest)
{
  uiCloseWindow (uitest->wcont [UITEST_W_WINDOW]);
  uiCleanup ();

  for (int i = 0; i < UITEST_W_MAX; ++i) {
    uiwcontFree (uitest->wcont [i]);
  }

  for (int i = 0; i < UITEST_CB_MAX; ++i) {
    callbackFree (uitest->callbacks [i]);
  }

  bdjoptCleanup ();
  localeCleanup ();
}

