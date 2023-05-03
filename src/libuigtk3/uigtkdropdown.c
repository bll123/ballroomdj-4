/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include <gtk/gtk.h>

#include "bdj4.h"
#include "bdjstring.h"
#include "callback.h"
#include "ilist.h"
#include "mdebug.h"
#include "nlist.h"
#include "slist.h"
#include "uiwcont.h"

#if BDJ4_USE_GTK3
# include "ui-gtk3.h"
#endif

#include "ui/uiwcont-int.h"

#include "ui/uibox.h"
#include "ui/uibutton.h"
#include "ui/uidialog.h"
#include "ui/uidropdown.h"
#include "ui/uitreeview.h"
#include "ui/uiwidget.h"
#include "ui/uiwindow.h"

enum {
  UIUTILS_DROPDOWN_COL_IDX,
  UIUTILS_DROPDOWN_COL_STR,
  UIUTILS_DROPDOWN_COL_DISP,
  UIUTILS_DROPDOWN_COL_SB_PAD,
  UIUTILS_DROPDOWN_COL_MAX,
};

typedef struct uidropdown {
  char          *title;
  callback_t    *selectcb;
  uiwcont_t     *parentwin;
  uibutton_t    *button;
  callback_t    *buttoncb;
  uiwcont_t     *window;
  callback_t    *closecb;
  uitree_t      *uitree;
  slist_t       *strIndexMap;
  nlist_t       *keylist;
  gulong        closeHandlerId;
  char          *strSelection;
  int           maxwidth;
  bool          open : 1;
  bool          iscombobox : 1;
} uidropdown_t;

/* drop-down/combobox handling */
static bool uiDropDownWindowShow (void *udata);
static bool uiDropDownClose (void *udata);
static void uiDropDownButtonCreate (uidropdown_t *dropdown);
static void uiDropDownWindowCreate (uidropdown_t *dropdown, callback_t *uicb, void *udata);
static void uiDropDownSelectionSet (uidropdown_t *dropdown, nlistidx_t internalidx);
static void uiDropDownSelectHandler (GtkTreeView *tv, GtkTreePath *path, GtkTreeViewColumn *column, gpointer udata);
static nlistidx_t uiDropDownSelectionGet (uidropdown_t *dropdown, GtkTreePath *path);

uidropdown_t *
uiDropDownInit (void)
{
  uidropdown_t  *dropdown;

  dropdown = mdmalloc (sizeof (uidropdown_t));

  dropdown->title = NULL;
  dropdown->parentwin = NULL;
  dropdown->button = NULL;
  dropdown->window = NULL;
  dropdown->uitree = NULL;
  dropdown->closeHandlerId = 0;
  dropdown->strSelection = NULL;
  dropdown->strIndexMap = NULL;
  dropdown->keylist = NULL;
  dropdown->open = false;
  dropdown->iscombobox = false;
  dropdown->maxwidth = 10;
  dropdown->buttoncb = NULL;
  dropdown->closecb = NULL;

  return dropdown;
}

void
uiDropDownFree (uidropdown_t *dropdown)
{
  if (dropdown != NULL) {
    uiwcontFree (dropdown->window);
    callbackFree (dropdown->buttoncb);
    callbackFree (dropdown->closecb);
    uiButtonFree (dropdown->button);
    dataFree (dropdown->title);
    if (dropdown->strSelection != NULL) {
      mdfree (dropdown->strSelection);        // allocated by gtk
    }
    slistFree (dropdown->strIndexMap);
    nlistFree (dropdown->keylist);
    uiTreeViewFree (dropdown->uitree);
    mdfree (dropdown);
  }
}

uiwcont_t *
uiDropDownCreate (uidropdown_t *dropdown, uiwcont_t *parentwin,
    const char *title, callback_t *uicb, void *udata)
{
  if (dropdown == NULL) {
    return NULL;
  }

  dropdown->parentwin = parentwin;
  dropdown->title = mdstrdup (title);
  uiDropDownButtonCreate (dropdown);
  uiDropDownWindowCreate (dropdown, uicb, udata);

  return uiButtonGetWidgetContainer (dropdown->button);
}

uiwcont_t *
uiComboboxCreate (uidropdown_t *dropdown, uiwcont_t *parentwin,
    const char *title, callback_t *uicb, void *udata)
{
  if (dropdown == NULL) {
    return NULL;
  }

  dropdown->iscombobox = true;
  return uiDropDownCreate (dropdown, parentwin, title, uicb, udata);
}

