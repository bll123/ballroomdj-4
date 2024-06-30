/*
 * Copyright 2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
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
#include "nlist.h"
#include "pathbld.h"
#include "tmutil.h"
#include "ui.h"
#include "uidd.h"
#include "uiutils.h"
#include "uivirtlist.h"
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
  UITEST_W_SW,
  UITEST_W_NB_V,
  UITEST_W_NB_H,
  UITEST_W_NB_HI,
  UITEST_W_MAX,
};

enum {
  UITEST_CB_CLOSE,
  UITEST_CB_B,
  UITEST_CB_B_IMG_A,
  UITEST_CB_B_IMG_B,
  UITEST_CB_DD_STR,
  UITEST_CB_DD_NUM,
  UITEST_CB_MAX,
};

enum {
  UITEST_TB_I_LED_OFF,
  UITEST_TB_I_LED_ON,
  UITEST_NB1_I_LED_OFF,
  UITEST_NB1_I_LED_ON,
  UITEST_NB2_I_LED_OFF,
  UITEST_NB2_I_LED_ON,
  UITEST_I_MAX,
};

enum {
  UITEST_DD_A,
  UITEST_DD_B,
  UITEST_DD_C,
  UITEST_DD_D,
  UITEST_DD_E,
  UITEST_DD_MAX,
};

enum {
  UITEST_VL_A,
  UITEST_VL_B,
  UITEST_VL_C,
  UITEST_VL_MAX,
};

enum {
  UITEST_VL_COL_MAX = 6,
  UITEST_VL_DISPROWS = 10,
  UITEST_VL_MAXROWS = 100,
};

typedef struct {
  uiwcont_t     *wcont [UITEST_W_MAX];
  uivirtlist_t  *vl [UITEST_VL_MAX];
  callback_t    *callbacks [UITEST_CB_MAX];
  uiwcont_t     *images [UITEST_I_MAX];
  callback_t    *chgcb;
  ilist_t       *lista;
  ilist_t       *listb;
  long          counter;
  bool          stop : 1;
} uitest_t;

typedef struct {
  int           idx;
  uitest_t      *uitest;
} uitestvl_t;

static const int vlcols [UITEST_VL_MAX] = {
  3, 2, 2,
};
static const int vlmaxrows [UITEST_VL_MAX] = {
  5, 40, 25,
};
static const char * vllabs [UITEST_VL_MAX][UITEST_VL_COL_MAX] = {
  { "One", "Two", "☆", NULL, NULL, NULL },
  { "A", "B", NULL, NULL, NULL, NULL },
  { "Three", "Four", NULL, NULL, NULL, NULL },
};
static uitestvl_t vlcb [UITEST_VL_MAX];

static void uitestMainLoop (uitest_t *uitest);
static void uitestBuildUI (uitest_t *uitest);
static void uitestUIButtons (uitest_t *uitest);
static void uitestUIChgInd (uitest_t *uitest);
static void uitestUIDropdown (uitest_t *uitest);
static void uitestUIEntry (uitest_t *uitest);
static void uitestUIImage (uitest_t *uitest);
static void uitestUILabels (uitest_t *uitest);
static void uitestUILink (uitest_t *uitest);
static void uitestUINotebook (uitest_t *uitest);
static void uitestUIPanedWin (uitest_t *uitest);
static void uitestUIMisc (uitest_t *uitest);
static void uitestUISizeGroup (uitest_t *uitest);
static void uitestUISpinboxes (uitest_t *uitest);
static void uitestUITextBox (uitest_t *uitest);
static void uitestUIVirtList (uitest_t *uitest);

static bool uitestCloseWin (void *udata);
static void uitestCounterDisp (uitest_t *uitest, uiwcont_t *uiwidgetp);
static bool uitestCBButton (void *udata);
static bool uitestCBButtonImgA (void *udata);
static bool uitestCBButtonImgB (void *udata);
static void uitestCleanup (uitest_t *uitest);

static void uitestVLFillCB (void *udata, uivirtlist_t *vl, int32_t rownum);
static void uitestVLSelectCB (void *udata, uivirtlist_t *vl, int32_t rownum, int colidx);

static int32_t uitestDDStr (void *udata, const char *str);
static bool uitestDDNum (void *udata, int32_t val);

int
main (int argc, char *argv[])
{
  uitest_t    uitest;
  const char  *targ;
  bdj4arg_t   *bdj4arg;
  bool        isbdj4 = false;
  int         c;
  int         option_index;

  static struct option bdj_options [] = {
    { "bdj4",         no_argument,      NULL,   'B' },
    { "uitest",       no_argument,      NULL,   0 },
    { "debugself",    no_argument,      NULL,   0 },
    { "verbose",      no_argument,      NULL,   0, },
    { "quiet",        no_argument,      NULL,   0, },
    { "nodetach",     no_argument,      NULL,   0, },
    { "origcwd",      required_argument,  NULL,   0 },
    { "scale",        required_argument,NULL,   0 },
    { "theme",        required_argument,NULL,   0 },
  };

#if BDJ4_MEM_DEBUG
  mdebugInit ("uitest");
#endif

  bdj4arg = bdj4argInit (argc, argv);

  while ((c = getopt_long_only (argc, bdj4argGetArgv (bdj4arg),
      "BCp:d:mnNRs", bdj_options, &option_index)) != -1) {
    switch (c) {
      case 'B': {
        isbdj4 = true;
        break;
      }
      default: {
        break;
      }
    }
  }

  if (! isbdj4) {
    fprintf (stderr, "not started with launcher\n");
    exit (1);
  }

  for (int i = 0; i < UITEST_W_MAX; ++i) {
    uitest.wcont [i] = NULL;
  }
  for (int i = 0; i < UITEST_CB_MAX; ++i) {
    uitest.callbacks [i] = NULL;
  }
  for (int i = 0; i < UITEST_I_MAX; ++i) {
    uitest.images [i] = NULL;
  }
  for (int i = 0; i < UITEST_VL_MAX; ++i) {
    uitest.vl [i] = NULL;
    vlcb [i].idx = i;
    vlcb [i].uitest = &uitest;
  }
  uitest.stop = false;
  uitest.counter = 1;
  uitest.lista = NULL;
  uitest.listb = NULL;
  uitest.chgcb = NULL;

  uitest.lista = ilistAlloc ("dd-a", LIST_ORDERED);
  uitest.listb = ilistAlloc ("dd-b", LIST_ORDERED);
  for (int i = 0; i < 3; ++i) {
    char    tbuff [200];

    snprintf (tbuff, sizeof (tbuff), "Item %d", i);
    ilistSetNum (uitest.listb, i, DD_LIST_KEY_NUM, 3 - i);
    ilistSetStr (uitest.listb, i, DD_LIST_DISP, tbuff);
  }

  for (int i = 0; i < 20; ++i) {
    char    tbuff [200];
    char    dbuff [200];

    if (i == 19) {
      snprintf (tbuff, sizeof (tbuff), "Long Item Number %d", i);
    } else {
      snprintf (tbuff, sizeof (tbuff), "Item %d", i);
    }
    snprintf (dbuff, sizeof (dbuff), "data-%d", i);
    ilistSetStr (uitest.lista, i, DD_LIST_KEY_STR, dbuff);
    ilistSetNum (uitest.lista, i, DD_LIST_KEY_NUM, 20 - i);
    ilistSetStr (uitest.lista, i, DD_LIST_DISP, tbuff);
  }

  targ = bdj4argGet (bdj4arg, 0, argv [0]);
  sysvarsInit (targ);
  localeInit ();
  bdjoptInit ();

  uiUIInitialize (sysvarsGetNum (SVL_LOCALE_DIR));
  uiSetUICSS (uiutilsGetCurrentFont (),
      uiutilsGetListingFont (),
      bdjoptGetStr (OPT_P_UI_ACCENT_COL),
      bdjoptGetStr (OPT_P_UI_ERROR_COL),
      bdjoptGetStr (OPT_P_UI_ROWSEL_COL));

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
  uiWindowSetDefaultSize (uitest->wcont [UITEST_W_WINDOW], -1, 400);

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);
  uiWindowPackInWindow (uitest->wcont [UITEST_W_WINDOW], vbox);

  uiutilsAddProfileColorDisplay (vbox, &accent);
  hbox = accent.hbox;
  uiwcontFree (accent.cbox);

  uitest->wcont [UITEST_W_MENUBAR] = uiCreateMenubar ();
  uiBoxPackStartExpand (hbox, uitest->wcont [UITEST_W_MENUBAR]);

  uiwidgetp = uiCreateLabel ("");
  uiWidgetAddClass (uiwidgetp, ACCENT_CLASS);
  uiBoxPackEnd (hbox, uiwidgetp);
  uitest->wcont [UITEST_W_STATUS_MSG] = uiwidgetp;
  uiwcontFree (hbox);

  /* main notebook */

  uitest->wcont [UITEST_W_MAIN_NB] = uiCreateNotebook ();
  uiWidgetAddClass (uitest->wcont [UITEST_W_MAIN_NB], LEFT_NB_CLASS);
  uiNotebookTabPositionLeft (uitest->wcont [UITEST_W_MAIN_NB]);
  uiBoxPackStartExpand (vbox, uitest->wcont [UITEST_W_MAIN_NB]);

  uitestUIButtons (uitest);
  uitestUIChgInd (uitest);
  uitestUIDropdown (uitest);
  uitestUIEntry (uitest);
  uitestUIImage (uitest);
  uitestUILabels (uitest);
  uitestUILink (uitest);
  uitestUINotebook (uitest);
  uitestUIPanedWin (uitest);
  uitestUIMisc (uitest);
  uitestUISizeGroup (uitest);
  uitestUISpinboxes (uitest);
  uitestUITextBox (uitest);
  uitestUIVirtList (uitest);

  uiWidgetShowAll (uitest->wcont [UITEST_W_WINDOW]);
  uiwcontFree (vbox);
}

