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
  UITEST_W_MAIN_NB,
  UITEST_W_MENUBAR,
  UITEST_W_STATUS_MSG,
  UITEST_W_B,
  UITEST_W_B_IMG_A,
  UITEST_W_B_IMG_B,
  UITEST_W_B_MSG,
  UITEST_W_B_IMG_A_MSG,
  UITEST_W_B_IMG_B_MSG,
  UITEST_W_MAX,
};

enum {
  UITEST_CB_CLOSE,
  UITEST_CB_B,
  UITEST_CB_B_IMG_A,
  UITEST_CB_B_IMG_B,
  UITEST_CB_MAX,
};

enum {
  UITEST_I_LED_OFF,
  UITEST_I_MAX,
};

typedef struct {
  uiwcont_t     *wcont [UITEST_W_MAX];
  callback_t    *callbacks [UITEST_CB_MAX];
  uiwcont_t     *images [UITEST_I_MAX];
  long          counter;
  bool          stop : 1;
} uitest_t;

static void uitestMainLoop (uitest_t *uitest);
static void uitestBuildUI (uitest_t *uitest);
static bool uitestCloseWin (void *udata);
static void uitestCounterDisp (uitest_t *uitest, uiwcont_t *uiwidgetp);
static bool uitestCBButton (void *udata);
static bool uitestCBButtonImgA (void *udata);
static bool uitestCBButtonImgB (void *udata);
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
  for (int i = 0; i < UITEST_I_MAX; ++i) {
    uitest.images [i] = NULL;
  }
  uitest.stop = false;
  uitest.counter = 1;

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
  uiwcont_t   *twidgetp;
  char        imgbuff [MAXPATHLEN];
  uiutilsaccent_t accent;

  uitest->callbacks [UITEST_CB_CLOSE] = callbackInit (
      uitestCloseWin, uitest, NULL);

  uitest->callbacks [UITEST_CB_B] = callbackInit (
      uitestCBButton, uitest, NULL);
  uitest->callbacks [UITEST_CB_B_IMG_A] = callbackInit (
      uitestCBButtonImgA, uitest, NULL);
  uitest->callbacks [UITEST_CB_B_IMG_B] = callbackInit (
      uitestCBButtonImgB, uitest, NULL);

  pathbldMakePath (imgbuff, sizeof (imgbuff),
      "bdj4_icon", BDJ4_IMG_SVG_EXT, PATHBLD_MP_DIR_IMG);
  uitest->wcont [UITEST_W_WINDOW] = uiCreateMainWindow (
      uitest->callbacks [UITEST_CB_CLOSE], "uitest", imgbuff);

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);
  uiWindowPackInWindow (uitest->wcont [UITEST_W_WINDOW], vbox);

  uiutilsAddProfileColorDisplay (vbox, &accent);
  hbox = accent.hbox;
  uiwcontFree (accent.label);

  uitest->wcont [UITEST_W_MENUBAR] = uiCreateMenubar ();
  uiBoxPackStart (hbox, uitest->wcont [UITEST_W_MENUBAR]);

  uiwidgetp = uiCreateLabel ("");
  uiWidgetSetClass (uiwidgetp, ACCENT_CLASS);
  uiBoxPackEnd (hbox, uiwidgetp);
  uitest->wcont [UITEST_W_STATUS_MSG] = uiwidgetp;

  /* main notebook */

  uitest->wcont [UITEST_W_MAIN_NB] = uiCreateNotebook ();
  uiWidgetSetClass (uitest->wcont [UITEST_W_MAIN_NB], LEFT_NB_CLASS);
  uiNotebookTabPositionLeft (uitest->wcont [UITEST_W_MAIN_NB]);
  uiBoxPackStartExpand (vbox, uitest->wcont [UITEST_W_MAIN_NB]);

  uiwcontFree (vbox);

  /* buttons, switch */

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);

  uiwidgetp = uiCreateLabel ("Button/Switch");
  uiNotebookAppendPage (uitest->wcont [UITEST_W_MAIN_NB], vbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  /* button: normal */

  hbox = uiCreateHorizBox ();
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  uiwidgetp = uiCreateButton (uitest->callbacks [UITEST_CB_B],
      "button", NULL);
  uiBoxPackStart (hbox, uiwidgetp);
  uitest->wcont [UITEST_W_B] = uiwidgetp;

  uiwidgetp = uiCreateLabel ("");
  uiWidgetSetMarginStart (uiwidgetp, 4);
  uiBoxPackStart (hbox, uiwidgetp);
  uitest->wcont [UITEST_W_B_MSG] = uiwidgetp;

  uiwcontFree (hbox);

  /* button: image */

  hbox = uiCreateHorizBox ();
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  uiwidgetp = uiCreateButton (uitest->callbacks [UITEST_CB_B_IMG_A],
      NULL, "button_pause");
  uiBoxPackStart (hbox, uiwidgetp);
  uitest->wcont [UITEST_W_B_IMG_A] = uiwidgetp;

  uiwidgetp = uiCreateLabel ("");
  uiWidgetSetMarginStart (uiwidgetp, 4);
  uiBoxPackStart (hbox, uiwidgetp);
  uitest->wcont [UITEST_W_B_IMG_A_MSG] = uiwidgetp;

  uiwcontFree (hbox);

  /* button: image, text */

  hbox = uiCreateHorizBox ();
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  uiwidgetp = uiCreateButton (uitest->callbacks [UITEST_CB_B_IMG_B],
      "img-tooltip", "button_play");
  uiBoxPackStart (hbox, uiwidgetp);
  uitest->wcont [UITEST_W_B_IMG_B] = uiwidgetp;

  uiwidgetp = uiCreateLabel ("");
  uiWidgetSetMarginStart (uiwidgetp, 4);
  uiBoxPackStart (hbox, uiwidgetp);
  uitest->wcont [UITEST_W_B_IMG_B_MSG] = uiwidgetp;

  uiwcontFree (hbox);

  /* toggle button: normal */

  hbox = uiCreateHorizBox ();
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  uiwidgetp = uiCreateToggleButton ("toggle button",
      NULL, NULL, NULL, 0);
  uiBoxPackStart (hbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwcontFree (hbox);

  /* toggle button: tooltip */

  hbox = uiCreateHorizBox ();
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  uiwidgetp = uiCreateToggleButton ("toggle tooltip", NULL, "tool-tip", NULL, 0);
  uiBoxPackStart (hbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwcontFree (hbox);

  /* toggle button: with image */

  hbox = uiCreateHorizBox ();
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  pathbldMakePath (imgbuff, sizeof (imgbuff), "led_off", ".svg",
      PATHBLD_MP_DIR_IMG);
  uitest->images [UITEST_I_LED_OFF] = uiImageFromFile (imgbuff);
  uiWidgetMakePersistent (uitest->images [UITEST_I_LED_OFF]);

  uiwidgetp = uiCreateToggleButton ("toggle image", NULL, "tool-tip",
      uitest->images [UITEST_I_LED_OFF], 0);
  uiBoxPackStart (hbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwcontFree (hbox);

  /* font button */

  hbox = uiCreateHorizBox ();
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  uiwidgetp = uiCreateFontButton ("Sans Bold 10");
  uiBoxPackStart (hbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwcontFree (hbox);

  /* color button */

  hbox = uiCreateHorizBox ();
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  uiwidgetp = uiCreateColorButton ("#aacc00");
  uiBoxPackStart (hbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwcontFree (hbox);

  /* radio button */

  hbox = uiCreateHorizBox ();
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  uiwidgetp = uiCreateRadioButton (NULL, "radio a", UI_TOGGLE_BUTTON_ON);
  uiBoxPackStart (hbox, uiwidgetp);
  twidgetp = uiwidgetp;

  uiwcontFree (hbox);

  hbox = uiCreateHorizBox ();
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  uiwidgetp = uiCreateRadioButton (twidgetp, "radio b", UI_TOGGLE_BUTTON_OFF);
  uiBoxPackStart (hbox, uiwidgetp);
  uiwcontFree (uiwidgetp);
  uiwcontFree (twidgetp);

  uiwcontFree (hbox);

  /* check button */

  hbox = uiCreateHorizBox ();
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  uiwidgetp = uiCreateCheckButton ("check button", UI_TOGGLE_BUTTON_OFF);
  uiBoxPackStart (hbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwcontFree (hbox);

  /* switch */

  uiwcontFree (vbox);

  /* change indicator */

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);

  uiwidgetp = uiCreateLabel ("Change Indicator");
  uiNotebookAppendPage (uitest->wcont [UITEST_W_MAIN_NB], vbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwcontFree (vbox);

  /* drop-down */

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);

  uiwidgetp = uiCreateLabel ("Drop-Down");
  uiNotebookAppendPage (uitest->wcont [UITEST_W_MAIN_NB], vbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwcontFree (vbox);

  /* entry */

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);

  uiwidgetp = uiCreateLabel ("Entry");
  uiNotebookAppendPage (uitest->wcont [UITEST_W_MAIN_NB], vbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwcontFree (vbox);

  /* image */

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);

  uiwidgetp = uiCreateLabel ("Image");
  uiNotebookAppendPage (uitest->wcont [UITEST_W_MAIN_NB], vbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwcontFree (vbox);

  /* labels */

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);

  uiwidgetp = uiCreateLabel ("Label");
  uiNotebookAppendPage (uitest->wcont [UITEST_W_MAIN_NB], vbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  /* label: pack start */

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

  /* label: pack end */

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

  /* label: pack start expand */

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

  /* label: pack start expand / align end */

  hbox = uiCreateHorizBox ();
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  uiwidgetp = uiCreateLabel ("s-e-align-end");
  uiBoxPackStartExpand (hbox, uiwidgetp);
  uiWidgetAlignHorizEnd (uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiCreateLabel ("a");
  uiBoxPackStartExpand (hbox, uiwidgetp);
  uiWidgetAlignHorizEnd (uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiCreateLabel ("b");
  uiBoxPackStartExpand (hbox, uiwidgetp);
  uiWidgetAlignHorizEnd (uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwcontFree (hbox);

  /* label: pack start expand / align center */

  hbox = uiCreateHorizBox ();
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  uiwidgetp = uiCreateLabel ("s-e-align-center");
  uiBoxPackStartExpand (hbox, uiwidgetp);
  uiWidgetAlignHorizCenter (uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiCreateLabel ("a");
  uiBoxPackStartExpand (hbox, uiwidgetp);
  uiWidgetAlignHorizCenter (uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiCreateLabel ("b");
  uiBoxPackStartExpand (hbox, uiwidgetp);
  uiWidgetAlignHorizCenter (uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwcontFree (hbox);

  /* label: pack start expand / ellipsize */

  hbox = uiCreateHorizBox ();
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  uiwidgetp = uiCreateLabel ("s-e-ellipsize-max ellipsize ellipsize ellipsize ellipsize ellipsize ellipsize ellipsize ellipsize ellipsize");
  uiBoxPackStartExpand (hbox, uiwidgetp);
  uiLabelEllipsizeOn (uiwidgetp);
  uiLabelSetMaxWidth (uiwidgetp, 20);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiCreateLabel ("a");
  uiBoxPackStartExpand (hbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiCreateLabel ("b");
  uiBoxPackStartExpand (hbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwcontFree (hbox);

  /* label: pack start expand / ellipsize */

  hbox = uiCreateHorizBox ();
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  uiwidgetp = uiCreateLabel ("s-e-ellipsize-max-fill ellipsize ellipsize ellipsize ellipsize ellipsize ellipsize ellipsize ellipsize ellipsize");
  uiBoxPackStartExpand (hbox, uiwidgetp);
  uiWidgetAlignHorizFill (uiwidgetp);
  uiLabelEllipsizeOn (uiwidgetp);
  uiLabelSetMaxWidth (uiwidgetp, 20);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiCreateLabel ("a");
  uiBoxPackStartExpand (hbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiCreateLabel ("b");
  uiBoxPackStartExpand (hbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwcontFree (hbox);

  /* label: pack end expand */

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

  /* label: pack start / accent */

  hbox = uiCreateHorizBox ();
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  uiwidgetp = uiCreateLabel ("accent");
  uiBoxPackStart (hbox, uiwidgetp);
  uiWidgetSetClass (uiwidgetp, ACCENT_CLASS);
  uiwcontFree (uiwidgetp);

  uiwcontFree (hbox);

  /* label: pack start / error */

  hbox = uiCreateHorizBox ();
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  uiwidgetp = uiCreateLabel ("error");
  uiBoxPackStart (hbox, uiwidgetp);
  uiWidgetSetClass (uiwidgetp, ERROR_CLASS);
  uiwcontFree (uiwidgetp);

  uiwcontFree (hbox);

  /* label: pack start / dark-accent */

  hbox = uiCreateHorizBox ();
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  uiwidgetp = uiCreateLabel ("dark-accent");
  uiBoxPackStart (hbox, uiwidgetp);
  uiWidgetSetClass (uiwidgetp, DARKACCENT_CLASS);
  uiwcontFree (uiwidgetp);

  uiwcontFree (hbox);

  uiwcontFree (vbox);

  /* link */

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);

  uiwidgetp = uiCreateLabel ("Link");
  uiNotebookAppendPage (uitest->wcont [UITEST_W_MAIN_NB], vbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwcontFree (vbox);

  /* notebook */

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);

  uiwidgetp = uiCreateLabel ("Notebook");
  uiNotebookAppendPage (uitest->wcont [UITEST_W_MAIN_NB], vbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwcontFree (vbox);

  /* paned window */

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);

  uiwidgetp = uiCreateLabel ("Paned Window");
  uiNotebookAppendPage (uitest->wcont [UITEST_W_MAIN_NB], vbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwcontFree (vbox);

  /* progress bar */

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);

  uiwidgetp = uiCreateLabel ("Progress Bar");
  uiNotebookAppendPage (uitest->wcont [UITEST_W_MAIN_NB], vbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwcontFree (vbox);

  /* scale */

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);

  uiwidgetp = uiCreateLabel ("Scale");
  uiNotebookAppendPage (uitest->wcont [UITEST_W_MAIN_NB], vbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwcontFree (vbox);

  /* separator */

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);

  uiwidgetp = uiCreateLabel ("Separator");
  uiNotebookAppendPage (uitest->wcont [UITEST_W_MAIN_NB], vbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwcontFree (vbox);

  /* size group */

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);

  uiwidgetp = uiCreateLabel ("Size Group");
  uiNotebookAppendPage (uitest->wcont [UITEST_W_MAIN_NB], vbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwcontFree (vbox);

  /* spinboxes */

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);

  uiwidgetp = uiCreateLabel ("Spin Box");
  uiNotebookAppendPage (uitest->wcont [UITEST_W_MAIN_NB], vbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwcontFree (vbox);

  /* text box */

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);

  uiwidgetp = uiCreateLabel ("Text Box");
  uiNotebookAppendPage (uitest->wcont [UITEST_W_MAIN_NB], vbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwcontFree (vbox);

  /* tree view */

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);

  uiwidgetp = uiCreateLabel ("Tree View");
  uiNotebookAppendPage (uitest->wcont [UITEST_W_MAIN_NB], vbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

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
uitestCounterDisp (uitest_t *uitest, uiwcont_t *uiwidgetp)
{
  char      tmp [40];

  snprintf (tmp, sizeof (tmp), "%ld", uitest->counter);
  uiLabelSetText (uiwidgetp, tmp);
  uitest->counter += 1;
}

static bool
uitestCBButton (void *udata)
{
  uitest_t  *uitest = udata;

  uitestCounterDisp (uitest, uitest->wcont [UITEST_W_B_MSG]);
  return UICB_CONT;
}

static bool
uitestCBButtonImgA (void *udata)
{
  uitest_t  *uitest = udata;

  uitestCounterDisp (uitest, uitest->wcont [UITEST_W_B_IMG_A_MSG]);
  return UICB_CONT;
}

static bool
uitestCBButtonImgB (void *udata)
{
  uitest_t  *uitest = udata;

  uitestCounterDisp (uitest, uitest->wcont [UITEST_W_B_IMG_B_MSG]);
  return UICB_CONT;
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

  for (int i = 0; i < UITEST_I_MAX; ++i) {
    uiwcontFree (uitest->images [i]);
  }

  bdjoptCleanup ();
  localeCleanup ();
}