void
uiDropDownSetList (uidropdown_t *dropdown, slist_t *list,
    const char *selectLabel)
{
  char              *strval;
  char              *dispval;
  GtkTreeIter       iter;
  GtkListStore      *store = NULL;
  ilistidx_t        iteridx;
  nlistidx_t        internalidx;
  uiwcont_t        *uitreewidgetp;
  char              tbuff [200];

  if (dropdown == NULL || list == NULL) {
    return;
  }

  store = gtk_list_store_new (UIUTILS_DROPDOWN_COL_MAX,
      G_TYPE_LONG, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

  slistFree (dropdown->strIndexMap);
  dropdown->strIndexMap = slistAlloc ("uiutils-str-index", LIST_ORDERED, NULL);
  internalidx = 0;

  dropdown->maxwidth = slistGetMaxKeyWidth (list);
  uitreewidgetp = uiTreeViewGetWidgetContainer (dropdown->uitree);

  snprintf (tbuff, sizeof (tbuff), "%-*s",
      dropdown->maxwidth, dropdown->title);
  uiButtonSetText (dropdown->button, tbuff);

  if (dropdown->iscombobox && selectLabel != NULL) {
    snprintf (tbuff, sizeof (tbuff), "%-*s",
        dropdown->maxwidth, selectLabel);
    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter,
        UIUTILS_DROPDOWN_COL_IDX, (glong) -1,
        UIUTILS_DROPDOWN_COL_STR, "",
        UIUTILS_DROPDOWN_COL_DISP, tbuff,
        UIUTILS_DROPDOWN_COL_SB_PAD, "  ",
        -1);
    slistSetNum (dropdown->strIndexMap, "", internalidx++);
  }

  slistStartIterator (list, &iteridx);

  while ((dispval = slistIterateKey (list, &iteridx)) != NULL) {
    strval = slistGetStr (list, dispval);
    slistSetNum (dropdown->strIndexMap, strval, internalidx);

    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter,
        UIUTILS_DROPDOWN_COL_IDX, (glong) internalidx,
        UIUTILS_DROPDOWN_COL_STR, strval,
        UIUTILS_DROPDOWN_COL_DISP, dispval,
        UIUTILS_DROPDOWN_COL_SB_PAD, "  ",
        -1);
    ++internalidx;
  }

  gtk_tree_view_set_model (GTK_TREE_VIEW (uitreewidgetp->widget),
      GTK_TREE_MODEL (store));
  g_object_unref (store);
}

void
uiDropDownSetNumList (uidropdown_t *dropdown, slist_t *list,
    const char *selectLabel)
{
  char              *dispval;
  GtkTreeIter       iter;
  GtkListStore      *store = NULL;
  char              tbuff [200];
  ilistidx_t        iteridx;
  nlistidx_t        internalidx;
  nlistidx_t        idx;
  uiwcont_t         *uitreewidgetp;

  store = gtk_list_store_new (UIUTILS_DROPDOWN_COL_MAX,
      G_TYPE_LONG, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

  dropdown->keylist = nlistAlloc ("uiutils-keylist", LIST_ORDERED, NULL);
  internalidx = 0;

  dropdown->maxwidth = slistGetMaxKeyWidth (list);
  uitreewidgetp = uiTreeViewGetWidgetContainer (dropdown->uitree);

  snprintf (tbuff, sizeof (tbuff), "%-*s",
      dropdown->maxwidth, dropdown->title);
  uiButtonSetText (dropdown->button, tbuff);

  if (dropdown->iscombobox && selectLabel != NULL) {
    snprintf (tbuff, sizeof (tbuff), "%-*s",
        dropdown->maxwidth, selectLabel);
    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter,
        UIUTILS_DROPDOWN_COL_IDX, (glong) -1,
        UIUTILS_DROPDOWN_COL_STR, "",
        UIUTILS_DROPDOWN_COL_DISP, tbuff,
        UIUTILS_DROPDOWN_COL_SB_PAD, "  ",
        -1);
    nlistSetNum (dropdown->keylist, -1, internalidx);
    ++internalidx;
  }

  slistStartIterator (list, &iteridx);

  while ((dispval = slistIterateKey (list, &iteridx)) != NULL) {
    idx = slistGetNum (list, dispval);
    nlistSetNum (dropdown->keylist, idx, internalidx);

    gtk_list_store_append (store, &iter);
    snprintf (tbuff, sizeof (tbuff), "%-*s",
        dropdown->maxwidth, dispval);
    gtk_list_store_set (store, &iter,
        UIUTILS_DROPDOWN_COL_IDX, (glong) idx,
        UIUTILS_DROPDOWN_COL_STR, "",
        UIUTILS_DROPDOWN_COL_DISP, tbuff,
        UIUTILS_DROPDOWN_COL_SB_PAD, "  ",
        -1);
    ++internalidx;
  }

  gtk_tree_view_set_model (GTK_TREE_VIEW (uitreewidgetp->widget),
      GTK_TREE_MODEL (store));
  g_object_unref (store);
}

