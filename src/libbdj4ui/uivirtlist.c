/*
 * Copyright 2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include "mdebug.h"
#include "ui.h"
#include "uivirtlist.h"
#include "uiwcont.h"

/* initialization states */
enum {
  VL_NOT_INIT = 0,
  VL_INIT_COLS = 1,
  VL_INIT_DISP = 2,
};

enum {
  VL_ROW_UNKNOWN = -1,
  VL_ROW_HEADING = -2,
  VL_MAX_WIDTH_ANY = -1,
};

enum {
  VL_IDENT_COLDATA  = 0x766c636f6c646174,
  VL_IDENT_COL      = 0x766c636f6caabbcc,
  VL_IDENT_ROW      = 0x766c726f77aabbcc,
  VL_IDENT          = 0x766caabbccddeeff,
};

typedef struct {
  uiwcont_t *szgrp;
  uint64_t  ident;
  /* the following data is specific to a column */
  vltype_t  type;
  int       colident;
  int       maxwidth;
  bool      alignend: 1;
  bool      hidden : 1;
  bool      ellipsize : 1;
} uivlcoldata_t;

typedef struct {
  uint64_t  ident;
  uiwcont_t *uiwidget;
  /* the following data is specific to a row and column */
  char      *class;
} uivlcol_t;

typedef struct {
  uint64_t    ident;
  uiwcont_t   *hbox;
  uivlcol_t   *cols;
  bool        selected : 1;
  bool        initialized : 1;
} uivlrow_t;

typedef struct uivirtlist {
  uint64_t      ident;
  uiwcont_t     *hbox;
  uiwcont_t     *vbox;
  uiwcont_t     *sb;
  uivlfillcb_t  fillcb;
  uivlselcb_t   selcb;
  uivlchangecb_t changecb;
  void          *udata;
  uivlrow_t     *rows;
  uivlrow_t     headingrow;
  uivlcoldata_t *coldata;
  int           numcols;
  uint32_t      rowoffset;
  int32_t       numrows;
  int           disprows;
  int           initialized;
} uivirtlist_t;

static void uivlFreeRow (uivirtlist_t *vl, uivlrow_t *row);
static void uivlFreeCol (uivlcol_t *col);
static void uivlInitRow (uivirtlist_t *vl, uivlrow_t *row, bool isheading);
static uivlrow_t * uivlGetRow (uivirtlist_t *vl, int32_t rownum);
static void uivlPackRow (uivirtlist_t *vl, uivlrow_t *row);

uivirtlist_t *
uiCreateVirtList (uiwcont_t *boxp, int disprows)
{
  uivirtlist_t  *vl;

  vl = mdmalloc (sizeof (uivirtlist_t));
  vl->ident = VL_IDENT;
  vl->fillcb = NULL;
  vl->selcb = NULL;
  vl->changecb = NULL;
  vl->hbox = uiCreateHorizBox ();
  uiWidgetAlignHorizFill (vl->hbox);
  vl->vbox = uiCreateVertBox ();
  uiWidgetAlignHorizFill (vl->vbox);
  uiWidgetAlignVertFill (vl->vbox);
  uiBoxPackStartExpand (vl->hbox, vl->vbox);
  vl->sb = uiCreateVerticalScrollbar (10.0);
  uiBoxPackEnd (vl->hbox, vl->sb);

  vl->coldata = NULL;
  vl->rows = NULL;
  vl->numcols = 0;
  vl->numrows = 0;
  vl->rowoffset = 0;
  vl->initialized = VL_NOT_INIT;

  vl->disprows = disprows;
  vl->rows = mdmalloc (sizeof (uivlrow_t) * disprows);
  for (int i = 0; i < disprows; ++i) {
    vl->rows [i].ident = VL_IDENT_ROW;
    vl->rows [i].hbox = NULL;
    vl->rows [i].cols = NULL;
    vl->rows [i].selected = false;
    vl->rows [i].initialized = false;
  }

  vl->headingrow.ident = VL_IDENT_ROW;
  vl->headingrow.hbox = NULL;
  vl->headingrow.cols = NULL;
  vl->headingrow.selected = false;
  vl->headingrow.initialized = false;

  uiBoxPackStart (boxp, vl->hbox);
  uiWidgetAlignVertStart (boxp);

  return vl;
}

