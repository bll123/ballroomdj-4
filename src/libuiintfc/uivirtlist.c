/*
 * Copyright 2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "bdj4.h"
#include "callback.h"
#include "log.h"
#include "mdebug.h"
#include "nlist.h"
#include "tmutil.h"
#include "ui.h"
#include "uiclass.h"
#include "uivirtlist.h"
#include "uiwcont.h"

/* initialization states */
enum {
  VL_INIT_NONE,
  VL_INIT_BASIC,
  VL_INIT_ROWS,
  VL_INIT_DISP,
};

enum {
  VL_SCROLL_NORM,
  VL_SCROLL_KEY,
  VL_SCROLL_FORCE,
};

enum {
  VL_ROW_UNKNOWN = -753,
  VL_MIN_WIDTH_ANY = -755,
  VL_ROW_HEADING = -754,
  VL_ROW_HEADING_IDX = 0,
  VL_DOUBLE_CLICK_TIME = 250,   // milliseconds
  VL_ROW_NO_LOCK = -1,
  VL_UNK_ROW = -1,
  VL_REUSE_HEIGHT = -2,
};


enum {
  VL_W_VBOX,            // contains scroll-win
  VL_W_SCROLL_WIN,      // contains hbox-cont
  VL_W_HBOX_CONT,       // contains event-box and scrollbar
  VL_W_EVENT_BOX,       // contains main-vbox
  VL_W_MAIN_VBOX,       // has all the rows
  VL_W_SB_VBOX,         // contains optional head-filler, scrollbar
  VL_W_SB,
  VL_W_EVENTH,
  VL_W_MAX,
};

enum {
  VL_CB_SB,
  VL_CB_KEY,
  VL_CB_MBUTTON,
  VL_CB_SCROLL,
  VL_CB_VERT_SZ_CHG,
  VL_CB_HEADING_SZ_CHG,
  VL_CB_ROW_SZ_CHG,
  VL_CB_ENTER_WIN,
  VL_CB_MOTION_WIN,
  VL_CB_MAX,
};

enum {
  VL_USER_CB_ENTRY_VAL,
  VL_USER_CB_RB_CHG,
  VL_USER_CB_CB_CHG,
  VL_USER_CB_KEY,
  VL_USER_CB_DISP_CHG,
  VL_USER_CB_MAX,
};

enum {
  VL_IDENT_COLDATA  = 0x0074646c6f636c76,
  VL_IDENT_COL      = 0xccbb006c6f636c76,
  VL_IDENT_ROW      = 0xccbb00776f726c76,
  VL_IDENT          = 0xffeeddccbb006c76,
};

typedef struct uivlcol uivlcol_t;
typedef struct uivlcoldata uivlcoldata_t;
typedef struct uivlrow uivlrow_t;

typedef struct {
  uivirtlist_t    *vl;
  int             dispidx;
  callback_t      *focuscb;
} uivlrowcb_t;

typedef struct uivlcoldata {
  uint64_t      ident;
  uivirtlist_t  *vl;
  uiwcont_t     *szgrp;
  /* the following data is specific to a column */
  vltype_t      type;
  /* the baseclass is always applied */
  uientryval_t  entrycb;
  void          *entryudata;
  callback_t    *togglecb;      // radio buttons and check buttons
  callback_t    *spinboxcb;
  callback_t    *spinboxtimecb;
  callback_t    *colgrowonlycb;
  uiwcont_t     *extrarb;
  uivlcol_t     *col0;
  const char    *tag;
  char          *heading;
  char          *baseclass;
  int           minwidth;
  int           entrySz;
  int           entryMaxSz;
  int           sbtype;
  int           colidx;
  int           colwidth;
  int           clickmap;
  callback_t    *sbcb;
  double        sbmin;
  double        sbmax;
  double        sbincr;
  double        sbpageincr;
  int           grow;
  /* is the entire column hidden? */
  /* note that this is not a true/false value */
  int           hidden;
  bool          alignend: 1;
  bool          aligncenter: 1;
  bool          ellipsize : 1;
} uivlcoldata_t;

typedef struct uivlcol {
  uint64_t    ident;
  uiwcont_t   *uiwidget;
  /* class needs to be held temporarily so it can be removed */
  char        *class;
  int         colidx;
  int32_t     value;        // internal numeric value
} uivlcol_t;

typedef struct uivlrow {
  uint64_t      ident;
  uivirtlist_t  *vl;
  uiwcont_t     *hbox;
  /* the row size-group is used to set all of the ellipsized columns */
  /* in a row to the same size so that their widths don't bounce */
  /* around when scrolling */
  uiwcont_t     *szgrp;
  uivlcol_t     *cols;
  uivlrowcb_t   *rowcb;             // must have a stable address
  /* applied to every column in the row */
  char          *oldclass;
  char          *currclass;
  char          *newclass;
  int           dispidx;
  int32_t       lockrownum;
  /* cleared: row is on-screen, no display */
  /* all column widgets are hidden */
  bool          cleared : 1;
  bool          created : 1;
  /* offscreen: row is off-screen, entire row-box is hidden */
  bool          offscreen : 1;
  bool          initialized : 1;
  bool          selected : 1;       // a temporary flag to ease processing
} uivlrow_t;

/* need a container of stable allocated addresses */
typedef struct {
  uivlrowcb_t     *rowcb;
} uivlcbcont_t;

typedef struct uivirtlist {
  uint64_t      ident;
  const char    *tag;
  uiwcont_t     *parentwin;
  uiwcont_t     *wcont [VL_W_MAX];
  callback_t    *callbacks [VL_CB_MAX];
  callback_t    *usercb [VL_USER_CB_MAX];
  mstime_t      doubleClickCheck;
  int           numcols;
  uivlcoldata_t *coldata;
  uivlrow_t     *rows;
  /* the actual number of rows that can be displayed */
  int           dispsize;
  int           dispalloc;
  int           lastdisphighlight;
  int           headingoffset;
  int           vboxheight;
  int           headingheight;
  int           rowheight;
  int           lastselidx;       // used to know when to clear the focus
  int32_t       numrows;
  int32_t       rowoffset;
  int32_t       lastSelection;    // used for shift-click
  int32_t       currSelection;
  nlist_t       *selected;
  int           lockcount;
  int           initialized;
  /* user callbacks */
  uivlfillcb_t  fillcb;
  void          *filludata;
  uivlselcb_t   selchgcb;
  void          *seludata;
  uivlselcb_t   rowselcb;
  void          *rowseludata;
  uivlselcb_t   dblclickcb;
  void          *dblclickudata;
  uivlselcb_t   rightclickcb;
  void          *rightclickudata;
  /* flags */
  bool          allowdblclick : 1;
  bool          allowmultiple : 1;
  bool          darkbg : 1;
  bool          dispheading : 1;
  bool          inscroll : 1;
  bool          keyhandling : 1;
  bool          uselistingfont : 1;
  bool          numrowchg : 1;
} uivirtlist_t;

static void uivlFreeRow (uivirtlist_t *vl, uivlrow_t *row);
static void uivlFreeCol (uivlcol_t *col);
static void uivlCreateRow (uivirtlist_t *vl, uivlrow_t *row, int dispidx, bool isheading);
static uivlrow_t * uivlGetRow (uivirtlist_t *vl, int32_t rownum);
static void uivlPackRow (uivirtlist_t *vl, uivlrow_t *row);
static bool uivlScrollbarCallback (void *udata, double value);
static bool uivlKeyEvent (void *udata);
static bool uivlMButtonEvent (void *udata, int32_t dispidx, int32_t colidx);
static bool uivlScrollEvent (void *udata, int32_t dir);
static bool uivlMotionEvent (void *udata, int32_t dispidx);
static void uivlClearDisplaySelections (uivirtlist_t *vl);
static void uivlSetDisplaySelections (uivirtlist_t *vl);
static void uivlClearAllSelections (uivirtlist_t *vl);
static void uivlClearSelection (uivirtlist_t *vl, int32_t rownum);
static void uivlAddSelection (uivirtlist_t *vl, uint32_t rownum);
static void uivlProcessScroll (uivirtlist_t *vl, int32_t start, int sctype);
static bool uivlVertSizeChg (void *udata, int32_t width, int32_t height);
static bool uivlHeadingSizeChg (void *udata, int32_t width, int32_t height);
static bool uivlRowSizeChg (void *udata, int32_t width, int32_t height);
static bool uivlColGrowOnlyChg (void *udata, int32_t width, int32_t height);
static void uivlRowBasicInit (uivirtlist_t *vl, uivlrow_t *row, int dispidx);
static bool uivlFocusCallback (void *udata);
static void uivlUpdateSelections (uivirtlist_t *vl, int32_t rownum);
int32_t uivlRowOffsetLimit (uivirtlist_t *vl, int32_t rowoffset);
int32_t uivlRownumLimit (uivirtlist_t *vl, int32_t rownum);
static void uivlSelectChgHandler (uivirtlist_t *vl, int32_t rownum, int32_t colidx);
static void uivlDisplayChgHandler (uivirtlist_t *vl);
static void uivlRowClickHandler (uivirtlist_t *vl, int32_t rownum, int32_t colidx);
static void uivlDoubleClickHandler (uivirtlist_t *vl, int32_t rownum, int32_t colidx);
static void uivlRightClickHandler (uivirtlist_t *vl, int32_t rownum, int32_t colidx);
static void uivlSetToggleChangeCallback (uivirtlist_t *vl, int colidx, callback_t *cb);
static void uivlClearRowDisp (uivirtlist_t *vl, int dispidx);
static bool uivlValidateColumn (uivirtlist_t *vl, int initstate, int colidx, const char *func);
static bool uivlValidateRowColumn (uivirtlist_t *vl, int initstate, int32_t rownum, int colidx, const char *func);
static void uivlChangeDisplaySize (uivirtlist_t *vl, int newdispsize);
static void uivlConfigureScrollbar (uivirtlist_t *vl);
static int uivlCalcDispidx (uivirtlist_t *vl, int32_t rownum);
static int32_t uivlCalcRownum (uivirtlist_t *vl, int dispidx);
static void uivlShowRow (uivirtlist_t *vl, uivlrow_t *row);
static bool uivlEnterEvent (void *udata);
static void uivlCheckDisplay (uivirtlist_t *vl);
static void uivlRemoveLastHighlight (uivirtlist_t *vl);

