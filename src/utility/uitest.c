/*
 * Copyright 2024-2026 Brad Lanam Pleasant Hill CA
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

#include "bdj4arg.h"
#include "bdj4.h"
#include "bdj4arg.h"
#include "bdj4intl.h"
#include "bdjopt.h"
#include "callback.h"
#include "localeutil.h"
#include "mdebug.h"
#include "nlist.h"
#include "osuiutils.h"
#include "pathbld.h"
#include "sysvars.h"
#include "tmutil.h"
#include "uidd.h"
#include "ui.h"
#include "uiclass.h"
#include "uihnb.h"
#include "uisbnum.h"
#include "uisbtext.h"
#include "uiutils.h"
#include "uivirtlist.h"
#include "uivnb.h"
#include "uiwcont.h"

enum {
  UITEST_W_B,
  UITEST_W_B_FLAT,
  UITEST_W_B_IMG_A,
  UITEST_W_B_IMG_A_MSG,
  UITEST_W_B_IMG_B,
  UITEST_W_B_IMG_B_MSG,
  UITEST_W_B_IMG_C,
  UITEST_W_B_IMG_C_MSG,
  UITEST_W_B_IMG_D,
  UITEST_W_B_LEFT,
  UITEST_W_B_LONG,
  UITEST_W_B_MSG,
  UITEST_W_CI_A,
  UITEST_W_CI_B,
  UITEST_W_CI_BUTTON,
  UITEST_W_IMG_A,
  UITEST_W_LINK_A,
  UITEST_W_MENU_A,
  UITEST_W_MENU_B,
  UITEST_W_MENUBAR,
  UITEST_W_NB_ACTION,
  UITEST_W_PBAR,
  UITEST_W_SCALE,
  UITEST_W_SEP,
  UITEST_W_SW,
  UITEST_W_TOGGLE_C,
  UITEST_W_WINDOW,
  UITEST_W_MAX,
};

enum {
  UITEST_SB_INT,
  UITEST_SB_DBL,
  UITEST_SB_DBL_B,
  UITEST_SB_DBL_DFLT,
  UITEST_SB_TM_A,
  UITEST_SB_TM_B,
  UITEST_SB_MAX,
};

enum {
  UITEST_CB_CLOSE,
  UITEST_CB_B,
  UITEST_CB_B_IMG_A,
  UITEST_CB_B_IMG_B,
  UITEST_CB_B_IMG_C,
  UITEST_CB_NB_ACTION,
  UITEST_CB_CHG_IND,
  UITEST_CB_DD_STR,
  UITEST_CB_DD_NUM,
  UITEST_CB_LINK_A,
  UITEST_CB_TOGGLE,
  UITEST_CB_MAX,
};

enum {
  UITEST_LED_OFF,
  UITEST_LED_ON,
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
  uihdrline_t     *hdrline;
  uivnb_t         *mainvnb;
  uiwcont_t       *wcont [UITEST_W_MAX];
  uisbnum_t       *sbnum [UITEST_SB_MAX];
  uivirtlist_t    *vl [UITEST_VL_MAX];
  callback_t      *callbacks [UITEST_CB_MAX];
  char            *images [UITEST_I_MAX];
  callback_t      *chgcb;
  ilist_t         *lista;
  ilist_t         *listb;
  uivnb_t         *vnb;
  uihnb_t         *hnb;
  uihnb_t         *hnbimg;
  uisbtext_t      *sbtext;
  uisbtext_t      *sbtextb;
  nlist_t         *sbtxtlist;
  uidd_t          *uidd [UITEST_DD_MAX];
  long            counter;
  int             hnbimgsel;
  bool            stop;
  bool            chgind;
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
static bool uitestCBButtonImgC (void *udata);
static bool uitestCBNBAction (void *udata);
static bool uitestCBNull (void *udata);
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
  char        imgbuff [BDJ4_PATH_MAX];

  static struct option bdj_options [] = {
    { "bdj4",         no_argument,        NULL,   'B' },
    { "uitest",       no_argument,        NULL,   0 },
    { "debugself",    no_argument,        NULL,   0 },
    { "verbose",      no_argument,        NULL,   0, },
    { "quiet",        no_argument,        NULL,   0, },
    { "nodetach",     no_argument,        NULL,   0, },
    { "origcwd",      required_argument,  NULL,   0 },
    { "scale",        required_argument,  NULL,   0 },
    { "theme",        required_argument,  NULL,   0 },
    { "wait",         no_argument,NULL,   0 },
    { "memwatch",     required_argument,  NULL,   126 },
  };

  mdebugInit ("uitest");

  bdj4arg = bdj4argInit (argc, argv);

  while ((c = getopt_long_only (argc, bdj4argGetArgv (bdj4arg),
      "BCp:d:mnNRs", bdj_options, &option_index)) != -1) {
    switch (c) {
      case 'B': {
        isbdj4 = true;
        break;
      }
      case 126: {
        targ = bdj4argGet (bdj4arg, optind - 1, optarg);
        mdebugSetWatch (strtoll (targ, NULL, 16));
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
  uitest.hdrline = NULL;
  uitest.stop = false;
  uitest.chgind = false;
  uitest.hnbimgsel = 0;
  uitest.counter = 1;
  uitest.lista = NULL;
  uitest.listb = NULL;
  uitest.chgcb = NULL;
  uitest.sbtext = NULL;
  uitest.sbtextb = NULL;
  for (int i = 0; i < UITEST_SB_MAX; ++i) {
    uitest.sbnum [i] = NULL;
  }

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

  pathbldMakePath (imgbuff, sizeof (imgbuff), "led_off", ".svg",
      PATHBLD_MP_DIR_IMG);
  uitest.images [UITEST_LED_OFF] = strdup (imgbuff);

  pathbldMakePath (imgbuff, sizeof (imgbuff), "led_on", ".svg",
      PATHBLD_MP_DIR_IMG);
  uitest.images [UITEST_LED_ON] = strdup (imgbuff);

  uitestBuildUI (&uitest);
  osuiFinalize ();

  uitestMainLoop (&uitest);

  bdj4argCleanup (bdj4arg);
  uitestCleanup (&uitest);
  mdebugReport ();
  mdebugCleanup ();
  return 0;
}

static void
uitestMainLoop (uitest_t *uitest)
{
  while (uitest->stop == false) {
    uiUIProcessEvents ();

    for (int i = 0; i < UITEST_SB_MAX; ++i) {
      uisbnumCheck (uitest->sbnum [i]);
    }

    mssleep (50);
  }
}

static void
uitestBuildUI (uitest_t *uitest)
{
  uiwcont_t   *vbox;
  uiwcont_t   *menuitem;
  uiwcont_t   *menu;

  uiwcontInitID (UI_UITEST_ID);

  uitest->callbacks [UITEST_CB_CLOSE] = callbackInit (
      uitestCloseWin, uitest, NULL);

  uitest->callbacks [UITEST_CB_B] = callbackInit (
      uitestCBButton, uitest, "button-1");
  uitest->callbacks [UITEST_CB_B_IMG_A] = callbackInit (
      uitestCBButtonImgA, uitest, NULL);
  uitest->callbacks [UITEST_CB_B_IMG_B] = callbackInit (
      uitestCBButtonImgB, uitest, NULL);
  uitest->callbacks [UITEST_CB_B_IMG_C] = callbackInit (
      uitestCBButtonImgC, uitest, NULL);
  uitest->callbacks [UITEST_CB_TOGGLE] = callbackInit (
      uitestCBNull, uitest, "toggle-1");

  uitest->wcont [UITEST_W_WINDOW] = uiCreateMainWindow (
      uitest->callbacks [UITEST_CB_CLOSE], "uitest", "bdj4_icon");
  uiWindowSetDefaultSize (uitest->wcont [UITEST_W_WINDOW], -1, 800);

  vbox = uiCreateVertBox ();
  uiWindowPackInWindow (uitest->wcont [UITEST_W_WINDOW], vbox);
  uiWidgetSetAllMargins (vbox, 4);

  uitest->hdrline = uiutilsHeaderLineSetup (vbox);
  uitest->wcont [UITEST_W_MENUBAR] = uiutilsHeaderLineAddMenubar (uitest->hdrline);
  uiutilsHeaderLinePostProcess (uitest->hdrline);


  uitest->wcont [UITEST_W_MENU_A] = uiMenuAlloc ();

  menuitem = uiMenuAddMainItem (uitest->wcont [UITEST_W_MENUBAR],
      uitest->wcont [UITEST_W_MENU_A], "Menu A");
  menu = uiCreateSubMenu (menuitem);
  uiwcontFree (menuitem);

  menuitem = uiMenuCreateItem (menu, "Menu A 1", NULL);
  uiwcontFree (menuitem);

  menuitem = uiMenuCreateItem (menu, "Menu A 2", NULL);
  uiwcontFree (menuitem);

  uiMenuAddSeparator (menu);

  menuitem = uiMenuCreateCheckbox (menu, "Menu A 3", 0, NULL);
  uiwcontFree (menuitem);

  menuitem = uiMenuCreateCheckbox (menu, "Menu A 4", 1, NULL);
  uiwcontFree (menuitem);

  uiMenuSetInitialized (uitest->wcont [UITEST_W_MENU_A]);
  uiMenuDisplay (uitest->wcont [UITEST_W_MENU_A]);

  uitest->wcont [UITEST_W_MENU_B] = uiMenuAlloc ();

  menuitem = uiMenuAddMainItem (uitest->wcont [UITEST_W_MENUBAR],
      uitest->wcont [UITEST_W_MENU_B], "Menu B");
  menu = uiCreateSubMenu (menuitem);
  uiwcontFree (menuitem);

  menuitem = uiMenuCreateItem (menu, "Menu B 1", NULL);
  uiwcontFree (menuitem);

  menuitem = uiMenuCreateItem (menu, "Menu B 2", NULL);
  uiwcontFree (menuitem);

  menuitem = uiMenuCreateItem (menu, "Dis B 3", NULL);
  uiWidgetSetState (menuitem, UIWIDGET_DISABLE);
  uiwcontFree (menuitem);

  uiMenuSetInitialized (uitest->wcont [UITEST_W_MENU_B]);
  uiMenuDisplay (uitest->wcont [UITEST_W_MENU_B]);

  /* main notebook */

  uitest->mainvnb = uivnbCreate (vbox, "uitest-main");

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
  uivnbPostProcess (uitest->mainvnb);

  uiWidgetShowAll (uitest->wcont [UITEST_W_WINDOW]);
  uiBoxPostProcess (vbox);
  uiwcontFree (vbox);
}

