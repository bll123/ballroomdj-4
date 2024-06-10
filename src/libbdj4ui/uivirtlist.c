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

typedef struct {
  uiwcont_t *uiwidget;
  /* the following data is specific to a column */
  /* it is only stored in the heading row */
  vltype_t  type;
  int       ident;
  int       maxwidth;
  bool      alignend: 1;
  bool      hidden : 1;
  bool      ellipsize : 1;
  /* the following data is specific to a row and column */
  char      *class;
} uivlcol_t;

typedef struct {
  uiwcont_t   *hbox;
  uivlcol_t   *cols;
  bool        selected : 1;
} uivlrow_t;

typedef struct uivirtlist {
  uiwcont_t     *hbox;
  uiwcont_t     *vbox;
  uiwcont_t     *sb;
  uivlrow_t     *rows;
  uivlrow_t     headingrow;
  int           numcols;
  uint32_t      rowoffset;
  int32_t       numrows;
  int           disprows;
} uivirtlist_t;

static void uivlFreeRow (uivirtlist_t *vl, uivlrow_t *row);
static void uivlFreeCol (uivlcol_t *col);
static void uivlInitRow (uivirtlist_t *vl, uivlrow_t *row);
static uivlrow_t * uivlGetRow (uivirtlist_t *vl, int32_t rownum);
static void uivlPackRow (uivirtlist_t *vl, int32_t rownum);

uivirtlist_t *
uiCreateVirtList (uiwcont_t *boxp, int disprows)
{
  uivirtlist_t  *vl;

  vl = mdmalloc (sizeof (uivirtlist_t));
  vl->hbox = uiCreateHorizBox ();
  uiWidgetAlignHorizFill (vl->hbox);
  uiWidgetAlignVertFill (vl->hbox);
  vl->vbox = uiCreateVertBox ();
  uiWidgetAlignHorizFill (vl->vbox);
  uiWidgetAlignVertFill (vl->vbox);
  uiBoxPackStartExpand (vl->hbox, vl->vbox);
  vl->sb = uiCreateVerticalScrollbar (10.0);
  uiBoxPackEnd (vl->hbox, vl->sb);
  vl->rows = NULL;
  vl->numcols = 0;
  vl->numrows = 0;

  vl->disprows = disprows;
  vl->rows = mdmalloc (sizeof (uivlcol_t) * disprows);
  for (int i = 0; i < disprows; ++i) {
    vl->rows [i].hbox = NULL;
    vl->rows [i].cols = NULL;
    vl->rows [i].selected = false;
  }

  vl->headingrow.hbox = NULL;
  vl->headingrow.cols = NULL;
  vl->headingrow.selected = false;

  uiBoxPackStart (boxp, vl->hbox);

  return vl;
}

void
uivlFree (uivirtlist_t *vl)
{
  if (vl == NULL) {
    return;
  }

  uiScrollbarFree (vl->sb);

  uivlFreeRow (vl, &vl->headingrow);

  for (int i = 0; i < vl->disprows; ++i) {
    uivlFreeRow (vl, &vl->rows [i]);
  }
  mdfree (vl->rows);
  vl->rows = NULL;

  uiwcontFree (vl->vbox);
  vl->vbox = NULL;
  uiwcontFree (vl->hbox);
  vl->hbox = NULL;

  mdfree (vl);
}

void
uivlSetNumRows (uivirtlist_t *vl, uint32_t numrows)
{
  if (vl == NULL) {
    return;
  }

  vl->numrows = numrows;
}

/* headings */

void
uivlSetHeadings (uivirtlist_t *vl, ...)
{
  va_list       args;
  const char    *heading;
  int           count = 0;

  if (vl == NULL) {
    return;
  }

  va_start (args, vl);
  heading = va_arg (args, const char *);
  while (heading != NULL) {
    ++count;
    heading = va_arg (args, const char *);
  }
  va_end (args);

  vl->numcols = count;
  uivlInitRow (vl, &vl->headingrow);

  va_start (args, vl);
  heading = va_arg (args, const char *);
  for (int i = 0; i < count; ++i) {
    uivlSetColumn (vl, i, VL_TYPE_LABEL, 0, false);
//    uivlSetHeadingFont (vl, VL_ROW_HEADING, i, need-bold-listing-font);
    uivlSetColumnValue (vl, VL_ROW_HEADING, i, heading);
    heading = va_arg (args, const char *);
  }
  va_end (args);

  uivlPackRow (vl, VL_ROW_HEADING);
}

void
uivlSetHeadingFont (uivirtlist_t *vl, int colnum, const char *font)
{
  if (vl == NULL) {
    return;
  }

}

void
uivlSetHeadingClass (uivirtlist_t *vl, int colnum, const char *class)
{
  if (vl == NULL) {
    return;
  }
}


/* column set */