/* listings with focusable widgets should pass in the parent window */
/* parameter.  otherwise, it can be null */
/* the initial display size must be smaller than the initial box */
/* otherwise the scrollbar does not get sized properly. */
uivirtlist_t *
uivlCreate (const char *tag, uiwcont_t *parentwin, uiwcont_t *boxp,
    int dispsize, int minwidth, int vlflags)
{
  uivirtlist_t  *vl;

  logProcBegin ();
  vl = mdmalloc (sizeof (uivirtlist_t));
  vl->ident = VL_IDENT;
  vl->tag = tag;
  vl->parentwin = parentwin;
  vl->fillcb = NULL;
  vl->filludata = NULL;
  vl->selchgcb = NULL;
  vl->seludata = NULL;
  vl->rowselcb = NULL;
  vl->rowseludata = NULL;
  vl->dblclickcb = NULL;
  vl->dblclickudata = NULL;
  vl->rightclickcb = NULL;
  vl->rightclickudata = NULL;
  vl->inscroll = false;
  vl->dispheading = true;

  vl->dispalloc = 0;
  vl->lastdisphighlight = VL_UNK_ROW;
  /* display size is set to the number of rows that can be displayed */
  /* it includes the heading row if headings are on */
  vl->dispsize = dispsize;
  /* display offset excludes the heading rows */
  vl->headingoffset = 1;
  if ((vlflags & VL_NO_HEADING) == VL_NO_HEADING) {
    vl->dispheading = false;
    vl->headingoffset = 0;
  }

  vl->keyhandling = false;
  if ((vlflags & VL_ENABLE_KEYS) == VL_ENABLE_KEYS) {
    vl->keyhandling = true;
  }
  vl->allowmultiple = false;
  vl->allowdblclick = false;
  vl->darkbg = false;
  vl->uselistingfont = false;
  vl->numrowchg = false;
  vl->vboxheight = 0;
  vl->headingheight = 0;
  vl->rowheight = 0;
  vl->lastselidx = 0;
  vl->lockcount = 0;
  vl->coldata = NULL;
  vl->rows = NULL;
  vl->numcols = 0;
  vl->numrows = 0;
  vl->rowoffset = 0;
  vl->currSelection = 0;
  vl->selected = nlistAlloc ("vl-selected", LIST_ORDERED, NULL);
  /* default selection */
  nlistSetNum (vl->selected, 0, true);
  vl->initialized = VL_INIT_NONE;

  for (int i = 0; i < VL_W_MAX; ++i) {
    vl->wcont [i] = NULL;
  }
  for (int i = 0; i < VL_CB_MAX; ++i) {
    vl->callbacks [i] = NULL;
  }
  for (int i = 0; i < VL_USER_CB_MAX; ++i) {
    vl->usercb [i] = NULL;
  }

  vl->wcont [VL_W_EVENTH] = uiEventAlloc ();
  if (vl->keyhandling) {
    vl->callbacks [VL_CB_KEY] = callbackInit (uivlKeyEvent, vl, NULL);
  }
  vl->callbacks [VL_CB_MBUTTON] = callbackInitII (uivlMButtonEvent, vl);
  vl->callbacks [VL_CB_SCROLL] = callbackInitI (uivlScrollEvent, vl);
  vl->callbacks [VL_CB_VERT_SZ_CHG] = callbackInitII (uivlVertSizeChg, vl);
  vl->callbacks [VL_CB_HEADING_SZ_CHG] = callbackInitII (uivlHeadingSizeChg, vl);
  vl->callbacks [VL_CB_ROW_SZ_CHG] = callbackInitII (uivlRowSizeChg, vl);
  vl->callbacks [VL_CB_ENTER_WIN] = callbackInit (uivlEnterEvent, vl, NULL);
  vl->callbacks [VL_CB_MOTION_WIN] = callbackInitI (uivlMotionEvent, vl);

  vl->wcont [VL_W_VBOX] = uiCreateVertBox ();
  uiWidgetAlignHorizFill (vl->wcont [VL_W_VBOX]);
  uiBoxPackStartExpand (boxp, vl->wcont [VL_W_VBOX]);

  /* a scrolled window is necessary to allow the window to shrink */
  /* keep the minimum height very small so that drop-downs do not get */
  /* created with extra space */
  vl->wcont [VL_W_SCROLL_WIN] = uiCreateScrolledWindow (50);
  uiWindowSetPolicyExternal (vl->wcont [VL_W_SCROLL_WIN]);
  uiBoxPackStartExpand (vl->wcont [VL_W_VBOX], vl->wcont [VL_W_SCROLL_WIN]);

  vl->wcont [VL_W_HBOX_CONT] = uiCreateHorizBox ();
  uiWindowPackInWindow (vl->wcont [VL_W_SCROLL_WIN], vl->wcont [VL_W_HBOX_CONT]);
  /* need a minimum width so it looks nice */
  if (minwidth != VL_NO_WIDTH) {
    uiWidgetSetSizeRequest (vl->wcont [VL_W_HBOX_CONT], minwidth, -1);
  }

  vl->wcont [VL_W_MAIN_VBOX] = uiCreateVertBox ();
  uiWidgetExpandHoriz (vl->wcont [VL_W_MAIN_VBOX]);
  if (vl->keyhandling) {
    /* in order to handle key events */
    uiWidgetEnableFocus (vl->wcont [VL_W_MAIN_VBOX]);
  }

  /* the event box is necessary to receive mouse clicks */
  vl->wcont [VL_W_EVENT_BOX] = uiEventCreateEventBox (vl->wcont [VL_W_MAIN_VBOX]);
  uiBoxPackStartExpand (vl->wcont [VL_W_HBOX_CONT], vl->wcont [VL_W_EVENT_BOX]);
  uiWidgetSetEnterCallback (vl->wcont [VL_W_EVENT_BOX], vl->callbacks [VL_CB_ENTER_WIN]);

  /* important */
  /* the size change callback must be set on the scroll-window */
  /* as the child windows within it only grow */
  uiWidgetSetSizeChgCallback (vl->wcont [VL_W_SCROLL_WIN], vl->callbacks [VL_CB_VERT_SZ_CHG]);

  vl->wcont [VL_W_SB_VBOX] = uiCreateVertBox ();
  uiBoxPackEnd (vl->wcont [VL_W_HBOX_CONT], vl->wcont [VL_W_SB_VBOX]);

  vl->wcont [VL_W_SB] = uiCreateVerticalScrollbar (10.0);
  uiBoxPackEndExpand (vl->wcont [VL_W_SB_VBOX], vl->wcont [VL_W_SB]);

  vl->callbacks [VL_CB_SB] = callbackInitD (uivlScrollbarCallback, vl);
  uiScrollbarSetStepIncrement (vl->wcont [VL_W_SB], 1.0);
  uivlConfigureScrollbar (vl);
  uiScrollbarSetPosition (vl->wcont [VL_W_SB], 0.0);
  uiScrollbarSetChangeCallback (vl->wcont [VL_W_SB], vl->callbacks [VL_CB_SB]);

  logMsg (LOG_DBG, LOG_VIRTLIST, "vl: %s init-disp-size: %d", vl->tag, dispsize);
  vl->dispalloc = dispsize;
  vl->rows = mdmalloc (sizeof (uivlrow_t) * vl->dispalloc);
  for (int dispidx = 0; dispidx < vl->dispalloc; ++dispidx) {
    uivlRowBasicInit (vl, &vl->rows [dispidx], dispidx);
  }

  if (vl->keyhandling) {
    uiEventSetKeyCallback (vl->wcont [VL_W_EVENTH], vl->wcont [VL_W_MAIN_VBOX],
        vl->callbacks [VL_CB_KEY]);
  }
  uiEventSetButtonCallback (vl->wcont [VL_W_EVENTH], vl->wcont [VL_W_EVENT_BOX],
      vl->callbacks [VL_CB_MBUTTON]);
  uiEventSetScrollCallback (vl->wcont [VL_W_EVENTH], vl->wcont [VL_W_EVENT_BOX],
      vl->callbacks [VL_CB_SCROLL]);
  uiEventSetButtonCallback (vl->wcont [VL_W_EVENTH], vl->wcont [VL_W_MAIN_VBOX],
      vl->callbacks [VL_CB_MBUTTON]);
  uiEventSetScrollCallback (vl->wcont [VL_W_EVENTH], vl->wcont [VL_W_MAIN_VBOX],
      vl->callbacks [VL_CB_SCROLL]);
  uiEventSetMotionCallback (vl->wcont [VL_W_EVENTH], vl->wcont [VL_W_EVENT_BOX],
      vl->callbacks [VL_CB_MOTION_WIN]);

  logProcEnd ("");
  return vl;
}

void
uivlFree (uivirtlist_t *vl)
{
  if (vl == NULL || vl->ident != VL_IDENT) {
    return;
  }

  logProcBegin ();
  for (int i = 0; i < VL_CB_MAX; ++i) {
    callbackFree (vl->callbacks [i]);
  }

  for (int dispidx = 0; dispidx < vl->dispalloc; ++dispidx) {
    uivlFreeRow (vl, &vl->rows [dispidx]);
  }
  dataFree (vl->rows);

  for (int colidx = 0; colidx < vl->numcols; ++colidx) {
    dataFree (vl->coldata [colidx].baseclass);
    dataFree (vl->coldata [colidx].heading);
    callbackFree (vl->coldata [colidx].colgrowonlycb);
    uiwcontFree (vl->coldata [colidx].szgrp);
    uiwcontFree (vl->coldata [colidx].extrarb);
  }
  dataFree (vl->coldata);

  nlistFree (vl->selected);

  for (int i = 0; i < VL_W_MAX; ++i) {
    uiwcontFree (vl->wcont [i]);
  }

  vl->ident = BDJ4_IDENT_FREE;
  mdfree (vl);
  logProcEnd ("");
}

void
uivlSetNumRows (uivirtlist_t *vl, int32_t numrows)
{
  logProcBegin ();
  if (vl == NULL || vl->ident != VL_IDENT) {
    logProcEnd ("bad-vl");
    return;
  }

  vl->numrows = numrows;
  logMsg (LOG_DBG, LOG_VIRTLIST, "vl: %s num-rows: %" PRId32, vl->tag, numrows);

  vl->numrowchg = true;
  uivlConfigureScrollbar (vl);
  uivlPopulate (vl);

  logProcEnd ("");
}

void
uivlSetNumColumns (uivirtlist_t *vl, int numcols)
{
  logProcBegin ();
  if (vl == NULL || vl->ident != VL_IDENT) {
    logProcEnd ("bad-vl");
    return;
  }

  vl->numcols = numcols;
  vl->coldata = mdmalloc (sizeof (uivlcoldata_t) * numcols);
  for (int colidx = 0; colidx < numcols; ++colidx) {
    uivlcoldata_t *coldata = &vl->coldata [colidx];

    coldata->vl = vl;
    coldata->szgrp = uiCreateSizeGroupHoriz ();
    coldata->ident = VL_IDENT_COLDATA;
    coldata->tag = "unk";
    coldata->heading = NULL;
    coldata->type = VL_TYPE_NONE;
    coldata->entrycb = NULL;
    coldata->entryudata = NULL;
    coldata->togglecb = NULL;
    coldata->spinboxcb = NULL;
    coldata->spinboxtimecb = NULL;
    coldata->colgrowonlycb = NULL;
    coldata->extrarb = NULL;
    coldata->colidx = colidx;
    coldata->colwidth = -1;
    coldata->baseclass = NULL;
    coldata->minwidth = VL_MIN_WIDTH_ANY;
    coldata->entrySz = 0;
    coldata->entryMaxSz = 0;
    coldata->sbtype = 0;
    coldata->sbcb = NULL;
    coldata->sbmin = 0;
    coldata->sbmax = 10;
    coldata->sbincr = 1;
    coldata->sbpageincr = 5;
    coldata->alignend = false;
    coldata->aligncenter = false;
    coldata->ellipsize = false;
    coldata->grow = VL_COL_WIDTH_FIXED;
    coldata->hidden = VL_COL_SHOW;
    coldata->clickmap = VL_UNK_ROW;
  }

  vl->initialized = VL_INIT_BASIC;
  logMsg (LOG_DBG, LOG_VIRTLIST, "vl: %s num-cols: %d [init-basic]", vl->tag, numcols);
  logProcEnd ("");
}

void
uivlSetDarkBackground (uivirtlist_t *vl)
{
  logProcBegin ();
  if (vl == NULL || vl->ident != VL_IDENT) {
    logProcEnd ("bad-vl");
    return;
  }

  vl->darkbg = true;
  uiWidgetAddClass (vl->wcont [VL_W_HBOX_CONT], DARKBG_CLASS);
  logProcEnd ("");
}

void
uivlSetDropdownBackground (uivirtlist_t *vl)
{
  logProcBegin ();
  if (vl == NULL || vl->ident != VL_IDENT) {
    logProcEnd ("bad-vl");
    return;
  }

  uiWidgetAddClass (vl->wcont [VL_W_MAIN_VBOX], NORMBG_CLASS);
  logProcEnd ("");
}

void
uivlSetUseListingFont (uivirtlist_t *vl)
{
  logProcBegin ();
  if (vl == NULL || vl->ident != VL_IDENT) {
    logProcEnd ("bad-vl");
    return;
  }

  vl->uselistingfont = true;
  logProcEnd ("");
}

void
uivlSetAllowMultiple (uivirtlist_t *vl)
{
  logProcBegin ();
  if (vl == NULL || vl->ident != VL_IDENT) {
    logProcEnd ("bad-vl");
    return;
  }

  vl->allowmultiple = true;
  logProcEnd ("");
}

void
uivlSetAllowDoubleClick (uivirtlist_t *vl)
{
  logProcBegin ();
  if (vl == NULL || vl->ident != VL_IDENT) {
    logProcEnd ("bad-vl");
    return;
  }

  vl->allowdblclick = true;
  logProcEnd ("");
}

/* row set */

/* though a rownum is passed in, the class applies to the display index */
/* do not validate the row-number */
void
uivlSetRowClass (uivirtlist_t *vl, int32_t rownum, const char *class)
{
  uivlrow_t   *row;

  logProcBegin ();
  if (! uivlValidateColumn (vl, VL_INIT_BASIC, 0, __func__)) {
    logProcEnd ("not-valid");
    return;
  }

  row = uivlGetRow (vl, rownum);
  if (row == NULL) {
    return;
  }

  dataFree (row->oldclass);
  row->oldclass = row->currclass;
  row->currclass = NULL;
  row->newclass = mdstrdup (class);
  logProcEnd ("");
}