void
uitestUIButtons (uitest_t *uitest)
{
  uiwcont_t   *vbox;
  uiwcont_t   *hbox;
  uiwcont_t   *sg;
  uiwcont_t   *uiwidgetp;

fprintf (stderr, "Buttons\n");
  sg = uiCreateSizeGroupHoriz ();

  /* buttons */

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);

  uivnbAppendPage (uitest->mainvnb, vbox, "Buttons", VNB_NO_ID);

  /* button: normal */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox, WCONT_FREE);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiCreateButton (
      uitest->callbacks [UITEST_CB_B], "button", NULL, NULL);
  uiBoxPackStart (hbox, uiwidgetp, WCONT_KEEP);
  uiSizeGroupAdd (sg, uiwidgetp);
  uitest->wcont [UITEST_W_B] = uiwidgetp;

  uiwidgetp = uiCreateLabel ("");
  uiBoxPackStart (hbox, uiwidgetp, WCONT_KEEP);
  uiWidgetSetMarginStart (uiwidgetp, 4);
  uitest->wcont [UITEST_W_B_MSG] = uiwidgetp;

  uiBoxPostProcess (hbox);

  /* button: align-left */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox, WCONT_FREE);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiCreateButton (
      uitest->callbacks [UITEST_CB_B], "button-left", NULL, NULL);
  uiBoxPackStart (hbox, uiwidgetp, WCONT_KEEP);
  uiSizeGroupAdd (sg, uiwidgetp);
  uiButtonAlignLeft (uiwidgetp);
  uitest->wcont [UITEST_W_B_LEFT] = uiwidgetp;

  uiBoxPostProcess (hbox);

  /* button: long text */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox, WCONT_FREE);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiCreateButton (
      uitest->callbacks [UITEST_CB_B], "button long text", NULL, NULL);
  uiBoxPackStart (hbox, uiwidgetp, WCONT_KEEP);
  uiSizeGroupAdd (sg, uiwidgetp);
  uitest->wcont [UITEST_W_B_LONG] = uiwidgetp;

  uiBoxPostProcess (hbox);

  /* button: flat */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox, WCONT_FREE);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiCreateButton (
      uitest->callbacks [UITEST_CB_B], "button-flat", NULL, NULL);
  uiButtonSetFlat (uiwidgetp);
  uiBoxPackStart (hbox, uiwidgetp, WCONT_KEEP);
  uiSizeGroupAdd (sg, uiwidgetp);
  uiButtonAlignLeft (uiwidgetp);
  uitest->wcont [UITEST_W_B_FLAT] = uiwidgetp;

  uiBoxPostProcess (hbox);

  /* button: image */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox, WCONT_FREE);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiCreateButton (
      uitest->callbacks [UITEST_CB_B_IMG_A], NULL, "button_pause", NULL);
  uiBoxPackStart (hbox, uiwidgetp, WCONT_KEEP);
  uitest->wcont [UITEST_W_B_IMG_A] = uiwidgetp;

  uiwidgetp = uiCreateLabel ("");
  uiBoxPackStart (hbox, uiwidgetp, WCONT_KEEP);
  uiWidgetSetMarginStart (uiwidgetp, 4);
  uitest->wcont [UITEST_W_B_IMG_A_MSG] = uiwidgetp;

  uiBoxPostProcess (hbox);

  /* button: image, tooltip */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox, WCONT_FREE);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiCreateButton (
      uitest->callbacks [UITEST_CB_B_IMG_B], NULL, "button_play", "img-tooltip");
  uiBoxPackStart (hbox, uiwidgetp, WCONT_KEEP);
  uitest->wcont [UITEST_W_B_IMG_B] = uiwidgetp;

  uiwidgetp = uiCreateLabel ("");
  uiBoxPackStart (hbox, uiwidgetp, WCONT_KEEP);
  uiWidgetSetMarginStart (uiwidgetp, 4);
  uitest->wcont [UITEST_W_B_IMG_B_MSG] = uiwidgetp;

  uiBoxPostProcess (hbox);

  /* button: image, text, tooltip, chg-state */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox, WCONT_FREE);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiCreateButton (
      uitest->callbacks [UITEST_CB_B_IMG_C], "Image-toggle",
      uitest->images [UITEST_LED_OFF], "img-tooltip");
  uiButtonSetAltImage (uiwidgetp, uitest->images [UITEST_LED_ON]);
  uiBoxPackStart (hbox, uiwidgetp, WCONT_KEEP);
  uitest->wcont [UITEST_W_B_IMG_C] = uiwidgetp;

  uiwidgetp = uiCreateLabel ("");
  uiBoxPackStart (hbox, uiwidgetp, WCONT_KEEP);
  uiWidgetSetMarginStart (uiwidgetp, 4);
  uitest->wcont [UITEST_W_B_IMG_C_MSG] = uiwidgetp;

  uiBoxPostProcess (hbox);

  /* button w/image icon */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox, WCONT_FREE);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiCreateButton (
      uitest->callbacks [UITEST_CB_B], NULL, NULL, NULL);
  uiButtonSetImageIcon (uiwidgetp, "folder");
  uiBoxPackStart (hbox, uiwidgetp, WCONT_KEEP);
  uitest->wcont [UITEST_W_B_IMG_D] = uiwidgetp;

  uiBoxPostProcess (hbox);
  uiBoxPostProcess (vbox);
  uiwcontFree (sg);
}