void
uivlFree (uivirtlist_t *vl)
{
  if (vl == NULL || vl->ident != VL_IDENT) {
    return;
  }

  uiwcontFree (vl->sb);

  uivlFreeRow (vl, &vl->headingrow);

  for (int i = 0; i < vl->disprows; ++i) {
    uivlFreeRow (vl, &vl->rows [i]);
  }
  dataFree (vl->rows);
  vl->rows = NULL;

  for (int i = 0; i < vl->numcols; ++i) {
    uiwcontFree (vl->coldata [i].szgrp);
  }
  dataFree (vl->coldata);
  vl->coldata = NULL;

  uiwcontFree (vl->vbox);
  vl->vbox = NULL;
  uiwcontFree (vl->hbox);
  vl->hbox = NULL;

  vl->ident = 0;
  mdfree (vl);
}

void
uivlSetNumRows (uivirtlist_t *vl, uint32_t numrows)
{
  if (vl == NULL || vl->ident != VL_IDENT) {
    return;
  }

  vl->numrows = numrows;
}

void
uivlSetNumColumns (uivirtlist_t *vl, int numcols)
{
  if (vl == NULL || vl->ident != VL_IDENT) {
    return;
  }

  vl->numcols = numcols;
  vl->coldata = mdmalloc (sizeof (uivlcoldata_t) * numcols);
  for (int i = 0; i < numcols; ++i) {
    vl->coldata [i].szgrp = uiCreateSizeGroupHoriz ();
    vl->coldata [i].ident = VL_IDENT_COLDATA;
    vl->coldata [i].type = VL_TYPE_LABEL;
    vl->coldata [i].colident = 0;
    vl->coldata [i].maxwidth = VL_MAX_WIDTH_ANY;
    vl->coldata [i].hidden = VL_COL_SHOW;
    vl->coldata [i].ellipsize = false;
  }

  vl->initialized = VL_INIT_COLS;
}

void
uivlSetHeadingFont (uivirtlist_t *vl, int colnum, const char *font)
{
  if (vl == NULL || vl->ident != VL_IDENT) {
    return;
  }
  if (vl->initialized < VL_INIT_COLS) {
    return;
  }

}

void
uivlSetHeadingClass (uivirtlist_t *vl, int colnum, const char *class)
{
  if (vl == NULL || vl->ident != VL_IDENT) {
    return;
  }
  if (vl->initialized < VL_INIT_COLS) {
    return;
  }
}

/* column set */

void
uivlSetColumnHeading (uivirtlist_t *vl, int colnum, const char *heading)
{
  if (vl == NULL || vl->ident != VL_IDENT) {
    return;
  }
  if (vl->initialized < VL_INIT_COLS) {
    return;
  }
  if (colnum < 0 || colnum >= vl->numcols) {
    return;
  }

  uivlInitRow (vl, &vl->headingrow, true);
//    uivlSetHeadingFont (vl, VL_ROW_HEADING, colnum, need-bold-listing-font);
  uivlSetColumnValue (vl, VL_ROW_HEADING, colnum, heading);
}

void
uivlSetColumn (uivirtlist_t *vl, int colnum, vltype_t type, int colident, bool hidden)
{
  if (vl == NULL || vl->ident != VL_IDENT) {
    return;
  }
  if (vl->initialized < VL_INIT_COLS) {
    return;
  }
  if (colnum < 0 || colnum >= vl->numcols) {
    return;
  }

  vl->coldata [colnum].type = type;
  vl->coldata [colnum].colident = colident;
  vl->coldata [colnum].hidden = hidden;
  vl->coldata [colnum].alignend = false;
}

void
uivlSetColumnMaxWidth (uivirtlist_t *vl, int colnum, int maxwidth)
{
  if (vl == NULL || vl->ident != VL_IDENT) {
    return;
  }
  if (vl->initialized < VL_INIT_COLS) {
    return;
  }
  if (colnum < 0 || colnum >= vl->numcols) {
    return;
  }

  vl->coldata [colnum].maxwidth = maxwidth;
}

void
uivlSetColumnFont (uivirtlist_t *vl, int colnum, const char *font)
{
  if (vl == NULL || vl->ident != VL_IDENT) {
    return;
  }
  if (vl->initialized < VL_INIT_COLS) {
    return;
  }
  if (colnum < 0 || colnum >= vl->numcols) {
    return;
  }
}