/* a rownum is passed in, the lock applies to the display idx associated */
/* with the rownum */
/* do not validate the row-number */
void
uivlSetRowLock (uivirtlist_t *vl, int32_t rownum)
{
  uivlrow_t   *row;

  logProcBegin ();
  if (! uivlValidateColumn (vl, VL_INIT_BASIC, 0, __func__)) {
    logProcEnd ("not-valid");
    return;
  }

  row = uivlGetRow (vl, rownum);
  if (row == NULL) {
    logProcEnd ("bad-row");
    return;
  }

  if (row->lockrownum == VL_ROW_NO_LOCK) {
    vl->lockcount += 1;
  }
  row->lockrownum = rownum;
  uivlConfigureScrollbar (vl);
  logProcEnd ("");
}

/* column set */

void
uivlSetColumnHeading (uivirtlist_t *vl, int colidx, const char *heading)
{
  if (! uivlValidateColumn (vl, VL_INIT_BASIC, colidx, __func__)) {
    return;
  }
  if (vl->dispheading == false) {
    return;
  }

  if (heading != NULL) {
    vl->coldata [colidx].heading = mdstrdup (heading);
  }
}

void
uivlMakeColumn (uivirtlist_t *vl, const char *tag, int colidx, vltype_t type)
{
  if (! uivlValidateColumn (vl, VL_INIT_BASIC, colidx, __func__)) {
    return;
  }

  logMsg (LOG_DBG, LOG_VIRTLIST, "vl: %s mkcol %d %d %s", vl->tag, colidx, type, tag);
  vl->coldata [colidx].type = type;
  vl->coldata [colidx].tag = tag;
  if (type == VL_TYPE_INTERNAL_NUMERIC) {
    vl->coldata [colidx].hidden = VL_COL_DISABLE;
  }
}

void
uivlMakeColumnEntry (uivirtlist_t *vl, const char *tag, int colidx, int sz, int maxsz)
{
  if (! uivlValidateColumn (vl, VL_INIT_BASIC, colidx, __func__)) {
    return;
  }

  logMsg (LOG_DBG, LOG_VIRTLIST, "vl: %s mkcol %d entry %s", vl->tag, colidx, tag);
  vl->coldata [colidx].type = VL_TYPE_ENTRY;
  vl->coldata [colidx].entrySz = sz;
  vl->coldata [colidx].entryMaxSz = maxsz;
  vl->coldata [colidx].tag = tag;
}

void
uivlMakeColumnSpinboxTime (uivirtlist_t *vl, const char *tag, int colidx,
    int sbtype, callback_t *uicb)
{
  if (! uivlValidateColumn (vl, VL_INIT_BASIC, colidx, __func__)) {
    return;
  }

  logMsg (LOG_DBG, LOG_VIRTLIST, "vl: %s mkcol %d sbtime %s", vl->tag, colidx, tag);
  vl->coldata [colidx].type = VL_TYPE_SPINBOX_TIME;
  vl->coldata [colidx].sbtype = sbtype;
  vl->coldata [colidx].sbcb = uicb;
  vl->coldata [colidx].tag = tag;
}

void
uivlMakeColumnSpinboxNum (uivirtlist_t *vl, const char *tag, int colidx,
    double min, double max, double incr, double pageincr)
{
  if (! uivlValidateColumn (vl, VL_INIT_BASIC, colidx, __func__)) {
    return;
  }

  logMsg (LOG_DBG, LOG_VIRTLIST, "vl: %s mkcol %d sbnum %s", vl->tag, colidx, tag);
  vl->coldata [colidx].type = VL_TYPE_SPINBOX_NUM;
  vl->coldata [colidx].sbmin = min;
  vl->coldata [colidx].sbmax = max;
  vl->coldata [colidx].sbincr = incr;
  vl->coldata [colidx].sbpageincr = pageincr;
  vl->coldata [colidx].tag = tag;
}

/* column set */

void
uivlSetColumnMinWidth (uivirtlist_t *vl, int colidx, int minwidth)
{
  if (! uivlValidateColumn (vl, VL_INIT_BASIC, colidx, __func__)) {
    return;
  }

  vl->coldata [colidx].minwidth = minwidth;
}

void
uivlSetColumnEllipsizeOn (uivirtlist_t *vl, int colidx)
{
  if (! uivlValidateColumn (vl, VL_INIT_BASIC, colidx, __func__)) {
    return;
  }

  vl->coldata [colidx].ellipsize = true;
  vl->coldata [colidx].grow = VL_COL_WIDTH_GROW_SHRINK;
}

void
uivlSetColumnAlignEnd (uivirtlist_t *vl, int colidx)
{
  if (! uivlValidateColumn (vl, VL_INIT_BASIC, colidx, __func__)) {
    return;
  }

  vl->coldata [colidx].alignend = true;
}

void
uivlSetColumnAlignCenter (uivirtlist_t *vl, int colidx)
{
  if (! uivlValidateColumn (vl, VL_INIT_BASIC, colidx, __func__)) {
    return;
  }

  vl->coldata [colidx].aligncenter = true;
}

void
uivlSetColumnGrow (uivirtlist_t *vl, int colidx, int grow)
{
  if (! uivlValidateColumn (vl, VL_INIT_BASIC, colidx, __func__)) {
    return;
  }

  vl->coldata [colidx].grow = grow;
}

void
uivlSetColumnDisplay (uivirtlist_t *vl, int colidx, int hidden)
{
  int     washidden;

  logProcBegin ();
  if (! uivlValidateColumn (vl, VL_INIT_BASIC, colidx, __func__)) {
    logProcEnd ("not-valid");
    return;
  }

  if (vl->coldata [colidx].type == VL_TYPE_INTERNAL_NUMERIC) {
    logProcEnd ("is-intern-num");
    return;
  }

  washidden = vl->coldata [colidx].hidden;
  vl->coldata [colidx].hidden = hidden;
  if (hidden == VL_COL_DISABLE) {
    vl->coldata [colidx].type = VL_TYPE_NONE;
  }

  if (vl->initialized >= VL_INIT_ROWS && washidden != hidden) {
    for (int dispidx = 0; dispidx < vl->dispsize; ++dispidx) {
      uivlrow_t   *row;

      row = &vl->rows [dispidx];
      if (row->offscreen || row->cleared) {
        continue;
      }

      uivlShowRow (vl, row);
    }
  }
  logProcEnd ("");
}

void
uivlSetColumnClass (uivirtlist_t *vl, int colidx, const char *class)
{
  logProcBegin ();
  if (! uivlValidateColumn (vl, VL_INIT_BASIC, colidx, __func__)) {
    logProcEnd ("not-valid");
    return;
  }

  dataFree (vl->coldata [colidx].baseclass);
  vl->coldata [colidx].baseclass = mdstrdup (class);
  logProcEnd ("");
}

void
uivlSetRowColumnEditable (uivirtlist_t *vl, int32_t rownum, int colidx, int state)
{
  uivlrow_t   *row;

  logProcBegin ();
  if (! uivlValidateRowColumn (vl, VL_INIT_BASIC, rownum, colidx, __func__)) {
    logProcEnd ("not-valid");
    return;
  }

  row = uivlGetRow (vl, rownum);
  if (row == NULL) {
    logProcEnd ("bad-row");
    return;
  }

  /* at this time, the user interfaces only have read-only entries */
  switch (vl->coldata [colidx].type) {
    case VL_TYPE_ENTRY: {
      uiEntrySetState (row->cols [colidx].uiwidget, state);
      break;
    }
    case VL_TYPE_SPINBOX_NUM:
    case VL_TYPE_SPINBOX_TIME: {
      uiSpinboxSetState (row->cols [colidx].uiwidget, state);
      break;
    }
    case VL_TYPE_CHECKBOX:
    case VL_TYPE_RADIO_BUTTON: {
      /* not supported */
      break;
    }
    case VL_TYPE_NONE:
    case VL_TYPE_IMAGE:
    case VL_TYPE_INTERNAL_NUMERIC:
    case VL_TYPE_LABEL: {
      /* not necessary */
      break;
    }
  }
  logProcEnd ("");
}

void
uivlSetRowColumnClass (uivirtlist_t *vl, int32_t rownum, int colidx, const char *class)
{
  uivlrow_t *row = NULL;
  uivlcol_t *col = NULL;

  logProcBegin ();
  if (! uivlValidateRowColumn (vl, VL_INIT_BASIC, rownum, colidx, __func__)) {
    logProcEnd ("not-valid");
    return;
  }

  row = uivlGetRow (vl, rownum);
  if (row == NULL) {
    logProcEnd ("bad-row");
    return;
  }

  col = &row->cols [colidx];
  dataFree (col->class);
  col->class = NULL;
  if (class != NULL) {
    /* save for removal process */
    col->class = mdstrdup (class);
    uiWidgetAddClass (col->uiwidget, class);
  }
  logProcEnd ("");
}

void
uivlClearRowColumnClass (uivirtlist_t *vl, int32_t rownum, int colidx)
{
  uivlrow_t *row = NULL;
  uivlcol_t *col = NULL;

  logProcBegin ();
  if (! uivlValidateRowColumn (vl, VL_INIT_BASIC, rownum, colidx, __func__)) {
    logProcEnd ("not-valid");
    return;
  }

  row = uivlGetRow (vl, rownum);
  if (row == NULL) {
    logProcEnd ("bad-row");
    return;
  }

  col = &row->cols [colidx];
  if (col->class != NULL) {
    uiWidgetRemoveClass (col->uiwidget, col->class);
    dataFree (col->class);
    col->class = NULL;
  }
  logProcEnd ("");
}

void
uivlSetRowColumnStr (uivirtlist_t *vl, int32_t rownum, int colidx, const char *value)
{
  uivlrow_t   *row = NULL;
  int         type;

  if (! uivlValidateRowColumn (vl, VL_INIT_ROWS, rownum, colidx, __func__)) {
    return;
  }

  row = uivlGetRow (vl, rownum);
  if (row == NULL) {
    return;
  }

  type = vl->coldata [colidx].type;
  if (vl->coldata [colidx].hidden == VL_COL_DISABLE) {
    type = VL_TYPE_NONE;
  }
  if (rownum == VL_ROW_HEADING && type != VL_TYPE_NONE) {
    type = VL_TYPE_LABEL;
  }

  switch (type) {
    case VL_TYPE_NONE: {
      break;
    }
    case VL_TYPE_LABEL: {
      uiLabelSetText (row->cols [colidx].uiwidget, value);
      if (vl->coldata [colidx].ellipsize) {
        uiLabelSetTooltip (row->cols [colidx].uiwidget, value);
      }
      break;
    }
    case VL_TYPE_ENTRY: {
      uiEntrySetValue (row->cols [colidx].uiwidget, value);
      break;
    }
    case VL_TYPE_IMAGE: {
      break;
    }
    case VL_TYPE_CHECKBOX:
    case VL_TYPE_INTERNAL_NUMERIC:
    case VL_TYPE_RADIO_BUTTON:
    case VL_TYPE_SPINBOX_NUM:
    case VL_TYPE_SPINBOX_TIME: {
      /* not handled here */
      break;
    }
  }

  if (row->offscreen == false && row->cleared) {
    uivlShowRow (vl, row);
  }
}

void
uivlSetRowColumnImage (uivirtlist_t *vl, int32_t rownum, int colidx,
    uiwcont_t *img, int width)
{
  uivlrow_t   *row = NULL;

  if (! uivlValidateRowColumn (vl, VL_INIT_ROWS, rownum, colidx, __func__)) {
    return;
  }

  row = uivlGetRow (vl, rownum);
  if (row == NULL) {
    return;
  }

  switch (vl->coldata [colidx].type) {
    case VL_TYPE_IMAGE: {
      if (img == NULL) {
        uiImageClear (row->cols [colidx].uiwidget);
      } else {
        uiWidgetSetSizeRequest (row->cols [colidx].uiwidget, width, -1);
        uiImageSetFromPixbuf (row->cols [colidx].uiwidget, img);
      }
      break;
    }
    default: {
      /* not handled here */
      break;
    }
  }

  if (row->offscreen == false && row->cleared) {
    uivlShowRow (vl, row);
  }
}