void
uitestUIToggleButtons (uitest_t *uitest)
{
  uiwcont_t   *vbox;
  uiwcont_t   *hbox;
  uiwcont_t   *sg;
  uiwcont_t   *uiwidgetp;
  uiwcont_t   *twidgetp;

fprintf (stderr, "ToggleButtons\n");
  sg = uiCreateSizeGroupHoriz ();

  /* toggle buttons */

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);

  uivnbAppendPage (uitest->mainvnb, vbox, "Toggle Buttons", VNB_NO_ID);

  /* toggle button: normal */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox, WCONT_FREE);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiCreateToggleButton ("toggle button", NULL, NULL, 0);
  uiBoxPackStart (hbox, uiwidgetp, WCONT_FREE);

  uiBoxPostProcess (hbox);

  /* toggle button: tooltip */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox, WCONT_FREE);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiCreateToggleButton ("toggle tooltip", NULL, "tool-tip", 0);
  uiBoxPackStart (hbox, uiwidgetp, WCONT_FREE);

  uiBoxPostProcess (hbox);

  /* toggle button: with image */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox, WCONT_FREE);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiCreateToggleButton (
      "toggle image", uitest->images [UITEST_LED_OFF], "tool-tip", 0);
  uiToggleButtonSetCallback (uiwidgetp, uitest->callbacks [UITEST_CB_TOGGLE]);
  uiToggleButtonSetAltImage (uiwidgetp, uitest->images [UITEST_LED_ON]);
  uiBoxPackStart (hbox, uiwidgetp, WCONT_KEEP);
  uiWidgetAlignHorizCenter (uiwidgetp);
  uiWidgetAlignVertCenter (uiwidgetp);
  uitest->wcont [UITEST_W_TOGGLE_C] = uiwidgetp;

  uiBoxPostProcess (hbox);

  /* switch */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox, WCONT_FREE);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uitest->wcont [UITEST_W_SW] = uiCreateSwitch (1);
  uiBoxPackStart (hbox, uitest->wcont [UITEST_W_SW], WCONT_KEEP);

  uiBoxPostProcess (hbox);

  /* radio button */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox, WCONT_FREE);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiCreateRadioButton (NULL, "radio a", UI_TOGGLE_BUTTON_ON);
  uiBoxPackStart (hbox, uiwidgetp, WCONT_KEEP);
  twidgetp = uiwidgetp;

  uiBoxPostProcess (hbox);

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox, WCONT_FREE);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiCreateRadioButton (twidgetp, "radio b", UI_TOGGLE_BUTTON_OFF);
  uiBoxPackStart (hbox, uiwidgetp, WCONT_FREE);

  uiBoxPostProcess (hbox);
  uiwcontFree (twidgetp);

  /* check button */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox, WCONT_FREE);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiCreateCheckButton ("check button", UI_TOGGLE_BUTTON_OFF);
  uiBoxPackStart (hbox, uiwidgetp, WCONT_FREE);

  uiBoxPostProcess (hbox);
  uiBoxPostProcess (vbox);
  uiwcontFree (sg);
}

void
uitestUIMiscButtons (uitest_t *uitest)
{
  uiwcont_t   *vbox;
  uiwcont_t   *hbox;
  uiwcont_t   *sg;
  uiwcont_t   *uiwidgetp;

fprintf (stderr, "MiscButtons\n");
  sg = uiCreateSizeGroupHoriz ();

  /* misc buttons */

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);

  uivnbAppendPage (uitest->mainvnb, vbox, "Misc Buttons", VNB_NO_ID);

  /* font button */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox, WCONT_FREE);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiCreateFontButton ("Sans Bold 10");
  uiBoxPackStart (hbox, uiwidgetp, WCONT_FREE);

  uiBoxPostProcess (hbox);

  /* color button */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox, WCONT_FREE);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiCreateColorButton ("#aacc00");
  uiBoxPackStart (hbox, uiwidgetp, WCONT_FREE);

  uiBoxPostProcess (hbox);

  uiBoxPostProcess (vbox);
  uiwcontFree (sg);
}

