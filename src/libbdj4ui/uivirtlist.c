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

#include "callback.h"
#include "log.h"
#include "mdebug.h"
#include "nlist.h"
#include "tmutil.h"
#include "ui.h"
#include "uivirtlist.h"
#include "uiwcont.h"

/* initialization states */
enum {
  VL_INIT_NONE,
  VL_INIT_BASIC,
  VL_INIT_HEADING,      // may be skipped
  VL_INIT_ROWS,
  VL_INIT_DISP,
};

enum {
  VL_ROW_UNKNOWN = -753,
  VL_ROW_HEADING = -754,      // some weird number
  VL_MIN_WIDTH_ANY = -755,
  VL_SCROLL_NORM = false,
  VL_SCROLL_FORCE = true,
  VL_DOUBLE_CLICK_TIME = 200,   // milliseconds
};

enum {
  VL_W_SCROLL_WIN,
  VL_W_HEADBOX,
  VL_W_EVENT_BOX,
  VL_W_MAIN_HBOX,
  VL_W_MAIN_VBOX,
  VL_W_SB,
  VL_W_SB_SZGRP,
  VL_W_HEAD_FILLER,
  VL_W_KEYH,
  VL_W_MAX,
};

enum {
  VL_CB_SB,
  VL_CB_KEY,
  VL_CB_MBUTTON,
  VL_CB_SCROLL,
  VL_CB_VERT_SZ_CHG,
  VL_CB_ROW_SZ_CHG,
  VL_CB_MAX,
};

enum {
  VL_USER_CB_ENTRY_VAL,
  VL_USER_CB_RB_CHG,
  VL_USER_CB_CB_CHG,
  VL_USER_CB_MAX,
};

enum {
  VL_IDENT_COLDATA  = 0x766c636f6c647400,
  VL_IDENT_COL      = 0x766c636f6c00bbcc,
  VL_IDENT_ROW      = 0x766c726f7700bbcc,
  VL_IDENT          = 0x766c00bbccddeeff,
};

static const char * const VL_SELECTED_CLASS = "bdj-selected";
static const char * const VL_LIST_CLASS = "bdj-listing";
static const char * const VL_HEAD_CLASS = "bdj-heading";
static const char * const VL_DARKBG_CLASS = "bdj-dark-bg";

typedef struct uivlcol uivlcol_t;

typedef struct {
  uivirtlist_t    *vl;
  int             dispidx;
  callback_t      *focuscb;
} uivlrowcb_t;

typedef struct {
  uint64_t    ident;
  uiwcont_t   *szgrp;
  /* the following data is specific to a column */
  vltype_t    type;
  /* the baseclass is always applied */
  uientryval_t  entrycb;
  void        *entryudata;
  callback_t  *togglecb;      // radio buttons and check buttons
  callback_t  *spinboxcb;
  callback_t  *spinboxtimecb;
  callback_t  *colszcb;
  uivlcol_t   *col0;
  const char  *tag;
  char        *baseclass;
  int         minwidth;
  int         entrySz;
  int         entryMaxSz;
  int         sbtype;
  int         colidx;
  int         colwidth;
  callback_t  *sbcb;
  double      sbmin;
  double      sbmax;
  double      sbincr;
  double      sbpageincr;
  int         grow;
  /* is the entire column hidden? */
  /* note that this is not a true/false value */
  int         hidden;
  bool        alignend: 1;
  bool        ellipsize : 1;
} uivlcoldata_t;

typedef struct uivlcol {
  uint64_t  ident;
  uiwcont_t *box;
  uiwcont_t *uiwidget;
  /* class needs to be held temporarily so it can be removed */
  char      *class;
  int       colidx;
  int32_t   value;        // internal numeric value
} uivlcol_t;