void
uitestUIButtons (uitest_t *uitest)
{
  uiwcont_t   *vbox;
  uiwcont_t   *hbox;
  uiwcont_t   *uiwidgetp;
  uiwcont_t   *twidgetp;
  char        imgbuff [MAXPATHLEN];

  /* buttons */

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);

  uiwidgetp = uiCreateLabel ("Buttons");
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
  uitest->images [UITEST_TB_I_LED_OFF] = uiImageFromFile (imgbuff);
  uiWidgetSetMarginStart (uitest->images [UITEST_TB_I_LED_OFF], 1);
  uiWidgetMakePersistent (uitest->images [UITEST_TB_I_LED_OFF]);

  pathbldMakePath (imgbuff, sizeof (imgbuff), "led_on", ".svg",
      PATHBLD_MP_DIR_IMG);
  uitest->images [UITEST_TB_I_LED_ON] = uiImageFromFile (imgbuff);
  uiWidgetSetMarginStart (uitest->images [UITEST_TB_I_LED_ON], 1);
  uiWidgetMakePersistent (uitest->images [UITEST_TB_I_LED_ON]);

  uiwidgetp = uiCreateToggleButton ("toggle image", NULL, "tool-tip",
      uitest->images [UITEST_TB_I_LED_OFF], 0);
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

  hbox = uiCreateHorizBox ();
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  uitest->wcont [UITEST_W_SW] = uiCreateSwitch (1);
  uiBoxPackStart (hbox, uitest->wcont [UITEST_W_SW]);

  uiwcontFree (hbox);

  uiwcontFree (vbox);
}