void
uitestUIChgInd (uitest_t *uitest)
{
  uiwcont_t   *vbox;
  uiwcont_t   *hbox;
  uiwcont_t   *uiwidgetp;

fprintf (stderr, "ChgInd\n");
  /* change indicator */

  uitest->callbacks [UITEST_CB_CHG_IND] = callbackInit (
      uitestCBchgind, uitest, NULL);

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);

  uivnbAppendPage (uitest->mainvnb, vbox, "Change Indicator", VNB_NO_ID);

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox, WCONT_FREE);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiCreateChangeIndicator (hbox);
  uichgindMarkNormal (uiwidgetp);
  uitest->wcont [UITEST_W_CI_A] = uiwidgetp;

  uiwidgetp = uiCreateLabel ("aaa");
  uiBoxPackStart (hbox, uiwidgetp, WCONT_FREE);

  uiwidgetp = uiCreateButton (
      uitest->callbacks [UITEST_CB_CHG_IND], "switch", NULL, NULL);
  uiBoxPackStart (hbox, uiwidgetp, WCONT_KEEP);
  uitest->wcont [UITEST_W_CI_BUTTON] = uiwidgetp;

  uiBoxPostProcess (hbox);

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox, WCONT_FREE);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiCreateChangeIndicator (hbox);
  uichgindMarkChanged (uiwidgetp);
  uitest->wcont [UITEST_W_CI_B] = uiwidgetp;

  uiwidgetp = uiCreateLabel ("bbb");
  uiBoxPackStart (hbox, uiwidgetp, WCONT_FREE);

  uiBoxPostProcess (hbox);
  uiBoxPostProcess (vbox);
}

void
uitestUIDropdown (uitest_t *uitest)
{
  uiwcont_t   *vbox;

fprintf (stderr, "Dropdown\n");
  /* drop-downs */

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);

  uivnbAppendPage (uitest->mainvnb, vbox, "Drop-Down", VNB_NO_ID);

  uitest->callbacks [UITEST_CB_DD_STR] = callbackInitS (uitestDDStr, uitest);
  uitest->callbacks [UITEST_CB_DD_NUM] = callbackInitI (uitestDDNum, uitest);

  uitest->uidd [UITEST_DD_A] = uiddCreate ("dd-a",
      uitest->wcont [UITEST_W_WINDOW], vbox, DD_PACK_START,
      uitest->lista, DD_LIST_TYPE_STR,
      "Title A", DD_KEEP_TITLE,
      uitest->callbacks [UITEST_CB_DD_STR]);
  uiddSetSelection (uitest->uidd [UITEST_DD_A], 2);

  uitest->uidd [UITEST_DD_B] = uiddCreate ("dd-b",
      uitest->wcont [UITEST_W_WINDOW], vbox, DD_PACK_START,
      uitest->lista, DD_LIST_TYPE_STR,
      "Item 6", DD_REPLACE_TITLE,
      uitest->callbacks [UITEST_CB_DD_STR]);
  uiddSetSelection (uitest->uidd [UITEST_DD_B], 5);

  uitest->uidd [UITEST_DD_C] = uiddCreate ("dd-c",
      uitest->wcont [UITEST_W_WINDOW], vbox, DD_PACK_START,
      uitest->lista, DD_LIST_TYPE_NUM,
      "Title C", DD_KEEP_TITLE,
      uitest->callbacks [UITEST_CB_DD_NUM]);
  uiddSetSelection (uitest->uidd [UITEST_DD_C], 4);

  uitest->uidd [UITEST_DD_D] = uiddCreate ("dd-d",
      uitest->wcont [UITEST_W_WINDOW], vbox, DD_PACK_START,
      uitest->lista, DD_LIST_TYPE_NUM,
      "Item 4", DD_REPLACE_TITLE,
      uitest->callbacks [UITEST_CB_DD_NUM]);
  uiddSetSelection (uitest->uidd [UITEST_DD_D], 3);

  uitest->uidd [UITEST_DD_E] = uiddCreate ("dd-e",
      uitest->wcont [UITEST_W_WINDOW], vbox, DD_PACK_START,
      uitest->listb, DD_LIST_TYPE_NUM,
      "Item 1", DD_REPLACE_TITLE,
      uitest->callbacks [UITEST_CB_DD_NUM]);
  uiddSetSelection (uitest->uidd [UITEST_DD_E], 1);

  uiBoxPostProcess (vbox);
}

void
uitestUIEntry (uitest_t *uitest)
{
  uiwcont_t   *vbox;
  uiwcont_t   *uiwidgetp;

fprintf (stderr, "Entry\n");
  /* entry */

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);

  uivnbAppendPage (uitest->mainvnb, vbox, "Entry", VNB_NO_ID);

  uiwidgetp = uiEntryInit (10, 100);
  uiBoxPackStart (vbox, uiwidgetp, WCONT_FREE);

  uiwidgetp = uiEntryInit (10, 100);
  uiBoxPackStart (vbox, uiwidgetp, WCONT_FREE);

  uiBoxPostProcess (vbox);
}

void
uitestUIImage (uitest_t *uitest)
{
  uiwcont_t   *vbox;
  uiwcont_t   *uiwidgetp;
  char        tbuff [BDJ4_PATH_MAX];

fprintf (stderr, "Image\n");
  /* image */

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);

  uivnbAppendPage (uitest->mainvnb, vbox, "Image", VNB_NO_ID);

  pathbldMakePath (tbuff, sizeof (tbuff),
     "bdj4_icon", BDJ4_IMG_SVG_EXT, PATHBLD_MP_DIR_IMG);
  uiwidgetp = uiImageScaledFromFile (tbuff, 64);
  uiBoxPackStart (vbox, uiwidgetp, WCONT_KEEP);
  uitest->wcont [UITEST_W_IMG_A] = uiwidgetp;

  uiBoxPostProcess (vbox);
}

