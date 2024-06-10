/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
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

#include "ui-gtk3.h"

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
  uiwcont_t     *button;
  callback_t    *buttoncb;
  uiwcont_t     *window;
  callback_t    *closecb;
  uiwcont_t     *uitree;
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
static void uiDropDownButtonCreate (uiwcont_t *uiwidget);
static void uiDropDownWindowCreate (uiwcont_t *uiwidget, callback_t *uicb, void *udata);
static void uiDropDownSelectionSet (uiwcont_t *uiwidget, nlistidx_t internalidx);
static void uiDropDownSelectHandler (GtkTreeView *tv, GtkTreePath *path, GtkTreeViewColumn *column, gpointer udata);
static nlistidx_t uiDropDownSelectionGet (uiwcont_t *uiwidget, GtkTreePath *path);

uiwcont_t *
uiDropDownInit (void)
{
  uiwcont_t     *uiwidget;
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

  uiwidget = uiwcontAlloc ();
  uiwidget->wbasetype = WCONT_T_DROPDOWN;
  uiwidget->wtype = WCONT_T_DROPDOWN;
  uiwidget->uiint.uidropdown = dropdown;
  /* empty widget is used so that the validity check works */
  uiwidget->uidata.widget = (GtkWidget *) WCONT_EMPTY_WIDGET;
  uiwidget->uidata.packwidget = (GtkWidget *) WCONT_EMPTY_WIDGET;

  return uiwidget;
}

/* only frees the internals */
void
uiDropDownFree (uiwcont_t *uiwidget)
{
  uidropdown_t    *dropdown;

  if (! uiwcontValid (uiwidget, WCONT_T_DROPDOWN, "dropdown-free")) {
    return;
  }

  dropdown = uiwidget->uiint.uidropdown;

  uiwcontBaseFree (dropdown->window);
  uiButtonFree (dropdown->button);
  uiwcontBaseFree (dropdown->button);
  callbackFree (dropdown->buttoncb);
  callbackFree (dropdown->closecb);
  dataFree (dropdown->title);
  if (dropdown->strSelection != NULL) {
    mdextfree (dropdown->strSelection);        // allocated by gtk
  }
  slistFree (dropdown->strIndexMap);
  nlistFree (dropdown->keylist);
  uiTreeViewFree (dropdown->uitree);
  uiwcontBaseFree (dropdown->uitree);
  mdfree (dropdown);
  /* the container is freed by uiwcontFree() */
}

/* returns the button */
uiwcont_t *
uiDropDownCreate (uiwcont_t *uiwidget, uiwcont_t *parentwin,
    const char *title, callback_t *uicb, void *udata)
{
  uidropdown_t  *dropdown;

  if (! uiwcontValid (uiwidget, WCONT_T_DROPDOWN, "dropdown-create")) {
    return NULL;
  }
  if (! uiwcontValid (parentwin, WCONT_T_WINDOW, "dropdown-create-win")) {
    return NULL;
  }

  dropdown = uiwidget->uiint.uidropdown;
  dropdown->parentwin = parentwin;
  if (title != NULL) {
    dropdown->title = mdstrdup (title);
  }
  uiDropDownButtonCreate (uiwidget);
  uiDropDownWindowCreate (uiwidget, uicb, udata);

  uiwidget->uidata.packwidget = dropdown->button->uidata.packwidget;

  return dropdown->button;
}

/* returns the button */
uiwcont_t *
uiComboboxCreate (uiwcont_t *uiwidget, uiwcont_t *parentwin,
    const char *title, callback_t *uicb, void *udata)
{
  uidropdown_t    *dropdown;

  if (! uiwcontValid (uiwidget, WCONT_T_DROPDOWN, "combobox-create")) {
    return NULL;
  }

  dropdown = uiwidget->uiint.uidropdown;
  dropdown->iscombobox = true;
  return uiDropDownCreate (uiwidget, parentwin, title, uicb, udata);
}