typedef struct {
  uint64_t      ident;
  uivirtlist_t  *vl;
  uiwcont_t     *hbox;
  uivlcol_t     *cols;
  uivlrowcb_t   *rowcb;             // must have a stable address
  int           dispidx;
  /* cleared: row is on-screen, no display */
  /* all widgets are hidden */
  bool          cleared : 1;
  bool          created : 1;
  /* offscreen: row is off-screen, entire row is hidden */
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
  uiwcont_t     *wcont [VL_W_MAX];
  callback_t    *callbacks [VL_CB_MAX];
  callback_t    *usercb [VL_USER_CB_MAX];
  mstime_t      doubleClickCheck;
  int           numcols;
  uivlcoldata_t *coldata;
  uivlrow_t     *rows;
  uivlrow_t     headingrow;
  int           dispsize;
  int           dispalloc;
  int           vboxheight;
  int           vboxwidth;
  int           rowheight;
  int32_t       numrows;
  int32_t       rowoffset;
  int32_t       lastSelection;    // used for shift-click
  int32_t       currSelection;
  nlist_t       *selected;
  int           initialized;
  /* user callbacks */
  uivlfillcb_t  fillcb;
  void          *filludata;
  uivlselcb_t   selcb;
  void          *seludata;
  uivlselcb_t   dclickcb;
  void          *dclickudata;
  /* flags */
  bool          inscroll : 1;
  bool          dispheading : 1;
  bool          darkbg : 1;
  bool          uselistingfont : 1;
  bool          allowmultiple : 1;
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
static void uivlClearDisplaySelections (uivirtlist_t *vl);
static void uivlSetDisplaySelections (uivirtlist_t *vl);
static void uivlClearSelections (uivirtlist_t *vl);
static void uivlAddSelection (uivirtlist_t *vl, uint32_t rownum);
static void uivlProcessScroll (uivirtlist_t *vl, int32_t start, int sctype);
static bool uivlVboxSizeChg (void *udata, int32_t width, int32_t height);
static bool uivlRowSizeChg (void *udata, int32_t width, int32_t height);
static bool uivlColSizeChg (void *udata, int32_t width, int32_t height);
static void uivlRowBasicInit (uivirtlist_t *vl, uivlrow_t *row, int dispidx);
static bool uivlFocusCallback (void *udata);
static void uivlUpdateSelections (uivirtlist_t *vl, int32_t rownum);
int32_t uivlRowOffsetLimit (uivirtlist_t *vl, int32_t rowoffset);
int32_t uivlRownumLimit (uivirtlist_t *vl, int32_t rownum);
static void uivlSelectionHandler (uivirtlist_t *vl, int32_t rownum, int32_t colidx);
static void uivlDoubleClickHandler (uivirtlist_t *vl, int32_t rownum, int32_t colidx);
static void uivlSetToggleChangeCallback (uivirtlist_t *vl, int colidx, callback_t *cb);
static void uivlClearRowDisp (uivirtlist_t *vl, int dispidx);
static bool uivlValidateColumn (uivirtlist_t *vl, int initstate, int colidx, const char *func);
static bool uivlValidateRowColumn (uivirtlist_t *vl, int initstate, int32_t rownum, int colidx, const char *func);
static void uivlRowDisplay (uivirtlist_t *vl, uivlrow_t *row);

uivirtlist_t *
uiCreateVirtList (const char *tag, uiwcont_t *boxp,
    int dispsize, int headingflag, int minwidth)
{
  uivirtlist_t  *vl;

  vl = mdmalloc (sizeof (uivirtlist_t));
  vl->ident = VL_IDENT;
  vl->tag = tag;
  vl->fillcb = NULL;
  vl->filludata = NULL;
  vl->selcb = NULL;
  vl->seludata = NULL;
  vl->dclickcb = NULL;
  vl->dclickudata = NULL;
  vl->inscroll = false;
  vl->dispheading = true;
  if (headingflag == VL_NO_HEADING) {
    vl->dispheading = false;
  }
  vl->allowmultiple = false;
  vl->darkbg = false;
  vl->uselistingfont = false;
  vl->vboxheight = -1;
  vl->vboxwidth = -1;
  vl->rowheight = -1;

  for (int i = 0; i < VL_W_MAX; ++i) {
    vl->wcont [i] = NULL;
  }
  for (int i = 0; i < VL_CB_MAX; ++i) {
    vl->callbacks [i] = NULL;
  }
  for (int i = 0; i < VL_USER_CB_MAX; ++i) {
    vl->usercb [i] = NULL;
  }

  vl->wcont [VL_W_KEYH] = uiEventAlloc ();
  vl->callbacks [VL_CB_KEY] = callbackInit (uivlKeyEvent, vl, NULL);
  vl->callbacks [VL_CB_MBUTTON] = callbackInitII (uivlMButtonEvent, vl);
  vl->callbacks [VL_CB_SCROLL] = callbackInitI (uivlScrollEvent, vl);
  vl->callbacks [VL_CB_VERT_SZ_CHG] = callbackInitII (uivlVboxSizeChg, vl);
  vl->callbacks [VL_CB_ROW_SZ_CHG] = callbackInitII (uivlRowSizeChg, vl);

  if (vl->dispheading) {
    vl->wcont [VL_W_HEADBOX] = uiCreateHorizBox ();
    uiWidgetAlignHorizStart (vl->wcont [VL_W_HEADBOX]);
    uiWidgetAlignVertStart (vl->wcont [VL_W_HEADBOX]);
    uiWidgetExpandHoriz (vl->wcont [VL_W_HEADBOX]);
    uiBoxPackStart (boxp, vl->wcont [VL_W_HEADBOX]);
  }

  /* a scrolled window is necessary to allow the window to shrink */
  vl->wcont [VL_W_SCROLL_WIN] = uiCreateScrolledWindow (400);
  uiWindowSetPolicyExternal (vl->wcont [VL_W_SCROLL_WIN]);
  uiWidgetExpandVert (vl->wcont [VL_W_SCROLL_WIN]);
  uiBoxPackStartExpand (boxp, vl->wcont [VL_W_SCROLL_WIN]);

  vl->wcont [VL_W_MAIN_HBOX] = uiCreateHorizBox ();
  uiWindowPackInWindow (vl->wcont [VL_W_SCROLL_WIN], vl->wcont [VL_W_MAIN_HBOX]);
  /* need a minimum width so it looks nice */
  if (minwidth != VL_NO_WIDTH) {
    uiWidgetSetSizeRequest (vl->wcont [VL_W_MAIN_HBOX], minwidth, -1);
  }

  vl->wcont [VL_W_MAIN_VBOX] = uiCreateVertBox ();
  uiWidgetExpandHoriz (vl->wcont [VL_W_MAIN_VBOX]);
  uiWidgetEnableFocus (vl->wcont [VL_W_MAIN_VBOX]);    // for mouse events

  /* the event box is necessary to receive mouse clicks */
  vl->wcont [VL_W_EVENT_BOX] = uiEventCreateEventBox (vl->wcont [VL_W_MAIN_VBOX]);
  uiWidgetExpandHoriz (vl->wcont [VL_W_EVENT_BOX]);
  uiBoxPackStartExpand (vl->wcont [VL_W_MAIN_HBOX], vl->wcont [VL_W_EVENT_BOX]);

  /* the size change callback must be set on the scroll-window */
  /* as the child windows within it only grow */
  uiWidgetSetSizeChgCallback (vl->wcont [VL_W_SCROLL_WIN], vl->callbacks [VL_CB_VERT_SZ_CHG]);

  vl->wcont [VL_W_SB_SZGRP] = uiCreateSizeGroupHoriz ();
  vl->wcont [VL_W_SB] = uiCreateVerticalScrollbar (10.0);
  uiSizeGroupAdd (vl->wcont [VL_W_SB_SZGRP], vl->wcont [VL_W_SB]);
  uiBoxPackEnd (vl->wcont [VL_W_MAIN_HBOX], vl->wcont [VL_W_SB]);

  vl->callbacks [VL_CB_SB] = callbackInitD (uivlScrollbarCallback, vl);
  uiScrollbarSetPageIncrement (vl->wcont [VL_W_SB], (double) (dispsize / 2));
  uiScrollbarSetStepIncrement (vl->wcont [VL_W_SB], 1.0);
  uiScrollbarSetPageSize (vl->wcont [VL_W_SB], (double) dispsize);
  uiScrollbarSetPosition (vl->wcont [VL_W_SB], 0.0);
  uiScrollbarSetUpper (vl->wcont [VL_W_SB], 0.0);
  uiScrollbarSetChangeCallback (vl->wcont [VL_W_SB], vl->callbacks [VL_CB_SB]);

  if (vl->dispheading) {
    vl->wcont [VL_W_HEAD_FILLER] = uiCreateLabel ("");
    uiSizeGroupAdd (vl->wcont [VL_W_SB_SZGRP], vl->wcont [VL_W_HEAD_FILLER]);
    uiBoxPackEnd (vl->wcont [VL_W_HEADBOX], vl->wcont [VL_W_HEAD_FILLER]);
  }

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

  vl->dispsize = dispsize;
  vl->dispalloc = dispsize;
  vl->rows = mdmalloc (sizeof (uivlrow_t) * vl->dispalloc);
  for (int dispidx = 0; dispidx < vl->dispalloc; ++dispidx) {
    uivlRowBasicInit (vl, &vl->rows [dispidx], dispidx);
  }

  uivlRowBasicInit (vl, &vl->headingrow, VL_ROW_HEADING);

  uiEventSetKeyCallback (vl->wcont [VL_W_KEYH], vl->wcont [VL_W_MAIN_VBOX],
      vl->callbacks [VL_CB_KEY]);
  uiEventSetButtonCallback (vl->wcont [VL_W_KEYH], vl->wcont [VL_W_EVENT_BOX],
      vl->callbacks [VL_CB_MBUTTON]);
  uiEventSetScrollCallback (vl->wcont [VL_W_KEYH], vl->wcont [VL_W_EVENT_BOX],
      vl->callbacks [VL_CB_SCROLL]);
  uiEventSetButtonCallback (vl->wcont [VL_W_KEYH], vl->wcont [VL_W_MAIN_VBOX],
      vl->callbacks [VL_CB_MBUTTON]);
  uiEventSetScrollCallback (vl->wcont [VL_W_KEYH], vl->wcont [VL_W_MAIN_VBOX],
      vl->callbacks [VL_CB_SCROLL]);

  return vl;
}

void
uivlFree (uivirtlist_t *vl)
{
  if (vl == NULL || vl->ident != VL_IDENT) {
    return;
  }

  for (int i = 0; i < VL_CB_MAX; ++i) {
    callbackFree (vl->callbacks [i]);
  }

  uivlFreeRow (vl, &vl->headingrow);

  for (int dispidx = 0; dispidx < vl->dispalloc; ++dispidx) {
    uivlFreeRow (vl, &vl->rows [dispidx]);
  }
  dataFree (vl->rows);

  for (int colidx = 0; colidx < vl->numcols; ++colidx) {
    dataFree (vl->coldata [colidx].baseclass);
    callbackFree (vl->coldata [colidx].colszcb);
    uiwcontFree (vl->coldata [colidx].szgrp);
  }
  dataFree (vl->coldata);

  nlistFree (vl->selected);

  for (int i = 0; i < VL_W_MAX; ++i) {
    uiwcontFree (vl->wcont [i]);
  }

  vl->ident = 0;
  mdfree (vl);
}

void
uivlSetNumRows (uivirtlist_t *vl, int32_t numrows)
{
  if (vl == NULL || vl->ident != VL_IDENT) {
    return;
  }

  vl->numrows = numrows;
  uiScrollbarSetUpper (vl->wcont [VL_W_SB], (double) numrows);
  logMsg (LOG_DBG, LOG_VIRTLIST, "vl: %s num-rows: %" PRId32, vl->tag, numrows);

  if (vl->initialized >= VL_INIT_ROWS) {
    if (numrows <= vl->currSelection) {
      uivlMoveSelection (vl, VL_DIR_PREV);
    }
    if (numrows < vl->dispsize) {
      for (int dispidx = numrows; dispidx < vl->dispsize; ++dispidx) {
        uivlClearRowDisp (vl, dispidx);
      }
    }
  }

  if (numrows <= vl->dispsize) {
    uiWidgetHide (vl->wcont [VL_W_SB]);
  } else {
    uiWidgetShow (vl->wcont [VL_W_SB]);
  }
}

void
uivlSetNumColumns (uivirtlist_t *vl, int numcols)
{
  if (vl == NULL || vl->ident != VL_IDENT) {
    return;
  }

  vl->numcols = numcols;
  vl->coldata = mdmalloc (sizeof (uivlcoldata_t) * numcols);
  for (int colidx = 0; colidx < numcols; ++colidx) {
    uivlcoldata_t *coldata = &vl->coldata [colidx];

    coldata->szgrp = uiCreateSizeGroupHoriz ();
    coldata->ident = VL_IDENT_COLDATA;
    coldata->tag = "unk";
    coldata->type = VL_TYPE_LABEL;
    coldata->entrycb = NULL;
    coldata->entryudata = NULL;
    coldata->togglecb = NULL;
    coldata->spinboxcb = NULL;
    coldata->spinboxtimecb = NULL;
    coldata->colszcb = NULL;
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
    coldata->ellipsize = false;
    coldata->grow = VL_COL_WIDTH_FIXED;
    coldata->hidden = VL_COL_SHOW;
  }

  vl->initialized = VL_INIT_BASIC;
  logMsg (LOG_DBG, LOG_VIRTLIST, "vl: %s num-cols: %d [init-basic]", vl->tag, numcols);
}

void
uivlSetDarkBackground (uivirtlist_t *vl)
{
  if (vl == NULL || vl->ident != VL_IDENT) {
    return;
  }

  vl->darkbg = true;
  uiWidgetAddClass (vl->wcont [VL_W_MAIN_HBOX], VL_DARKBG_CLASS);
  if (vl->dispheading) {
    uiWidgetAddClass (vl->wcont [VL_W_HEADBOX], VL_DARKBG_CLASS);
  }
}

void
uivlSetUseListingFont (uivirtlist_t *vl)
{
  if (vl == NULL || vl->ident != VL_IDENT) {
    return;
  }

  vl->uselistingfont = true;
}

void
uivlSetAllowMultiple (uivirtlist_t *vl)
{
  if (vl == NULL || vl->ident != VL_IDENT) {
    return;
  }

  vl->allowmultiple = true;
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

  if (vl->initialized < VL_INIT_HEADING) {
    uivlCreateRow (vl, &vl->headingrow, VL_ROW_HEADING, true);
    vl->initialized = VL_INIT_HEADING;
    logMsg (LOG_DBG, LOG_VIRTLIST, "vl: %s [init-heading]", vl->tag);
  }

  uivlSetRowColumnValue (vl, VL_ROW_HEADING, colidx, heading);
}

void
uivlMakeColumn (uivirtlist_t *vl, const char *tag, int colidx, vltype_t type)
{
  if (! uivlValidateColumn (vl, VL_INIT_BASIC, colidx, __func__)) {
    return;
  }

  vl->coldata [colidx].type = type;
  vl->coldata [colidx].tag = tag;
}

void
uivlMakeColumnEntry (uivirtlist_t *vl, const char *tag, int colidx, int sz, int maxsz)
{
  if (! uivlValidateColumn (vl, VL_INIT_BASIC, colidx, __func__)) {
    return;
  }

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

  vl->coldata [colidx].type = VL_TYPE_SPINBOX_NUM;
  vl->coldata [colidx].sbmin = min;
  vl->coldata [colidx].sbmax = max;
  vl->coldata [colidx].sbincr = incr;
  vl->coldata [colidx].sbpageincr = pageincr;
  vl->coldata [colidx].tag = tag;
}

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
  vl->coldata [colidx].grow = VL_COL_WIDTH_GROW;
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

  if (! uivlValidateColumn (vl, VL_INIT_BASIC, colidx, __func__)) {
    return;
  }

  if (vl->coldata [colidx].type == VL_TYPE_INTERNAL_NUMERIC) {
    return;
  }

  washidden = vl->coldata [colidx].hidden;
  vl->coldata [colidx].hidden = hidden;

  if (washidden != hidden) {
    if (vl->dispheading) {
      if (hidden == VL_COL_HIDE) {
        uiWidgetHide (vl->headingrow.cols [colidx].uiwidget);
      }
      if (hidden == VL_COL_SHOW) {
        uiWidgetShow (vl->headingrow.cols [colidx].uiwidget);
      }
    }
    for (int dispidx = 0; dispidx < vl->dispsize; ++dispidx) {
      uivlrow_t   *row;

      row = &vl->rows [dispidx];
      if (row->offscreen || row->cleared) {
        continue;
      }

      if (hidden == VL_COL_HIDE) {
        uiWidgetHide (row->cols [colidx].uiwidget);
      }
      if (hidden == VL_COL_SHOW) {
        uiWidgetShow (row->cols [colidx].uiwidget);
      }
    }
  }
}

