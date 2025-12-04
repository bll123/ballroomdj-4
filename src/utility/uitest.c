/*
 * Copyright 2024-2025 Brad Lanam Pleasant Hill CA
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
#include "uivnb.h"
#include "uiwcont.h"
#include "sysvars.h"

enum {
  UITEST_W_WINDOW,
  UITEST_W_MENUBAR,
  UITEST_W_STATUS_MSG,
  UITEST_W_B,
  UITEST_W_B_IMG_A,
  UITEST_W_B_IMG_B,
  UITEST_W_B_MSG,
  UITEST_W_B_IMG_A_MSG,
  UITEST_W_B_IMG_B_MSG,
  UITEST_W_SW,
  UITEST_W_NB_H,
  UITEST_W_NB_HI,
  UITEST_W_NB_V,
  UITEST_W_NB_V_NEW,
  UITEST_W_CI_A,
  UITEST_W_CI_B,
  UITEST_W_CI_BUTTON,
  UITEST_W_LINK_A,
  UITEST_W_SB_TEXT,
  UITEST_W_SB_INT,
  UITEST_W_SB_DBL_A,
  UITEST_W_SB_DBL_B,
  UITEST_W_SB_DBL_DFLT,
  UITEST_W_SB_TIME_A,
  UITEST_W_SB_TIME_B,
  UITEST_W_MAX,
};

enum {
  UITEST_CB_CLOSE,
  UITEST_CB_B,
  UITEST_CB_B_IMG_A,
  UITEST_CB_B_IMG_B,
  UITEST_CB_CHG_IND,
  UITEST_CB_DD_STR,
  UITEST_CB_DD_NUM,
  UITEST_CB_LINK_A,
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
  uivnb_t       *mainvnb;
  uiwcont_t     *wcont [UITEST_W_MAX];
  uivirtlist_t  *vl [UITEST_VL_MAX];
  callback_t    *callbacks [UITEST_CB_MAX];
  uiwcont_t     *images [UITEST_I_MAX];
  callback_t    *chgcb;
  ilist_t       *lista;
  ilist_t       *listb;
  uivnb_t       *vnb;
  long          counter;
  bool          stop;
  bool          chgind;
} uitest_t;

typedef struct {
  int           idx;
  uitest_t      *uitest;
} uitestvl_t;

static const int vlcols [UITEST_VL_MAX] = {
  3, 2, 3,
};
static const int vlmaxrows [UITEST_VL_MAX] = {
  5, 40, 25,
};
static const char * vllabs [UITEST_VL_MAX][UITEST_VL_COL_MAX] = {
  { "One", "Two", "☆", NULL, NULL, NULL },
  { "A", "B", NULL, NULL, NULL, NULL },
  { "Hidden", "Three", "Four", NULL, NULL, NULL },
};
static uitestvl_t vlcb [UITEST_VL_MAX];

static void uitestMainLoop (uitest_t *uitest);
static void uitestBuildUI (uitest_t *uitest);
static void uitestUIButtons (uitest_t *uitest);
static void uitestUIToggleButtons (uitest_t *uitest);
static void uitestUIMiscButtons (uitest_t *uitest);
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
static void uitestUISpinbox (uitest_t *uitest);
static void uitestUITextBox (uitest_t *uitest);
static void uitestUIVirtList (uitest_t *uitest);

static bool uitestCloseWin (void *udata);
static void uitestCounterDisp (uitest_t *uitest, uiwcont_t *uiwidgetp);
static bool uitestCBButton (void *udata);
static bool uitestCBButtonImgA (void *udata);
static bool uitestCBButtonImgB (void *udata);
static bool uitestCBchgind (void *udata);
static bool uitestCBLink (void *udata);
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
  uisetup_t   uisetup;

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
  uitest.chgind = false;
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
  sysvarsInit (targ, SYSVARS_FLAG_ALL);
  localeInit ();
  bdjoptInit ();

  uiUIInitialize (sysvarsGetNum (SVL_LOCALE_TEXT_DIR));
  uiutilsInitSetup (&uisetup);
  uiSetUICSS (&uisetup);

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
  uiutilsaccent_t accent;

  uitest->callbacks [UITEST_CB_CLOSE] = callbackInit (
      uitestCloseWin, uitest, NULL);

  uitest->callbacks [UITEST_CB_B] = callbackInit (
      uitestCBButton, uitest, NULL);
  uitest->callbacks [UITEST_CB_B_IMG_A] = callbackInit (
      uitestCBButtonImgA, uitest, NULL);
  uitest->callbacks [UITEST_CB_B_IMG_B] = callbackInit (
      uitestCBButtonImgB, uitest, NULL);

  uitest->wcont [UITEST_W_WINDOW] = uiCreateMainWindow (
      uitest->callbacks [UITEST_CB_CLOSE], "uitest", "bdj4_icon");
  uiWindowSetDefaultSize (uitest->wcont [UITEST_W_WINDOW], -1, 800);

  vbox = uiCreateVertBox ();
  uiWindowPackInWindow (uitest->wcont [UITEST_W_WINDOW], vbox);
  uiWidgetSetAllMargins (vbox, 4);

  uiutilsAddProfileColorDisplay (vbox, &accent);
  hbox = accent.hbox;
  uiwcontFree (accent.cbox);

  uitest->wcont [UITEST_W_MENUBAR] = uiCreateMenubar ();
  uiBoxPackStartExpand (hbox, uitest->wcont [UITEST_W_MENUBAR]);

  uiwidgetp = uiCreateLabel ("");
  uiBoxPackEnd (hbox, uiwidgetp);
  uiWidgetAddClass (uiwidgetp, ACCENT_CLASS);
  uitest->wcont [UITEST_W_STATUS_MSG] = uiwidgetp;
  uiwcontFree (hbox);

  /* main notebook */

  uitest->mainvnb = uivnbCreate (vbox);

  uitestUIButtons (uitest);
  uitestUIToggleButtons (uitest);
  uitestUIMiscButtons (uitest);
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
  uitestUISpinbox (uitest);
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
  uiwcont_t   *sg;
  uiwcont_t   *uiwidgetp;
  uiwcont_t   *twidgetp;

  sg = uiCreateSizeGroupHoriz ();

  /* buttons */

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);

  uivnbAppendPage (uitest->mainvnb, vbox, "Buttons");

  /* button: normal */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiCreateButton ("uitest-a",
      uitest->callbacks [UITEST_CB_B], "button", NULL);
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (sg, uiwidgetp);
  uitest->wcont [UITEST_W_B] = uiwidgetp;

  uiwidgetp = uiCreateLabel ("");
  uiBoxPackStart (hbox, uiwidgetp);
  uiWidgetSetMarginStart (uiwidgetp, 4);
  uitest->wcont [UITEST_W_B_MSG] = uiwidgetp;

  uiwcontFree (hbox);

  /* button: align-left */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiCreateButton ("uitest-b",
      uitest->callbacks [UITEST_CB_B], "button-left", NULL);
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (sg, uiwidgetp);
  uiButtonAlignLeft (uiwidgetp);

  uiwcontFree (hbox);

  /* button: long text */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiCreateButton ("uitest-long",
      uitest->callbacks [UITEST_CB_B], "button long text", NULL);
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (sg, uiwidgetp);

  uiwcontFree (hbox);

  /* button: flat */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiCreateButton ("uitest-b",
      uitest->callbacks [UITEST_CB_B], "button-flat", NULL);
  uiButtonSetFlat (uiwidgetp);
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (sg, uiwidgetp);
  uiButtonAlignLeft (uiwidgetp);

  uiwcontFree (hbox);

  /* button: image */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiCreateButton ("uitest-pause",
      uitest->callbacks [UITEST_CB_B_IMG_A], NULL, "button_pause");
  uiBoxPackStart (hbox, uiwidgetp);
  uitest->wcont [UITEST_W_B_IMG_A] = uiwidgetp;

  uiwidgetp = uiCreateLabel ("");
  uiBoxPackStart (hbox, uiwidgetp);
  uiWidgetSetMarginStart (uiwidgetp, 4);
  uitest->wcont [UITEST_W_B_IMG_A_MSG] = uiwidgetp;

  uiwcontFree (hbox);

  /* button: image, text */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiCreateButton ("uitest-img-tt",
      uitest->callbacks [UITEST_CB_B_IMG_B], "img-tooltip", "button_play");
  uiBoxPackStart (hbox, uiwidgetp);
  uitest->wcont [UITEST_W_B_IMG_B] = uiwidgetp;

  uiwidgetp = uiCreateLabel ("");
  uiBoxPackStart (hbox, uiwidgetp);
  uiWidgetSetMarginStart (uiwidgetp, 4);
  uitest->wcont [UITEST_W_B_IMG_B_MSG] = uiwidgetp;

  uiwcontFree (hbox);

  /* radio button */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiCreateRadioButton (NULL, "radio a", UI_TOGGLE_BUTTON_ON);
  uiBoxPackStart (hbox, uiwidgetp);
  twidgetp = uiwidgetp;

  uiwcontFree (hbox);

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiCreateRadioButton (twidgetp, "radio b", UI_TOGGLE_BUTTON_OFF);
  uiBoxPackStart (hbox, uiwidgetp);
  uiwcontFree (uiwidgetp);
  uiwcontFree (twidgetp);

  uiwcontFree (hbox);

  /* check button */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiCreateCheckButton ("check button", UI_TOGGLE_BUTTON_OFF);
  uiBoxPackStart (hbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwcontFree (hbox);

  uiwcontFree (vbox);
}