void
uiDropDownSetList (uiwcont_t *uiwidget, slist_t *list,
    const char *selectLabel)
{
  uidropdown_t  *dropdown;
  const char    *strval;
  const char    *dispval;
  GtkTreeIter   iter;
  GtkListStore  *store = NULL;
  ilistidx_t    iteridx;
  nlistidx_t    internalidx;
  char          tbuff [200];

  if (! uiwcontValid (uiwidget, WCONT_T_DROPDOWN, "dropdown-set-list")) {
    return;
  }
  if (list == NULL) {
    return;
  }

  dropdown = uiwidget->uiint.uidropdown;

  store = gtk_list_store_new (UIUTILS_DROPDOWN_COL_MAX,
      G_TYPE_LONG, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

  slistFree (dropdown->strIndexMap);
  dropdown->strIndexMap = slistAlloc ("uiutils-str-index", LIST_ORDERED, NULL);
  internalidx = 0;

  dropdown->maxwidth = slistGetMaxKeyWidth (list);

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

  gtk_tree_view_set_model (GTK_TREE_VIEW (dropdown->uitree->uidata.widget),
      GTK_TREE_MODEL (store));
  g_object_unref (store);
}

void
uiDropDownSetNumList (uiwcont_t *uiwidget, slist_t *list,
    const char *selectLabel)
{
  uidropdown_t      *dropdown;
  const char        *dispval;
  GtkTreeIter       iter;
  GtkListStore      *store = NULL;
  char              tbuff [200];
  ilistidx_t        iteridx;
  nlistidx_t        internalidx;
  nlistidx_t        idx;

  if (! uiwcontValid (uiwidget, WCONT_T_DROPDOWN, "dropdown-set-num-list")) {
    return;
  }
  if (list == NULL) {
    return;
  }

  dropdown = uiwidget->uiint.uidropdown;

  store = gtk_list_store_new (UIUTILS_DROPDOWN_COL_MAX,
      G_TYPE_LONG, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

  dropdown->keylist = nlistAlloc ("uiutils-keylist", LIST_ORDERED, NULL);
  internalidx = 0;

  dropdown->maxwidth = slistGetMaxKeyWidth (list);

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

  gtk_tree_view_set_model (GTK_TREE_VIEW (dropdown->uitree->uidata.widget),
      GTK_TREE_MODEL (store));
  g_object_unref (store);
}

void
uiDropDownSelectionSetNum (uiwcont_t *uiwidget, nlistidx_t idx)
{
  uidropdown_t  *dropdown;
  nlistidx_t    internalidx;

  if (! uiwcontValid (uiwidget, WCONT_T_DROPDOWN, "dropdown-sel-set-num")) {
    return;
  }

  dropdown = uiwidget->uiint.uidropdown;

  if (dropdown->keylist == NULL) {
    internalidx = 0;
  } else {
    internalidx = nlistGetNum (dropdown->keylist, idx);
  }
  uiDropDownSelectionSet (uiwidget, internalidx);
}

void
uiDropDownSelectionSetStr (uiwcont_t *uiwidget, const char *stridx)
{
  uidropdown_t  *dropdown;
  nlistidx_t    internalidx;

  if (! uiwcontValid (uiwidget, WCONT_T_DROPDOWN, "dropdown-sel-set-str")) {
    return;
  }

  dropdown = uiwidget->uiint.uidropdown;

  if (dropdown->strIndexMap == NULL) {
    internalidx = 0;
  } else {
    internalidx = slistGetNum (dropdown->strIndexMap, stridx);
    if (internalidx < 0) {
      internalidx = 0;
    }
  }
  uiDropDownSelectionSet (uiwidget, internalidx);
}

void
uiDropDownSetState (uiwcont_t *uiwidget, int state)
{
  uidropdown_t    *dropdown;

  if (! uiwcontValid (uiwidget, WCONT_T_DROPDOWN, "dropdown-set-state")) {
    return;
  }

  dropdown = uiwidget->uiint.uidropdown;
  uiWidgetSetState (dropdown->button, state);
}

char *
uiDropDownGetString (uiwcont_t *uiwidget)
{
  uidropdown_t    *dropdown;

  if (! uiwcontValid (uiwidget, WCONT_T_DROPDOWN, "dropdown-get-str")) {
    return NULL;
  }

  dropdown = uiwidget->uiint.uidropdown;
  return dropdown->strSelection;
}

/* internal routines */

static bool
uiDropDownWindowShow (void *udata)
{
  uidropdown_t  *dropdown = udata;
  int           x, y, ws;
  int           bx, by;


  if (dropdown == NULL) {
    return UICB_STOP;
  }

  bx = 0;
  by = 0;
  uiWindowGetPosition (dropdown->parentwin, &x, &y, &ws);
  if (dropdown->button != NULL) {
    uiWidgetGetPosition (dropdown->button, &bx, &by);
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
  uiwcont_t     *uiwidget = udata;
  uidropdown_t  *dropdown;

  if (uiwidget == NULL) {
    return UICB_CONT;
  }

  dropdown = uiwidget->uiint.uidropdown;

  if (dropdown->open) {
    uiWidgetHide (dropdown->window);
    dropdown->open = false;
  }
  uiWindowPresent (dropdown->parentwin);

  return UICB_CONT;
}

static void
uiDropDownButtonCreate (uiwcont_t *uiwidget)
{
  uidropdown_t    *dropdown;

  dropdown = uiwidget->uiint.uidropdown;

  dropdown->buttoncb = callbackInit (uiDropDownWindowShow, dropdown, NULL);
  dropdown->button = uiCreateButton (dropdown->buttoncb, NULL,
      "button_down_small");
  uiButtonAlignLeft (dropdown->button);
  uiButtonSetImagePosRight (dropdown->button);
  uiWidgetSetMarginTop (dropdown->button, 1);
  uiWidgetSetMarginStart (dropdown->button, 1);
}


static void
uiDropDownWindowCreate (uiwcont_t *uiwidget,
    callback_t *uicb, void *udata)
{
  uidropdown_t      *dropdown;
  uiwcont_t         *vbox = NULL;
  uiwcont_t         *mainvbox = NULL;
  uiwcont_t         *uiscwin;
  GtkCellRenderer   *renderer = NULL;
  GtkTreeViewColumn *column = NULL;

  dropdown = uiwidget->uiint.uidropdown;

  dropdown->closecb = callbackInit (uiDropDownClose, uiwidget, NULL);
  dropdown->window = uiCreateDialogWindow (dropdown->parentwin,
      dropdown->button, dropdown->closecb, "");

  mainvbox = uiCreateVertBox ();
  uiWidgetExpandHoriz (mainvbox);
  uiWidgetExpandVert (mainvbox);
  uiWindowPackInWindow (dropdown->window, mainvbox);

  vbox = uiCreateVertBox ();
  uiBoxPackStartExpand (mainvbox, vbox);

  uiscwin = uiCreateScrolledWindow (300);
  uiWidgetExpandHoriz (uiscwin);
  uiBoxPackStartExpand (vbox, uiscwin);

  dropdown->uitree = uiCreateTreeView ();
  if (G_IS_OBJECT (dropdown->uitree->uidata.widget)) {
    g_object_ref_sink (G_OBJECT (dropdown->uitree->uidata.widget));
  }
  uiTreeViewDisableHeaders (dropdown->uitree);
  uiTreeViewSelectSetMode (dropdown->uitree, SELECT_SINGLE);
  uiWidgetExpandHoriz (dropdown->uitree);
  uiWidgetExpandVert (dropdown->uitree);
  uiWindowPackInWindow (uiscwin, dropdown->uitree);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("",
      renderer, "text", UIUTILS_DROPDOWN_COL_DISP, NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  gtk_tree_view_append_column (GTK_TREE_VIEW (dropdown->uitree->uidata.widget), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("",
      renderer, "text", UIUTILS_DROPDOWN_COL_SB_PAD, NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
  gtk_tree_view_append_column (GTK_TREE_VIEW (dropdown->uitree->uidata.widget), column);

  if (uicb != NULL) {
    dropdown->selectcb = uicb;
    g_signal_connect (dropdown->uitree->uidata.widget, "row-activated",
        G_CALLBACK (uiDropDownSelectHandler), uiwidget);
  }

  uiwcontBaseFree (mainvbox);
  uiwcontBaseFree (vbox);
  uiwcontBaseFree (uiscwin);
}

static void
uiDropDownSelectionSet (uiwcont_t *uiwidget, nlistidx_t internalidx)
{
  uidropdown_t  *dropdown;
  GtkTreePath   *path = NULL;
  GtkTreeModel  *model = NULL;
  GtkTreeIter   iter;
  char          tbuff [200];
  char          *p;

  if (uiwidget == NULL) {
    return;
  }

  dropdown = uiwidget->uiint.uidropdown;

  if (dropdown->uitree == NULL) {
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
    model = gtk_tree_view_get_model (GTK_TREE_VIEW (dropdown->uitree->uidata.widget));
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
  uiwcont_t     *uiwidget = udata;
  uidropdown_t  *dropdown;
  long          idx;

  idx = uiDropDownSelectionGet (uiwidget, path);
  dropdown = uiwidget->uiint.uidropdown;
  callbackHandlerLong (dropdown->selectcb, idx);
}

static nlistidx_t
uiDropDownSelectionGet (uiwcont_t *uiwidget, GtkTreePath *path)
{
  uidropdown_t  *dropdown;
  GtkTreeIter   iter;
  GtkTreeModel  *model = NULL;
  glong         idx = 0;
  nlistidx_t    retval;
  char          tbuff [200];

  dropdown = uiwidget->uiint.uidropdown;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (dropdown->uitree->uidata.widget));
  if (gtk_tree_model_get_iter (model, &iter, path)) {
    gtk_tree_model_get (model, &iter, UIUTILS_DROPDOWN_COL_IDX, &idx, -1);
    retval = idx;
    if (dropdown->strSelection != NULL) {
      mdextfree (dropdown->strSelection);        // allocated by gtk
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