void
uiDropDownSelectionSetNum (uidropdown_t *dropdown, nlistidx_t idx)
{
  nlistidx_t    internalidx;

  if (dropdown == NULL) {
    return;
  }

  if (dropdown->keylist == NULL) {
    internalidx = 0;
  } else {
    internalidx = nlistGetNum (dropdown->keylist, idx);
  }
  uiDropDownSelectionSet (dropdown, internalidx);
}

void
uiDropDownSelectionSetStr (uidropdown_t *dropdown, const char *stridx)
{
  nlistidx_t    internalidx;


  if (dropdown == NULL) {
    return;
  }

  if (dropdown->strIndexMap == NULL) {
    internalidx = 0;
  } else {
    internalidx = slistGetNum (dropdown->strIndexMap, stridx);
    if (internalidx < 0) {
      internalidx = 0;
    }
  }
  uiDropDownSelectionSet (dropdown, internalidx);
}

void
uiDropDownSetState (uidropdown_t *dropdown, int state)
{
  if (dropdown == NULL) {
    return;
  }
  uiButtonSetState (dropdown->button, state);
}

char *
uiDropDownGetString (uidropdown_t *dropdown)
{
  if (dropdown == NULL) {
    return NULL;
  }
  return dropdown->strSelection;
}

/* internal routines */

static bool
uiDropDownWindowShow (void *udata)
{
  uidropdown_t  *dropdown = udata;
  uiwcont_t    *uiwidgetp;
  int           x, y, ws;
  int           bx, by;


  if (dropdown == NULL) {
    return UICB_STOP;
  }

  bx = 0;
  by = 0;
  uiWindowGetPosition (dropdown->parentwin, &x, &y, &ws);
  uiwidgetp = uiButtonGetWidgetContainer (dropdown->button);
  if (uiwidgetp != NULL) {
    uiWidgetGetPosition (uiwidgetp, &bx, &by);
  }
  uiWidgetShowAll (dropdown->window);
  uiWindowMove (dropdown->window, bx + x + 4, by + y + 4 + 30, -1);
  uiWindowPresent (dropdown->window);
  dropdown->open = true;
  return UICB_CONT;
}

static bool
uiDropDownClose (void *udata)
{
  uidropdown_t *dropdown = udata;

  if (dropdown->open) {
    uiWidgetHide (dropdown->window);
    dropdown->open = false;
  }
  uiWindowPresent (dropdown->parentwin);

  return UICB_CONT;
}

static void
uiDropDownButtonCreate (uidropdown_t *dropdown)
{
  uiwcont_t  *uiwidgetp;

  dropdown->buttoncb = callbackInit (uiDropDownWindowShow, dropdown, NULL);
  dropdown->button = uiCreateButton (dropdown->buttoncb, NULL,
      "button_down_small");
  uiButtonAlignLeft (dropdown->button);
  uiButtonSetImagePosRight (dropdown->button);
  uiwidgetp = uiButtonGetWidgetContainer (dropdown->button);
  uiWidgetSetMarginTop (uiwidgetp, 1);
  uiWidgetSetMarginStart (uiwidgetp, 1);
}


