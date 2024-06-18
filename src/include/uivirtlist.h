/*
 * Copyright 2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIVIRTLIST_H
#define INC_UIVIRTLIST_H

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

#include <stdint.h>

#include "callback.h"
#include "uiwcont.h"

typedef enum {
  VL_TYPE_LABEL,
  VL_TYPE_IMAGE,
  VL_TYPE_ENTRY,
  VL_TYPE_RADIO_BUTTON,
  VL_TYPE_CHECK_BUTTON,
  VL_TYPE_SPINBOX_NUM,
  VL_TYPE_SPINBOX_TIME,
  VL_TYPE_INT_NUMERIC,
} vltype_t;

enum {
  VL_COL_HIDE = false,
  VL_COL_SHOW = true,
  VL_COL_WIDTH_FIXED = false,
  VL_COL_WIDTH_GROW = true,
  VL_COL_UNKNOWN = -1,
};

typedef struct uivirtlist uivirtlist_t;
typedef void (*uivlfillcb_t) (void *udata, uivirtlist_t *vl, uint32_t rownum);
typedef void (*uivlselcb_t) (void *udata, uivirtlist_t *vl, uint32_t rownum, int colidx);
typedef void (*uivlchangecb_t) (void *udata, uivirtlist_t *vl, uint32_t rownum, int colidx);

uivirtlist_t *uiCreateVirtList (uiwcont_t *boxp, int initialdisprows);
void  uivlFree (uivirtlist_t *vl);
void  uivlSetNumRows (uivirtlist_t *vl, uint32_t numrows);
void  uivlSetNumColumns (uivirtlist_t *vl, int numcols);

/* headings */
void  uivlSetHeadingFont (uivirtlist_t *vl, int colidx, const char *font);
void  uivlSetHeadingClass (uivirtlist_t *vl, int colidx, const char *class);

/* make columns */
void  uivlMakeColumn (uivirtlist_t *vl, int colidx, vltype_t type, int ident, bool hidden);
void  uivlMakeColumnEntry (uivirtlist_t *vl, int colidx, int colident, int sz, int maxsz);
void  uivlMakeColumnSpinboxTime (uivirtlist_t *vl, int colidx, int colident, int sbtype, callback_t *uicb);
void  uivlMakeColumnSpinboxNum (uivirtlist_t *vl, int colidx, int colident, double min, double max, double incr, double pageincr);

/* column set */
void  uivlSetColumnHeading (uivirtlist_t *vl, int colidx, const char *heading);
void  uivlSetColumnMinWidth (uivirtlist_t *vl, int colidx, int minwidth);
void  uivlSetColumnEllipsizeOn (uivirtlist_t *vl, int col);
void  uivlSetColumnAlignEnd (uivirtlist_t *vl, int col);
void  uivlSetColumnGrow (uivirtlist_t *vl, int col, bool grow);
void  uivlSetColumnClass (uivirtlist_t *vl, int col, const char *class);

/* column set specific to a row */
void  uivlSetRowColumnClass (uivirtlist_t *vl, int32_t rownum, int colidx, const char *class);
void  uivlSetRowColumnImage (uivirtlist_t *vl, int32_t rownum, int colidx, uiwcont_t *img, int width);
void  uivlSetRowColumnValue (uivirtlist_t *vl, int32_t rownum, int colidx, const char *value);
void  uivlSetRowColumnNum (uivirtlist_t *vl, int32_t rownum, int colidx, int32_t val);

/* column get */
int   uivlGetColumnIdent (uivirtlist_t *vl, int col);

/* column get specific to a row */
const char *uivlGetRowColumnValue (uivirtlist_t *vl, int row, int col);
const char *uivlGetRowColumnEntryValue (uivirtlist_t *vl, int row, int col);

/* callbacks */
void  uivlSetSelectionCallback (uivirtlist_t *vl, uivlselcb_t cb, void *udata);
void  uivlSetChangeCallback (uivirtlist_t *vl, uivlchangecb_t cb, void *udata);
void  uivlSetRowFillCallback (uivirtlist_t *vl, uivlfillcb_t cb, void *udata);

/* processing */
void  uivlDisplay (uivirtlist_t *vl);


#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_UIVIRTLIST_H */