void
uitestUIChgInd (uitest_t *uitest)
{
  uiwcont_t   *vbox;
  uiwcont_t   *uiwidgetp;

  /* change indicator */

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);

  uiwidgetp = uiCreateLabel ("Change Indicator");
  uiNotebookAppendPage (uitest->wcont [UITEST_W_MAIN_NB], vbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwcontFree (vbox);
}

void
uitestUIDropdown (uitest_t *uitest)
{
  uiwcont_t   *vbox;
  uiwcont_t   *uiwidgetp;
  uidd_t      *uidd [UITEST_DD_MAX];

  /* drop-downs */

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);

  uiwidgetp = uiCreateLabel ("Drop-Down");
  uiNotebookAppendPage (uitest->wcont [UITEST_W_MAIN_NB], vbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uitest->callbacks [UITEST_CB_DD_STR] = callbackInitS (uitestDDStr, uitest);
  uitest->callbacks [UITEST_CB_DD_NUM] = callbackInitI (uitestDDNum, uitest);

  uidd [UITEST_DD_A] = uiddCreate ("dd-a",
      uitest->wcont [UITEST_W_WINDOW], vbox, DD_PACK_START,
      uitest->lista, DD_LIST_TYPE_STR,
      "Title A", DD_KEEP_TITLE,
      uitest->callbacks [UITEST_CB_DD_STR]);
  uiddSetSelection (uidd [UITEST_DD_A], 2);

  uidd [UITEST_DD_B] = uiddCreate ("dd-b",
      uitest->wcont [UITEST_W_WINDOW], vbox, DD_PACK_START,
      uitest->lista, DD_LIST_TYPE_STR,
      "Item 6", DD_REPLACE_TITLE,
      uitest->callbacks [UITEST_CB_DD_STR]);
  uiddSetSelection (uidd [UITEST_DD_B], 5);

  uidd [UITEST_DD_C] = uiddCreate ("dd-c",
      uitest->wcont [UITEST_W_WINDOW], vbox, DD_PACK_START,
      uitest->lista, DD_LIST_TYPE_NUM,
      "Title C", DD_KEEP_TITLE,
      uitest->callbacks [UITEST_CB_DD_NUM]);
  uiddSetSelection (uidd [UITEST_DD_C], 4);

  uidd [UITEST_DD_D] = uiddCreate ("dd-d",
      uitest->wcont [UITEST_W_WINDOW], vbox, DD_PACK_START,
      uitest->lista, DD_LIST_TYPE_NUM,
      "Item 4", DD_REPLACE_TITLE,
      uitest->callbacks [UITEST_CB_DD_NUM]);
  uiddSetSelection (uidd [UITEST_DD_D], 3);

  uidd [UITEST_DD_E] = uiddCreate ("dd-e",
      uitest->wcont [UITEST_W_WINDOW], vbox, DD_PACK_START,
      uitest->listb, DD_LIST_TYPE_NUM,
      "Item 1", DD_REPLACE_TITLE,
      uitest->callbacks [UITEST_CB_DD_NUM]);
  uiddSetSelection (uidd [UITEST_DD_E], 1);

  uiwcontFree (vbox);
}