void
uitestUILabels (uitest_t *uitest)
{
  uiwcont_t   *vbox;
  uiwcont_t   *hbox;
  uiwcont_t   *uiwidgetp;

fprintf (stderr, "Labels\n");
  uiLabelAddClass ("ra", "#cc0000");
  uiLabelAddClass ("ga", "#00cc00");
  uiLabelAddClass ("ba", "#0000cc");
  uiLabelAddClass ("rb", "#cc5555");
  uiLabelAddClass ("gb", "#55cc55");
  uiLabelAddClass ("bb", "#5555cc");

  /* labels */

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);

  uivnbAppendPage (uitest->mainvnb, vbox, "Label", VNB_NO_ID);

  /* label: translated */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox, WCONT_FREE);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiCreateLabel (_("Actions"));
  uiBoxPackStart (hbox, uiwidgetp, WCONT_FREE);

  uiBoxPostProcess (hbox);

  /* label: pack start */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox, WCONT_FREE);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiCreateLabel ("ps-a");
  uiBoxPackStart (hbox, uiwidgetp, WCONT_FREE);
  uiWidgetSetClass (uiwidgetp, "gb");

  uiwidgetp = uiCreateLabel ("ps-b");
  uiBoxPackStart (hbox, uiwidgetp, WCONT_FREE);
  uiWidgetSetClass (uiwidgetp, "bb");

  uiwidgetp = uiCreateLabel ("ps-c");
  uiBoxPackStart (hbox, uiwidgetp, WCONT_FREE);
  uiWidgetSetClass (uiwidgetp, "rb");

  uiBoxPostProcess (hbox);

  /* label: pack start / expand horiz */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox, WCONT_FREE);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiCreateLabel ("ps-eh");
  uiBoxPackStart (hbox, uiwidgetp, WCONT_FREE);
  uiWidgetSetClass (uiwidgetp, "gb");
  uiWidgetExpandHoriz (uiwidgetp);

  uiwidgetp = uiCreateLabel ("ps-b");
  uiBoxPackStart (hbox, uiwidgetp, WCONT_FREE);
  uiWidgetSetClass (uiwidgetp, "bb");

  uiwidgetp = uiCreateLabel ("ps-c");
  uiBoxPackStart (hbox, uiwidgetp, WCONT_FREE);
  uiWidgetSetClass (uiwidgetp, "rb");

  uiBoxPostProcess (hbox);

  /* label: pack start / expand horiz / align center */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox, WCONT_FREE);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiCreateLabel ("ps-eh-ac");
  uiBoxPackStart (hbox, uiwidgetp, WCONT_FREE);
  uiWidgetSetClass (uiwidgetp, "gb");
  uiWidgetAlignHorizCenter (uiwidgetp);
  uiWidgetExpandHoriz (uiwidgetp);

  uiwidgetp = uiCreateLabel ("b");
  uiBoxPackStart (hbox, uiwidgetp, WCONT_FREE);
  uiWidgetSetClass (uiwidgetp, "bb");

  uiwidgetp = uiCreateLabel ("c");
  uiBoxPackStart (hbox, uiwidgetp, WCONT_FREE);
  uiWidgetSetClass (uiwidgetp, "rb");

  uiBoxPostProcess (hbox);

  /* label: pack start / expand horiz / align start */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox, WCONT_FREE);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiCreateLabel ("ps-eh-as");
  uiBoxPackStart (hbox, uiwidgetp, WCONT_FREE);
  uiWidgetSetClass (uiwidgetp, "gb");
  uiWidgetExpandHoriz (uiwidgetp);
  uiWidgetAlignHorizStart (uiwidgetp);

  uiwidgetp = uiCreateLabel ("b");
  uiBoxPackStart (hbox, uiwidgetp, WCONT_FREE);
  uiWidgetSetClass (uiwidgetp, "bb");

  uiwidgetp = uiCreateLabel ("c");
  uiBoxPackStart (hbox, uiwidgetp, WCONT_FREE);
  uiWidgetSetClass (uiwidgetp, "rb");

  uiBoxPostProcess (hbox);

  /* label: pack end */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox, WCONT_FREE);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiCreateLabel ("pe-a");
  uiBoxPackEnd (hbox, uiwidgetp, WCONT_FREE);
  uiWidgetSetClass (uiwidgetp, "gb");

  uiwidgetp = uiCreateLabel ("pe-b");
  uiBoxPackEnd (hbox, uiwidgetp, WCONT_FREE);
  uiWidgetSetClass (uiwidgetp, "bb");

  uiwidgetp = uiCreateLabel ("pe-c");
  uiBoxPackEnd (hbox, uiwidgetp, WCONT_FREE);
  uiWidgetSetClass (uiwidgetp, "rb");

  uiBoxPostProcess (hbox);

  /* label: pack start / pack end */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox, WCONT_FREE);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiCreateLabel ("ps-a");
  uiBoxPackStart (hbox, uiwidgetp, WCONT_FREE);
  uiWidgetSetClass (uiwidgetp, "gb");

  uiwidgetp = uiCreateLabel ("ps-b");
  uiBoxPackStart (hbox, uiwidgetp, WCONT_FREE);
  uiWidgetSetClass (uiwidgetp, "bb");

  uiwidgetp = uiCreateLabel ("ps-c");
  uiBoxPackStart (hbox, uiwidgetp, WCONT_FREE);
  uiWidgetSetClass (uiwidgetp, "rb");

  uiwidgetp = uiCreateLabel ("pe-a");
  uiBoxPackEnd (hbox, uiwidgetp, WCONT_FREE);
  uiWidgetSetClass (uiwidgetp, "gb");

  uiwidgetp = uiCreateLabel ("pe-b");
  uiBoxPackEnd (hbox, uiwidgetp, WCONT_FREE);
  uiWidgetSetClass (uiwidgetp, "bb");

  uiwidgetp = uiCreateLabel ("pe-c");
  uiBoxPackEnd (hbox, uiwidgetp, WCONT_FREE);
  uiWidgetSetClass (uiwidgetp, "rb");

  uiBoxPostProcess (hbox);

  /* label: pack start / ellipsize */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox, WCONT_FREE);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiCreateLabel ("ps-el ellipsize ellipsize ellipsize ellipsize ellipsize ellipsize ellipsize ellipsize ellipsize");
  uiBoxPackStart (hbox, uiwidgetp, WCONT_FREE);
  uiLabelEllipsizeOn (uiwidgetp);
  uiLabelSetMaxWidth (uiwidgetp, 20);
  uiWidgetSetClass (uiwidgetp, "gb");

  uiwidgetp = uiCreateLabel ("a");
  uiBoxPackStart (hbox, uiwidgetp, WCONT_FREE);
  uiWidgetSetClass (uiwidgetp, "bb");

  uiwidgetp = uiCreateLabel ("b");
  uiBoxPackStart (hbox, uiwidgetp, WCONT_FREE);
  uiWidgetSetClass (uiwidgetp, "rb");

  uiBoxPostProcess (hbox);

  /* label: pack start / accent */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox, WCONT_FREE);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiCreateLabel ("accent");
  uiBoxPackStart (hbox, uiwidgetp, WCONT_FREE);
  uiWidgetSetClass (uiwidgetp, ACCENT_CLASS);

  uiBoxPostProcess (hbox);

  /* label: pack start / bold-accent */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox, WCONT_FREE);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiCreateLabel ("bold-accent");
  uiBoxPackStart (hbox, uiwidgetp, WCONT_FREE);
  uiWidgetSetClass (uiwidgetp, BOLD_ACCENT_CLASS);

  uiBoxPostProcess (hbox);

  /* label: pack start / error */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox, WCONT_FREE);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiCreateLabel ("error");
  uiBoxPackStart (hbox, uiwidgetp, WCONT_FREE);
  uiWidgetSetClass (uiwidgetp, ERROR_CLASS);

  uiBoxPostProcess (hbox);

  /* label: pack start / dark-accent */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox, WCONT_FREE);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiCreateLabel ("dark-accent");
  uiBoxPackStart (hbox, uiwidgetp, WCONT_FREE);
  uiWidgetSetClass (uiwidgetp, DARKACCENT_CLASS);

  uiBoxPostProcess (hbox);
  uiBoxPostProcess (vbox);
}