void
uivlSetColumnEllipsizeOn (uivirtlist_t *vl, int colnum)
{
  if (vl == NULL || vl->ident != VL_IDENT) {
    return;
  }
  if (vl->initialized < VL_INIT_COLS) {
    return;
  }
  if (colnum < 0 || colnum >= vl->numcols) {
    return;
  }

  vl->coldata [colnum].ellipsize = true;
}

void
uivlSetColumnClass (uivirtlist_t *vl, uint32_t rownum, int colnum, const char *class)
{
  uivlrow_t   *row;
  uivlcol_t   *col;

  if (vl == NULL || vl->ident != VL_IDENT) {
    return;
  }
  if (vl->initialized < VL_INIT_COLS) {
    return;
  }
  if (colnum < 0 || colnum >= vl->numcols) {
    return;
  }

  row = uivlGetRow (vl, rownum);
  if (row == NULL) {
    return;
  }

  col = &row->cols [colnum];
  col->class = mdstrdup (class);
}

void
uivlSetColumnAlignEnd (uivirtlist_t *vl, int colnum)
{
  if (vl == NULL || vl->ident != VL_IDENT) {
    return;
  }
  if (vl->initialized < VL_INIT_COLS) {
    return;
  }
  if (colnum < 0 || colnum >= vl->numcols) {
    return;
  }

  vl->coldata [colnum].alignend = true;
}

void
uivlSetColumnValue (uivirtlist_t *vl, int32_t rownum, int colnum, const char *value)
{
  uivlrow_t       *row = NULL;

  if (vl == NULL || vl->ident != VL_IDENT) {
    return;
  }
  if (vl->initialized < VL_INIT_COLS) {
    return;
  }
  if (colnum < 0 || colnum >= vl->numcols) {
    return;
  }
  if (rownum != VL_ROW_HEADING && (rownum < 0 || rownum >= vl->numrows)) {
    return;
  }

  row = uivlGetRow (vl, rownum);
  if (row == NULL) {
    return;
  }

  switch (vl->coldata [colnum].type) {
    case VL_TYPE_LABEL: {
      uiLabelSetText (row->cols [colnum].uiwidget, value);
      break;
    }
    case VL_TYPE_IMAGE: {
      break;
    }
    case VL_TYPE_ENTRY: {
      break;
    }
    case VL_TYPE_INT_NUMERIC:
    case VL_TYPE_RADIO_BUTTON:
    case VL_TYPE_CHECK_BUTTON:
    case VL_TYPE_SPINBOX_NUM: {
      /* not handled here */
      break;
    }
  }
}

void
uivlSetColumnValueNum (uivirtlist_t *vl, int32_t rownum, int colnum, int32_t val)
{
  uivlrow_t       *row = NULL;

  if (vl == NULL || vl->ident != VL_IDENT) {
    return;
  }
  if (vl->initialized < VL_INIT_COLS) {
    return;
  }
  if (rownum != VL_ROW_HEADING && (rownum < 0 || rownum >= vl->numrows)) {
    return;
  }
  if (colnum < 0 || colnum >= vl->numcols) {
    return;
  }

  row = uivlGetRow (vl, rownum);
  if (row == NULL) {
    return;
  }

  switch (vl->coldata [colnum].type) {
    case VL_TYPE_LABEL:
    case VL_TYPE_IMAGE:
    case VL_TYPE_ENTRY: {
      /* not handled here */
      break;
    }
    case VL_TYPE_INT_NUMERIC: {
      break;
    }
    case VL_TYPE_RADIO_BUTTON: {
      break;
    }
    case VL_TYPE_CHECK_BUTTON: {
      break;
    }
    case VL_TYPE_SPINBOX_NUM: {
      break;
    }
  }
}

/* column get */

int
uivlGetColumnIdent (uivirtlist_t *vl, int colnum)
{
  if (vl == NULL || vl->ident != VL_IDENT) {
    return VL_COL_UNKNOWN;
  }
  if (vl->initialized < VL_INIT_COLS) {
    return VL_COL_UNKNOWN;
  }

  return 0;
}

const char *
uivlGetColumnValue (uivirtlist_t *vl, int row, int colnum)
{
  if (vl == NULL || vl->ident != VL_IDENT) {
    return NULL;
  }
  if (vl->initialized < VL_INIT_COLS) {
    return NULL;
  }
  return NULL;
}

