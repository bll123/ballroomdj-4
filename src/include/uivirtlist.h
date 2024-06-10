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
  VL_TYPE_INT_NUMERIC,
} vltype_t;

enum {
  VL_COL_HIDDEN = false,
  VL_COL_SHOW = true,
  VL_COL_UNKNOWN = -1,
  VL_ROW_UNKNOWN = -1,
  VL_ROW_HEADING = -2,
  VL_MAX_WIDTH_ANY = -1,
};

typedef struct uivirtlist uivirtlist_t;

uivirtlist_t *uiCreateVirtList (uiwcont_t *boxp, int initialdisprows);
void  uivlFree (uivirtlist_t *vl);
void  uivlSetNumRows (uivirtlist_t *vl, uint32_t numrows);

/* headings */
/* setting the headings also determines the number of columns */
void  uivlSetHeadings (uivirtlist_t *vl, ...);
void  uivlSetHeadingFont (uivirtlist_t *vl, int colnum, const char *font);
void  uivlSetHeadingClass (uivirtlist_t *vl, int colnum, const char *class);

/* column operations */
void  uivlSetColumn (uivirtlist_t *vl, int colnum, vltype_t type, int ident, bool hidden);
void  uivlSetColumnMaxWidth (uivirtlist_t *vl, int colnum, int maxwidth);
void  uivlSetColumnFont (uivirtlist_t *vl, int colnum, const char *font);
void  uivlSetColumnEllipsizeOn (uivirtlist_t *vl, int col);
void  uivlSetColumnAlignEnd (uivirtlist_t *vl, int col);
void  uivlSetColumnClass (uivirtlist_t *vl, uint32_t rownum, int colnum, const char *class);
void  uivlSetColumnValue (uivirtlist_t *vl, int32_t rownum, int colnum, const char *value);
void  uivlSetColumnValueNum (uivirtlist_t *vl, int32_t rownum, int colnum, int32_t val);

int   uivlGetColumnIdent (uivirtlist_t *vl, int col);
const char *uivlGetColumnValue (uivirtlist_t *vl, int row, int col);
const char *uivlGetColumnEntryValue (uivirtlist_t *vl, int row, int col);

/* callbacks */
void  uivlSetSelectionCallback (uivirtlist_t *vl, callback_t *cb, void *udata);
void  uivlSetEntryCallback (uivirtlist_t *vl, callback_t *cb, void *udata);
void  uivlSetRowFillCallback (uivirtlist_t *vl, callback_t *cb, void *udata);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_UIVIRTLIST_H */
