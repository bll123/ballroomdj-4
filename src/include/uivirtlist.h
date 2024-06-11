/*
 * Copyright 2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIVIRTLIST_H
#define INC_UIVIRTLIST_H

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

#include <stdint.h>

#include "uiwcont.h"

typedef enum {
  VL_TYPE_LABEL,
  VL_TYPE_IMAGE,
  VL_TYPE_ENTRY,
  VL_TYPE_RADIO_BUTTON,
  VL_TYPE_CHECK_BUTTON,
  VL_TYPE_SPINBOX_NUM,
  VL_TYPE_INT_NUMERIC,
} vltype_t;

enum {
  VL_COL_HIDE = false,
  VL_COL_SHOW = true,
  VL_COL_UNKNOWN = -1,
};

typedef struct uivirtlist uivirtlist_t;
typedef void (*uivlfillcb_t) (void *udata, uivirtlist_t *vl, uint32_t rownum);
typedef void (*uivlselcb_t) (void *udata, uivirtlist_t *vl, uint32_t rownum, int colnum);
typedef void (*uivlchangecb_t) (void *udata, uivirtlist_t *vl, uint32_t rownum, int colnum);

uivirtlist_t *uiCreateVirtList (uiwcont_t *boxp, int initialdisprows);
void  uivlFree (uivirtlist_t *vl);
void  uivlSetNumRows (uivirtlist_t *vl, uint32_t numrows);
void  uivlSetNumColumns (uivirtlist_t *vl, int numcols);

/* headings */
void  uivlSetHeadingFont (uivirtlist_t *vl, int colnum, const char *font);
void  uivlSetHeadingClass (uivirtlist_t *vl, int colnum, const char *class);

/* column set */
void  uivlSetColumn (uivirtlist_t *vl, int colnum, vltype_t type, int ident, bool hidden);
void  uivlSetColumnHeading (uivirtlist_t *vl, int colnum, const char *heading);
void  uivlSetColumnMaxWidth (uivirtlist_t *vl, int colnum, int maxwidth);
void  uivlSetColumnFont (uivirtlist_t *vl, int colnum, const char *font);
void  uivlSetColumnEllipsizeOn (uivirtlist_t *vl, int col);
void  uivlSetColumnAlignEnd (uivirtlist_t *vl, int col);

/* column set specific to a row */
void  uivlSetColumnClass (uivirtlist_t *vl, uint32_t rownum, int colnum, const char *class);
void  uivlSetColumnValue (uivirtlist_t *vl, int32_t rownum, int colnum, const char *value);
void  uivlSetColumnValueNum (uivirtlist_t *vl, int32_t rownum, int colnum, int32_t val);

/* column get */
int   uivlGetColumnIdent (uivirtlist_t *vl, int col);

/* column get specific to a row */
const char *uivlGetColumnValue (uivirtlist_t *vl, int row, int col);
const char *uivlGetColumnEntryValue (uivirtlist_t *vl, int row, int col);

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