static void
uiDropDownWindowCreate (uidropdown_t *dropdown,
    callback_t *uicb, void *udata)
{
  uiwcont_t         *uiwidgetp;
  uiwcont_t         *vbox = NULL;
  uiwcont_t         *mainvbox = NULL;
  uiwcont_t         *uiscwin;
  GtkCellRenderer   *renderer = NULL;
  GtkTreeViewColumn *column = NULL;


  dropdown->closecb = callbackInit ( uiDropDownClose, dropdown, NULL);
  dropdown->window = uiCreateDialogWindow (dropdown->parentwin,
      uiButtonGetWidgetContainer (dropdown->button), dropdown->closecb, "");

  mainvbox = uiCreateVertBox ();
  uiWidgetExpandHoriz (mainvbox);
  uiWidgetExpandVert (mainvbox);
  uiBoxPackInWindow (dropdown->window, mainvbox);

  vbox = uiCreateVertBox ();
  uiBoxPackStartExpand (mainvbox, vbox);

  uiscwin = uiCreateScrolledWindow (300);
  uiWidgetExpandHoriz (uiscwin);
  uiBoxPackStartExpand (vbox, uiscwin);

  dropdown->uitree = uiCreateTreeView ();
  uiwidgetp = uiTreeViewGetWidgetContainer (dropdown->uitree);
  if (G_IS_OBJECT (uiwidgetp->widget)) {
    g_object_ref_sink (G_OBJECT (uiwidgetp->widget));
  }
  uiTreeViewDisableHeaders (dropdown->uitree);
  uiTreeViewSelectSetMode (dropdown->uitree, SELECT_SINGLE);
  uiWidgetExpandHoriz (uiwidgetp);
  uiWidgetExpandVert (uiwidgetp);
  uiBoxPackInWindow (uiscwin, uiwidgetp);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("",
      renderer, "text", UIUTILS_DROPDOWN_COL_DISP, NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  gtk_tree_view_append_column (GTK_TREE_VIEW (uiwidgetp->widget), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("",
      renderer, "text", UIUTILS_DROPDOWN_COL_SB_PAD, NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
  gtk_tree_view_append_column (GTK_TREE_VIEW (uiwidgetp->widget), column);

  if (uicb != NULL) {
    dropdown->selectcb = uicb;
    g_signal_connect (uiwidgetp->widget, "row-activated",
        G_CALLBACK (uiDropDownSelectHandler), dropdown);
  }

  uiwcontFree (mainvbox);
  uiwcontFree (vbox);
  uiwcontFree (uiscwin);
}

static void
uiDropDownSelectionSet (uidropdown_t *dropdown, nlistidx_t internalidx)
{
  GtkTreePath   *path = NULL;
  GtkTreeModel  *model = NULL;
  GtkTreeIter   iter;
  char          tbuff [200];
  char          *p;
  uiwcont_t     *uitreewidgetp;


  if (dropdown == NULL || dropdown->uitree == NULL) {
    return;
  }
  uitreewidgetp = uiTreeViewGetWidgetContainer (dropdown->uitree);
  if (uitreewidgetp->widget == NULL) {
    return;
  }

  if (internalidx < 0) {
    internalidx = 0;
  }

  uiTreeViewSelectSet (dropdown->uitree, internalidx);

  snprintf (tbuff, sizeof (tbuff), "%d", internalidx);
  path = gtk_tree_path_new_from_string (tbuff);
  mdextalloc (path);
  if (path != NULL) {
    model = gtk_tree_view_get_model (GTK_TREE_VIEW (uitreewidgetp->widget));
    if (model != NULL) {
      gtk_tree_model_get_iter (model, &iter, path);
      gtk_tree_model_get (model, &iter, UIUTILS_DROPDOWN_COL_DISP, &p, -1);
      if (p != NULL) {
        snprintf (tbuff, sizeof (tbuff), "%-*s",
            dropdown->maxwidth, p);
        uiButtonSetText (dropdown->button, tbuff);
      }
    }
    mdextfree (path);
    gtk_tree_path_free (path);
  }
}

static void
uiDropDownSelectHandler (GtkTreeView *tv, GtkTreePath *path,
    GtkTreeViewColumn *column, gpointer udata)
{
  uidropdown_t  *dropdown = udata;
  long          idx;

  idx = uiDropDownSelectionGet (dropdown, path);
  callbackHandlerLong (dropdown->selectcb, idx);
}

static nlistidx_t
uiDropDownSelectionGet (uidropdown_t *dropdown, GtkTreePath *path)
{
  GtkTreeIter   iter;
  GtkTreeModel  *model = NULL;
  glong         idx = 0;
  nlistidx_t    retval;
  char          tbuff [200];
  uiwcont_t     *uitreewidgetp;

  uitreewidgetp = uiTreeViewGetWidgetContainer (dropdown->uitree);

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (uitreewidgetp->widget));
  if (gtk_tree_model_get_iter (model, &iter, path)) {
    gtk_tree_model_get (model, &iter, UIUTILS_DROPDOWN_COL_IDX, &idx, -1);
    retval = idx;
    if (dropdown->strSelection != NULL) {
      mdfree (dropdown->strSelection);        // allocated by gtk
    }
    gtk_tree_model_get (model, &iter, UIUTILS_DROPDOWN_COL_STR,
        &dropdown->strSelection, -1);
    mdextalloc (dropdown->strSelection);
    if (dropdown->iscombobox) {
      char  *p;

      gtk_tree_model_get (model, &iter, UIUTILS_DROPDOWN_COL_DISP, &p, -1);
      snprintf (tbuff, sizeof (tbuff), "%-*s", dropdown->maxwidth, p);
      uiButtonSetText (dropdown->button, tbuff);
    }
    uiWidgetHide (dropdown->window);
    dropdown->open = false;
  } else {
    return -1;
  }

  return retval;
}