void
uivlSetColumn (uivirtlist_t *vl, int colnum, vltype_t type, int ident, bool hidden)
{
  uivlrow_t       *row;

  if (vl == NULL) {
    return;
  }

  if (colnum < 0 || colnum >= vl->numcols) {
    return;
  }

  row = &vl->headingrow;
  row->cols [colnum].type = type;
  row->cols [colnum].ident = ident;
  row->cols [colnum].hidden = hidden;
  row->cols [colnum].alignend = false;
  row->cols [colnum].class = NULL;
}

void
uivlSetColumnMaxWidth (uivirtlist_t *vl, int colnum, int maxwidth)
{
  uivlrow_t   *row;
  uivlcol_t   *col;

  if (vl == NULL) {
    return;
  }
  if (colnum < 0 || colnum >= vl->numcols) {
    return;
  }

  row = &vl->headingrow;
  col = &row->cols [colnum];
  col->maxwidth = maxwidth;
}

void
uivlSetColumnFont (uivirtlist_t *vl, int colnum, const char *font)
{
  if (vl == NULL) {
    return;
  }
  if (colnum < 0 || colnum >= vl->numcols) {
    return;
  }
}

void
uivlSetColumnEllipsizeOn (uivirtlist_t *vl, int colnum)
{
  uivlrow_t   *row;
  uivlcol_t   *col;

  if (vl == NULL) {
    return;
  }
  if (colnum < 0 || colnum >= vl->numcols) {
    return;
  }

  row = &vl->headingrow;
  col = &row->cols [colnum];
  col->ellipsize = true;
}

void
uivlSetColumnClass (uivirtlist_t *vl, uint32_t rownum, int colnum, const char *class)
{
  uivlrow_t   *row;
  uivlcol_t   *col;

  if (vl == NULL) {
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
  uivlrow_t   *row;
  uivlcol_t   *col;

  if (vl == NULL) {
    return;
  }
  if (colnum < 0 || colnum >= vl->numcols) {
    return;
  }

  row = &vl->headingrow;
  col = &row->cols [colnum];
  col->alignend = true;
}

void
uivlSetColumnValue (uivirtlist_t *vl, int32_t rownum, int colnum, const char *value)
{
  uivlrow_t       *row = NULL;

  if (vl == NULL) {
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

  switch (row->cols [colnum].type) {
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

  if (vl == NULL) {
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

  switch (row->cols [colnum].type) {
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
  if (vl == NULL) {
    return VL_COL_UNKNOWN;
  }

  return 0;
}

const char *
uivlGetColumnValue (uivirtlist_t *vl, int row, int colnum)
{
  if (vl == NULL) {
    return NULL;
  }
  return NULL;
}

const char *
uivlGetColumnEntryValue (uivirtlist_t *vl, int row, int colnum)
{
  if (vl == NULL) {
    return NULL;
  }
  return NULL;
}

/* callbacks */

void
uivlSetSelectionCallback (uivirtlist_t *vl, callback_t *cb, void *udata)
{
  if (vl == NULL) {
    return;
  }
}

void
uivlSetEntryCallback (uivirtlist_t *vl, callback_t *cb, void *udata)
{
  if (vl == NULL) {
    return;
  }
}

void
uivlSetRowFillCallback (uivirtlist_t *vl, callback_t *cb, void *udata)
{
  if (vl == NULL) {
    return;
  }
}


/* internal routines */

static void
uivlFreeRow (uivirtlist_t *vl, uivlrow_t *row)
{
  if (row == NULL) {
    return;
  }

  for (int i = 0; i < vl->numcols; ++i) {
    if (row->cols != NULL) {
      uivlFreeCol (&row->cols [i]);
    }
  }
  uiwcontFree (row->hbox);
  row->hbox = NULL;
  mdfree (row->cols);
  row->cols = NULL;
}

static void
uivlFreeCol (uivlcol_t *col)
{
  if (col == NULL) {
    return;
  }

  dataFree (col->class);
  col->class = NULL;
  uiwcontFree (col->uiwidget);
  col->uiwidget = NULL;
}

static void
uivlInitRow (uivirtlist_t *vl, uivlrow_t *row)
{
  row->hbox = uiCreateHorizBox ();
  uiWidgetAlignHorizFill (row->hbox);
  row->cols = mdmalloc (sizeof (uivlcol_t) * vl->numcols);
  for (int i = 0; i < vl->numcols; ++i) {
    row->cols [i].type = VL_TYPE_LABEL;
    row->cols [i].uiwidget = uiCreateLabel ("");
    uiBoxPackStartExpand (row->hbox, row->cols [i].uiwidget);
    row->cols [i].ident = 0;
    row->cols [i].maxwidth = VL_MAX_WIDTH_ANY;
    row->cols [i].hidden = VL_COL_SHOW;
    row->cols [i].ellipsize = false;
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
    if (rowidx >= 0 && rowidx < vl->disprows) {
      row = &vl->rows [rowidx];
    }
  }

  return row;
}

static void
uivlPackRow (uivirtlist_t *vl, int32_t rownum)
{
  uivlrow_t   *row;

  row = uivlGetRow (vl, rownum);
  if (row == NULL) {
    return;
  }

  uiBoxPackStartExpand (vl->vbox, row->hbox);
}