void
uivlSetColumnClass (uivirtlist_t *vl, int colidx, const char *class)
{
  if (! uivlValidateColumn (vl, VL_INIT_BASIC, colidx, __func__)) {
    return;
  }

  dataFree (vl->coldata [colidx].baseclass);
  vl->coldata [colidx].baseclass = mdstrdup (class);
}

void
uivlSetRowColumnEditable (uivirtlist_t *vl, int32_t rownum, int colidx, int state)
{
  uivlrow_t   *row;

  if (! uivlValidateRowColumn (vl, VL_INIT_BASIC, rownum, colidx, __func__)) {
    return;
  }
  if (rownum == VL_ROW_HEADING) {
    return;
  }

  row = uivlGetRow (vl, rownum);
  if (row == NULL) {
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
    case VL_TYPE_CHECK_BUTTON:
    case VL_TYPE_RADIO_BUTTON: {
      uiToggleButtonSetState (row->cols [colidx].uiwidget, state);
      break;
    }
    case VL_TYPE_IMAGE:
    case VL_TYPE_INTERNAL_NUMERIC:
    case VL_TYPE_LABEL: {
      /* not necessary */
      break;
    }
  }

}

void
uivlSetRowColumnClass (uivirtlist_t *vl, int32_t rownum, int colidx, const char *class)
{
  uivlrow_t *row = NULL;
  uivlcol_t *col = NULL;

  if (! uivlValidateRowColumn (vl, VL_INIT_BASIC, rownum, colidx, __func__)) {
    return;
  }

  row = uivlGetRow (vl, rownum);
  if (row == NULL) {
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
}

void
uivlSetRowColumnValue (uivirtlist_t *vl, int32_t rownum, int colidx, const char *value)
{
  uivlrow_t   *row = NULL;
  vltype_t    type;

  if (vl == NULL || vl->ident != VL_IDENT) {
    return;
  }
  if (rownum == VL_ROW_HEADING && vl->initialized < VL_INIT_HEADING) {
    logMsg (LOG_DBG, LOG_VIRTLIST, "vl: %s not-init (%d<%d)", vl->tag, vl->initialized, VL_INIT_HEADING);
    return;
  }
  if (rownum != VL_ROW_HEADING && vl->initialized < VL_INIT_ROWS) {
    logMsg (LOG_DBG, LOG_VIRTLIST, "vl: %s not-init (%d<%d)", vl->tag, vl->initialized, VL_INIT_ROWS);
    return;
  }
  if (colidx < 0 || colidx >= vl->numcols) {
    return;
  }
  if (rownum != VL_ROW_HEADING && (rownum < 0 || rownum >= vl->numrows)) {
    return;
  }

  row = uivlGetRow (vl, rownum);
  if (row == NULL) {
    return;
  }

  type = vl->coldata [colidx].type;
  if (rownum == VL_ROW_HEADING) {
    type = VL_TYPE_LABEL;
  }

  switch (type) {
    case VL_TYPE_LABEL: {
      uiLabelSetText (row->cols [colidx].uiwidget, value);
      break;
    }
    case VL_TYPE_ENTRY: {
      uiEntrySetValue (row->cols [colidx].uiwidget, value);
      break;
    }
    case VL_TYPE_CHECK_BUTTON:
    case VL_TYPE_IMAGE:
    case VL_TYPE_INTERNAL_NUMERIC:
    case VL_TYPE_RADIO_BUTTON:
    case VL_TYPE_SPINBOX_NUM:
    case VL_TYPE_SPINBOX_TIME: {
      /* not handled here */
      break;
    }
  }

  if (row->offscreen == false && row->cleared) {
    uivlRowDisplay (vl, row);
  }
  row->cleared = false;
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
      uiWidgetSetSizeRequest (row->cols [colidx].uiwidget, width, -1);
      uiImageSetFromPixbuf (row->cols [colidx].uiwidget, img);
      break;
    }
    default: {
      /* not handled here */
      break;
    }
  }

  if (row->offscreen == false && row->cleared) {
    uivlRowDisplay (vl, row);
  }
  row->cleared = false;
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
    case VL_TYPE_CHECK_BUTTON:
    case VL_TYPE_RADIO_BUTTON: {
      int   nstate = UI_TOGGLE_BUTTON_OFF;

      if (val) {
        nstate = UI_TOGGLE_BUTTON_ON;
      }
      uiToggleButtonSetState (row->cols [colidx].uiwidget, nstate);
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
    uivlRowDisplay (vl, row);
  }
  row->cleared = false;
}