void
uivlSetRowColumnNum (uivirtlist_t *vl, int32_t rownum, int colidx, int32_t val)
{
  uivlrow_t       *row = NULL;

  if (! uivlValidateRowColumn (vl, VL_INIT_ROWS, rownum, colidx, __func__)) {
    return;
  }

  row = uivlGetRow (vl, rownum);
  if (row == NULL) {
    return;
  }

  switch (vl->coldata [colidx].type) {
    case VL_TYPE_NONE:
    case VL_TYPE_ENTRY:
    case VL_TYPE_IMAGE:
    case VL_TYPE_LABEL: {
      /* not handled here */
      break;
    }
    case VL_TYPE_INTERNAL_NUMERIC: {
      row->cols [colidx].value = val;
      break;
    }
    case VL_TYPE_CHECKBOX:
    case VL_TYPE_RADIO_BUTTON: {
      int   nstate = UI_TOGGLE_BUTTON_OFF;
      bool  oldval;

      oldval = uiToggleButtonIsActive (row->cols [colidx].uiwidget);

      if (val) {
        nstate = UI_TOGGLE_BUTTON_ON;
      } else {
        /* turning the button off, turn on the extra-rb */
        if (oldval) {
          uiToggleButtonSetValue (vl->coldata [colidx].extrarb, UI_TOGGLE_BUTTON_ON);
        }
      }
      uiToggleButtonSetValue (row->cols [colidx].uiwidget, nstate);
      break;
    }
    case VL_TYPE_SPINBOX_NUM: {
      uiSpinboxSetValue (row->cols [colidx].uiwidget, val);
      break;
    }
    case VL_TYPE_SPINBOX_TIME: {
      uiSpinboxTimeSetValue (row->cols [colidx].uiwidget, val);
      break;
    }
  }

  if (row->offscreen == false && row->cleared) {
    uivlShowRow (vl, row);
  }
}

const char *
uivlGetRowColumnEntry (uivirtlist_t *vl, int32_t rownum, int colidx)
{
  uivlrow_t   *row;
  const char  *val = NULL;

  logProcBegin ();
  if (! uivlValidateRowColumn (vl, VL_INIT_ROWS, rownum, colidx, __func__)) {
    logProcEnd ("not-valid");
    return NULL;
  }

  row = uivlGetRow (vl, rownum);
  if (row == NULL) {
    logProcEnd ("bad-row");
    return NULL;
  }

  val = uiEntryGetValue (row->cols [colidx].uiwidget);
  logProcEnd ("");
  return val;
}

int32_t
uivlGetRowColumnNum (uivirtlist_t *vl, int32_t rownum, int colidx)
{
  uivlrow_t   *row;
  int32_t     value = VL_UNK_ROW;

  logProcBegin ();
  if (! uivlValidateRowColumn (vl, VL_INIT_ROWS, rownum, colidx, __func__)) {
    logProcEnd ("not-valid");
    return value;
  }

  row = uivlGetRow (vl, rownum);
  if (row == NULL) {
    logProcEnd ("bad-row");
    return value;
  }

  switch (vl->coldata [colidx].type) {
    case VL_TYPE_NONE:
    case VL_TYPE_ENTRY:
    case VL_TYPE_IMAGE:
    case VL_TYPE_LABEL: {
      /* not handled here */
      break;
    }
    case VL_TYPE_INTERNAL_NUMERIC: {
      value = row->cols [colidx].value;
      break;
    }
    case VL_TYPE_CHECKBOX:
    case VL_TYPE_RADIO_BUTTON: {
      int   tval;

      tval = uiToggleButtonIsActive (row->cols [colidx].uiwidget);
      value = false;
      if (tval == UI_TOGGLE_BUTTON_ON) {
        value = true;
      }
      break;
    }
    case VL_TYPE_SPINBOX_NUM: {
      value = uiSpinboxGetValue (row->cols [colidx].uiwidget);
      break;
    }
    case VL_TYPE_SPINBOX_TIME: {
      value = uiSpinboxTimeGetValue (row->cols [colidx].uiwidget);
      break;
    }
  }

  logProcEnd ("");
  return value;
}

/* callbacks */

void
uivlSetSelectChgCallback (uivirtlist_t *vl, uivlselcb_t cb, void *udata)
{
  logProcBegin ();
  if (! uivlValidateColumn (vl, VL_INIT_BASIC, 0, __func__)) {
    logProcEnd ("not-valid");
    return;
  }

  vl->selchgcb = cb;
  vl->seludata = udata;
  logProcEnd ("");
}

void
uivlSetDisplayChgCallback (uivirtlist_t *vl, callback_t *cb)
{
  logProcBegin ();
  if (vl == NULL) {
    logProcEnd ("bad-vl");
    return;
  }

  vl->usercb [VL_USER_CB_DISP_CHG] = cb;
  logProcEnd ("");
}

void
uivlSetRowClickCallback (uivirtlist_t *vl, uivlselcb_t cb, void *udata)
{
  logProcBegin ();
  if (! uivlValidateColumn (vl, VL_INIT_BASIC, 0, __func__)) {
    logProcEnd ("not-valid");
    return;
  }

  vl->rowselcb = cb;
  vl->rowseludata = udata;
  logProcEnd ("");
}

void
uivlSetDoubleClickCallback (uivirtlist_t *vl, uivlselcb_t cb, void *udata)
{
  logProcBegin ();
  if (! uivlValidateColumn (vl, VL_INIT_BASIC, 0, __func__)) {
    logProcEnd ("not-valid");
    return;
  }

  vl->dblclickcb = cb;
  vl->dblclickudata = udata;
  logProcEnd ("");
}

void
uivlSetRightClickCallback (uivirtlist_t *vl, uivlselcb_t cb, void *udata)
{
  logProcBegin ();
  if (! uivlValidateColumn (vl, VL_INIT_BASIC, 0, __func__)) {
    logProcEnd ("not-valid");
    return;
  }

  vl->rightclickcb = cb;
  vl->rightclickudata = udata;
  logProcEnd ("");
}

void
uivlSetRowFillCallback (uivirtlist_t *vl, uivlfillcb_t cb, void *udata)
{
  logProcBegin ();
  if (! uivlValidateColumn (vl, VL_INIT_BASIC, 0, __func__)) {
    logProcEnd ("not-valid");
    return;
  }
  if (cb == NULL) {
    logProcEnd ("no-cb");
    return;
  }

  vl->fillcb = cb;
  vl->filludata = udata;
  logProcEnd ("");
}

void
uivlSetEntryValidation (uivirtlist_t *vl, int colidx,
    uientryval_t cb, void *udata)
{
  logProcBegin ();
  if (! uivlValidateColumn (vl, VL_INIT_BASIC, colidx, __func__)) {
    logProcEnd ("not-valid");
    return;
  }
  if (vl->coldata [colidx].type != VL_TYPE_ENTRY) {
    logProcEnd ("not-entry");
    return;
  }

  vl->coldata [colidx].entrycb = cb;
  vl->coldata [colidx].entryudata = udata;
  logProcEnd ("");
}

void
uivlSetRadioChangeCallback (uivirtlist_t *vl, int colidx, callback_t *cb)
{
  logProcBegin ();
  uivlSetToggleChangeCallback (vl, colidx, cb);
  logProcEnd ("");
}

void
uivlSetCheckboxChangeCallback (uivirtlist_t *vl, int colidx, callback_t *cb)
{
  logProcBegin ();
  uivlSetToggleChangeCallback (vl, colidx, cb);
  logProcEnd ("");
}

void
uivlSetSpinboxTimeChangeCallback (uivirtlist_t *vl, int colidx, callback_t *cb)
{
  logProcBegin ();
  if (! uivlValidateColumn (vl, VL_INIT_BASIC, colidx, __func__)) {
    logProcEnd ("not-valid");
    return;
  }
  if (vl->coldata [colidx].type != VL_TYPE_SPINBOX_TIME) {
    logProcEnd ("not-spinbox-time");
    return;
  }

  vl->coldata [colidx].spinboxtimecb = cb;
  logProcEnd ("");
}

void
uivlSetSpinboxChangeCallback (uivirtlist_t *vl, int colidx, callback_t *cb)
{
  logProcBegin ();
  if (! uivlValidateColumn (vl, VL_INIT_BASIC, colidx, __func__)) {
    logProcEnd ("not-valid");
    return;
  }
  if (vl->coldata [colidx].type != VL_TYPE_SPINBOX_NUM) {
    logProcEnd ("not-spinbox");
    return;
  }

  vl->coldata [colidx].spinboxcb = cb;
  logProcEnd ("");
}

/* the vl key callback is called for any unhandled keys */
void
uivlSetKeyCallback (uivirtlist_t *vl, callback_t *cb)
{
  logProcBegin ();
  if (vl == NULL) {
    logProcEnd ("bad-vl");
    return;
  }

  vl->usercb [VL_USER_CB_KEY] = cb;
  logProcEnd ("");
}

/* selection handling */

void
uivlStartSelectionIterator (uivirtlist_t *vl, nlistidx_t *iteridx)
{
  logProcBegin ();
  if (! uivlValidateColumn (vl, VL_INIT_BASIC, 0, __func__)) {
    logProcEnd ("not-valid");
    return;
  }

  nlistStartIterator (vl->selected, iteridx);
  logProcEnd ("");
}

int32_t
uivlIterateSelection (uivirtlist_t *vl, nlistidx_t *iteridx)
{
  nlistidx_t    key = -1;

  logProcBegin ();
  if (! uivlValidateColumn (vl, VL_INIT_BASIC, 0, __func__)) {
    logProcEnd ("not-valid");
    return key;
  }

  key = nlistIterateKey (vl->selected, iteridx);
  logProcEnd ("");
  return key;
}

int32_t
uivlIterateSelectionPrevious (uivirtlist_t *vl, nlistidx_t *iteridx)
{
  nlistidx_t    key = -1;

  logProcBegin ();
  if (! uivlValidateColumn (vl, VL_INIT_BASIC, 0, __func__)) {
    logProcEnd ("not-valid");
    return key;
  }

  key = nlistIterateKeyPrevious (vl->selected, iteridx);
  logProcEnd ("");
  return key;
}

int32_t
uivlSelectionCount (uivirtlist_t *vl)
{
  logProcBegin ();
  if (! uivlValidateColumn (vl, VL_INIT_BASIC, 0, __func__)) {
    logProcEnd ("not-valid");
    return 0;
  }

  logProcEnd ("");
  return nlistGetCount (vl->selected);
}

int32_t
uivlGetCurrSelection (uivirtlist_t *vl)
{
  logProcBegin ();
  if (! uivlValidateColumn (vl, VL_INIT_DISP, 0, __func__)) {
    logProcEnd ("not-valid");
    return 0;
  }

  if (nlistGetCount (vl->selected) == 0) {
    logProcEnd ("no-sel");
    return VL_UNK_ROW;
  }

  logProcEnd ("");
  return vl->currSelection;
}

void
uivlSetSelection (uivirtlist_t *vl, int32_t rownum)
{
  logProcBegin ();
  if (! uivlValidateRowColumn (vl, VL_INIT_DISP, rownum, 0, __func__)) {
    logProcEnd ("not-valid");
    return;
  }

  logMsg (LOG_DBG, LOG_VIRTLIST, "vl: -- %s set-sel %" PRId32, vl->tag, rownum);
  rownum = uivlRownumLimit (vl, rownum);
  uivlProcessScroll (vl, rownum, VL_SCROLL_NORM);
  uivlUpdateSelections (vl, rownum);
  uivlSelectChgHandler (vl, rownum, VL_COL_UNKNOWN);
  logProcEnd ("");
}

void
uivlAppendSelection (uivirtlist_t *vl, int32_t rownum)
{
  logProcBegin ();
  if (! uivlValidateRowColumn (vl, VL_INIT_DISP, rownum, 0, __func__)) {
    logProcEnd ("not-valid");
    return;
  }

  rownum = uivlRownumLimit (vl, rownum);
  uivlAddSelection (vl, rownum);
  uivlSetDisplaySelections (vl);
  if (nlistGetCount (vl->selected) == 1) {
    vl->lastSelection = vl->currSelection;
  }
  logProcEnd ("");
}

int32_t
uivlMoveSelection (uivirtlist_t *vl, int dir)
{
  int32_t     rownum;

  logProcBegin ();
  if (! uivlValidateColumn (vl, VL_INIT_DISP, 0, __func__)) {
    logProcEnd ("not-valid");
    return 0;
  }

  rownum = vl->currSelection;
  if (dir == VL_DIR_PREV) {
    rownum -= 1;
  }
  if (dir == VL_DIR_NEXT) {
    rownum += 1;
  }
  rownum = uivlRownumLimit (vl, rownum);

  uivlSetSelection (vl, rownum);

  logProcEnd ("");
  return rownum;
}

