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

uiwcont_t *uiCreateVirtList (int initialdisprows);
void  uivlFree (uiwcont_t *vlcont);
void  uivlSetNumRows (uiwcont_t *vlcont, uint32_t numrows);

/* headings */
/* setting the headings also determines the number of columns */
void  uivlSetHeadings (uiwcont_t *vlcont, ...);
void  uivlSetHeadingFont (uiwcont_t *vlcont, int col, const char *font);
void  uivlSetHeadingClass (uiwcont_t *vlcont, int col, const char *class);

/* column operations */
void  uivlSetColumn (uiwcont_t *vlcont, int col, vltype_t type, int ident, bool hidden);
void  uivlSetColumnMaxWidth (uiwcont_t *vlcont, int col, int maxwidth);
void  uivlSetColumnFont (uiwcont_t *vlcont, int col, const char *font);
void  uivlSetColumnEllipsizeOn (uiwcont_t *vlcont, int col);
void  uivlSetColumnClass (uiwcont_t *vlcont, int col, const char *class);
void  uivlSetColumnAlignEnd (uiwcont_t *vlcont, int col);
void  uivlSetColumnValue (uiwcont_t *vlcont, int32_t rownum, int col, const char *value);
void  uivlSetColumnValueNum (uiwcont_t *vlcont, int32_t rownum, int col, int32_t val);

int   uivlGetColumnIdent (uiwcont_t *vlcont, int col);
const char *uivlGetColumnValue (uiwcont_t *vlcont, int row, int col);
const char *uivlGetColumnEntryValue (uiwcont_t *vlcont, int row, int col);

/* callbacks */
void  uivlSetSelectionCallback (uiwcont_t *vlcont, callback_t *cb, void *udata);
void  uivlSetEntryCallback (uiwcont_t *vlcont, callback_t *cb, void *udata);
void  uivlSetRowFillCallback (uiwcont_t *vlcont, callback_t *cb, void *udata);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_UIVIRTLIST_H */