void
uitestUILink (uitest_t *uitest)
{
  uiwcont_t   *vbox;
  uiwcont_t   *hbox;
  uiwcont_t   *uiwidgetp;

fprintf (stderr, "Link\n");
  uitest->callbacks [UITEST_CB_LINK_A] = callbackInit (
      uitestCBLink, uitest, NULL);

  /* link */

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);

  uivnbAppendPage (uitest->mainvnb, vbox, "Link", VNB_NO_ID);

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox, WCONT_FREE);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiCreateLink ("bdj", "https://ballroomdj.org");
  if (isMacOS ()) {
    uiLinkSetActivateCallback (uiwidgetp, uitest->callbacks [UITEST_CB_LINK_A]);
  }
  uiBoxPackStart (hbox, uiwidgetp, WCONT_KEEP);
  uitest->wcont [UITEST_W_LINK_A] = uiwidgetp;

  uiBoxPostProcess (hbox);
  uiBoxPostProcess (vbox);
}

void
uitestUINotebook (uitest_t *uitest)
{
  uiwcont_t   *vbox;
  uiwcont_t   *vboxb;
  uiwcont_t   *uiwidgetp;
  uivnb_t     *vnb;
  uihnb_t     *hnb;

fprintf (stderr, "Notebook\n");
  /* notebook */

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);
  uiWidgetExpandVert (vbox);
  uiWidgetExpandHoriz (vbox);

  uivnbAppendPage (uitest->mainvnb, vbox, "Notebook", VNB_NO_ID);

  /* hnb */

fprintf (stderr, "hnb-a\n");
  hnb = uihnbCreate (vbox, "uitest-hnb-a");
  uitest->hnb = hnb;

  /* hnb %d */

  for (int i = 1; i < 4; ++i) {
    char    tbuff [200];

    snprintf (tbuff, sizeof (tbuff), "Horiz %d", i);
    vboxb = uiCreateVertBox ();
    uihnbAppendPage (hnb, vboxb, tbuff, NULL, NULL, HNB_NO_ID);

    uiwidgetp = uiCreateLabel (tbuff);
    uiBoxPackStart (vboxb, uiwidgetp, WCONT_FREE);
    uiBoxPostProcess (vboxb);
  }

  uihnbPostProcess (hnb);

  /* hnb w/images */

fprintf (stderr, "hnb-img\n");
  hnb = uihnbCreate (vbox, "uitest-hnb-img");
  uitest->hnbimg = hnb;

  /* horiz-img 1 */

  vboxb = uiCreateVertBox ();
  uiwidgetp = uiCreateLabel ("h-img One");
  uiBoxPackStart (vboxb, uiwidgetp, WCONT_FREE);

  uihnbAppendPage (hnb, vboxb, "h-img 1",
      uitest->images [UITEST_LED_OFF],
      uitest->images [UITEST_LED_ON], HNB_NO_ID);
  uiBoxPostProcess (vboxb);

  /* horiz-img 2 */

  vboxb = uiCreateVertBox ();
  uiwidgetp = uiCreateLabel ("h-img Two");
  uiBoxPackStart (vboxb, uiwidgetp, WCONT_FREE);

  uihnbAppendPage (hnb, vboxb, "h-img 2",
      uitest->images [UITEST_LED_OFF],
      uitest->images [UITEST_LED_ON], HNB_NO_ID);
  uiBoxPostProcess (vboxb);

  /* horiz-img 3 */

  vboxb = uiCreateVertBox ();
  uiwidgetp = uiCreateLabel ("h-img Three");
  uiBoxPackStart (vboxb, uiwidgetp, WCONT_FREE);

  uihnbAppendPage (hnb, vboxb, "h-img 3", NULL, NULL, HNB_NO_ID);
  uiBoxPostProcess (vboxb);

  /* action widget */

  uitest->callbacks [UITEST_CB_NB_ACTION] = callbackInit (
      uitestCBNBAction, uitest, NULL);
  uiwidgetp = uiCreateButton (
      uitest->callbacks [UITEST_CB_NB_ACTION], "Action", NULL, NULL);
  uihnbSetActionWidget (hnb, uiwidgetp);
  uitest->wcont [UITEST_W_NB_ACTION] = uiwidgetp;

  uihnbPostProcess (hnb);

  /* VNB */

fprintf (stderr, "vnb-a\n");
  vnb = uivnbCreate (vbox, "uitest-vnb-a");
  uitest->vnb = vnb;

  /* VNB pages */

  for (int i = 1; i < 10; ++i) {
    char    tbuff [200];

    snprintf (tbuff, sizeof (tbuff), "VNB %d", i);
    vboxb = uiCreateVertBox ();
    uiWidgetSetAllMargins (vboxb, 4);
    uivnbAppendPage (vnb, vboxb, tbuff, VNB_NO_ID);

    uiwidgetp = uiCreateLabel (tbuff);
    uiBoxPackStart (vboxb, uiwidgetp, WCONT_FREE);
    uiBoxPostProcess (vboxb);
  }
  uivnbPostProcess (vnb);

  /* main vert box display */

  uiBoxPostProcess (vbox);
}