const char *
uivlGetRowColumnEntry (uivirtlist_t *vl, int32_t rownum, int colidx)
{
  uivlrow_t   *row;
  const char  *val = NULL;

  if (! uivlValidateRowColumn (vl, VL_INIT_ROWS, rownum, colidx, __func__)) {
    return NULL;
  }

  row = uivlGetRow (vl, rownum);
  if (row == NULL) {
    return NULL;
  }

  val = uiEntryGetValue (row->cols [colidx].uiwidget);
  return val;
}

int32_t
uivlGetRowColumnNum (uivirtlist_t *vl, int32_t rownum, int colidx)
{
  uivlrow_t   *row;
  int32_t     value = -1;

  if (! uivlValidateRowColumn (vl, VL_INIT_ROWS, rownum, colidx, __func__)) {
    return value;
  }

  row = uivlGetRow (vl, rownum);
  if (row == NULL) {
    return value;
  }

  switch (vl->coldata [colidx].type) {
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
    case VL_TYPE_CHECK_BUTTON:
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

  return value;
}

/* callbacks */

void
uivlSetSelectionCallback (uivirtlist_t *vl, uivlselcb_t cb, void *udata)
{
  if (! uivlValidateColumn (vl, VL_INIT_BASIC, 0, __func__)) {
    return;
  }

  vl->selcb = cb;
  vl->seludata = udata;
}

void
uivlSetDoubleClickCallback (uivirtlist_t *vl, uivlselcb_t cb, void *udata)
{
  if (! uivlValidateColumn (vl, VL_INIT_BASIC, 0, __func__)) {
    return;
  }

  vl->dclickcb = cb;
  vl->dclickudata = udata;
}

void
uivlSetRowFillCallback (uivirtlist_t *vl, uivlfillcb_t cb, void *udata)
{
  if (! uivlValidateColumn (vl, VL_INIT_BASIC, 0, __func__)) {
    return;
  }
  if (cb == NULL) {
    return;
  }

  vl->fillcb = cb;
  vl->filludata = udata;
}

void
uivlSetEntryValidation (uivirtlist_t *vl, int colidx,
    uientryval_t cb, void *udata)
{
  if (! uivlValidateColumn (vl, VL_INIT_BASIC, colidx, __func__)) {
    return;
  }
  if (vl->coldata [colidx].type != VL_TYPE_ENTRY) {
    return;
  }

  vl->coldata [colidx].entrycb = cb;
  vl->coldata [colidx].entryudata = udata;
}

void
uivlSetRadioChangeCallback (uivirtlist_t *vl, int colidx, callback_t *cb)
{
  uivlSetToggleChangeCallback (vl, colidx, cb);
}

void
uivlSetCheckboxChangeCallback (uivirtlist_t *vl, int colidx, callback_t *cb)
{
  uivlSetToggleChangeCallback (vl, colidx, cb);
}

void
uivlSetSpinboxTimeChangeCallback (uivirtlist_t *vl, int colidx, callback_t *cb)
{
  if (! uivlValidateColumn (vl, VL_INIT_BASIC, colidx, __func__)) {
    return;
  }
  if (vl->coldata [colidx].type != VL_TYPE_SPINBOX_TIME) {
    return;
  }

  vl->coldata [colidx].spinboxtimecb = cb;
}

void
uivlSetSpinboxChangeCallback (uivirtlist_t *vl, int colidx, callback_t *cb)
{
  if (! uivlValidateColumn (vl, VL_INIT_BASIC, colidx, __func__)) {
    return;
  }
  if (vl->coldata [colidx].type != VL_TYPE_SPINBOX_NUM) {
    return;
  }

  vl->coldata [colidx].spinboxcb = cb;
}


/* processing */

/* the initial display */
void
uivlDisplay (uivirtlist_t *vl)
{
  uivlrow_t   *row;

  if (! uivlValidateColumn (vl, VL_INIT_BASIC, 0, __func__)) {
    return;
  }

  for (int dispidx = 0; dispidx < vl->dispsize; ++dispidx) {
    uivlrow_t *row;

    row = &vl->rows [dispidx];
    uivlCreateRow (vl, row, dispidx, false);
  }
  vl->initialized = VL_INIT_ROWS;
  logMsg (LOG_DBG, LOG_VIRTLIST, "vl: %s [init-rows]", vl->tag);

  if (vl->dispheading) {
    uiBoxPackStart (vl->wcont [VL_W_HEADBOX], vl->headingrow.hbox);
  }

  for (int dispidx = 0; dispidx < vl->dispsize; ++dispidx) {
    row = &vl->rows [dispidx];
    uivlPackRow (vl, row);
    row->cleared = false;
    if (dispidx == 0) {
      uiBoxSetSizeChgCallback (row->hbox, vl->callbacks [VL_CB_ROW_SZ_CHG]);
    }
  }

  vl->initialized = VL_INIT_DISP;
  logMsg (LOG_DBG, LOG_VIRTLIST, "vl: %s [init-disp]", vl->tag);
  uiScrollbarSetPosition (vl->wcont [VL_W_SB], 0.0);
  uivlPopulate (vl);
}

/* display after a change */
void
uivlPopulate (uivirtlist_t *vl)
{
  if (! uivlValidateColumn (vl, VL_INIT_DISP, 0, __func__)) {
    return;
  }

  for (int dispidx = vl->dispsize; dispidx < vl->dispalloc; ++dispidx) {
    if (vl->rows [dispidx].offscreen) {
      continue;
    }
    vl->rows [dispidx].offscreen = true;
    uiWidgetHide (vl->rows [dispidx].hbox);
  }

  for (int dispidx = 0; dispidx < vl->dispsize; ++dispidx) {
    uivlrow_t   *row;
    uivlcol_t   *col;

    if (dispidx + vl->rowoffset >= vl->numrows) {
      break;
    }

    row = &vl->rows [dispidx];

    for (int colidx = 0; colidx < vl->numcols; ++colidx) {
      if (vl->coldata [colidx].hidden == VL_COL_HIDE) {
        continue;
      }
      col = &row->cols [colidx];
      if (col->class != NULL) {
        uiWidgetRemoveClass (col->uiwidget, col->class);
        dataFree (col->class);
        col->class = NULL;
      }
    }
  }

  for (int dispidx = 0; dispidx < vl->dispsize; ++dispidx) {
    uivlrow_t   *row;

    if (dispidx + vl->rowoffset >= vl->numrows) {
      break;
    }

    if (vl->fillcb != NULL) {
      vl->fillcb (vl->filludata, vl, dispidx + vl->rowoffset);
    }

    row = &vl->rows [dispidx];
    if (row->offscreen) {
      row->offscreen = false;
      uiWidgetShowAll (row->hbox);
    }
  }

  uivlClearDisplaySelections (vl);
  uivlSetDisplaySelections (vl);
}

void
uivlStartRowDispIterator (uivirtlist_t *vl, int32_t *rowiter)
{
  if (! uivlValidateColumn (vl, VL_INIT_BASIC, 0, __func__)) {
    return;
  }

  *rowiter = vl->rowoffset - 1;
}

int32_t
uivlIterateRowDisp (uivirtlist_t *vl, int32_t *rowiter)
{
  if (! uivlValidateColumn (vl, VL_INIT_BASIC, 0, __func__)) {
    return LIST_LOC_INVALID;
  }

  ++(*rowiter);
  if (*rowiter >= vl->numrows) {
    return LIST_LOC_INVALID;
  }
  if (*rowiter - vl->rowoffset >= vl->dispsize) {
    return LIST_LOC_INVALID;
  }
  return *rowiter;
}

void
uivlStartSelectionIterator (uivirtlist_t *vl, int32_t *iteridx)
{
  if (! uivlValidateColumn (vl, VL_INIT_BASIC, 0, __func__)) {
    return;
  }

  nlistStartIterator (vl->selected, iteridx);
}

int32_t
uivlIterateSelection (uivirtlist_t *vl, int32_t *iteridx)
{
  nlistidx_t    key = -1;

  if (! uivlValidateColumn (vl, VL_INIT_BASIC, 0, __func__)) {
    return key;
  }

  key = nlistIterateKey (vl->selected, iteridx);
  return key;
}

int32_t
uivlSelectionCount (uivirtlist_t *vl)
{
  if (! uivlValidateColumn (vl, VL_INIT_BASIC, 0, __func__)) {
    return 0;
  }

  return nlistGetCount (vl->selected);
}

int32_t
uivlGetCurrSelection (uivirtlist_t *vl)
{
  if (! uivlValidateColumn (vl, VL_INIT_DISP, 0, __func__)) {
    return 0;
  }

  return vl->currSelection;
}

void
uivlSetSelection (uivirtlist_t *vl, int32_t rownum)
{
  if (! uivlValidateRowColumn (vl, VL_INIT_DISP, rownum, 0, __func__)) {
    return;
  }
  if (rownum == VL_ROW_HEADING) {
    return;
  }

  uivlProcessScroll (vl, rownum, VL_SCROLL_NORM);
  uivlUpdateSelections (vl, rownum);
  uivlSelectionHandler (vl, rownum, VL_COL_UNKNOWN);
}

int32_t
uivlMoveSelection (uivirtlist_t *vl, int dir)
{
  int32_t     rownum;

  if (! uivlValidateColumn (vl, VL_INIT_DISP, 0, __func__)) {
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

  return rownum;
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

  for (int colidx = 0; colidx < vl->numcols; ++colidx) {
    if (row->cols != NULL) {
      uivlFreeCol (&row->cols [colidx]);
    }
  }
  uiwcontFree (row->hbox);
  dataFree (row->cols);
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
  uiwcontFree (col->box);
  uiwcontFree (col->uiwidget);
  col->box = NULL;
  col->uiwidget = NULL;
  col->ident = 0;
}

static void
uivlCreateRow (uivirtlist_t *vl, uivlrow_t *row, int dispidx, bool isheading)
{
  uiwcont_t   *uiwidget;

  if (row == NULL) {
    return;
  }
  if (! row->initialized) {
    fprintf (stderr, "%s create-row: not-init %d\n", vl->tag, dispidx);
    return;
  }
  if (row->created) {
    return;
  }

  row->hbox = uiCreateHorizBox ();
  uiWidgetAlignHorizFill (row->hbox);
  uiWidgetAlignVertStart (row->hbox);   // no vertical growth
  uiWidgetExpandHoriz (row->hbox);

  row->offscreen = false;
  row->selected = false;
  row->created = true;
  row->cleared = true;

  row->cols = mdmalloc (sizeof (uivlcol_t) * vl->numcols);

  /* create a label so that cleared rows with only widgets will still */
  /* have a height */
  /* hair-space */
  uiwidget = uiCreateLabel ("\xe2\x80\x8a");
  uiBoxPackStart (row->hbox, uiwidget);
  if (vl->uselistingfont) {
    uiWidgetAddClass (uiwidget, VL_LIST_CLASS);
  }
  uiWidgetShow (uiwidget);
  uiwcontFree (uiwidget);

  for (int colidx = 0; colidx < vl->numcols; ++colidx) {
    vltype_t      type;
    uivlcol_t     *col;
    uivlcoldata_t *coldata;

    col = &row->cols [colidx];
    coldata = &vl->coldata [colidx];

    col->ident = VL_IDENT_COL;
    col->box = NULL;
    col->uiwidget = NULL;
    col->class = NULL;
    col->colidx = colidx;
    col->value = LIST_VALUE_INVALID;

    type = coldata->type;
    if (isheading) {
      type = VL_TYPE_LABEL;
    }

    switch (type) {
      case VL_TYPE_LABEL: {
        col->uiwidget = uiCreateLabel ("");
        break;
      }
      case VL_TYPE_IMAGE: {
        col->uiwidget = uiImageNew ();
        uiImageClear (col->uiwidget);
        uiWidgetSetMarginStart (col->uiwidget, 1);
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
        if (dispidx == 0) {
          col->uiwidget = uiCreateRadioButton (NULL, "", 0);
        } else {
          uivlrow_t   *trow;

          trow = &vl->rows [0];
          col->uiwidget =
              uiCreateRadioButton (trow->cols [colidx].uiwidget, "", 0);
        }
        uiWidgetEnableFocus (col->uiwidget);
        uiToggleButtonSetFocusCallback (col->uiwidget, row->rowcb->focuscb);
        if (coldata->togglecb != NULL) {
          uiToggleButtonSetCallback (col->uiwidget, coldata->togglecb);
        }
        break;
      }
      case VL_TYPE_CHECK_BUTTON: {
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
        /* an internal numeric type will always be hidden */
        /* used to associate values with the row */
        coldata->hidden = VL_COL_HIDE;
        break;
      }
    }

    if (coldata->hidden == VL_COL_HIDE) {
      continue;
    }

    /* need a box for the size change callback */
    col->box = uiCreateHorizBox ();
    uiWidgetAlignHorizFill (col->box);
    uiWidgetSetAllMargins (col->box, 0);

    uiWidgetAlignHorizFill (col->uiwidget);
    uiBoxPackStart (col->box, col->uiwidget);
    uiWidgetSetAllMargins (col->uiwidget, 0);
    if (isheading) {
      uiWidgetAlignVertEnd (col->uiwidget);
    }

    if (row->dispidx == 0 &&
        col->uiwidget != NULL &&
        (type == VL_TYPE_LABEL || type == VL_TYPE_IMAGE)) {
      /* set up the size change callback so that the columns */
      /* can be made to only grow, and never shrink on their own */
      /* only need this on labels and images. */
      coldata->colszcb = callbackInitII (uivlColSizeChg, coldata);
      coldata->col0 = col;
      uiBoxSetSizeChgCallback (col->box, coldata->colszcb);
    }

    uiWidgetSetMarginEnd (col->uiwidget, 3);
    if (vl->uselistingfont) {
      uiWidgetAddClass (col->uiwidget, VL_LIST_CLASS);
      if (isheading) {
        uiWidgetAddClass (col->uiwidget, VL_HEAD_CLASS);
      }
    }

    if (coldata->grow == VL_COL_WIDTH_GROW) {
      uiBoxPackStartExpand (row->hbox, col->box);
    } else {
      uiBoxPackStart (row->hbox, col->box);
    }
    uiSizeGroupAdd (coldata->szgrp, col->box);

    if (coldata->baseclass != NULL) {
      uiWidgetAddClass (col->uiwidget, coldata->baseclass);
    }
    if (coldata->type == VL_TYPE_LABEL) {
      if (coldata->minwidth != VL_MIN_WIDTH_ANY) {
        uiLabelSetMinWidth (col->uiwidget, coldata->minwidth);
      }
      if (! isheading && coldata->ellipsize) {
        uiLabelEllipsizeOn (col->uiwidget);
      }
      if (coldata->alignend) {
        uiLabelAlignEnd (col->uiwidget);
      }
    }

    /* when a row is first created, it is in the cleared state */
    uiWidgetHide (col->uiwidget);
  }
}

static uivlrow_t *
uivlGetRow (uivirtlist_t *vl, int32_t rownum)
{
  uivlrow_t   *row = NULL;

  if (rownum == VL_ROW_HEADING) {
    row = &vl->headingrow;
  } else {
    int32_t   rowidx;

    rowidx = rownum - vl->rowoffset;
    if (rowidx >= 0 && rowidx < vl->dispsize) {
      row = &vl->rows [rowidx];
      if (row->ident != VL_IDENT_ROW) {
        fprintf (stderr, "ERR: invalid row: rownum %" PRId32 " rowoffset: %" PRId32 " rowidx: %" PRId32, rownum, vl->rowoffset, rowidx);
      }
    }
  }

  return row;
}

static void
uivlPackRow (uivirtlist_t *vl, uivlrow_t *row)
{
  if (row == NULL) {
    return;
  }

  uiBoxPackStart (vl->wcont [VL_W_MAIN_VBOX], row->hbox);
  row->offscreen = false;
}

static bool
uivlScrollbarCallback (void *udata, double value)
{
  uivirtlist_t  *vl = udata;
  int32_t       start;

  if (vl->inscroll) {
    return UICB_STOP;
  }

  start = (uint32_t) floor (value);

  uivlProcessScroll (vl, start, VL_SCROLL_FORCE);

  return UICB_CONT;
}

static bool
uivlKeyEvent (void *udata)
{
  uivirtlist_t  *vl = udata;

  if (vl == NULL || vl->ident != VL_IDENT) {
    return UICB_CONT;
  }
  if (vl->inscroll) {
    return UICB_CONT;
  }

  if (uiEventIsKeyReleaseEvent (vl->wcont [VL_W_KEYH])) {
    return UICB_CONT;
  }

  if (uiEventIsMovementKey (vl->wcont [VL_W_KEYH])) {
    int32_t     dir = 1;
    int32_t     nsel;

    if (uiEventIsKeyPressEvent (vl->wcont [VL_W_KEYH])) {
      if (uiEventIsPageUpDownKey (vl->wcont [VL_W_KEYH])) {
        dir = vl->dispsize;
      }
      if (uiEventIsUpKey (vl->wcont [VL_W_KEYH])) {
        dir = - dir;
      }
    }

    if (nlistGetCount (vl->selected) == 1) {
      nsel = vl->currSelection + dir;
      nsel = uivlRownumLimit (vl, nsel);
      uivlProcessScroll (vl, nsel, VL_SCROLL_NORM);
      uivlUpdateSelections (vl, nsel);
      uivlSelectionHandler (vl, nsel, VL_COL_UNKNOWN);
    }

    /* movement keys are handled internally */
    return UICB_STOP;
  }

  return UICB_CONT;
}

static bool
uivlMButtonEvent (void *udata, int32_t dispidx, int32_t colidx)
{
  uivirtlist_t  *vl = udata;
  int           button;
  int32_t       rownum = -1;

  if (vl == NULL || vl->ident != VL_IDENT) {
    return UICB_CONT;
  }
  if (vl->inscroll) {
    return UICB_CONT;
  }

  if (! uiEventIsButtonPressEvent (vl->wcont [VL_W_KEYH]) &&
      ! uiEventIsButtonDoublePressEvent (vl->wcont [VL_W_KEYH])) {
    return UICB_CONT;
  }

  uiWidgetGrabFocus (vl->wcont [VL_W_MAIN_VBOX]);
  button = uiEventButtonPressed (vl->wcont [VL_W_KEYH]);

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
    return UICB_CONT;
  }

  /* all other buttons (1-3) cause a selection */

  if (dispidx >= 0) {
    rownum = dispidx + vl->rowoffset;
  }

  if (rownum < 0) {
    /* not found */
    return UICB_CONT;
  }

  uivlUpdateSelections (vl, rownum);
  /* call the selection handler before the double-click handler */
  uivlSelectionHandler (vl, rownum, colidx);
  if (uiEventIsButtonDoublePressEvent (vl->wcont [VL_W_KEYH])) {
    uivlDoubleClickHandler (vl, rownum, colidx);
  }

  return UICB_CONT;
}

static bool
uivlScrollEvent (void *udata, int32_t dir)
{
  uivirtlist_t  *vl = udata;
  int32_t       start;

  if (vl == NULL || vl->ident != VL_IDENT) {
    return UICB_CONT;
  }
  if (vl->inscroll) {
    return UICB_CONT;
  }

  uiWidgetGrabFocus (vl->wcont [VL_W_MAIN_VBOX]);

  start = vl->rowoffset;
  if (dir == UIEVENT_DIR_PREV || dir == UIEVENT_DIR_LEFT) {
    start -= 1;
  }
  if (dir == UIEVENT_DIR_NEXT || dir == UIEVENT_DIR_RIGHT) {
    start += 1;
  }
  uivlProcessScroll (vl, start, VL_SCROLL_FORCE);

  return UICB_CONT;
}

static void
uivlClearDisplaySelections (uivirtlist_t *vl)
{
  for (int dispidx = 0; dispidx < vl->dispsize; ++dispidx) {
    uivlrow_t   *row = &vl->rows [dispidx];

    if (row->selected) {
      uiWidgetRemoveClass (row->hbox, VL_SELECTED_CLASS);

      for (int colidx = 0; colidx < vl->numcols; ++colidx) {
        if (vl->coldata [colidx].hidden == VL_COL_SHOW) {
          uiWidgetRemoveClass (row->cols [colidx].uiwidget, VL_SELECTED_CLASS);
        }
      }
      row->selected = false;
    }
  }
}

static void
uivlSetDisplaySelections (uivirtlist_t *vl)
{
  nlistidx_t    iter;
  nlistidx_t    val;

  nlistStartIterator (vl->selected, &iter);
  while ((val = nlistIterateKey (vl->selected, &iter)) >= 0) {
    int32_t     tval;

    tval = val - vl->rowoffset;
    if (tval >= 0 && tval < vl->dispsize) {
      uivlrow_t   *row;

      row = uivlGetRow (vl, val);
      uiWidgetAddClass (row->hbox, VL_SELECTED_CLASS);
      row->selected = true;
      for (int colidx = 0; colidx < vl->numcols; ++colidx) {
        if (vl->coldata [colidx].hidden == VL_COL_SHOW) {
          uiWidgetAddClass (row->cols [colidx].uiwidget, VL_SELECTED_CLASS);
        }
      }
    }
  }
}

static void
uivlClearSelections (uivirtlist_t *vl)
{
  nlistFree (vl->selected);
  vl->selected = nlistAlloc ("vl-selected", LIST_ORDERED, NULL);
}

static void
uivlAddSelection (uivirtlist_t *vl, uint32_t rownum)
{
  uivlrow_t   *row = NULL;
  int32_t     rowidx;

  rowidx = rownum - vl->rowoffset;
  if (rowidx >= 0 && rowidx < vl->dispsize) {
    row = &vl->rows [rowidx];
  }
  nlistSetNum (vl->selected, rownum, true);

  vl->currSelection = rownum;
}

static void
uivlProcessScroll (uivirtlist_t *vl, int32_t start, int sctype)
{
  int32_t       wantrow = start;

  vl->inscroll = true;

  start = uivlRowOffsetLimit (vl, start);

  /* if this is a keyboard movement, and there's only one selection */
  /* and the desired row-number is on-screen */
  if (sctype == VL_SCROLL_NORM &&
      nlistGetCount (vl->selected) == 1 &&
      wantrow >= vl->rowoffset &&
      wantrow < vl->rowoffset + vl->dispsize) {
    if (wantrow < vl->currSelection) {
      /* selection up */
      if (wantrow < vl->rowoffset + vl->dispsize / 2 - 1) {
        start = vl->rowoffset - 1;
      } else {
        vl->inscroll = false;
        return;
      }
    } else {
      /* selection down */
      if (wantrow >= vl->rowoffset + vl->dispsize / 2) {
        start = vl->rowoffset + 1;
      } else {
        vl->inscroll = false;
        return;
      }
    }

    start = uivlRowOffsetLimit (vl, start);
  }

  if (start == vl->rowoffset) {
    vl->inscroll = false;
    return;
  }

  vl->rowoffset = start;
  uivlPopulate (vl);
  uiScrollbarSetPosition (vl->wcont [VL_W_SB], (double) start);
  vl->inscroll = false;
}

static bool
uivlVboxSizeChg (void *udata, int32_t width, int32_t height)
{
  uivirtlist_t  *vl = udata;
  int           calcrows;
  int           theight;

  /* see the comments in colsizechg */

  if (width < vl->vboxwidth - 5) {
    for (int colidx = 0; colidx < vl->numcols; ++colidx) {
      uivlrow_t   *row;
      uivlcol_t   *col;

      row = &vl->rows [0];
      if (vl->coldata [colidx].hidden == VL_COL_SHOW) {
        vl->coldata [colidx].colwidth = -1;
        col = &row->cols [colidx];
        uiWidgetSetSizeRequest (col->box, -1, -1);
      }
    }
  }

  vl->vboxwidth = width;

  if (uiWidgetIsMapped (vl->wcont [VL_W_MAIN_VBOX]) &&
      vl->vboxheight == height) {
    return UICB_CONT;
  }

  vl->vboxheight = height;

  if (vl->vboxheight > 0 && vl->rowheight > 0) {
    int   odispsize = vl->dispsize;

    theight = vl->vboxheight;
    calcrows = theight / vl->rowheight;

    if (calcrows != vl->dispsize) {

      /* only if the number of rows has increased */
      if (vl->dispalloc < calcrows) {
        vl->rows = mdrealloc (vl->rows, sizeof (uivlrow_t) * calcrows);

        for (int dispidx = vl->dispsize; dispidx < calcrows; ++dispidx) {
          uivlrow_t *row;

          row = &vl->rows [dispidx];
          uivlRowBasicInit (vl, row, dispidx);
          uivlCreateRow (vl, row, dispidx, false);

          uivlPackRow (vl, row);
          /* rows packed after the initial display need */
          /* to have their contents shown */
          if (row->offscreen == false) {
            uiWidgetShowAll (row->hbox);
          }
        }

        vl->dispalloc = calcrows;
      }

      if (calcrows < vl->dispsize) {
        for (int dispidx = calcrows; dispidx < vl->dispsize; ++dispidx) {
          uivlClearRowDisp (vl, dispidx);
        }
      }

      vl->dispsize = calcrows;

      for (int dispidx = odispsize; dispidx < calcrows; ++dispidx) {
        uivlrow_t *row;

        /* force a reset as a show-all was done, the rows must be cleared */
        row = &vl->rows [dispidx];
        row->cleared = false;
        uivlClearRowDisp (vl, dispidx);
      }

      if (vl->dispsize > vl->numrows) {
        for (int dispidx = vl->numrows; dispidx < vl->dispsize; ++dispidx) {
          uivlrow_t *row;

          row = &vl->rows [dispidx];
          if (! row->cleared) {
            uivlClearRowDisp (vl, dispidx);
          }
        }
      }

      uiScrollbarSetPageIncrement (vl->wcont [VL_W_SB],
          (double) (vl->dispsize / 2));
      uiScrollbarSetPageSize (vl->wcont [VL_W_SB], (double) vl->dispsize);
      uivlPopulate (vl);
    }
  }

  if (vl->numrows <= vl->dispsize) {
    uiWidgetHide (vl->wcont [VL_W_SB]);
  } else {
    uiWidgetShow (vl->wcont [VL_W_SB]);
  }

  return UICB_CONT;
}

static bool
uivlRowSizeChg (void *udata, int32_t width, int32_t height)
{
  uivirtlist_t  *vl = udata;

  vl->rowheight = height;
  return UICB_CONT;
}

static bool
uivlColSizeChg (void *udata, int32_t width, int32_t height)
{
  uivlcoldata_t   *coldata = udata;

  /* gtk seems to resize the box some number of pixels */
  /* greater than requested */
  /* just need a stable number */
  /* a smaller size allows user-shrinking */
  if (width > 0 && width > coldata->colwidth) {
    if (uiWidgetIsMapped (coldata->col0->box)) {
      coldata->colwidth = width;

      /* if the size is set to the actual size, it can no longer shrink. */
      /* if the size is set to the actual size, in gtk, and the vbox size */
      /* change test is not modified, the vbox will keep growing. */
      /* the vbox size change test is modified to ignore minor changes */
      /* so that it does not think the window is changing. */
      /* because of this, columns can still bounce their width by 5 pixels */
      width -= 5;
      uiWidgetSetSizeRequest (coldata->col0->box, width, -1);
    }
  }

  return UICB_CONT;
}

static void
uivlRowBasicInit (uivirtlist_t *vl, uivlrow_t *row, int dispidx)
{
  row->vl = vl;
  row->ident = VL_IDENT_ROW;
  row->hbox = NULL;
  row->cols = NULL;
  row->created = false;
  row->cleared = false;
  row->rowcb = NULL;
  row->dispidx = dispidx;
  row->initialized = true;

  if (dispidx != VL_ROW_HEADING) {
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

  rownum = rowcb->dispidx + vl->rowoffset;

  uivlUpdateSelections (vl, rownum);
  uivlSelectionHandler (vl, rownum, VL_COL_UNKNOWN);

  return UICB_CONT;
}

static void
uivlUpdateSelections (uivirtlist_t *vl, int32_t rownum)
{
  if (vl->wcont [VL_W_KEYH] != NULL) {
    if (vl->allowmultiple == false ||
        ! uiEventIsControlPressed (vl->wcont [VL_W_KEYH])) {
      uivlClearSelections (vl);
      uivlClearDisplaySelections (vl);
    }
  }
  if (vl->allowmultiple == true &&
      uiEventIsShiftPressed (vl->wcont [VL_W_KEYH])) {
    int32_t   min = rownum;
    int32_t   max = rownum;

    if (vl->lastSelection < rownum) {
      min = vl->lastSelection;
      max = rownum;
    } else {
      min = rownum;
      max = vl->lastSelection;
    }
    for (int32_t rowidx = min; rowidx <= max; ++rowidx) {
      uivlAddSelection (vl, rowidx);
    }
  }
  uivlAddSelection (vl, rownum);
  uivlSetDisplaySelections (vl);
  if (nlistGetCount (vl->selected) == 1) {
    vl->lastSelection = vl->currSelection;
  }
}


int32_t
uivlRowOffsetLimit (uivirtlist_t *vl, int32_t rowoffset)
{
  int   tdispsize;

  if (rowoffset < 0) {
    rowoffset = 0;
  }

  /* the number of rows may be less than the available display size */
  tdispsize = vl->dispsize;
  if (vl->numrows < vl->dispsize) {
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
uivlSelectionHandler (uivirtlist_t *vl, int32_t rownum, int32_t colidx)
{
  if (vl->selcb != NULL) {
    vl->selcb (vl->seludata, vl, rownum, colidx);
  }
}

static void
uivlDoubleClickHandler (uivirtlist_t *vl, int32_t rownum, int32_t colidx)
{
  if (vl->dclickcb != NULL) {
    vl->dclickcb (vl->dclickudata, vl, rownum, colidx);
  }
}

static void
uivlSetToggleChangeCallback (uivirtlist_t *vl, int colidx, callback_t *cb)
{
  if (vl == NULL || vl->ident != VL_IDENT) {
    return;
  }
  if (colidx < 0 || colidx >= vl->numcols) {
    return;
  }
  if (vl->coldata [colidx].type != VL_TYPE_RADIO_BUTTON &&
     vl->coldata [colidx].type != VL_TYPE_CHECK_BUTTON) {
    return;
  }

  vl->coldata [colidx].togglecb = cb;
}

static void
uivlClearRowDisp (uivirtlist_t *vl, int dispidx)
{
  uivlrow_t   *row;

  if (vl == NULL || vl->ident != VL_IDENT) {
    return;
  }
  if (dispidx < 0 || dispidx >= vl->dispsize) {
    return;
  }

  row = &vl->rows [dispidx];
  if (! row->created) {
    return;
  }
  if (row->cleared) {
    return;
  }

  row->cleared = true;

  for (int colidx = 0; colidx < vl->numcols; ++colidx) {
    if (vl->coldata [colidx].hidden == VL_COL_HIDE) {
      continue;
    }

    if (vl->coldata [colidx].type != VL_TYPE_INTERNAL_NUMERIC) {
      uiWidgetHide (row->cols [colidx].uiwidget);
    }
  }
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
  if (rownum != VL_ROW_HEADING && (rownum < 0 || rownum >= vl->numrows)) {
    rc = false;
  }

  return rc;
}

static void
uivlRowDisplay (uivirtlist_t *vl, uivlrow_t *row)
{
  uiWidgetShowAll (row->hbox);

  /* re-hide any colums that should be hidden */
  for (int colidx = 0; colidx < vl->numcols; ++colidx) {
    if (vl->coldata [colidx].type == VL_TYPE_INTERNAL_NUMERIC) {
      continue;
    }
    if (vl->coldata [colidx].hidden == VL_COL_HIDE) {
      uiWidgetHide (vl->headingrow.cols [colidx].uiwidget);
      for (int dispidx = 0; dispidx < vl->dispsize; ++dispidx) {
        row = &vl->rows [dispidx];
        uiWidgetHide (row->cols [colidx].uiwidget);
      }
    }
  }
}