void
uivlCopySelectList (uivirtlist_t *vl_a, uivirtlist_t *vl_b)
{
  nlistidx_t    iter_a;
  int32_t       rowidx;

  logProcBegin ();
  if (vl_a == NULL || vl_b == NULL) {
    logProcEnd ("bad-vl");
    return;
  }

  /* copy the selected list to vl_b */
  logMsg (LOG_DBG, LOG_VIRTLIST, "vl: copy sel from %s to %s", vl_a->tag, vl_b->tag);
  uivlClearAllSelections (vl_b);
  nlistStartIterator (vl_a->selected, &iter_a);
  while ((rowidx = nlistIterateKey (vl_a->selected, &iter_a)) >= 0) {
    uivlAddSelection (vl_b, rowidx);
    uivlSelectChgHandler (vl_b, rowidx, VL_COL_UNKNOWN);
  }
  uivlClearDisplaySelections (vl_b);
  uivlSetDisplaySelections (vl_b);
  uivlCopyPosition (vl_a, vl_b);
  logProcEnd ("");
}

void
uivlCopyPosition (uivirtlist_t *vl_a, uivirtlist_t *vl_b)
{
  logProcBegin ();
  if (vl_a == NULL || vl_b == NULL) {
    logProcEnd ("bad-vl");
    return;
  }
  /* position the scroll in the same place */
  uivlProcessScroll (vl_b, vl_a->rowoffset, VL_SCROLL_FORCE);
  logProcEnd ("");
}

/* processing */

/* the initial display */
void
uivlDisplay (uivirtlist_t *vl)
{
  uivlrow_t   *row;
  int         clickmap;

  logProcBegin ();
  if (! uivlValidateColumn (vl, VL_INIT_BASIC, 0, __func__)) {
    logProcEnd ("not-valid");
    return;
  }

  for (int dispidx = 0; dispidx < vl->dispsize; ++dispidx) {
    uivlrow_t *row;
    bool      isheading = false;

    row = &vl->rows [dispidx];
    if (vl->dispheading && dispidx == VL_ROW_HEADING_IDX) {
      isheading = true;
    }
    uivlCreateRow (vl, row, dispidx, isheading);
  }
  vl->initialized = VL_INIT_ROWS;
  logMsg (LOG_DBG, LOG_VIRTLIST, "vl: %s [init-rows]", vl->tag);

  vl->initialized = VL_INIT_DISP;
  logMsg (LOG_DBG, LOG_VIRTLIST, "vl: %s [init-disp]", vl->tag);
  uiScrollbarSetPosition (vl->wcont [VL_W_SB], 0.0);

  for (int dispidx = 0; dispidx < vl->dispsize; ++dispidx) {
    row = &vl->rows [dispidx];
    uivlPackRow (vl, row);
    /* the size change callback is based on the first data row */
    if (vl->dispheading && dispidx == VL_ROW_HEADING_IDX) {
      uiBoxSetSizeChgCallback (row->hbox, vl->callbacks [VL_CB_HEADING_SZ_CHG]);
    }
    if (dispidx == vl->headingoffset) {
      uiBoxSetSizeChgCallback (row->hbox, vl->callbacks [VL_CB_ROW_SZ_CHG]);
    }
  }

  clickmap = 0;
  for (int colidx = 0; colidx < vl->numcols; ++colidx) {
    int     hidden;

    hidden = vl->coldata [colidx].hidden;
    if (hidden == VL_COL_HIDE || hidden == VL_COL_DISABLE) {
      continue;
    }
    vl->coldata [colidx].clickmap = clickmap;
    ++clickmap;
  }

  /* load the headings */
  if (vl->dispheading) {
    uiwcont_t   *uiwidget;

    uiwidget = uiCreateLabel (" ");
    if (vl->uselistingfont) {
      uiWidgetAddClass (uiwidget, LISTING_CLASS);
    }
    uiBoxPackStart (vl->wcont [VL_W_SB_VBOX], uiwidget);
    uiwcontFree (uiwidget);

    for (int colidx = 0; colidx < vl->numcols; ++colidx) {
      if (vl->coldata [colidx].heading == NULL) {
        continue;
      }
      /* set-row-col-str will call show-row */
      uivlSetRowColumnStr (vl, VL_ROW_HEADING, colidx,
          vl->coldata [colidx].heading);
    }
  }

  uivlPopulate (vl);

  /* at this point, the heading row is not yet mapped */
  logProcEnd ("");
}

void
uivlUpdateDisplay (uivirtlist_t *vl)
{
  uivlConfigureScrollbar (vl);
}

/* display after a change */
void
uivlPopulate (uivirtlist_t *vl)
{
  logProcBegin ();
  if (! uivlValidateColumn (vl, VL_INIT_DISP, 0, __func__)) {
    logProcEnd ("not-valid");
    return;
  }

  for (int dispidx = vl->dispsize; dispidx < vl->dispalloc; ++dispidx) {
    if (vl->rows [dispidx].offscreen) {
      continue;
    }
    vl->rows [dispidx].offscreen = true;
    uiWidgetHide (vl->rows [dispidx].hbox);
  }

  uivlCheckDisplay (vl);

  for (int dispidx = vl->headingoffset; dispidx < vl->dispsize; ++dispidx) {
    uivlrow_t   *row;
    uivlcol_t   *col;

    if ((dispidx - vl->headingoffset) + vl->rowoffset >= vl->numrows) {
      break;
    }

    row = &vl->rows [dispidx];

    for (int colidx = 0; colidx < vl->numcols; ++colidx) {
      col = &row->cols [colidx];
      if (col->class != NULL) {
        uiWidgetRemoveClass (col->uiwidget, col->class);
        dataFree (col->class);
        col->class = NULL;
      }

      if (row->newclass != NULL &&
          vl->coldata [colidx].type == VL_TYPE_LABEL) {
        if (row->oldclass != NULL) {
          uiWidgetRemoveClass (col->uiwidget, row->oldclass);
        }

        uiWidgetAddClass (col->uiwidget, row->newclass);
      }
    }

    if (row->newclass != NULL) {
      dataFree (row->oldclass);
      row->oldclass = NULL;
      row->currclass = row->newclass;
      row->newclass = NULL;
    }
  }

  for (int dispidx = vl->headingoffset; dispidx < vl->dispsize; ++dispidx) {
    uivlrow_t   *row = NULL;
    int32_t     rownum;

    rownum = uivlCalcRownum (vl, dispidx);
    if (rownum >= vl->numrows) {
      break;
    }

    row = &vl->rows [dispidx];
    if (row->lockrownum != VL_ROW_NO_LOCK) {
      rownum = row->lockrownum;
    }

    if (vl->fillcb != NULL) {
      vl->fillcb (vl->filludata, vl, rownum);
    }

    if (row->offscreen) {
      uivlShowRow (vl, row);
    }
  }

  uivlClearDisplaySelections (vl);
  uivlSetDisplaySelections (vl);
  logProcEnd ("");
}

uiwcont_t *
uivlGetEventHandler (uivirtlist_t *vl)
{
  logProcBegin ();
  if (vl == NULL) {
    logProcEnd ("bad-vl");
    return NULL;
  }

  logProcEnd ("");
  return vl->wcont [VL_W_EVENTH];
}

/* internal routines */

static void
uivlFreeRow (uivirtlist_t *vl, uivlrow_t *row)
{
  if (row == NULL || row->ident != VL_IDENT_ROW) {
    return;
  }

  if (row->rowcb != NULL) {
    callbackFree (row->rowcb->focuscb);
    dataFree (row->rowcb);
  }

  uiwcontFree (row->hbox);
  uiwcontFree (row->szgrp);

  for (int colidx = 0; colidx < vl->numcols; ++colidx) {
    if (row->cols != NULL) {
      uivlFreeCol (&row->cols [colidx]);
    }
  }
  dataFree (row->cols);

  dataFree (row->oldclass);
  dataFree (row->currclass);
  dataFree (row->newclass);
  row->ident = 0;
}

static void
uivlFreeCol (uivlcol_t *col)
{
  if (col == NULL || col->ident != VL_IDENT_COL) {
    return;
  }

  dataFree (col->class);
  col->class = NULL;
  uiwcontFree (col->uiwidget);
  col->ident = 0;
}

static void
uivlCreateRow (uivirtlist_t *vl, uivlrow_t *row, int dispidx, bool isheading)
{
  uiwcont_t   *uiwidget;
  bool        found = false;

  logProcBegin ();
  if (vl == NULL || row == NULL) {
    logProcEnd ("bad-vl-row");
    return;
  }
  if (! row->initialized) {
    fprintf (stderr, "%s create-row: not-init %d\n", vl->tag, dispidx);
    logProcEnd ("not-init");
    return;
  }
  if (row->created) {
    logProcEnd ("created");
    return;
  }

  row->hbox = uiCreateHorizBox ();
  uiWidgetAlignHorizFill (row->hbox);
  uiWidgetAlignVertStart (row->hbox);   // no vertical growth
  uiWidgetExpandHoriz (row->hbox);

  found = false;
  for (int colidx = 0; colidx < vl->numcols; ++colidx) {
    if (vl->coldata [colidx].ellipsize) {
      found = true;
      break;
    }
  }
  if (found) {
    row->szgrp = uiCreateSizeGroupHoriz ();
  }

  row->offscreen = false;
  row->selected = false;
  row->created = true;
  row->cleared = true;

  row->cols = mdmalloc (sizeof (uivlcol_t) * vl->numcols);

  /* create a label so that cleared rows with only widgets will still */
  /* have a height (hair space) */
  /* note that this changes the mouse-button column return */
  /* the mouse button event handler adjusts for this column */
  uiwidget = uiCreateLabel ("\xe2\x80\x8a");
  uiBoxPackStart (row->hbox, uiwidget);
  if (vl->uselistingfont) {
    /* setting this to list-fav-class allows gtk to calculate the */
    /* row height better, otherwise there are annoying display glitches */
    /* when only the favorite (at the end of the row) has this class set. */
    uiWidgetAddClass (uiwidget, LIST_FAV_CLASS);
  }
  uiWidgetShow (uiwidget);
  uiwcontFree (uiwidget);

  for (int colidx = 0; colidx < vl->numcols; ++colidx) {
    vltype_t      origtype;
    vltype_t      type;
    uivlcol_t     *col;
    uivlcoldata_t *coldata;

    col = &row->cols [colidx];
    coldata = &vl->coldata [colidx];

    col->ident = VL_IDENT_COL;
    col->uiwidget = NULL;
    col->class = NULL;
    col->colidx = colidx;
    col->value = LIST_VALUE_INVALID;

    origtype = coldata->type;
    type = origtype;
    if (isheading && coldata->hidden != VL_COL_DISABLE) {
      type = VL_TYPE_LABEL;
    }

    switch (type) {
      case VL_TYPE_NONE: {
        break;
      }
      case VL_TYPE_LABEL: {
        col->uiwidget = uiCreateLabel ("");
        break;
      }
      case VL_TYPE_IMAGE: {
        col->uiwidget = uiImageNew ();
        uiImageClear (col->uiwidget);
        break;
      }
      case VL_TYPE_ENTRY: {
        col->uiwidget = uiEntryInit (coldata->entrySz, coldata->entryMaxSz);
        uiWidgetEnableFocus (col->uiwidget);
        uiEntrySetFocusCallback (col->uiwidget, row->rowcb->focuscb);
        if (coldata->entrycb != NULL) {
          uiEntrySetValidate (col->uiwidget, "", coldata->entrycb,
              coldata->entryudata, UIENTRY_IMMEDIATE);
        }
        break;
      }
      case VL_TYPE_RADIO_BUTTON: {
        if (coldata->extrarb == NULL) {
          coldata->extrarb = uiCreateRadioButton (NULL, "", 1);
        }

        col->uiwidget = uiCreateRadioButton (coldata->extrarb, "", 0);
        uiWidgetEnableFocus (col->uiwidget);
        uiToggleButtonSetFocusCallback (col->uiwidget, row->rowcb->focuscb);
        if (coldata->togglecb != NULL) {
          uiToggleButtonSetCallback (col->uiwidget, coldata->togglecb);
        }
        break;
      }
      case VL_TYPE_CHECKBOX: {
        col->uiwidget = uiCreateCheckButton ("", 0);
        uiWidgetEnableFocus (col->uiwidget);
        uiToggleButtonSetFocusCallback (col->uiwidget, row->rowcb->focuscb);
        if (coldata->togglecb != NULL) {
          uiToggleButtonSetCallback (col->uiwidget, coldata->togglecb);
        }
        break;
      }
      case VL_TYPE_SPINBOX_NUM: {
        col->uiwidget = uiSpinboxIntCreate ();
        uiSpinboxSetRange (col->uiwidget, coldata->sbmin, coldata->sbmax);
        uiSpinboxSetIncrement (col->uiwidget, coldata->sbincr, coldata->sbpageincr);
        uiWidgetEnableFocus (col->uiwidget);
        uiSpinboxSetFocusCallback (col->uiwidget, row->rowcb->focuscb);
        if (coldata->spinboxcb != NULL) {
          uiSpinboxSetValueChangedCallback (col->uiwidget, coldata->spinboxcb);
        }
        break;
      }
      case VL_TYPE_SPINBOX_TIME: {
        col->uiwidget = uiSpinboxTimeCreate (coldata->sbtype,
            vl, "", coldata->sbcb);
        uiWidgetEnableFocus (col->uiwidget);
        uiSpinboxSetFocusCallback (col->uiwidget, row->rowcb->focuscb);
        if (coldata->spinboxtimecb != NULL) {
          uiSpinboxTimeSetValueChangedCallback (col->uiwidget, coldata->spinboxtimecb);
        }
        break;
      }
      case VL_TYPE_INTERNAL_NUMERIC: {
        /* an internal numeric type will always be disabled */
        /* used to associate values with the row */
        break;
      }
    }

    if (coldata->hidden == VL_COL_DISABLE) {
      continue;
    }

    uiWidgetSetAllMargins (col->uiwidget, 0);
    if (isheading) {
      uiWidgetAlignVertEnd (col->uiwidget);
    }
    uiWidgetSetMarginEnd (col->uiwidget, 3);
    if (type == VL_TYPE_IMAGE) {
      uiWidgetSetMarginStart (col->uiwidget, 1);
    }

    if (coldata->alignend) {
      uiLabelAlignEnd (col->uiwidget);
    } else if (coldata->aligncenter) {
      /* this works if the widget is set to expand-horiz, and */
      /* it is not part of a size group */
      uiWidgetAlignHorizCenter (col->uiwidget);
      uiWidgetExpandHoriz (col->uiwidget);
    } else {
      uiWidgetAlignHorizStart (col->uiwidget);
    }

    if (coldata->grow == VL_COL_WIDTH_GROW_SHRINK &&
        origtype == VL_TYPE_ENTRY) {
      uiWidgetExpandHoriz (col->uiwidget);
    }

    if (coldata->grow == VL_COL_WIDTH_GROW_SHRINK) {
      uiBoxPackStartExpand (row->hbox, col->uiwidget);
    } else {
      uiBoxPackStart (row->hbox, col->uiwidget);
    }

    if (row->dispidx == vl->headingoffset &&
        col->uiwidget != NULL) {
      /* set up the size change callback so that grow-only columns */
      /* can be made to only grow, and never shrink on their own */
      /* only need this on labels and images. */
      coldata->col0 = col;
      if (coldata->grow == VL_COL_WIDTH_GROW_ONLY &&
          (type == VL_TYPE_LABEL || type == VL_TYPE_IMAGE)) {
        coldata->colgrowonlycb = callbackInitII (uivlColGrowOnlyChg, coldata);
        uiWidgetSetSizeChgCallback (col->uiwidget, coldata->colgrowonlycb);
      }
    }

    if (isheading) {
      uiWidgetAddClass (col->uiwidget, HEADING_CLASS);
    }
    if (vl->uselistingfont) {
      if (isheading) {
        uiWidgetAddClass (col->uiwidget, LIST_HEAD_CLASS);
      } else {
        uiWidgetAddClass (col->uiwidget, LISTING_CLASS);
      }
    }

    if (coldata->baseclass != NULL) {
      uiWidgetAddClass (col->uiwidget, coldata->baseclass);
    }
    if (coldata->type == VL_TYPE_LABEL) {
      if (coldata->minwidth != VL_MIN_WIDTH_ANY) {
        uiLabelSetMinWidth (col->uiwidget, coldata->minwidth);
      }
      if (coldata->ellipsize) {
        uiLabelEllipsizeOn (col->uiwidget);
      }
    }

    /* when a row is first created, it is in the cleared state */
    uiWidgetHide (col->uiwidget);
  }
  logProcEnd ("");
}

