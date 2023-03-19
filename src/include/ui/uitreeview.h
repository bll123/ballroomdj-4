/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UITREEVIEW_H
#define INC_UITREEVIEW_H

#if BDJ4_USE_GTK
# include <gtk/gtk.h>
#endif

#if BDJ4_USE_GTK
# include "ui-gtk3.h"
#endif

#include "callback.h"
#include "slist.h"

typedef long  treenum_t;
typedef int   treeint_t;
typedef int   treebool_t;

enum {
  TREE_COL_MODE_TEXT,
  TREE_COL_MODE_EDITABLE,
  TREE_COL_MODE_FONT,
  TREE_COL_MODE_MARKUP,
  TREE_COL_MODE_ADJUSTMENT,
  TREE_COL_MODE_DIGITS,
  TREE_COL_MODE_ACTIVE,
  TREE_COL_MODE_FOREGROUND,
  TREE_COL_MODE_FOREGROUND_SET,
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
  TREE_WIDGET_TIME,
  TREE_COLUMN_HIDDEN,
  TREE_COLUMN_SHOWN,
  SELECT_SINGLE,
  SELECT_MULTIPLE,
};

enum {
  TREE_TYPE_END = -1,
  TREE_COL_MODE_END = -1,
  TREE_VALUE_END = -1,
};

typedef struct uitree uitree_t;

uitree_t *uiCreateTreeView (void);
void  uiTreeViewFree (uitree_t *uitree);
void  uiTreeViewEnableHeaders (uitree_t *uitree);
void  uiTreeViewDisableHeaders (uitree_t *uitree);
void  uiTreeViewDarkBackground (uitree_t *uitree);
void  uiTreeViewDisableSingleClick (uitree_t *uitree);
void  uiTreeViewSelectSetMode (uitree_t *uitree, int mode);
void  uiTreeViewSetRowActivatedCallback (uitree_t *uitree, callback_t *cb);
void  uiTreeViewSetEditedCallback (uitree_t *uitree, callback_t *cb);
void  uiTreeViewSetRadioCallback (uitree_t *uitree, callback_t *cb);
UIWidget * uiTreeViewGetUIWidget (uitree_t *uitree);
void  uiTreeViewAppendColumn (uitree_t *uitree, int widgettype, int coldisp, const char *title, ...);
void  uiTreeViewColumnSetVisible (uitree_t *uitree, int col, int flag);
void  uiTreeViewCreateValueStore (uitree_t *uitree, int colmax, ...);
void  uiTreeViewCreateValueStoreFromList (uitree_t *uitree, int colmax, int *typelist);
void  uiTreeViewValueAppend (uitree_t *uitree);
void  uiTreeViewValueInsertBefore (uitree_t *uitree);
void  uiTreeViewValueInsertAfter (uitree_t *uitree);
void  uiTreeViewValueRemove (uitree_t *uitree);
void  uiTreeViewSetValues (uitree_t *uitree, ...);
int   uiTreeViewSelectGetCount (uitree_t *uitree);
int   uiTreeViewSelectGetIndex (uitree_t *uitree);
void  uiTreeViewSelectCurrent (uitree_t *uitree);
bool  uiTreeViewSelectFirst (uitree_t *uitree);
bool  uiTreeViewSelectNext (uitree_t *uitree);
bool  uiTreeViewSelectPrevious (uitree_t *uitree);
int   uiTreeViewSelectDefault (uitree_t *uitree);
void  uiTreeViewSelectSave (uitree_t *uitree);
void  uiTreeViewSelectRestore (uitree_t *uitree);
void  uiTreeViewMoveBefore (uitree_t *uitree);
void  uiTreeViewMoveAfter (uitree_t *uitree);
long  uiTreeViewGetValue (uitree_t *uitree, int col);
char *uiTreeViewGetValueStr (uitree_t *uitree, int col);
void  uiTreeViewForeach (uitree_t *uitree, callback_t *cb);
void  uiTreeViewSelectSet (uitree_t *uitree, int row);
/* these routines will be re-worked at a later date */
/* these routines will be moved into libbdj4ui later */
#if BDJ4_USE_GTK
GtkTreeViewColumn * uiTreeViewAddDisplayColumns (uitree_t *uitree,
    slist_t *sellist, int col, int fontcol, int ellipsizeCol);
void  uiTreeViewSetDisplayColumn (GtkTreeModel *model, GtkTreeIter *iter,
    int col, long num, const char *str);
#endif
/* these routines will be removed at a later date */
#if BDJ4_USE_GTK
int   uiTreeViewGetSelection (uitree_t *uitree, GtkTreeModel **model, GtkTreeIter *iter);
#endif

#endif /* INC_UITREEVIEW_H */