void
uitestUIPanedWin (uitest_t *uitest)
{
  uiwcont_t   *vbox;
  uiwcont_t   *pw;
  uiwcont_t   *tvbox;
  uiwcont_t   *uiwidgetp;

fprintf (stderr, "PanedWin\n");
  /* paned window */

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);

  uivnbAppendPage (uitest->mainvnb, vbox, "Paned Window", VNB_NO_ID);

  pw = uiPanedWindowCreateVert ();
  uiBoxPackStartExpandChildren (vbox, pw, WCONT_FREE);
  uiWidgetExpandHoriz (pw);
  uiWidgetAlignHorizFill (pw);
  uiWidgetSetClass (pw, ACCENT_CLASS);

  tvbox = uiCreateVertBox ();
  uiPanedWindowPackStart (pw, tvbox);

  uiwidgetp = uiCreateLabel ("AAA");
  uiBoxPackStart (tvbox, uiwidgetp, WCONT_FREE);
  uiBoxPostProcess (tvbox);
  uiwcontFree (tvbox);

  tvbox = uiCreateVertBox ();
  uiPanedWindowPackEnd (pw, tvbox);
  uiwidgetp = uiCreateLabel ("BBB");
  uiBoxPackStart (tvbox, uiwidgetp, WCONT_FREE);

  uiBoxPostProcess (tvbox);
  uiBoxPostProcess (vbox);
  uiwcontFree (tvbox);
}

void
uitestUIMisc (uitest_t *uitest)
{
  uiwcont_t   *vbox;
  uiwcont_t   *uiwidgetp;

fprintf (stderr, "Misc\n");
  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);

  uivnbAppendPage (uitest->mainvnb, vbox, "Misc", VNB_NO_ID);

  /* progress bar */
  uiwidgetp = uiCreateProgressBar ();
  uiBoxPackStart (vbox, uiwidgetp, WCONT_FREE);
  uiProgressBarSet (uiwidgetp, 0.33);
  uiWidgetSetMarginTop (uiwidgetp, 4);

  /* scale */
  uiwidgetp = uiCreateScale (0.0, 100.0, 1.0, 10.0, 33.0, 0);
  uiBoxPackStart (vbox, uiwidgetp, WCONT_FREE);
  uiWidgetSetMarginTop (uiwidgetp, 4);

  /* separator */
  uiSeparatorAddClass (ACCENT_CLASS, bdjoptGetStr (OPT_P_UI_ACCENT_COL));

  uiwidgetp = uiCreateHorizSeparator ();
  uiWidgetSetClass (uiwidgetp, ACCENT_CLASS);
  uiBoxPackStart (vbox, uiwidgetp, WCONT_FREE);
  uiWidgetExpandHoriz (uiwidgetp);
  uiWidgetSetMarginTop (uiwidgetp, 4);

  uiBoxPostProcess (vbox);
}

void
uitestUISizeGroup (uitest_t *uitest)
{
  uiwcont_t   *vbox;
  uiwcont_t   *hbox;
  uiwcont_t   *sg;
  uiwcont_t   *uiwidgetp;

fprintf (stderr, "SizeGroup\n");
  /* size group */

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);

  uivnbAppendPage (uitest->mainvnb, vbox, "Size Group", VNB_NO_ID);

  sg = uiCreateSizeGroupHoriz ();

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox, WCONT_FREE);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiCreateLabel ("aaa");
  uiBoxPackStart (hbox, uiwidgetp, WCONT_FREE);
  uiSizeGroupAdd (sg, uiwidgetp);

  uiwidgetp = uiCreateLabel ("aaa");
  uiBoxPackStart (hbox, uiwidgetp, WCONT_FREE);

  uiwidgetp = uiCreateLabel ("aaa");
  uiBoxPackStart (hbox, uiwidgetp, WCONT_FREE);

  uiBoxPostProcess (hbox);

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox, WCONT_FREE);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiCreateLabel ("bbbb");
  uiBoxPackStart (hbox, uiwidgetp, WCONT_FREE);
  uiSizeGroupAdd (sg, uiwidgetp);

  uiwidgetp = uiCreateLabel ("bbbb");
  uiBoxPackStart (hbox, uiwidgetp, WCONT_FREE);

  uiwidgetp = uiCreateLabel ("bbbb");
  uiBoxPackStart (hbox, uiwidgetp, WCONT_FREE);

  uiBoxPostProcess (hbox);

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox, WCONT_FREE);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiCreateLabel ("c");
  uiBoxPackStart (hbox, uiwidgetp, WCONT_FREE);
  uiSizeGroupAdd (sg, uiwidgetp);

  uiwidgetp = uiCreateLabel ("c");
  uiBoxPackStart (hbox, uiwidgetp, WCONT_FREE);

  uiwidgetp = uiCreateLabel ("c");
  uiBoxPackStart (hbox, uiwidgetp, WCONT_FREE);

  uiBoxPostProcess (hbox);

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox, WCONT_FREE);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiCreateLabel ("ddddddd");
  uiBoxPackStart (hbox, uiwidgetp, WCONT_FREE);
  uiSizeGroupAdd (sg, uiwidgetp);

  uiwidgetp = uiCreateLabel ("ddddddd");
  uiBoxPackStart (hbox, uiwidgetp, WCONT_FREE);

  uiwidgetp = uiCreateLabel ("ddddddd");
  uiBoxPackStart (hbox, uiwidgetp, WCONT_FREE);

  uiBoxPostProcess (hbox);

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox, WCONT_FREE);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uiwidgetp = uiCreateLabel ("eeeeee");
  uiBoxPackStart (hbox, uiwidgetp, WCONT_FREE);
  uiSizeGroupAdd (sg, uiwidgetp);

  uiwidgetp = uiCreateLabel ("eeeeee");
  uiBoxPackStart (hbox, uiwidgetp, WCONT_FREE);

  uiwidgetp = uiCreateLabel ("eeeeee");
  uiBoxPackStart (hbox, uiwidgetp, WCONT_FREE);

  uiBoxPostProcess (hbox);
  uiBoxPostProcess (vbox);
  uiwcontFree (sg);
}