static uivlrow_t *
uivlGetRow (uivirtlist_t *vl, int32_t rownum)
{
  uivlrow_t   *row = NULL;

  if (rownum == VL_ROW_HEADING) {
    row = &vl->rows [VL_ROW_HEADING_IDX];
  } else {
    int32_t   dispidx;

    dispidx = uivlCalcDispidx (vl, rownum);
    if (dispidx >= vl->headingoffset && dispidx < vl->dispsize) {
      row = &vl->rows [dispidx];
      if (row->ident != VL_IDENT_ROW) {
        fprintf (stderr, "ERR: invalid row: rownum %" PRId32 " rowoffset: %" PRId32 " dispidx: %" PRId32, rownum, vl->rowoffset, dispidx);
      }
    }
  }

  return row;
}

static void
uivlPackRow (uivirtlist_t *vl, uivlrow_t *row)
{
  if (vl == NULL || row == NULL) {
    return;
  }

  uiBoxPackStart (vl->wcont [VL_W_MAIN_VBOX], row->hbox);
  for (int colidx = 0; colidx < vl->numcols; ++colidx) {
    uivlcoldata_t *coldata;
    uivlcol_t     *col;

    coldata = &vl->coldata [colidx];
    col = &row->cols [colidx];

    if (! coldata->aligncenter) {
      uiSizeGroupAdd (coldata->szgrp, col->uiwidget);
    }
    if (coldata->ellipsize) {
      uiSizeGroupAdd (row->szgrp, col->uiwidget);
    }
  }

  /* any initial pack should be cleared */
  uivlClearRowDisp (vl, row->dispidx);
}

static bool
uivlScrollbarCallback (void *udata, double value)
{
  uivirtlist_t  *vl = udata;
  int32_t       start;

  logProcBegin ();
  if (vl->inscroll) {
    logProcEnd ("in-scroll");
    return UICB_STOP;
  }

  start = (uint32_t) floor (value);

  uivlProcessScroll (vl, start, VL_SCROLL_FORCE);

  logProcEnd ("");
  return UICB_CONT;
}

static bool
uivlKeyEvent (void *udata)
{
  uivirtlist_t  *vl = udata;

  logProcBegin ();
  if (vl == NULL || vl->ident != VL_IDENT) {
    logProcEnd ("bad-vl");
    return UICB_CONT;
  }
  if (vl->inscroll) {
    logProcEnd ("in-scroll");
    return UICB_CONT;
  }

  if (uiEventIsKeyReleaseEvent (vl->wcont [VL_W_EVENTH])) {
    logProcEnd ("is-release");
    return UICB_CONT;
  }

  if (uiEventIsMovementKey (vl->wcont [VL_W_EVENTH])) {
    int32_t     dir = 1;
    int32_t     nsel;

    if (uiEventIsPageUpDownKey (vl->wcont [VL_W_EVENTH])) {
      dir = vl->dispsize - vl->headingoffset - vl->lockcount;
    }
    if (uiEventIsUpKey (vl->wcont [VL_W_EVENTH])) {
      dir = - dir;
    }

    if (nlistGetCount (vl->selected) == 1) {
      nsel = vl->currSelection + dir;
      nsel = uivlRownumLimit (vl, nsel);
      /* use the keyboard scroll update mode */
      uivlProcessScroll (vl, nsel, VL_SCROLL_KEY);
      uivlUpdateSelections (vl, nsel);
      uivlSelectChgHandler (vl, nsel, VL_COL_UNKNOWN);
    }

    /* movement keys are handled internally */
    logProcEnd ("move-key");
    return UICB_STOP;
  }

  if (uiEventIsEnterKey (vl->wcont [VL_W_EVENTH])) {
    uivlRowClickHandler (vl, vl->currSelection, VL_COL_UNKNOWN);
    logProcEnd ("row-click");
    return UICB_STOP;
  }

  /* any unknown keys can be passed on to any registered user */
  if (vl->usercb [VL_USER_CB_KEY] != NULL) {
    int   rc;

    rc = callbackHandler (vl->usercb [VL_USER_CB_KEY]);
    logProcEnd ("user-key-cb");
    return rc;
  }

  logProcEnd ("");
  return UICB_CONT;
}

static bool
uivlMButtonEvent (void *udata, int32_t dispidx, int32_t colidx)
{
  uivirtlist_t  *vl = udata;
  int           button;
  int32_t       rownum = VL_UNK_ROW;

  logProcBegin ();
  if (vl == NULL || vl->ident != VL_IDENT) {
    logProcEnd ("bad-vl");
    return UICB_STOP;
  }
  if (vl->inscroll) {
    logProcEnd ("in-scroll");
    return UICB_CONT;
  }

  if (! uiEventIsButtonPressEvent (vl->wcont [VL_W_EVENTH]) &&
      ! uiEventIsButtonDoublePressEvent (vl->wcont [VL_W_EVENTH])) {
    logProcEnd ("not-press");
    return UICB_CONT;
  }

  /* double clicks on entry fields must be ignored, as the */
  /* row number is incorrect */
  if (vl->allowdblclick == false &&
      uiEventIsButtonDoublePressEvent (vl->wcont [VL_W_EVENTH])) {
    logProcEnd ("is-dbl-not-allow");
    return UICB_CONT;
  }

  button = uiEventGetButton (vl->wcont [VL_W_EVENTH]);

  /* button 4 and 5 cause a single scroll event */
  if (button == UIEVENT_BUTTON_4 || button == UIEVENT_BUTTON_5) {
    int32_t     start;

    start = vl->rowoffset;

    if (button == UIEVENT_BUTTON_4) {
      start -= 1;
    }
    if (button == UIEVENT_BUTTON_5) {
      start += 1;
    }

    uivlProcessScroll (vl, start, VL_SCROLL_FORCE);
    logProcEnd ("butt-4-5");
    return UICB_CONT;
  }

  /* all other buttons (1-3) cause a selection */

  if (dispidx >= 0) {
    rownum = uivlCalcRownum (vl, dispidx);
  }
  if (dispidx == 0 && vl->dispheading) {
    logProcEnd ("bad-dispidx");
    return UICB_CONT;
  }

  if (rownum < 0 || rownum >= vl->numrows) {
    /* not found or past limit */
    logProcEnd ("bad-rownum");
    return UICB_CONT;
  }

  /* the mouse button event handler returns the real column that */
  /* is displayed, search the column data to find the correct */
  /* column index. the hair space label is at index 0. */
  if (colidx >= 1) {
    /* adjust for the hair space label that is displayed */
    /* at the beginning of the row */
    --colidx;
    for (int tcolidx = 0; tcolidx < vl->numcols; ++tcolidx) {
      if (vl->coldata [tcolidx].clickmap == colidx) {
        colidx = tcolidx;
        break;
      }
    }
  }

  uivlUpdateSelections (vl, rownum);
  /* call the selection handler before the double-click handler */
  /* selection change */
  uivlSelectChgHandler (vl, rownum, colidx);
  if (uiEventIsButtonPressEvent (vl->wcont [VL_W_EVENTH])) {
    if (button == UIEVENT_BUTTON_3) {
      uivlRightClickHandler (vl, rownum, colidx);
    } else {
      uivlRowClickHandler (vl, rownum, colidx);
    }
  }
  if (uiEventIsButtonDoublePressEvent (vl->wcont [VL_W_EVENTH])) {
    uivlDoubleClickHandler (vl, rownum, colidx);
  }

  logProcEnd ("");
  return UICB_CONT;
}

static bool
uivlScrollEvent (void *udata, int32_t dir)
{
  uivirtlist_t  *vl = udata;
  int32_t       start;

  logProcBegin ();
  if (vl == NULL || vl->ident != VL_IDENT) {
    logProcEnd ("bad-vl");
    return UICB_CONT;
  }
  if (vl->inscroll) {
    logProcEnd ("in-scroll");
    return UICB_CONT;
  }

  start = vl->rowoffset;
  if (dir == UIEVENT_DIR_PREV || dir == UIEVENT_DIR_LEFT) {
    start -= 1;
  }
  if (dir == UIEVENT_DIR_NEXT || dir == UIEVENT_DIR_RIGHT) {
    start += 1;
  }
  uivlProcessScroll (vl, start, VL_SCROLL_FORCE);

  logProcEnd ("");
  return UICB_CONT;
}