void
uitestUIEntry (uitest_t *uitest)
{
  uiwcont_t   *vbox;
  uiwcont_t   *uiwidgetp;

  /* entry */

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);

  uiwidgetp = uiCreateLabel ("Entry");
  uiNotebookAppendPage (uitest->wcont [UITEST_W_MAIN_NB], vbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiEntryInit (10, 100);
  uiBoxPackStart (vbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiEntryInit (10, 100);
  uiBoxPackStart (vbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwcontFree (vbox);
}

void
uitestUIImage (uitest_t *uitest)
{
  uiwcont_t   *vbox;
  uiwcont_t   *uiwidgetp;

  /* image */

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);

  uiwidgetp = uiCreateLabel ("Image");
  uiNotebookAppendPage (uitest->wcont [UITEST_W_MAIN_NB], vbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwcontFree (vbox);
}

void
uitestUILabels (uitest_t *uitest)
{
  uiwcont_t   *vbox;
  uiwcont_t   *hbox;
  uiwcont_t   *uiwidgetp;

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
  uiWidgetAddClass (uiwidgetp, ACCENT_CLASS);
  uiwcontFree (uiwidgetp);

  uiwcontFree (hbox);

  /* label: pack start / error */

  hbox = uiCreateHorizBox ();
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  uiwidgetp = uiCreateLabel ("error");
  uiBoxPackStart (hbox, uiwidgetp);
  uiWidgetAddClass (uiwidgetp, ERROR_CLASS);
  uiwcontFree (uiwidgetp);

  uiwcontFree (hbox);

  /* label: pack start / dark-accent */

  hbox = uiCreateHorizBox ();
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (vbox, hbox);

  uiwidgetp = uiCreateLabel ("dark-accent");
  uiBoxPackStart (hbox, uiwidgetp);
  uiWidgetAddClass (uiwidgetp, DARKACCENT_CLASS);
  uiwcontFree (uiwidgetp);

  uiwcontFree (hbox);

  uiwcontFree (vbox);
}

void
uitestUILink (uitest_t *uitest)
{
  uiwcont_t   *vbox;
  uiwcont_t   *uiwidgetp;

  /* link */

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);

  uiwidgetp = uiCreateLabel ("Link");
  uiNotebookAppendPage (uitest->wcont [UITEST_W_MAIN_NB], vbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwcontFree (vbox);
}

void
uitestUINotebook (uitest_t *uitest)
{
  uiwcont_t   *vbox;
  uiwcont_t   *vboxb;
  uiwcont_t   *hbox;
  uiwcont_t   *uiwidgetp;
  char        imgbuff [MAXPATHLEN];

  /* notebook */

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);

  uiwidgetp = uiCreateLabel ("Notebook");
  uiNotebookAppendPage (uitest->wcont [UITEST_W_MAIN_NB], vbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uitest->wcont [UITEST_W_NB_V] = uiCreateNotebook ();
  uiWidgetAddClass (uitest->wcont [UITEST_W_NB_V], LEFT_NB_CLASS);
  uiNotebookTabPositionLeft (uitest->wcont [UITEST_W_NB_V]);
  uiBoxPackStart (vbox, uitest->wcont [UITEST_W_NB_V]);

  uiwcontFree (vbox);

  /* horizontal */

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);
  uiwidgetp = uiCreateLabel ("Horiz");
  uiNotebookAppendPage (uitest->wcont [UITEST_W_NB_V], vbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uitest->wcont [UITEST_W_NB_H] = uiCreateNotebook ();
  uiBoxPackStart (vbox, uitest->wcont [UITEST_W_NB_H]);
  uiwcontFree (vbox);

  vboxb = uiCreateVertBox ();
  uiWidgetSetAllMargins (vboxb, 4);
  uiwidgetp = uiCreateLabel ("First");
  uiNotebookAppendPage (uitest->wcont [UITEST_W_NB_H], vboxb, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiCreateLabel ("First");
  uiBoxPackStart (vboxb, uiwidgetp);
  uiwcontFree (uiwidgetp);
  uiwcontFree (vboxb);

  vboxb = uiCreateVertBox ();
  uiWidgetSetAllMargins (vboxb, 4);
  uiwidgetp = uiCreateLabel ("Second");
  uiNotebookAppendPage (uitest->wcont [UITEST_W_NB_H], vboxb, uiwidgetp);
  uiwcontFree (uiwidgetp);
  uiwidgetp = uiCreateLabel ("Second");
  uiBoxPackStart (vboxb, uiwidgetp);
  uiwcontFree (uiwidgetp);
  uiwcontFree (vboxb);

  vboxb = uiCreateVertBox ();
  uiWidgetSetAllMargins (vboxb, 4);
  uiwidgetp = uiCreateLabel ("Third");
  uiNotebookAppendPage (uitest->wcont [UITEST_W_NB_H], vboxb, uiwidgetp);
  uiwcontFree (uiwidgetp);
  uiwidgetp = uiCreateLabel ("Third");
  uiBoxPackStart (vboxb, uiwidgetp);
  uiwcontFree (uiwidgetp);
  uiwcontFree (vboxb);

  /* horizontal w/images */

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);
  uiwidgetp = uiCreateLabel ("Horiz Image");
  uiNotebookAppendPage (uitest->wcont [UITEST_W_NB_V], vbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uitest->wcont [UITEST_W_NB_HI] = uiCreateNotebook ();
  uiBoxPackStart (vbox, uitest->wcont [UITEST_W_NB_HI]);
  uiwcontFree (vbox);

  pathbldMakePath (imgbuff, sizeof (imgbuff), "led_off", ".svg",
      PATHBLD_MP_DIR_IMG);
  uitest->images [UITEST_NB1_I_LED_OFF] = uiImageFromFile (imgbuff);
  uiWidgetSetMarginStart (uitest->images [UITEST_NB1_I_LED_OFF], 1);
  uiWidgetMakePersistent (uitest->images [UITEST_NB1_I_LED_OFF]);

  pathbldMakePath (imgbuff, sizeof (imgbuff), "led_on", ".svg",
      PATHBLD_MP_DIR_IMG);
  uitest->images [UITEST_NB1_I_LED_ON] = uiImageFromFile (imgbuff);
  uiWidgetSetMarginStart (uitest->images [UITEST_NB1_I_LED_ON], 1);
  uiWidgetMakePersistent (uitest->images [UITEST_NB1_I_LED_ON]);

  vboxb = uiCreateVertBox ();
  uiWidgetSetAllMargins (vboxb, 4);
  hbox = uiCreateHorizBox ();
  uiwidgetp = uiCreateLabel ("One");
  uiBoxPackStart (hbox, uiwidgetp);
  uiwcontFree (uiwidgetp);
  uiBoxPackStart (hbox, uitest->images [UITEST_NB1_I_LED_OFF]);
  uiNotebookAppendPage (uitest->wcont [UITEST_W_NB_HI], vboxb, hbox);
  uiWidgetShowAll (hbox);
  uiwcontFree (hbox);

  uiwidgetp = uiCreateLabel ("One");
  uiBoxPackStart (vboxb, uiwidgetp);
  uiwcontFree (uiwidgetp);
  uiwcontFree (vboxb);

  pathbldMakePath (imgbuff, sizeof (imgbuff), "led_off", ".svg",
      PATHBLD_MP_DIR_IMG);
  uitest->images [UITEST_NB2_I_LED_OFF] = uiImageFromFile (imgbuff);
  uiWidgetSetMarginStart (uitest->images [UITEST_NB2_I_LED_OFF], 1);
  uiWidgetMakePersistent (uitest->images [UITEST_NB2_I_LED_OFF]);

  pathbldMakePath (imgbuff, sizeof (imgbuff), "led_on", ".svg",
      PATHBLD_MP_DIR_IMG);
  uitest->images [UITEST_NB2_I_LED_ON] = uiImageFromFile (imgbuff);
  uiWidgetSetMarginStart (uitest->images [UITEST_NB2_I_LED_ON], 1);
  uiWidgetMakePersistent (uitest->images [UITEST_NB2_I_LED_ON]);

  vboxb = uiCreateVertBox ();
  uiWidgetSetAllMargins (vboxb, 4);
  hbox = uiCreateHorizBox ();
  uiwidgetp = uiCreateLabel ("Two");
  uiBoxPackStart (hbox, uiwidgetp);
  uiwcontFree (uiwidgetp);
  uiBoxPackStart (hbox, uitest->images [UITEST_NB2_I_LED_OFF]);
  uiNotebookAppendPage (uitest->wcont [UITEST_W_NB_HI], vboxb, hbox);
  uiWidgetShowAll (hbox);
  uiwcontFree (hbox);

  uiwidgetp = uiCreateLabel ("Two");
  uiBoxPackStart (vboxb, uiwidgetp);
  uiwcontFree (uiwidgetp);
  uiwcontFree (vboxb);

  vboxb = uiCreateVertBox ();
  uiWidgetSetAllMargins (vboxb, 4);
  uiwidgetp = uiCreateLabel ("Three");
  uiNotebookAppendPage (uitest->wcont [UITEST_W_NB_HI], vboxb, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiCreateLabel ("Three");
  uiBoxPackStart (vboxb, uiwidgetp);
  uiwcontFree (uiwidgetp);
  uiwcontFree (vboxb);

  /* action widget */

  // uiNotebookSetActionWidget (plui->wcont [PLUI_W_NOTEBOOK], uiwidgetp);
}

void
uitestUIPanedWin (uitest_t *uitest)
{
  uiwcont_t   *vbox;
  uiwcont_t   *uiwidgetp;

  /* paned window */

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);

  uiwidgetp = uiCreateLabel ("Paned Window");
  uiNotebookAppendPage (uitest->wcont [UITEST_W_MAIN_NB], vbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwcontFree (vbox);
}

void
uitestUIMisc (uitest_t *uitest)
{
  uiwcont_t   *vbox;
  uiwcont_t   *uiwidgetp;

  /* progress bar */

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);

  uiwidgetp = uiCreateLabel ("Misc");
  uiNotebookAppendPage (uitest->wcont [UITEST_W_MAIN_NB], vbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwcontFree (vbox);
}

void
uitestUISizeGroup (uitest_t *uitest)
{
  uiwcont_t   *vbox;
  uiwcont_t   *uiwidgetp;

  /* size group */

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);

  uiwidgetp = uiCreateLabel ("Size Group");
  uiNotebookAppendPage (uitest->wcont [UITEST_W_MAIN_NB], vbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwcontFree (vbox);
}

void
uitestUISpinboxes (uitest_t *uitest)
{
  uiwcont_t   *vbox;
  uiwcont_t   *uiwidgetp;

  /* spinboxes */

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);

  uiwidgetp = uiCreateLabel ("Spin Box");
  uiNotebookAppendPage (uitest->wcont [UITEST_W_MAIN_NB], vbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwcontFree (vbox);
}

void
uitestUITextBox (uitest_t *uitest)
{
  uiwcont_t   *vbox;
  uiwcont_t   *uiwidgetp;

  /* text box */

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);

  uiwidgetp = uiCreateLabel ("Text Box");
  uiNotebookAppendPage (uitest->wcont [UITEST_W_MAIN_NB], vbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwcontFree (vbox);
}

void
uitestUIVirtList (uitest_t *uitest)
{
  uiwcont_t   *hbox;
  uiwcont_t   *uiwidgetp;

  /* tree view */

  hbox = uiCreateHorizBox ();
  uiWidgetSetAllMargins (hbox, 4);
//  uiWidgetExpandHoriz (hbox);
//  uiWidgetExpandVert (hbox);

  uiwidgetp = uiCreateLabel ("Virtual List");
  uiNotebookAppendPage (uitest->wcont [UITEST_W_MAIN_NB], hbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  for (int i = 0; i < UITEST_VL_MAX; ++i) {
    uitest->vl [i] = uivlCreate ("uitest", hbox, UITEST_VL_DISPROWS,
        VL_NO_WIDTH, VL_ENABLE_KEYS);
    uivlSetUseListingFont (uitest->vl [i]);
    uivlSetAllowMultiple (uitest->vl [i]);
    uivlSetNumColumns (uitest->vl [i], vlcols [i]);
    uivlSetNumRows (uitest->vl [i], vlmaxrows [i]);
    for (int j = 0; j < vlcols [i]; ++j) {
      uivlMakeColumn (uitest->vl [i], "label", j, VL_TYPE_LABEL);
      if (i != 1 && j == 0) {
        uivlSetColumnEllipsizeOn (uitest->vl [i], j);
      }
      if (i == 1 && j == 0) {
        uivlSetColumnGrow (uitest->vl [i], j, VL_COL_WIDTH_GROW_ONLY);
      }
    }
    if (i == 0) {
      uivlSetColumnClass (uitest->vl [i], 2, "bdj-list-fav");
    }
    for (int j = 0; j < vlcols [i]; ++j) {
      uivlSetColumnHeading (uitest->vl [i], j, vllabs [i][j]);
    }
    uivlSetRowFillCallback (uitest->vl [i], uitestVLFillCB, &vlcb [i]);
    uivlSetSelectionCallback (uitest->vl [i], uitestVLSelectCB, &vlcb [i]);
    uivlSetDoubleClickCallback (uitest->vl [i], uitestVLSelectCB, &vlcb [i]);
    uivlDisplay (uitest->vl [i]);
  }

  uiwcontFree (hbox);
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

  for (int i = 0; i < UITEST_VL_MAX; ++i) {
    uivlFree (uitest->vl [i]);
  }

  for (int i = 0; i < UITEST_W_MAX; ++i) {
    uiwcontFree (uitest->wcont [i]);
  }

  for (int i = 0; i < UITEST_CB_MAX; ++i) {
    callbackFree (uitest->callbacks [i]);
  }

  for (int i = 0; i < UITEST_I_MAX; ++i) {
    uiwcontFree (uitest->images [i]);
  }

  ilistFree (uitest->lista);
  ilistFree (uitest->listb);
  callbackFree (uitest->chgcb);
  bdjoptCleanup ();
  localeCleanup ();
}

static void
uitestVLFillCB (void *udata, uivirtlist_t *vl, int32_t rownum)
{
  uitestvl_t    *uitestvl = udata;
  int           vlidx = uitestvl->idx;
  char          tbuff [40];

  for (int j = 0; j < vlcols [vlidx]; ++j) {
    if (vlidx == 0 && j == 2) {
      snprintf (tbuff, sizeof (tbuff), "★");
    } else {
      snprintf (tbuff, sizeof (tbuff), "%" PRIu32 " / %d", rownum, j);
      if (vlidx != 1 && rownum % 5 == 0 && j == 0) {
        snprintf (tbuff, sizeof (tbuff), "%" PRIu32 " / %d stuff stuff stuff stuff", rownum, j);
      }
      if (vlidx == 1 && rownum == 35 && j == 0) {
        snprintf (tbuff, sizeof (tbuff), "%" PRIu32 " / %d stuff stuff stuff stuff", rownum, j);
      }
      if (rownum % 7 == 0 && j == 2) {
        snprintf (tbuff, sizeof (tbuff), "%" PRIu32, rownum);
      }
    }
    uivlSetRowColumnValue (vl, rownum, j, tbuff);
    if (rownum % 9 == 0 && j == 0) {
      uivlSetRowColumnClass (vl, rownum, j, ACCENT_CLASS);
    }
  }
}

static void
uitestVLSelectCB (void *udata, uivirtlist_t *vl, int32_t rownum, int colidx)
{
//  uitest_t  *uitest = udata;

  return;
}

static int32_t
uitestDDStr (void *udata, const char *key)
{
fprintf (stderr, "dd-str %s\n", key);
  return UICB_CONT;
}

static bool
uitestDDNum (void *udata, int32_t key)
{
fprintf (stderr, "dd-num %d\n", key);
  return UICB_CONT;
}

