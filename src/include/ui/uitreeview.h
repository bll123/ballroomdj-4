/*
 * Copyright 2023-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UITREEVIEW_H
#define INC_UITREEVIEW_H

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

#include "callback.h"
#include "slist.h"
#include "uiwcont.h"

typedef long  treenum_t;
typedef int   treeint_t;
typedef int   treebool_t;

enum {
  TREE_COL_TYPE_TEXT,
  TREE_COL_TYPE_EDITABLE,
  TREE_COL_TYPE_FONT,
  TREE_COL_TYPE_MARKUP,
  TREE_COL_TYPE_ADJUSTMENT,
  TREE_COL_TYPE_DIGITS,
  TREE_COL_TYPE_ACTIVE,
  TREE_COL_TYPE_FOREGROUND,
  TREE_COL_TYPE_FOREGROUND_SET,
  TREE_COL_TYPE_IMAGE,
  TREE_COL_DISP_NORM,
  TREE_COL_DISP_GROW,
  TREE_TYPE_STRING,
  TREE_TYPE_NUM,
  TREE_TYPE_INT,
  TREE_TYPE_WIDGET,
  TREE_TYPE_BOOLEAN,
  TREE_TYPE_ELLIPSIZE,
  TREE_TYPE_IMAGE,
  TREE_WIDGET_TEXT,
  TREE_WIDGET_SPINBOX,
  TREE_WIDGET_IMAGE,
  TREE_WIDGET_CHECKBOX,
  TREE_WIDGET_RADIO,
  TREE_ALIGN_NORM,
  TREE_ALIGN_RIGHT,
  TREE_ALIGN_CENTER,
  TREE_COLUMN_HIDDEN,
  TREE_COLUMN_SHOWN,
  SELECT_SINGLE,
  SELECT_MULTIPLE,
  TREE_SCROLL_NEXT,
  TREE_SCROLL_PREV,
};

enum {
  TREE_TYPE_END = -1,
  TREE_COL_TYPE_END = -1,
  TREE_VALUE_END = -1,
  TREE_NO_COLUMN = -1,
  TREE_NO_ROW = -1,
  TREE_NO_MIN_WIDTH = -1,
};

uiwcont_t *uiCreateTreeView (void);
void  uiTreeViewFree (uiwcont_t *uiwidget);
void  uiTreeViewEnableHeaders (uiwcont_t *uiwidget);
void  uiTreeViewDisableHeaders (uiwcont_t *uiwidget);
void  uiTreeViewDarkBackground (uiwcont_t *uiwidget);
void  uiTreeViewDisableSingleClick (uiwcont_t *uiwidget);
void  uiTreeViewSelectSetMode (uiwcont_t *uiwidget, int mode);
void  uiTreeViewSetSelectChangedCallback (uiwcont_t *uiwidget, callback_t *cb);
void  uiTreeViewSetSizeChangeCallback (uiwcont_t *uiwidget, callback_t *cb);
void  uiTreeViewSetScrollEventCallback (uiwcont_t *uiwidget, callback_t *cb);
void  uiTreeViewSetRowActivatedCallback (uiwcont_t *uiwidget, callback_t *cb);
void  uiTreeViewSetEditedCallback (uiwcont_t *uiwidget, callback_t *cb);
void  uiTreeViewRadioSetRow (uiwcont_t *uiwidget, int row);
uiwcont_t * uiTreeViewGetWidgetContainer (uiwcont_t *uiwidget);
void  uiTreeViewPreColumnSetMinWidth (uiwcont_t *uiwidget, int minwidth);
void  uiTreeViewPreColumnSetEllipsizeColumn (uiwcont_t *uiwidget, int ellipsizeColumn);
void  uiTreeViewPreColumnSetColorColumn (uiwcont_t *uiwidget, int colcol, int colsetcol);
void  uiTreeViewAppendColumn (uiwcont_t *uiwidget, int activecol, int widgettype, int alignment, int coldisp, const char *title, ...);
void  uiTreeViewColumnSetVisible (uiwcont_t *uiwidget, int col, int flag);
void  uiTreeViewCreateValueStore (uiwcont_t *uiwidget, int colmax, ...);
void  uiTreeViewCreateValueStoreFromList (uiwcont_t *uiwidget, int colmax, int *typelist);
void  uiTreeViewValueAppend (uiwcont_t *uiwidget);
void  uiTreeViewValueInsertBefore (uiwcont_t *uiwidget);
void  uiTreeViewValueInsertAfter (uiwcont_t *uiwidget);
void  uiTreeViewValueRemove (uiwcont_t *uiwidget);
void  uiTreeViewValueClear (uiwcont_t *uiwidget, int startidx);
void  uiTreeViewSetValueEllipsize (uiwcont_t *uiwidget, int col);
void  uiTreeViewSetValues (uiwcont_t *uiwidget, ...);
int   uiTreeViewSelectGetCount (uiwcont_t *uiwidget);
int   uiTreeViewSelectGetIndex (uiwcont_t *uiwidget);
void  uiTreeViewSelectCurrent (uiwcont_t *uiwidget);
bool  uiTreeViewSelectFirst (uiwcont_t *uiwidget);
bool  uiTreeViewSelectNext (uiwcont_t *uiwidget);
bool  uiTreeViewSelectPrevious (uiwcont_t *uiwidget);
void  uiTreeViewSelectDefault (uiwcont_t *uiwidget);
void  uiTreeViewSelectSave (uiwcont_t *uiwidget);
void  uiTreeViewSelectRestore (uiwcont_t *uiwidget);
void  uiTreeViewSelectClear (uiwcont_t *uiwidget);
void  uiTreeViewSelectClearAll (uiwcont_t *uiwidget);
void  uiTreeViewSelectForeach (uiwcont_t *uiwidget, callback_t *cb);
void  uiTreeViewMoveBefore (uiwcont_t *uiwidget);
void  uiTreeViewMoveAfter (uiwcont_t *uiwidget);
long  uiTreeViewGetValue (uiwcont_t *uiwidget, int col);
char *uiTreeViewGetValueStr (uiwcont_t *uiwidget, int col);
long  uiTreeViewSelectForeachGetValue (uiwcont_t *uiwidget, int col);
void  uiTreeViewForeach (uiwcont_t *uiwidget, callback_t *cb);
void  uiTreeViewSelectSet (uiwcont_t *uiwidget, int row);
void  uiTreeViewValueIteratorSet (uiwcont_t *uiwidget, int row);
void  uiTreeViewValueIteratorClear (uiwcont_t *uiwidget);
void  uiTreeViewScrollToCell (uiwcont_t *uiwidget);
void  uiTreeViewAttachScrollController (uiwcont_t *uiwidget, double upper);
int   uiTreeViewGetDragDropRow (uiwcont_t *uiwcont, int x, int y);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_UITREEVIEW_H */