static void
uivlClearDisplaySelections (uivirtlist_t *vl)
{
  logProcBegin ();
  for (int dispidx = vl->headingoffset; dispidx < vl->dispsize; ++dispidx) {
    uivlrow_t   *row = &vl->rows [dispidx];

    if (row->selected) {
      uiWidgetRemoveClass (row->hbox, SELECTED_CLASS);

      for (int colidx = 0; colidx < vl->numcols; ++colidx) {
        if (vl->coldata [colidx].hidden == VL_COL_SHOW) {
          uiWidgetRemoveClass (row->cols [colidx].uiwidget, SELECTED_CLASS);
        }
      }
      row->selected = false;
    }
  }
  logProcEnd ("");
}

static void
uivlSetDisplaySelections (uivirtlist_t *vl)
{
  nlistidx_t    iter;
  nlistidx_t    rownum;

  logProcBegin ();
  nlistStartIterator (vl->selected, &iter);
  while ((rownum = nlistIterateKey (vl->selected, &iter)) >= 0) {
    uivlrow_t   *row;

    row = uivlGetRow (vl, rownum);
    if (row == NULL) {
      continue;
    }
    if (row->cleared) {
      continue;
    }

    if (vl->keyhandling == false &&
        row->dispidx != vl->lastselidx) {
      /* if the actual row selection has changed, any focus that is set */
      /* needs to be cleared. */
      /* the focus callback sets lastselidx, so that the focus does not */
      /* get cleared inadverdently. */
      uiWindowClearFocus (vl->parentwin);
    }
    vl->lastselidx = row->dispidx;

    uivlRemoveLastHighlight (vl);
    uiWidgetAddClass (row->hbox, SELECTED_CLASS);
    row->selected = true;
    for (int colidx = 0; colidx < vl->numcols; ++colidx) {
      if (vl->coldata [colidx].hidden == VL_COL_SHOW) {
        uiWidgetAddClass (row->cols [colidx].uiwidget, SELECTED_CLASS);
      }
    }
  }
  logProcEnd ("");
}

static void
uivlClearAllSelections (uivirtlist_t *vl)
{
  logProcBegin ();
  nlistFree (vl->selected);
  vl->selected = nlistAlloc ("vl-selected", LIST_ORDERED, NULL);
  logProcEnd ("");
}

static void
uivlClearSelection (uivirtlist_t *vl, int32_t rownum)
{
  nlist_t     *newsel;
  nlistidx_t  iteridx;
  nlistidx_t  key;

  logProcBegin ();
  newsel = nlistAlloc ("vl-selected", LIST_UNORDERED, NULL);
  nlistSetSize (newsel, nlistGetCount (vl->selected));
  nlistStartIterator (vl->selected, &iteridx);
  while ((key = nlistIterateKey (vl->selected, &iteridx)) >= 0) {
    if (key == rownum) {
      continue;
    }
    nlistSetNum (newsel, key, true);
    vl->currSelection = key;
  }
  nlistSort (newsel);
  nlistFree (vl->selected);
  vl->selected = newsel;
  logProcEnd ("");
}

static void
uivlAddSelection (uivirtlist_t *vl, uint32_t rownum)
{
  logProcBegin ();
  nlistSetNum (vl->selected, rownum, true);
  vl->currSelection = rownum;
  logProcEnd ("");
}

static void
uivlProcessScroll (uivirtlist_t *vl, int32_t start, int sctype)
{
  int32_t       wantrow = start;

  logProcBegin ();
  if (vl->inscroll) {
    logProcEnd ("in-scroll");
    return;
  }

  vl->inscroll = true;

  start = uivlRowOffsetLimit (vl, start);

  /* if this is a keyboard movement, and there's only one selection */
  /* and the desired row-number is on-screen */
  if (sctype == VL_SCROLL_KEY &&
      nlistGetCount (vl->selected) == 1 &&
      wantrow >= vl->rowoffset &&
      wantrow < vl->rowoffset + (vl->dispsize - vl->headingoffset)) {
    if (wantrow < vl->currSelection) {
      /* selection up */
      if (wantrow < vl->rowoffset + (vl->dispsize - vl->headingoffset) / 2 - 1) {
        start = vl->rowoffset - 1;
      } else {
        vl->inscroll = false;
        logProcEnd ("at-beg");
        return;
      }
    } else {
      /* selection down */
      if (wantrow >= vl->rowoffset + (vl->dispsize - vl->headingoffset) / 2) {
        start = vl->rowoffset + 1;
      } else {
        vl->inscroll = false;
        logProcEnd ("at-end");
        return;
      }
    }

    start = uivlRowOffsetLimit (vl, start);
  }

  /* if this is a normal selection, and there's only one selection */
  /* and the desired row-number is on-screen */
  if (sctype == VL_SCROLL_NORM &&
      nlistGetCount (vl->selected) == 1 &&
      wantrow >= vl->rowoffset &&
      wantrow < vl->rowoffset + (vl->dispsize - vl->headingoffset)) {
    start = vl->rowoffset;
  }

  if (start == vl->rowoffset) {
    vl->inscroll = false;
    logProcEnd ("no-change");
    return;
  }

  vl->rowoffset = start;
  uivlPopulate (vl);
  uiScrollbarSetPosition (vl->wcont [VL_W_SB], (double) start);
  uivlDisplayChgHandler (vl);
  vl->inscroll = false;
  logProcEnd ("");
}

static bool
uivlVertSizeChg (void *udata, int32_t width, int32_t height)
{
  uivirtlist_t  *vl = udata;
  int           calcrows;
  int           theight;

  logProcBegin ();
  if (uiWidgetIsMapped (vl->wcont [VL_W_MAIN_VBOX]) &&
      vl->vboxheight == height) {
    logProcEnd ("same-height");
    return UICB_CONT;
  }

  if (height == VL_REUSE_HEIGHT) {
    height = vl->vboxheight;
  }

  vl->vboxheight = height;

  if (vl->vboxheight < 2 || vl->rowheight < 2) {
    logProcEnd ("no-height");
    return UICB_CONT;
  }

  theight = vl->vboxheight - vl->headingheight;
  calcrows = theight / vl->rowheight;
  if (vl->dispheading) {
    /* must include the heading as a row */
    calcrows += 1;
  }

  if (calcrows != vl->dispsize) {
    /* i don't recall why these were put in */
    /* i don't think they're needed */
    /* will leave them here, commented out for now */
    // uiWidgetSetSizeRequest (vl->wcont [VL_W_MAIN_VBOX], -1, height - 10);
    uivlChangeDisplaySize (vl, calcrows);
    // uiWidgetSetSizeRequest (vl->wcont [VL_W_MAIN_VBOX], -1, -1);
  }

  logProcEnd ("");
  return UICB_CONT;
}

static bool
uivlHeadingSizeChg (void *udata, int32_t width, int32_t height)
{
  uivirtlist_t  *vl = udata;
  bool          force = false;

  logProcBegin ();
  /* force a re-calculation */
  if (vl->headingheight != height && vl->vboxheight > 2) {
    force = true;
  }
  vl->headingheight = height;
  if (force) {
    vl->vboxheight = -1;
    uivlVertSizeChg (vl, VL_REUSE_HEIGHT, VL_REUSE_HEIGHT);
  }
  logProcEnd ("");
  return UICB_CONT;
}

static bool
uivlRowSizeChg (void *udata, int32_t width, int32_t height)
{
  uivirtlist_t  *vl = udata;
  bool          force = false;

  logProcBegin ();
  /* force a re-calculation */
  if (vl->rowheight != height && vl->vboxheight > 2) {
    force = true;
  }
  vl->rowheight = height;
  if (force) {
    vl->vboxheight = -1;
    uivlVertSizeChg (vl, VL_REUSE_HEIGHT, VL_REUSE_HEIGHT);
  }
  logProcEnd ("");
  return UICB_CONT;
}

/* the intention of the column size change is to prevent */
/* grow-only columns from shrinking */
static bool
uivlColGrowOnlyChg (void *udata, int32_t width, int32_t height)
{
  uivlcoldata_t   *coldata = udata;

  logProcBegin ();
  if (width > 0 && width > coldata->colwidth) {
    if (uiWidgetIsMapped (coldata->col0->uiwidget)) {
      coldata->colwidth = width;
      width -= 1;
      if (width > 5) {
        uiWidgetSetSizeRequest (coldata->col0->uiwidget, width, -1);
      }
    }
  }

  logProcEnd ("");
  return UICB_CONT;
}

static void
uivlRowBasicInit (uivirtlist_t *vl, uivlrow_t *row, int dispidx)
{
  row->vl = vl;
  row->ident = VL_IDENT_ROW;
  row->hbox = NULL;
  row->szgrp = NULL;
  row->cols = NULL;
  row->created = false;
  row->cleared = false;
  row->rowcb = NULL;
  row->dispidx = dispidx;
  row->initialized = true;
  row->oldclass = NULL;
  row->currclass = NULL;
  row->newclass = NULL;
  row->lockrownum = VL_ROW_NO_LOCK;

  if (dispidx != VL_ROW_HEADING_IDX) {
    row->rowcb = mdmalloc (sizeof (uivlrowcb_t));
    row->rowcb->vl = vl;
    row->rowcb->dispidx = dispidx;
    row->rowcb->focuscb = callbackInit (uivlFocusCallback, row->rowcb, NULL);
  }
}

/* this handler must return UICB_CONT */
static bool
uivlFocusCallback (void *udata)
{
  uivlrowcb_t   *rowcb = udata;
  uivirtlist_t  *vl = rowcb->vl;
  int32_t       rownum;

  logProcBegin ();
  rownum = uivlCalcRownum (vl, rowcb->dispidx);
  vl->lastselidx = rowcb->dispidx;

  uivlUpdateSelections (vl, rownum);
  uivlSelectChgHandler (vl, rownum, VL_COL_UNKNOWN);

  logProcEnd ("");
  return UICB_CONT;
}

static void
uivlUpdateSelections (uivirtlist_t *vl, int32_t rownum)
{
  bool    controlpressed = false;
  bool    shiftpressed = false;
  bool    already = false;

  logProcBegin ();

  if (vl->wcont [VL_W_EVENTH] != NULL) {
    if (uiEventIsControlPressed (vl->wcont [VL_W_EVENTH])) {
      controlpressed = true;
    }
    if (uiEventIsShiftPressed (vl->wcont [VL_W_EVENTH])) {
      shiftpressed = true;
    }
  }

  if (vl->allowmultiple == false || ! controlpressed) {
    uivlClearAllSelections (vl);
  }

  if (vl->allowmultiple == true && shiftpressed) {
    int32_t   min = rownum;
    int32_t   max = rownum;

    if (vl->lastSelection < rownum) {
      min = vl->lastSelection;
      max = rownum;
    } else {
      min = rownum;
      max = vl->lastSelection;
    }
    for (int32_t trownum = min; trownum <= max; ++trownum) {
      uivlAddSelection (vl, trownum);
    }
  }

  if (vl->allowmultiple == true && controlpressed) {
    if (nlistGetNum (vl->selected, rownum) == true) {
      already = true;
    }
  }

  if (vl->allowmultiple == true && controlpressed && already) {
    uivlClearSelection (vl, rownum);
  } else {
    /* in all other cases, the selection is added */
    uivlAddSelection (vl, rownum);
  }

  uivlClearDisplaySelections (vl);
  uivlSetDisplaySelections (vl);
  if (nlistGetCount (vl->selected) == 1) {
    vl->lastSelection = vl->currSelection;
  }
  logProcEnd ("");
}


int32_t
uivlRowOffsetLimit (uivirtlist_t *vl, int32_t rowoffset)
{
  int   tdispsize;

  if (rowoffset < 0) {
    rowoffset = 0;
  }

  /* the number of rows may be less than the available display size */
  tdispsize = vl->dispsize - vl->headingoffset;
  if (vl->numrows < tdispsize) {
    tdispsize = vl->numrows;
  }
  if (rowoffset > vl->numrows - tdispsize) {
    rowoffset = vl->numrows - tdispsize;
  }

  return rowoffset;
}

int32_t
uivlRownumLimit (uivirtlist_t *vl, int32_t rownum)
{
  if (rownum < 0) {
    rownum = 0;
  }
  if (rownum >= vl->numrows) {
    rownum = vl->numrows - 1;
  }

  return rownum;
}

static void
uivlSelectChgHandler (uivirtlist_t *vl, int32_t rownum, int32_t colidx)
{
  logProcBegin ();
  if (vl->inscroll) {
    logProcEnd ("in-scroll");
    return;
  }

  if (vl->selchgcb != NULL) {
    vl->selchgcb (vl->seludata, vl, rownum, colidx);
  }
  logProcEnd ("");
}