void
uitestUIToggleButtons (uitest_t *uitest)
{
  uiwcont_t   *vbox;
  uiwcont_t   *hbox;
  uiwcont_t   *sg;
  uiwcont_t   *uiwidgetp;
  char        imgbuff [MAXPATHLEN];

  sg = uiCreateSizeGroupHoriz ();

  /* toggle buttons */

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);

  uivnbAppendPage (uitest->mainvnb, vbox, "Toggle Buttons");

  /* toggle button: normal */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiCreateToggleButton ("toggle button",
      NULL, NULL, NULL, 0);
  uiBoxPackStart (hbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwcontFree (hbox);

  /* toggle button: tooltip */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiCreateToggleButton ("toggle tooltip", NULL, "tool-tip", NULL, 0);
  uiBoxPackStart (hbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwcontFree (hbox);

  /* toggle button: with image */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

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
  uiWidgetAlignHorizCenter (uiwidgetp);
  uiWidgetAlignVertCenter (uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwcontFree (hbox);

  /* switch */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uitest->wcont [UITEST_W_SW] = uiCreateSwitch (1);
  uiBoxPackStart (hbox, uitest->wcont [UITEST_W_SW]);

  uiwcontFree (hbox);

  uiwcontFree (vbox);
}

void
uitestUIMiscButtons (uitest_t *uitest)
{
  uiwcont_t   *vbox;
  uiwcont_t   *hbox;
  uiwcont_t   *sg;
  uiwcont_t   *uiwidgetp;

  sg = uiCreateSizeGroupHoriz ();

  /* misc buttons */

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);

  uivnbAppendPage (uitest->mainvnb, vbox, "Misc Buttons");

  /* font button */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiCreateFontButton ("Sans Bold 10");
  uiBoxPackStart (hbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwcontFree (hbox);

  /* color button */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiCreateColorButton ("#aacc00");
  uiBoxPackStart (hbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwcontFree (hbox);

  uiwcontFree (vbox);
}

void
uitestUIChgInd (uitest_t *uitest)
{
  uiwcont_t   *vbox;
  uiwcont_t   *hbox;
  uiwcont_t   *uiwidgetp;

  /* change indicator */

  uitest->callbacks [UITEST_CB_CHG_IND] = callbackInit (
      uitestCBchgind, uitest, NULL);

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);

  uivnbAppendPage (uitest->mainvnb, vbox, "Change Indicator");

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiCreateChangeIndicator (hbox);
  uichgindMarkNormal (uiwidgetp);
  uitest->wcont [UITEST_W_CI_A] = uiwidgetp;

  uiwidgetp = uiCreateLabel ("aaa");
  uiBoxPackStart (hbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiCreateButton ("uitest-switch",
      uitest->callbacks [UITEST_CB_CHG_IND], "switch", NULL);
  uiBoxPackStart (hbox, uiwidgetp);
  uitest->wcont [UITEST_W_CI_BUTTON] = uiwidgetp;

  uiwcontFree (hbox);

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiCreateChangeIndicator (hbox);
  uichgindMarkChanged (uiwidgetp);
  uitest->wcont [UITEST_W_CI_B] = uiwidgetp;

  uiwidgetp = uiCreateLabel ("bbb");
  uiBoxPackStart (hbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwcontFree (hbox);

  uiwcontFree (vbox);
}

void
uitestUIDropdown (uitest_t *uitest)
{
  uiwcont_t   *vbox;
  uidd_t      *uidd [UITEST_DD_MAX];

  /* drop-downs */

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);

  uivnbAppendPage (uitest->mainvnb, vbox, "Drop-Down");

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

  uivnbAppendPage (uitest->mainvnb, vbox, "Entry");

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
  char        tbuff [MAXPATHLEN];

  /* image */

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);

  uivnbAppendPage (uitest->mainvnb, vbox, "Image");

  pathbldMakePath (tbuff, sizeof (tbuff),
     "bdj4_icon", BDJ4_IMG_SVG_EXT, PATHBLD_MP_DIR_IMG);
  uiwidgetp = uiImageScaledFromFile (tbuff, 64);
  uiBoxPackStart (vbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwcontFree (vbox);
}

void
uitestUILabels (uitest_t *uitest)
{
  uiwcont_t   *vbox;
  uiwcont_t   *hbox;
  uiwcont_t   *uiwidgetp;

  uiLabelAddClass ("ra", "#cc0000");
  uiLabelAddClass ("ga", "#00cc00");
  uiLabelAddClass ("ba", "#0000cc");
  uiLabelAddClass ("rb", "#cc5555");
  uiLabelAddClass ("gb", "#55cc55");
  uiLabelAddClass ("bb", "#5555cc");

  /* labels */

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);

  uivnbAppendPage (uitest->mainvnb, vbox, "Label");

  /* label: pack start */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiCreateLabel ("ps-a");
  uiBoxPackStart (hbox, uiwidgetp);
  uiWidgetAddClass (uiwidgetp, "gb");
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiCreateLabel ("ps-b");
  uiBoxPackStart (hbox, uiwidgetp);
  uiWidgetAddClass (uiwidgetp, "bb");
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiCreateLabel ("ps-c");
  uiBoxPackStart (hbox, uiwidgetp);
  uiWidgetAddClass (uiwidgetp, "rb");
  uiwcontFree (uiwidgetp);

  uiwcontFree (hbox);

  /* label: pack start / expand horiz */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiCreateLabel ("ps-eh");
  uiBoxPackStart (hbox, uiwidgetp);
  uiWidgetAddClass (uiwidgetp, "gb");
  uiWidgetExpandHoriz (uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiCreateLabel ("ps-b");
  uiBoxPackStart (hbox, uiwidgetp);
  uiWidgetAddClass (uiwidgetp, "bb");
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiCreateLabel ("ps-c");
  uiBoxPackStart (hbox, uiwidgetp);
  uiWidgetAddClass (uiwidgetp, "rb");
  uiwcontFree (uiwidgetp);

  uiwcontFree (hbox);

  /* label: pack start / expand horiz / align center */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiCreateLabel ("ps-eh-ac");
  uiBoxPackStart (hbox, uiwidgetp);
  uiWidgetAddClass (uiwidgetp, "gb");
  uiWidgetAlignHorizCenter (uiwidgetp);
  uiWidgetExpandHoriz (uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiCreateLabel ("b");
  uiBoxPackStart (hbox, uiwidgetp);
  uiWidgetAddClass (uiwidgetp, "bb");
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiCreateLabel ("c");
  uiBoxPackStart (hbox, uiwidgetp);
  uiWidgetAddClass (uiwidgetp, "rb");
  uiwcontFree (uiwidgetp);

  uiwcontFree (hbox);

  /* label: pack start / expand horiz / align start */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiCreateLabel ("ps-eh-as");
  uiBoxPackStart (hbox, uiwidgetp);
  uiWidgetAddClass (uiwidgetp, "gb");
  uiWidgetExpandHoriz (uiwidgetp);
  uiWidgetAlignHorizStart (uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiCreateLabel ("b");
  uiBoxPackStart (hbox, uiwidgetp);
  uiWidgetAddClass (uiwidgetp, "bb");
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiCreateLabel ("c");
  uiBoxPackStart (hbox, uiwidgetp);
  uiWidgetAddClass (uiwidgetp, "rb");
  uiwcontFree (uiwidgetp);

  uiwcontFree (hbox);

  /* label: pack end */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiCreateLabel ("pe-a");
  uiBoxPackEnd (hbox, uiwidgetp);
  uiWidgetAddClass (uiwidgetp, "gb");
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiCreateLabel ("pe-b");
  uiBoxPackEnd (hbox, uiwidgetp);
  uiWidgetAddClass (uiwidgetp, "bb");
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiCreateLabel ("pe-c");
  uiBoxPackEnd (hbox, uiwidgetp);
  uiWidgetAddClass (uiwidgetp, "rb");
  uiwcontFree (uiwidgetp);

  uiwcontFree (hbox);

  /* label: pack start / pack end */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiCreateLabel ("ps-a");
  uiBoxPackStart (hbox, uiwidgetp);
  uiWidgetAddClass (uiwidgetp, "gb");
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiCreateLabel ("ps-b");
  uiBoxPackStart (hbox, uiwidgetp);
  uiWidgetAddClass (uiwidgetp, "bb");
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiCreateLabel ("ps-c");
  uiBoxPackStart (hbox, uiwidgetp);
  uiWidgetAddClass (uiwidgetp, "rb");
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiCreateLabel ("pe-a");
  uiBoxPackEnd (hbox, uiwidgetp);
  uiWidgetAddClass (uiwidgetp, "gb");
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiCreateLabel ("pe-b");
  uiBoxPackEnd (hbox, uiwidgetp);
  uiWidgetAddClass (uiwidgetp, "bb");
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiCreateLabel ("pe-c");
  uiBoxPackEnd (hbox, uiwidgetp);
  uiWidgetAddClass (uiwidgetp, "rb");
  uiwcontFree (uiwidgetp);

  uiwcontFree (hbox);

  /* label: pack start / ellipsize */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiCreateLabel ("ps-el ellipsize ellipsize ellipsize ellipsize ellipsize ellipsize ellipsize ellipsize ellipsize");
  uiBoxPackStart (hbox, uiwidgetp);
  uiLabelEllipsizeOn (uiwidgetp);
  uiLabelSetMaxWidth (uiwidgetp, 20);
  uiWidgetAddClass (uiwidgetp, "gb");
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiCreateLabel ("a");
  uiBoxPackStart (hbox, uiwidgetp);
  uiWidgetAddClass (uiwidgetp, "bb");
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiCreateLabel ("b");
  uiBoxPackStart (hbox, uiwidgetp);
  uiWidgetAddClass (uiwidgetp, "rb");
  uiwcontFree (uiwidgetp);

  uiwcontFree (hbox);

  /* label: pack start / accent */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiCreateLabel ("accent");
  uiBoxPackStart (hbox, uiwidgetp);
  uiWidgetAddClass (uiwidgetp, ACCENT_CLASS);
  uiwcontFree (uiwidgetp);

  uiwcontFree (hbox);

  /* label: pack start / error */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiCreateLabel ("error");
  uiBoxPackStart (hbox, uiwidgetp);
  uiWidgetAddClass (uiwidgetp, ERROR_CLASS);
  uiwcontFree (uiwidgetp);

  uiwcontFree (hbox);

  /* label: pack start / dark-accent */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

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
  uiwcont_t   *hbox;
  uiwcont_t   *uiwidgetp;

  uitest->callbacks [UITEST_CB_LINK_A] = callbackInit (
      uitestCBLink, uitest, NULL);

  /* link */

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);

  uivnbAppendPage (uitest->mainvnb, vbox, "Link");

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiCreateLink ("bdj", "https://ballroomdj.org");
  if (isMacOS ()) {
    uiLinkSetActivateCallback (uiwidgetp, uitest->callbacks [UITEST_CB_LINK_A]);
  }
  uiBoxPackStart (hbox, uiwidgetp);
  uitest->wcont [UITEST_W_LINK_A] = uiwidgetp;

  uiwcontFree (hbox);

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
  uivnb_t     *vnb;

  /* notebook */

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);
  uiWidgetExpandVert (vbox);
  uiWidgetExpandHoriz (vbox);

  uivnbAppendPage (uitest->mainvnb, vbox, "Notebook");

  /* vertical */

  uitest->wcont [UITEST_W_NB_V] = uiCreateNotebook ();
  uiWidgetAddClass (uitest->wcont [UITEST_W_NB_V], LEFT_NB_CLASS);
  uiNotebookTabPositionLeft (uitest->wcont [UITEST_W_NB_V]);
  uiNotebookSetScrollable (uitest->wcont [UITEST_W_NB_V]);
  uiBoxPackStartExpand (vbox, uitest->wcont [UITEST_W_NB_V]);

  /* vert %d */

  for (int i = 1; i < 4; ++i) {
    char    tbuff [200];

    snprintf (tbuff, sizeof (tbuff), "Vert %d", i);
    vboxb = uiCreateVertBox ();
    uiwidgetp = uiCreateLabel (tbuff);
    uiNotebookAppendPage (uitest->wcont [UITEST_W_NB_V], vboxb, uiwidgetp);
    uiWidgetSetAllMargins (vboxb, 4);
    uiwcontFree (uiwidgetp);

    uiwidgetp = uiCreateLabel (tbuff);
    uiBoxPackStart (vboxb, uiwidgetp);
    uiwcontFree (uiwidgetp);
    uiwcontFree (vboxb);
  }

  /* horizontal */

  uitest->wcont [UITEST_W_NB_H] = uiCreateNotebook ();
  uiBoxPackStartExpand (vbox, uitest->wcont [UITEST_W_NB_H]);

  /* horiz %d */

  for (int i = 1; i < 4; ++i) {
    char    tbuff [200];

    snprintf (tbuff, sizeof (tbuff), "Horiz %d", i);
    vboxb = uiCreateVertBox ();
    uiwidgetp = uiCreateLabel (tbuff);
    uiNotebookAppendPage (uitest->wcont [UITEST_W_NB_H], vboxb, uiwidgetp);
    uiWidgetSetAllMargins (vboxb, 4);
    uiwcontFree (uiwidgetp);

    uiwidgetp = uiCreateLabel (tbuff);
    uiBoxPackStart (vboxb, uiwidgetp);
    uiwcontFree (uiwidgetp);
    uiwcontFree (vboxb);
  }

  /* horizontal w/images */

  uitest->wcont [UITEST_W_NB_HI] = uiCreateNotebook ();
  uiBoxPackStartExpand (vbox, uitest->wcont [UITEST_W_NB_HI]);

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

  /* horiz-img 1 */

  vboxb = uiCreateVertBox ();
  hbox = uiCreateHorizBox ();
  uiwidgetp = uiCreateLabel ("HI One");
  uiBoxPackStart (hbox, uiwidgetp);

  uiwcontFree (uiwidgetp);
  uiBoxPackStart (hbox, uitest->images [UITEST_NB1_I_LED_OFF]);
  uiWidgetAlignHorizCenter (uiwidgetp);
  uiWidgetAlignVertCenter (uiwidgetp);

  uiNotebookAppendPage (uitest->wcont [UITEST_W_NB_HI], vboxb, hbox);
  uiWidgetSetAllMargins (vboxb, 4);
  uiWidgetShowAll (hbox);
  uiwcontFree (hbox);

  uiwidgetp = uiCreateLabel ("HI One");
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

  /* horiz-img 2 */

  vboxb = uiCreateVertBox ();
  hbox = uiCreateHorizBox ();
  uiwidgetp = uiCreateLabel ("HI Two");
  uiBoxPackStart (hbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiBoxPackStart (hbox, uitest->images [UITEST_NB2_I_LED_OFF]);
  uiWidgetAlignHorizCenter (uiwidgetp);
  uiWidgetAlignVertCenter (uiwidgetp);

  uiNotebookAppendPage (uitest->wcont [UITEST_W_NB_HI], vboxb, hbox);
  uiWidgetSetAllMargins (vboxb, 4);
  uiWidgetShowAll (hbox);
  uiwcontFree (hbox);

  uiwidgetp = uiCreateLabel ("HI Two");
  uiBoxPackStart (vboxb, uiwidgetp);
  uiwcontFree (uiwidgetp);
  uiwcontFree (vboxb);

  /* horiz-img 3 */

  vboxb = uiCreateVertBox ();
  uiwidgetp = uiCreateLabel ("HI Three");
  uiNotebookAppendPage (uitest->wcont [UITEST_W_NB_HI], vboxb, uiwidgetp);
  uiWidgetSetAllMargins (vboxb, 4);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiCreateLabel ("HI Three");
  uiBoxPackStart (vboxb, uiwidgetp);
  uiwcontFree (uiwidgetp);
  uiwcontFree (vboxb);

  /* action widget */

  // uiNotebookSetActionWidget (plui->wcont [PLUI_W_NOTEBOOK], uiwidgetp);

  /* VNB */

  vnb = uivnbCreate (vbox);
  uitest->vnb = vnb;

  /* VNB pages */

  for (int i = 1; i < 10; ++i) {
    char    tbuff [200];

    snprintf (tbuff, sizeof (tbuff), "VNB %d", i);
    vboxb = uiCreateVertBox ();
    uiWidgetSetAllMargins (vboxb, 4);
    uivnbAppendPage (vnb, vboxb, tbuff);

    uiwidgetp = uiCreateLabel (tbuff);
    uiBoxPackStart (vboxb, uiwidgetp);
    uiwcontFree (uiwidgetp);
    uiwcontFree (vboxb);
  }

  /* main vert box display */

  uiwcontFree (vbox);
}

void
uitestUIPanedWin (uitest_t *uitest)
{
  uiwcont_t   *vbox;
  uiwcont_t   *pw;
  uiwcont_t   *tvbox;
  uiwcont_t   *uiwidgetp;

  /* paned window */

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);

  uivnbAppendPage (uitest->mainvnb, vbox, "Paned Window");

  pw = uiPanedWindowCreateVert ();
  uiBoxPackStartExpand (vbox, pw);
  uiWidgetExpandHoriz (pw);
  uiWidgetAlignHorizFill (pw);
  uiWidgetAddClass (pw, ACCENT_CLASS);

  tvbox = uiCreateVertBox ();
  uiPanedWindowPackStart (pw, tvbox);

  uiwidgetp = uiCreateLabel ("AAA");
  uiBoxPackStart (tvbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwcontFree (tvbox);

  tvbox = uiCreateVertBox ();
  uiPanedWindowPackEnd (pw, tvbox);

  uiwidgetp = uiCreateLabel ("BBB");
  uiBoxPackStart (tvbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwcontFree (tvbox);

  uiwcontFree (pw);
  uiwcontFree (vbox);
}

void
uitestUIMisc (uitest_t *uitest)
{
  uiwcont_t   *vbox;

  /* progress bar */

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);

  uivnbAppendPage (uitest->mainvnb, vbox, "Misc");

  uiwcontFree (vbox);
}

void
uitestUISizeGroup (uitest_t *uitest)
{
  uiwcont_t   *vbox;
  uiwcont_t   *hbox;
  uiwcont_t   *sg;
  uiwcont_t   *uiwidgetp;

  /* size group */

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);

  uivnbAppendPage (uitest->mainvnb, vbox, "Size Group");

  sg = uiCreateSizeGroupHoriz ();

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiCreateLabel ("aaa");
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (sg, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiCreateLabel ("aaa");
  uiBoxPackStart (hbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiCreateLabel ("aaa");
  uiBoxPackStart (hbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwcontFree (hbox);

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiCreateLabel ("bbbb");
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (sg, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiCreateLabel ("bbbb");
  uiBoxPackStart (hbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiCreateLabel ("bbbb");
  uiBoxPackStart (hbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwcontFree (hbox);

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiCreateLabel ("c");
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (sg, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiCreateLabel ("c");
  uiBoxPackStart (hbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiCreateLabel ("c");
  uiBoxPackStart (hbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwcontFree (hbox);

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiCreateLabel ("ddddddd");
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (sg, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiCreateLabel ("ddddddd");
  uiBoxPackStart (hbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiCreateLabel ("ddddddd");
  uiBoxPackStart (hbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwcontFree (hbox);

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiCreateLabel ("eeeeee");
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (sg, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiCreateLabel ("eeeeee");
  uiBoxPackStart (hbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiCreateLabel ("eeeeee");
  uiBoxPackStart (hbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwcontFree (hbox);

  uiwcontFree (sg);
  uiwcontFree (vbox);
}

void
uitestUISpinbox (uitest_t *uitest)
{
  uiwcont_t   *vbox;
  uiwcont_t   *hbox;
  uiwcont_t   *uiwidgetp;
  nlist_t     *txtlist;

  /* spinboxes */

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);

  uivnbAppendPage (uitest->mainvnb, vbox, "Spin Box");

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiSpinboxTextCreate (uitest);
  uiBoxPackStart (hbox, uiwidgetp);
  txtlist = nlistAlloc ("txtlist", LIST_ORDERED, NULL);
  nlistSetStr (txtlist, 0, "aaa");
  nlistSetStr (txtlist, 1, "bbb");
  nlistSetStr (txtlist, 2, "ccc");
  nlistSetStr (txtlist, 3, "ddd");
  uiSpinboxTextSet (uiwidgetp, 0, nlistGetCount (txtlist), 10,
      txtlist, NULL, NULL);
  uiSpinboxTextSetValue (uiwidgetp, 0);
  uitest->wcont [UITEST_W_SB_TEXT] = uiwidgetp;

  uiwcontFree (hbox);

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiSpinboxIntCreate ();
  uiBoxPackStart (hbox, uiwidgetp);
  uiSpinboxSetRange (uiwidgetp, 3.0, 15.0);
  uitest->wcont [UITEST_W_SB_INT] = uiwidgetp;

  uiwcontFree (hbox);

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiSpinboxDoubleCreate ();
  uiBoxPackStart (hbox, uiwidgetp);
  uiSpinboxSetRange (uiwidgetp, 1.0, 20.0);
  uitest->wcont [UITEST_W_SB_DBL_A] = uiwidgetp;

  uiwcontFree (hbox);

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiSpinboxDoubleCreate ();
  uiBoxPackStart (hbox, uiwidgetp);
  uiSpinboxSetRange (uiwidgetp, 1.0, 20.0);
  uiSpinboxSetIncrement (uiwidgetp, 1.0, 5.0);
  uitest->wcont [UITEST_W_SB_DBL_B] = uiwidgetp;

  uiwcontFree (hbox);

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiSpinboxDoubleDefaultCreate ();
  uiBoxPackStart (hbox, uiwidgetp);
  uiSpinboxSetRange (uiwidgetp, 2.0, 10.0);
  uitest->wcont [UITEST_W_SB_DBL_DFLT] = uiwidgetp;

  uiwcontFree (hbox);

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiSpinboxTimeCreate (SB_TIME_BASIC, uitest, "basic", NULL);
  uiBoxPackStart (hbox, uiwidgetp);
  uitest->wcont [UITEST_W_SB_TIME_A] = uiwidgetp;

  uiwcontFree (hbox);

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiSpinboxTimeCreate (SB_TIME_PRECISE, uitest, "precise", NULL);
  uiBoxPackStart (hbox, uiwidgetp);
  uitest->wcont [UITEST_W_SB_TIME_B] = uiwidgetp;

  uiwcontFree (hbox);

  uiwcontFree (vbox);
}

void
uitestUITextBox (uitest_t *uitest)
{
  uiwcont_t   *vbox;

  /* text box */

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);

  uivnbAppendPage (uitest->mainvnb, vbox, "Text Box");

  uiwcontFree (vbox);
}

void
uitestUIVirtList (uitest_t *uitest)
{
  uiwcont_t   *hbox;

  /* tree view */

  hbox = uiCreateHorizBox ();
  uiWidgetSetAllMargins (hbox, 4);

  uivnbAppendPage (uitest->mainvnb, hbox, "Virtual List");

  for (int i = 0; i < UITEST_VL_MAX; ++i) {
    uitest->vl [i] = uivlCreate ("uitest", NULL, hbox,
        UITEST_VL_DISPROWS, VL_NO_WIDTH, VL_ENABLE_KEYS);
    uivlSetUseListingFont (uitest->vl [i]);
    uivlSetAllowMultiple (uitest->vl [i]);
    uivlSetNumColumns (uitest->vl [i], vlcols [i]);
    uivlSetNumRows (uitest->vl [i], vlmaxrows [i]);
    for (int j = 0; j < vlcols [i]; ++j) {
      if (i == 2 && j == 0) {
        uivlMakeColumn (uitest->vl [i], "hidden", j, VL_TYPE_INTERNAL_NUMERIC);
      } else {
        uivlMakeColumn (uitest->vl [i], "label", j, VL_TYPE_LABEL);
      }
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
    uivlSetSelectChgCallback (uitest->vl [i], uitestVLSelectCB, &vlcb [i]);
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

static bool
uitestCBchgind (void *udata)
{
  uitest_t  *uitest = udata;

  if (uitest->chgind) {
    uichgindMarkNormal (uitest->wcont [UITEST_W_CI_A]);
    uichgindMarkChanged (uitest->wcont [UITEST_W_CI_B]);
    uitest->chgind = false;
  } else {
    uichgindMarkChanged (uitest->wcont [UITEST_W_CI_A]);
    uichgindMarkNormal (uitest->wcont [UITEST_W_CI_B]);
    uitest->chgind = true;
  }

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
      snprintf (tbuff, sizeof (tbuff), "%d-%" PRIu32 "-%d", vlidx, rownum, j);
      if (vlidx != 1 && rownum % 5 == 0 && j == 0) {
        snprintf (tbuff, sizeof (tbuff), "%d-%" PRIu32 "-%d stuff stuff stuff stuff", vlidx, rownum, j);
      }
      if (vlidx == 1 && rownum == 35 && j == 0) {
        snprintf (tbuff, sizeof (tbuff), "%-d%" PRIu32 "-%d stuff stuff stuff stuff", vlidx, rownum, j);
      }
      if (rownum % 7 == 0 && j == 2) {
        snprintf (tbuff, sizeof (tbuff), "%d-%" PRIu32, vlidx, rownum);
      }
    }

    if (vlidx == 2 && j == 0) {
      uivlSetRowColumnNum (vl, rownum, j, rownum);
    } else {
      uivlSetRowColumnStr (vl, rownum, j, tbuff);
    }
    if (rownum % 9 == 0 && vlidx != 2 && j == 0) {
      uivlSetRowColumnClass (vl, rownum, j, ACCENT_CLASS);
    }
  }
}

static void
uitestVLSelectCB (void *udata, uivirtlist_t *vl, int32_t rownum, int colidx)
{
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

static bool
uitestCBLink (void *udata)
{
  char        tmp [MAXPATHLEN];

  snprintf (tmp, sizeof (tmp), "%s '%s'", sysvarsGetStr (SV_PATH_URI_OPEN),
      "https://ballroomdj.org");
  (void) ! system (tmp);
  return UICB_STOP;
}