void
uitestUISpinbox (uitest_t *uitest)
{
  uiwcont_t   *vbox;
  uiwcont_t   *hbox;
  uisbnum_t   *sb;

fprintf (stderr, "Spinbox\n");
  /* spinboxes */

  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);

  uivnbAppendPage (uitest->mainvnb, vbox, "Spin Box", VNB_NO_ID);

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox, WCONT_FREE);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uitest->sbtxtlist = nlistAlloc ("sbtxtlist", LIST_ORDERED, NULL);
  nlistSetStr (uitest->sbtxtlist, 0, "aaa");
  nlistSetStr (uitest->sbtxtlist, 1, "bbbb");
  nlistSetStr (uitest->sbtxtlist, 2, "ccc");
  nlistSetStr (uitest->sbtxtlist, 3, "ddddddddd");

  uitest->sbtext = uisbtextCreate (hbox, 4);
  uisbtextSetList (uitest->sbtext, uitest->sbtxtlist);
  uisbtextSetValue (uitest->sbtext, 0);

  uiBoxPostProcess (hbox);

  /* next line */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox, WCONT_FREE);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  uitest->sbtextb = uisbtextCreate (hbox, 4);
  uisbtextPrependList (uitest->sbtextb, "All");
  uisbtextSetList (uitest->sbtextb, uitest->sbtxtlist);
  uisbtextSetValue (uitest->sbtextb, -1);

  uiBoxPostProcess (hbox);

  /* next line - int */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox, WCONT_FREE);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  sb = uisbnumCreate (hbox, "a", 0);
  uisbnumSetLimits (sb, 3.0, 15.0, 0);
  uisbnumSetIncrements (sb, 1.0, 5.0);
  uisbnumSetValue (sb, 4.0);
  uitest->sbnum [UITEST_SB_INT] = sb;

  uiBoxPostProcess (hbox);

  /* next line - double */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox, WCONT_FREE);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  sb = uisbnumCreate (hbox, "dbl", 0);
  uisbnumSetLimits (sb, 1.0, 20.0, 1);
  uisbnumSetIncrements (sb, 0.1, 1.0);
  uisbnumSetValue (sb, 5.0);
  uitest->sbnum [UITEST_SB_DBL] = sb;

  uiBoxPostProcess (hbox);

  /* next line - double */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox, WCONT_FREE);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  sb = uisbnumCreate (hbox, "dbl-b", 0);
  uisbnumSetLimits (sb, 1.0, 20.0, 1);
  uisbnumSetIncrements (sb, 0.1, 5.0);
  uisbnumSetValue (sb, 5.0);
  uitest->sbnum [UITEST_SB_DBL_B] = sb;

  uiBoxPostProcess (hbox);

  /* next line - double/default */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox, WCONT_FREE);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  sb = uisbnumCreate (hbox, "dbl-def", 0);
  uisbnumSetLimits (sb, 0.0, 10.0, 1);
  uisbnumSetIncrements (sb, 0.1, 5.0);
  uisbnumSetType (sb, SBNUM_NUM_DEFAULT);
  uisbnumSetValue (sb, -0.1);
  uitest->sbnum [UITEST_SB_DBL_DFLT] = sb;

  uiBoxPostProcess (hbox);

  /* next line - time spinbox */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox, WCONT_FREE);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  sb = uisbnumCreate (hbox, "tm-a", 0);
  uisbnumSetTime (sb, 0.0, 1440000.0, SBNUM_TIME_BASIC);
  uisbnumSetValue (sb, 5.0);
  uitest->sbnum [UITEST_SB_TM_A] = sb;

  uiBoxPostProcess (hbox);

  /* next line - time spinbox/precise */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox, WCONT_FREE);
  uiWidgetSetAllMargins (hbox, 1);
  uiWidgetExpandHoriz (hbox);

  sb = uisbnumCreate (hbox, "tm-b", 0);
  uisbnumSetTime (sb, 0.0, 1440000.0, SBNUM_TIME_PRECISE);
  uisbnumSetValue (sb, 5.0);
  uitest->sbnum [UITEST_SB_TM_B] = sb;

  uiBoxPostProcess (hbox);
  uiBoxPostProcess (vbox);
}

void
uitestUITextBox (uitest_t *uitest)
{
  uiwcont_t   *vbox;
  uiwcont_t   *tb;

fprintf (stderr, "TextBox\n");
  vbox = uiCreateVertBox ();
  uiWidgetSetAllMargins (vbox, 4);

  uivnbAppendPage (uitest->mainvnb, vbox, "Text Box", VNB_NO_ID);

  /* text box */
  tb = uiTextBoxCreate (200, NULL);
  uiTextBoxHorizExpand (tb);
  uiTextBoxVertExpand (tb);
  uiBoxPackStartExpandChildren (vbox, tb, WCONT_FREE);

  uiBoxPostProcess (vbox);
}

void
uitestUIVirtList (uitest_t *uitest)
{
  uiwcont_t   *hbox;

fprintf (stderr, "VirtList\n");
  hbox = uiCreateHorizBox ();
  uiWidgetSetAllMargins (hbox, 4);

  uivnbAppendPage (uitest->mainvnb, hbox, "Virtual List", VNB_NO_ID);

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

  uiBoxPostProcess (hbox);
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
uitestCBButtonImgC (void *udata)
{
  uitest_t  *uitest = udata;
  int       state;
  char      msg [40];

  state = uiButtonGetState (uitest->wcont [UITEST_W_B_IMG_C]);
  uiButtonSetState (uitest->wcont [UITEST_W_B_IMG_C], 1 - state);
  snprintf (msg, sizeof (msg), "state: %d", 1 - state);
  uiLabelSetText (uitest->wcont [UITEST_W_B_IMG_C_MSG], msg);

  return UICB_CONT;
}

static bool
uitestCBNBAction (void *udata)
{
  uitest_t  *uitest = udata;

  if (uitest == NULL) {
    return UICB_STOP;
  }

  uitest->hnbimgsel = 1 - uitest->hnbimgsel;
  uihnbSelect (uitest->hnbimg, uitest->hnbimgsel);

  return UICB_CONT;
}

static bool
uitestCBNull (void *udata)
{
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

  uiutilsHeaderLineFree (uitest->hdrline);
  for (int i = 0; i < UITEST_DD_MAX; ++i) {
    uiddFree (uitest->uidd [i]);
  }
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
    dataFree (uitest->images [i]);
  }

  uisbtextFree (uitest->sbtext);
  uisbtextFree (uitest->sbtextb);
  for (int i = 0; i < UITEST_SB_MAX; ++i) {
    uisbnumFree (uitest->sbnum [i]);
  }

  uivnbFree (uitest->vnb);
  uihnbFree (uitest->hnb);
  uihnbFree (uitest->hnbimg);
  nlistFree (uitest->sbtxtlist);

  uivnbFree (uitest->mainvnb);

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
  char        tmp [BDJ4_PATH_MAX];

  snprintf (tmp, sizeof (tmp), "%s '%s'", sysvarsGetStr (SV_PATH_URI_OPEN),
      "https://ballroomdj.org");
  (void) ! system (tmp);
  return UICB_STOP;
}
