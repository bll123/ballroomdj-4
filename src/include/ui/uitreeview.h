/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UITREEVIEW_H
#define INC_UITREEVIEW_H

#if BDJ4_USE_GTK
# include "ui-gtk3.h"
#endif

#include "callback.h"
#include "slist.h"

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
};

enum {
  TREE_TYPE_END = -1,
  TREE_COL_TYPE_END = -1,
  TREE_VALUE_END = -1,
  TREE_NO_COLUMN = -1,
  TREE_NO_MIN_WIDTH = -1,
};

typedef struct uitree uitree_t;

uitree_t *uiCreateTreeView (void);
void  uiTreeViewFree (uitree_t *uitree);
void  uiTreeViewEnableHeaders (uitree_t *uitree);
void  uiTreeViewDisableHeaders (uitree_t *uitree);
void  uiTreeViewDarkBackground (uitree_t *uitree);
void  uiTreeViewDisableSingleClick (uitree_t *uitree);
void  uiTreeViewSelectSetMode (uitree_t *uitree, int mode);
void  uiTreeViewSetSelectChangedCallback (uitree_t *uitree, callback_t *cb);
void  uiTreeViewSetRowActivatedCallback (uitree_t *uitree, callback_t *cb);
void  uiTreeViewSetEditedCallback (uitree_t *uitree, callback_t *cb);
void  uiTreeViewRadioSetRow (uitree_t *uitree, int row);
uiwcont_t * uiTreeViewGetWidgetContainer (uitree_t *uitree);
void  uiTreeViewPreColumnSetMinWidth (uitree_t *uitree, int minwidth);
void  uiTreeViewPreColumnSetEllipsizeColumn (uitree_t *uitree, int ellipsizeColumn);
void  uiTreeViewAppendColumn (uitree_t *uitree, int activecol, int widgettype, int alignment, int coldisp, const char *title, ...);
void  uiTreeViewColumnSetVisible (uitree_t *uitree, int col, int flag);
void  uiTreeViewCreateValueStore (uitree_t *uitree, int colmax, ...);
void  uiTreeViewCreateValueStoreFromList (uitree_t *uitree, int colmax, int *typelist);
void  uiTreeViewValueAppend (uitree_t *uitree);
void  uiTreeViewValueInsertBefore (uitree_t *uitree);
void  uiTreeViewValueInsertAfter (uitree_t *uitree);
void  uiTreeViewValueRemove (uitree_t *uitree);
void  uiTreeViewValueClear (uitree_t *uitree, int startidx);
void  uiTreeViewSetValueEllipsize (uitree_t *uitree, int col);
void  uiTreeViewSetValues (uitree_t *uitree, ...);
int   uiTreeViewSelectGetCount (uitree_t *uitree);
int   uiTreeViewSelectGetIndex (uitree_t *uitree);
void  uiTreeViewSelectCurrent (uitree_t *uitree);
bool  uiTreeViewSelectFirst (uitree_t *uitree);
bool  uiTreeViewSelectNext (uitree_t *uitree);
bool  uiTreeViewSelectPrevious (uitree_t *uitree);
void  uiTreeViewSelectDefault (uitree_t *uitree);
void  uiTreeViewSelectSave (uitree_t *uitree);
void  uiTreeViewSelectRestore (uitree_t *uitree);
void  uiTreeViewSelectClearAll (uitree_t *uitree);
void  uiTreeViewSelectForeach (uitree_t *uitree, callback_t *cb);
void  uiTreeViewMoveBefore (uitree_t *uitree);
void  uiTreeViewMoveAfter (uitree_t *uitree);
long  uiTreeViewGetValue (uitree_t *uitree, int col);
char *uiTreeViewGetValueStr (uitree_t *uitree, int col);
void  uiTreeViewForeach (uitree_t *uitree, callback_t *cb);
void  uiTreeViewSelectSet (uitree_t *uitree, int row);
void  uiTreeViewValueIteratorSet (uitree_t *uitree, int row);
void  uiTreeViewValueIteratorClear (uitree_t *uitree);
void  uiTreeViewScrollToCell (uitree_t *uitree);

#endif /* INC_UITREEVIEW_H */
