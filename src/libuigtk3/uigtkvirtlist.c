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
#include "uiwcont.h"

#include "ui-gtk3.h"

#include "ui/uiwcont-int.h"

#include "ui/uibox.h"
#include "ui/uilabel.h"
#include "ui/uiui.h"
#include "ui/uivirtlist.h"
// #include "ui/uiwidget.h"
#include "ui/uiwindow.h"

typedef struct {
  uiwcont_t *uiwidget;
  /* the following data is only set in the heading row */
  vltype_t  type;
  int       ident;
  int       maxwidth;
  bool      hidden : 1;
  bool      ellipsize : 1;
} uivlcol_t;

typedef struct {
  uiwcont_t   *hbox;
  uivlcol_t   *cols;
  bool        selected : 1;
} uivlrow_t;

typedef struct uivirtlist {
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

uiwcont_t *
uiCreateVirtList (int disprows)
{
  uiwcont_t     *vlcont;
//  uiwcont_t     *sb;
  uivirtlist_t  *vl;

  vl = mdmalloc (sizeof (uivirtlist_t));
  vl->vbox = uiCreateVertBox ();
  vl->sb = NULL;
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

  vlcont = uiwcontAlloc ();
  vlcont->wbasetype = WCONT_T_VIRTLIST;
  vlcont->wtype = WCONT_T_VIRTLIST;
  vlcont->uidata.widget = (GtkWidget *) WCONT_EMPTY_WIDGET;
  vlcont->uidata.packwidget = vl->vbox->uidata.widget;
  vlcont->uiint.uivirtlist = vl;

  return vlcont;
}

void
uivlFree (uiwcont_t *vlcont)
{
  uivirtlist_t  *vl;

  if (! uiwcontValid (vlcont, WCONT_T_VIRTLIST, "vl-free")) {
    return;
  }
  if (vlcont->uiint.uivirtlist == NULL) {
    return;
  }

  vl = vlcont->uiint.uivirtlist;
  uiwcontBaseFree (vl->sb);

  uivlFreeRow (vl, &vl->headingrow);

  for (int i = 0; i < vl->disprows; ++i) {
    uivlFreeRow (vl, &vl->rows [i]);
  }
  mdfree (vl->rows);
  vl->rows = NULL;

  mdfree (vl);
}

void
uivlSetNumRows (uiwcont_t *vlcont, uint32_t numrows)
{
  uivirtlist_t  *vl;

  if (! uiwcontValid (vlcont, WCONT_T_VIRTLIST, "vl-set-num-rows")) {
    return;
  }
  if (vlcont->uiint.uivirtlist == NULL) {
    return;
  }

  vl = vlcont->uiint.uivirtlist;
  vl->numrows = numrows;
}

/* headings */

void
uivlSetHeadings (uiwcont_t *vlcont, ...)
{
  va_list       args;
  const char    *heading;
  int           count = 0;
  uivirtlist_t  *vl;

  if (! uiwcontValid (vlcont, WCONT_T_VIRTLIST, "vl-set-head")) {
    return;
  }

  va_start (args, vlcont);
  heading = va_arg (args, const char *);
  while (heading != NULL) {
    ++count;
    heading = va_arg (args, const char *);
  }
  va_end (args);

  vl = vlcont->uiint.uivirtlist;
  vl->numcols = count;
  uivlInitRow (vl, &vl->headingrow);

  va_start (args, vlcont);
  heading = va_arg (args, const char *);
  for (int i = 0; i < count; ++i) {
    uivlSetColumn (vlcont, i, VL_TYPE_LABEL, 0, false);
    uivlSetColumnValue (vlcont, VL_ROW_HEADING, i, heading);
    heading = va_arg (args, const char *);
  }
  va_end (args);

  uivlPackRow (vl, VL_ROW_HEADING);
}

void
uivlSetHeadingFont (uiwcont_t *vlcont, int col, const char *font)
{
  if (! uiwcontValid (vlcont, WCONT_T_VIRTLIST, "vl-set-head-font")) {
    return;
  }

}

void
uivlSetHeadingClass (uiwcont_t *vlcont, int col, const char *class)
{
  if (! uiwcontValid (vlcont, WCONT_T_VIRTLIST, "vl-set-head-class")) {
    return;
  }

}


/* column set */

void
uivlSetColumn (uiwcont_t *vlcont, int col, vltype_t type, int ident, bool hidden)
{
  uivirtlist_t    *vl;
  uivlrow_t       *row;

  if (! uiwcontValid (vlcont, WCONT_T_VIRTLIST, "vl-set-col")) {
    return;
  }
  if (vlcont->uiint.uivirtlist == NULL) {
    return;
  }

  vl = vlcont->uiint.uivirtlist;

  if (col < 0 || col >= vl->numcols) {
    return;
  }

  row = &vl->headingrow;
  row->cols [col].type = type;
  row->cols [col].ident = ident;
  row->cols [col].hidden = hidden;
}

void
uivlSetColumnMaxWidth (uiwcont_t *vlcont, int col, int maxwidth)
{
  if (! uiwcontValid (vlcont, WCONT_T_VIRTLIST, "vl-set-col-maxw")) {
    return;
  }
}

void
uivlSetColumnFont (uiwcont_t *vlcont, int col, const char *font)
{
  if (! uiwcontValid (vlcont, WCONT_T_VIRTLIST, "vl-set-col-font")) {
    return;
  }
}

void
uivlSetColumnEllipsizeOn (uiwcont_t *vlcont, int col)
{
  if (! uiwcontValid (vlcont, WCONT_T_VIRTLIST, "vl-set-col-ell")) {
    return;
  }
}

void
uivlSetColumnClass (uiwcont_t *vlcont, int col, const char *class)
{
  if (! uiwcontValid (vlcont, WCONT_T_VIRTLIST, "vl-set-col-class")) {
    return;
  }
}

void
uivlSetColumnAlignEnd (uiwcont_t *vlcont, int col)
{
  if (! uiwcontValid (vlcont, WCONT_T_VIRTLIST, "vl-set-col-align-end")) {
    return;
  }
}

void
uivlSetColumnValue (uiwcont_t *vlcont, int32_t rownum, int col, const char *value)
{
  uivirtlist_t    *vl;
  uivlrow_t       *row = NULL;

  if (! uiwcontValid (vlcont, WCONT_T_VIRTLIST, "vl-set-col-align-end")) {
    return;
  }
  if (vlcont->uiint.uivirtlist == NULL) {
    return;
  }

  vl = vlcont->uiint.uivirtlist;

  if (rownum != VL_ROW_HEADING && (rownum < 0 || rownum >= vl->numrows)) {
    return;
  }
  if (col < 0 || col >= vl->numcols) {
    return;
  }

  row = uivlGetRow (vl, rownum);
  if (row == NULL) {
    return;
  }

  switch (row->cols [col].type) {
    case VL_TYPE_LABEL: {
      uiLabelSetText (row->cols [col].uiwidget, value);
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
uivlSetColumnValueNum (uiwcont_t *vlcont, int32_t rownum, int col, int32_t val)
{
  uivirtlist_t    *vl;
  uivlrow_t       *row = NULL;

  if (! uiwcontValid (vlcont, WCONT_T_VIRTLIST, "vl-set-col-align-end")) {
    return;
  }
  if (vlcont->uiint.uivirtlist == NULL) {
    return;
  }

  vl = vlcont->uiint.uivirtlist;

  if (rownum != VL_ROW_HEADING && (rownum < 0 || rownum >= vl->numrows)) {
    return;
  }
  if (col < 0 || col >= vl->numcols) {
    return;
  }

  row = uivlGetRow (vl, rownum);
  if (row == NULL) {
    return;
  }

  switch (row->cols [col].type) {
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
uivlGetColumnIdent (uiwcont_t *vlcont, int col)
{
  if (! uiwcontValid (vlcont, WCONT_T_VIRTLIST, "vl-get-col-ident")) {
    return VL_COL_UNKNOWN;
  }

  return 0;
}

const char *
uivlGetColumnValue (uiwcont_t *vlcont, int row, int col)
{
  if (! uiwcontValid (vlcont, WCONT_T_VIRTLIST, "vl-get-col-value")) {
    return NULL;
  }
  return NULL;
}

const char *
uivlGetColumnEntryValue (uiwcont_t *vlcont, int row, int col)
{
  if (! uiwcontValid (vlcont, WCONT_T_VIRTLIST, "vl-get-col-entry-value")) {
    return NULL;
  }
  return NULL;
}

/* callbacks */

void
uivlSetSelectionCallback (uiwcont_t *vlcont, callback_t *cb, void *udata)
{
  if (! uiwcontValid (vlcont, WCONT_T_VIRTLIST, "vl-set-sel-cb")) {
    return;
  }
}

void
uivlSetEntryCallback (uiwcont_t *vlcont, callback_t *cb, void *udata)
{
  if (! uiwcontValid (vlcont, WCONT_T_VIRTLIST, "vl-set-entry-cb")) {
    return;
  }
}

void
uivlSetRowFillCallback (uiwcont_t *vlcont, callback_t *cb, void *udata)
{
  if (! uiwcontValid (vlcont, WCONT_T_VIRTLIST, "vl-set-row-fill-cb")) {
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
  uiwcontBaseFree (row->hbox);
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

  uiwcontBaseFree (col->uiwidget);
  col->uiwidget = NULL;
}

static void
uivlInitRow (uivirtlist_t *vl, uivlrow_t *row)
{
  row->hbox = uiCreateHorizBox ();
  row->cols = mdmalloc (sizeof (uivlcol_t) * vl->numcols);
  for (int i = 0; i < vl->numcols; ++i) {
    row->cols [i].type = VL_TYPE_LABEL;
    row->cols [i].uiwidget = uiCreateLabel ("");
    uiBoxPackStart (row->hbox, row->cols [i].uiwidget);
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

  uiBoxPackStart (vl->vbox, row->hbox);
}