const char *
uivlGetColumnEntryValue (uivirtlist_t *vl, int row, int colnum)
{
  if (vl == NULL || vl->ident != VL_IDENT) {
    return NULL;
  }
  if (vl->initialized < VL_INIT_COLS) {
    return NULL;
  }
  return NULL;
}

/* callbacks */

void
uivlSetSelectionCallback (uivirtlist_t *vl, uivlselcb_t cb, void *udata)
{
  if (vl == NULL || vl->ident != VL_IDENT) {
    return;
  }
  if (vl->initialized < VL_INIT_COLS) {
    return;
  }
}

void
uivlSetChangeCallback (uivirtlist_t *vl, uivlchangecb_t cb, void *udata)
{
  if (vl == NULL || vl->ident != VL_IDENT) {
    return;
  }
  if (vl->initialized < VL_INIT_COLS) {
    return;
  }
}

void
uivlSetRowFillCallback (uivirtlist_t *vl, uivlfillcb_t cb, void *udata)
{
  if (vl == NULL || vl->ident != VL_IDENT) {
    return;
  }
  if (vl->initialized < VL_INIT_COLS) {
    return;
  }
  if (cb == NULL) {
    return;
  }

  vl->fillcb = cb;
  vl->udata = udata;
}

/* processing */

void
uivlDisplay (uivirtlist_t *vl)
{
  uivlrow_t   *row;

  uivlPackRow (vl, &vl->headingrow);
  for (int i = 0; i < vl->disprows; ++i) {
    row = uivlGetRow (vl, i + vl->rowoffset);
    if (row == NULL) {
      break;
    }
    if (vl->fillcb != NULL) {
      vl->fillcb (vl->udata, vl, i + vl->rowoffset);
    }
    uivlPackRow (vl, row);
  }

  vl->initialized = VL_INIT_DISP;
}

/* internal routines */

static void
uivlFreeRow (uivirtlist_t *vl, uivlrow_t *row)
{
  if (row == NULL || row->ident != VL_IDENT_ROW) {
    return;
  }

  for (int i = 0; i < vl->numcols; ++i) {
    if (row->cols != NULL) {
      uivlFreeCol (&row->cols [i]);
    }
  }
  uiwcontFree (row->hbox);
  row->hbox = NULL;
  dataFree (row->cols);
  row->cols = NULL;
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
  col->uiwidget = NULL;
  col->ident = 0;
}

static void
uivlInitRow (uivirtlist_t *vl, uivlrow_t *row, bool isheading)
{
  if (row == NULL) {
    return;
  }
  if (row->initialized) {
    return;
  }

  row->hbox = uiCreateHorizBox ();
  uiWidgetAlignHorizFill (row->hbox);

  row->cols = mdmalloc (sizeof (uivlcol_t) * vl->numcols);

  for (int i = 0; i < vl->numcols; ++i) {
    row->cols [i].ident = VL_IDENT_COL;
    row->cols [i].uiwidget = uiCreateLabel ("");
    uiWidgetSetMarginEnd (row->cols [i].uiwidget, 2);

    if (vl->coldata [i].hidden == VL_COL_SHOW) {
      uiBoxPackStartExpand (row->hbox, row->cols [i].uiwidget);
      uiSizeGroupAdd (vl->coldata [i].szgrp, row->cols [i].uiwidget);
    }
    if (vl->coldata [i].type == VL_TYPE_LABEL) {
      if (vl->coldata [i].maxwidth != VL_MAX_WIDTH_ANY) {
        uiLabelSetMaxWidth (row->cols [i].uiwidget, vl->coldata [i].maxwidth);
      }
      if (! isheading && vl->coldata [i].ellipsize) {
        uiLabelEllipsizeOn (row->cols [i].uiwidget);
      }
      if (vl->coldata [i].alignend) {
        uiLabelAlignEnd (row->cols [i].uiwidget);
      }
    }
    row->cols [i].class = NULL;
  }
  row->initialized = true;
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
    if (rowidx >= 0 && rowidx < vl->disprows) {
      row = &vl->rows [rowidx];
      if (row->ident != VL_IDENT_ROW) {
        fprintf (stderr, "ERR: invalid row: rownum %d rowoffset: %d rowidx: %d\n", rownum, vl->rowoffset, rowidx);
      }
      if (! row->initialized) {
        uivlInitRow (vl, row, false);
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

  uiBoxPackStartExpand (vl->vbox, row->hbox);
}