static void
uivlDisplayChgHandler (uivirtlist_t *vl)
{
  logProcBegin ();
  if (vl->usercb [VL_USER_CB_DISP_CHG] != NULL) {
    callbackHandler (vl->usercb [VL_USER_CB_DISP_CHG]);
  }
  logProcEnd ("");
}

static void
uivlRowClickHandler (uivirtlist_t *vl, int32_t rownum, int32_t colidx)
{
  logProcBegin ();
  if (vl->rowselcb != NULL) {
    vl->rowselcb (vl->rowseludata, vl, rownum, colidx);
  }
  logProcEnd ("");
}

static void
uivlDoubleClickHandler (uivirtlist_t *vl, int32_t rownum, int32_t colidx)
{
  logProcBegin ();
  if (vl->dblclickcb != NULL) {
    vl->dblclickcb (vl->dblclickudata, vl, rownum, colidx);
  }
  logProcEnd ("");
}

static void
uivlRightClickHandler (uivirtlist_t *vl, int32_t rownum, int32_t colidx)
{
  logProcBegin ();
  if (vl->rightclickcb != NULL) {
    vl->rightclickcb (vl->rightclickudata, vl, rownum, colidx);
  }
  logProcEnd ("");
}

static void
uivlSetToggleChangeCallback (uivirtlist_t *vl, int colidx, callback_t *cb)
{
  logProcBegin ();
  if (vl == NULL || vl->ident != VL_IDENT) {
    logProcEnd ("bad-vl");
    return;
  }
  if (colidx < 0 || colidx >= vl->numcols) {
    logProcEnd ("bad-col");
    return;
  }
  if (vl->coldata [colidx].type != VL_TYPE_RADIO_BUTTON &&
     vl->coldata [colidx].type != VL_TYPE_CHECKBOX) {
    logProcEnd ("not-toggle");
    return;
  }

  vl->coldata [colidx].togglecb = cb;
  logProcEnd ("");
}

static void
uivlClearRowDisp (uivirtlist_t *vl, int dispidx)
{
  uivlrow_t   *row;

  logProcBegin ();
  if (vl == NULL || vl->ident != VL_IDENT) {
    logProcEnd ("bad-vl");
    return;
  }
  if (dispidx < 0 || dispidx >= vl->dispsize) {
    logProcEnd ("bad-dispidx");
    return;
  }

  row = &vl->rows [dispidx];
  if (! row->created) {
    logProcEnd ("not-created");
    return;
  }
  if (row->cleared) {
    logProcEnd ("cleared");
    return;
  }

  row->cleared = true;

  for (int colidx = 0; colidx < vl->numcols; ++colidx) {
    int     hidden;

    hidden = vl->coldata [colidx].hidden;
    if (hidden == VL_COL_HIDE || hidden == VL_COL_DISABLE) {
      continue;
    }

    uiWidgetHide (row->cols [colidx].uiwidget);
  }
  logProcEnd ("");
}

static bool
uivlValidateColumn (uivirtlist_t *vl, int initstate, int colidx, const char *func)
{
  if (vl == NULL || vl->ident != VL_IDENT) {
    return false;
  }
  if (vl->initialized < initstate) {
    logMsg (LOG_DBG, LOG_VIRTLIST, "vl: %s %s not-init (%d<%d)", vl->tag, func, vl->initialized, initstate);
    return false;
  }
  if (colidx < 0 || colidx >= vl->numcols) {
    return false;
  }

  return true;
}

static bool
uivlValidateRowColumn (uivirtlist_t *vl, int initstate, int32_t rownum, int colidx, const char *func)
{
  bool    rc;

  rc = uivlValidateColumn (vl, initstate, colidx, func);
  if (rc && rownum != VL_ROW_HEADING && (rownum < 0 || rownum >= vl->numrows)) {
    rc = false;
  }

  return rc;
}

static void
uivlChangeDisplaySize (uivirtlist_t *vl, int newdispsize)
{
  bool    dispchg = false;

  logProcBegin ();
  /* only if the number of rows has increased */
  if (vl->dispalloc < newdispsize) {
    vl->rows = mdrealloc (vl->rows, sizeof (uivlrow_t) * newdispsize);

    for (int dispidx = vl->dispalloc; dispidx < newdispsize; ++dispidx) {
      uivlrow_t *row;

      row = &vl->rows [dispidx];
      uivlRowBasicInit (vl, row, dispidx);
      uivlCreateRow (vl, row, dispidx, false);

      uivlPackRow (vl, row);
      /* rows packed after the initial display need */
      /* to have their contents shown */
      uiWidgetShowAll (row->hbox);
      uivlShowRow (vl, row);
    }

    vl->dispalloc = newdispsize;
  }

  /* if the number of display rows has decreased, */
  /* clear the row display, make sure these widgets are not displayed */
  if (newdispsize < vl->dispsize) {
    logMsg (LOG_DBG, LOG_VIRTLIST, "vl: %s disp-size-decrease %d < %d", vl->tag, newdispsize, vl->dispsize);
    for (int dispidx = newdispsize; dispidx < vl->dispsize; ++dispidx) {
      uivlClearRowDisp (vl, dispidx);
    }
  }

  vl->dispsize = newdispsize;
  logMsg (LOG_DBG, LOG_VIRTLIST, "vl: %s disp-size: %d", vl->tag, newdispsize);

  /* if the vertical size changes, and there are enough rows */
  /* to fill the display, change the row-offset so that the display */
  /* is filled. */
  /* this happens when the display is scrolled to the bottom, */
  /* and the vertical size is increased. */
  if ((vl->dispsize - vl->headingoffset) + vl->rowoffset > vl->numrows) {
    int32_t   diff;

    diff = vl->numrows - ((vl->dispsize - vl->headingoffset) + vl->rowoffset);
    vl->rowoffset += diff;
    vl->rowoffset = uivlRowOffsetLimit (vl, vl->rowoffset);
    dispchg = true;
  }

  /* if the display size is increased, make sure any newly allocated */
  /* rows are cleared. */
  /* this is necessary when the number of rows is less than the */
  /* display size. the clear must be forced. */
  if ((vl->dispsize - vl->headingoffset) > vl->numrows) {
    logMsg (LOG_DBG, LOG_VIRTLIST, "vl: %s disp-size-limit %d > %d", vl->tag, vl->dispsize - vl->headingoffset, vl->numrows);
    for (int dispidx = vl->numrows + vl->headingoffset; dispidx < vl->dispsize; ++dispidx) {
      uivlrow_t *row;

      /* force a reset. if the row was newly packed, a show-all was done, */
      /* and the rows must be cleared. */
      row = &vl->rows [dispidx];
      row->cleared = false;
      uivlClearRowDisp (vl, dispidx);
    }
  }

  uivlConfigureScrollbar (vl);
  uivlPopulate (vl);
  if (dispchg) {
    uivlDisplayChgHandler (vl);
  }
  logProcEnd ("");
}

static void
uivlConfigureScrollbar (uivirtlist_t *vl)
{
  logProcEnd ("");
  uiScrollbarSetUpper (vl->wcont [VL_W_SB], (double) vl->numrows);
  uiScrollbarSetPageIncrement (vl->wcont [VL_W_SB],
      (double) ((vl->dispsize - vl->headingoffset) - vl->lockcount) / 2);
  uiScrollbarSetPageSize (vl->wcont [VL_W_SB],
      (double) ((vl->dispsize - vl->headingoffset) - vl->lockcount));

  if (vl->numrows <= (vl->dispsize - vl->headingoffset)) {
    uiWidgetHide (vl->wcont [VL_W_SB]);
  } else {
    uiWidgetShow (vl->wcont [VL_W_SB]);
  }
  logProcEnd ("");
}

static int
uivlCalcDispidx (uivirtlist_t *vl, int32_t rownum)
{
  int   dispidx;

  dispidx = rownum - vl->rowoffset + vl->headingoffset;
  return dispidx;
}

static int32_t
uivlCalcRownum (uivirtlist_t *vl, int dispidx)
{
  int32_t   rownum;

  rownum = (dispidx - vl->headingoffset) + vl->rowoffset;
  return rownum;
}

static void
uivlShowRow (uivirtlist_t *vl, uivlrow_t *row)
{
  logProcEnd ("");
  if (row->offscreen) {
    uiWidgetShow (row->hbox);
    row->offscreen = false;
  }

  for (int colidx = 0; colidx < vl->numcols; ++colidx) {
    int     hidden;

    if (row->cols [colidx].uiwidget == NULL) {
      continue;
    }
    hidden = vl->coldata [colidx].hidden;

    if (hidden == VL_COL_HIDE) {
      uiWidgetHide (row->cols [colidx].uiwidget);
    }
    if (hidden == VL_COL_SHOW) {
      uiWidgetShow (row->cols [colidx].uiwidget);
    }
  }
  row->cleared = false;
  logProcEnd ("");
}

static bool
uivlEnterEvent (void *udata)
{
  uivirtlist_t  *vl = udata;

  logProcBegin ();
  /* this works very nicely so that after switching to a different */
  /* listing, the user can continue to work with the selections */
  /* without resetting them. */
  if (vl->keyhandling) {
    uiWidgetGrabFocus (vl->wcont [VL_W_MAIN_VBOX]);
  }
  uivlRemoveLastHighlight (vl);

  logProcEnd ("");
  return UICB_CONT;
}

/* the motion event handler handles highlighting the row as the mouse */
/* moves over it */
static bool
uivlMotionEvent (void *udata, int32_t dispidx)
{
  uivirtlist_t  *vl = udata;
  uivlrow_t     *row;

  logProcBegin ();
  if (vl == NULL || vl->ident != VL_IDENT) {
    logProcEnd ("bad-vl");
    return UICB_CONT;
  }

  /* always remove the last highlight */
  uivlRemoveLastHighlight (vl);

  if (dispidx < 0 || dispidx >= vl->dispsize) {
    return UICB_CONT;
  }

  if (vl->dispheading && dispidx == VL_ROW_HEADING_IDX) {
    return UICB_CONT;
  }

  row = &vl->rows [dispidx];
  if (row->selected) {
    /* do not highlight rows that are currently selected */
    return UICB_CONT;
  }
  if (row->cleared || row->offscreen) {
    /* do not highlight cleared or off-screen rows */
    return UICB_CONT;
  }

  uiWidgetAddClass (row->hbox, ROW_HL_CLASS);
  vl->lastdisphighlight = dispidx;

  logProcEnd ("");
  return UICB_CONT;
}


static void
uivlCheckDisplay (uivirtlist_t *vl)
{
  bool    dispchg = false;

  logProcBegin ();
  if (vl->initialized < VL_INIT_ROWS) {
    logProcEnd ("not-init");
    return;
  }
  if (! vl->numrowchg) {
    return;
  }
  vl->numrowchg = false;

  /* if the number of rows has changed, and the selection is no longer */
  /* valid, move it up */
  /* check this: this may be handled elsewhere */
  if (vl->numrows <= vl->currSelection) {
    uivlMoveSelection (vl, VL_DIR_PREV);
  }

  /* if the number of data rows is less than the display size, */
  /* the extra rows must have their display cleared */
  if ((vl->dispsize - vl->headingoffset) > vl->numrows) {
    for (int dispidx = vl->numrows + vl->headingoffset; dispidx < vl->dispsize; ++dispidx) {
      uivlClearRowDisp (vl, dispidx);
    }
  }

  /* if a row has been removed, but there are enough rows to fill the */
  /* display, change the row-offset so that the display is filled */
  /* note that the gtk scrollbar does not always update properly */
  /* when this is done. */
  if ((vl->dispsize - vl->headingoffset) + vl->rowoffset > vl->numrows) {
    int32_t   diff;

    /* usually this happens due to a removal of a row, */
    /* and diff will be -1 */
    diff = vl->numrows - ((vl->dispsize - vl->headingoffset) + vl->rowoffset);
    vl->rowoffset += diff;
    vl->rowoffset = uivlRowOffsetLimit (vl, vl->rowoffset);
    dispchg = true;
  }

  if (dispchg) {
    uivlDisplayChgHandler (vl);
  }
}

static void
uivlRemoveLastHighlight (uivirtlist_t *vl)
{
  uivlrow_t   *row;

  if (vl->lastdisphighlight >= 0) {
    row = &vl->rows [vl->lastdisphighlight];
    uiWidgetRemoveClass (row->hbox, ROW_HL_CLASS);
    vl->lastdisphighlight = VL_UNK_ROW;
  }
}
