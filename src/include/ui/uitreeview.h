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

typedef struct uitree uitree_t;

uitree_t *uiCreateTreeView (void);
void  uiTreeViewFree (uitree_t *uitree);
UIWidget * uiTreeViewGetUIWidget (uitree_t *uitree);
void  uiTreeViewCreateStorage (uitree_t *uitree, int colmax, ...);
void  uiTreeViewAppendColumn (uitree_t *uitree, int coldisp, const char *title, ...);
#if BDJ4_USE_GTK
int   uiTreeViewGetSelection (uitree_t *uitree, GtkTreeModel **model, GtkTreeIter *iter);
#endif
void  uiTreeViewAllowMultiple (uitree_t *uitree);
void  uiTreeViewEnableHeaders (uitree_t *uitree);
void  uiTreeViewDisableHeaders (uitree_t *uitree);
void  uiTreeViewDarkBackground (uitree_t *uitree);
void  uiTreeViewDisableSingleClick (uitree_t *uitree);
void  uiTreeViewSelectionSetMode (uitree_t *uitree, int mode);
void  uiTreeViewSelectionSet (uitree_t *uitree, int row);
int   uiTreeViewSelectionGetCount (uitree_t *uitree);
#if BDJ4_USE_GTK
GtkTreeViewColumn * uiTreeViewAddDisplayColumns (uitree_t *uitree,
    slist_t *sellist, int col, int fontcol, int ellipsizeCol);
GType * uiTreeViewAddDisplayType (GType *types, int valtype, int col);
void  uiTreeViewSetDisplayColumn (GtkTreeModel *model, GtkTreeIter *iter,
    int col, long num, const char *str);
#endif
void  uiTreeViewSetEditedCallback (uitree_t *uitree, callback_t *uicb);
void  uiTreeViewAddEditableColumn (uitree_t *uitree, int col, int editcol, const char *title);

#endif /* INC_UITREEVIEW_H */
