/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <math.h>

#include <gtk/gtk.h>

#include "bdj4.h"
#include "bdjstring.h"
#include "ui.h"

enum {
  UIUTILS_DROPDOWN_COL_IDX,
  UIUTILS_DROPDOWN_COL_STR,
  UIUTILS_DROPDOWN_COL_DISP,
  UIUTILS_DROPDOWN_COL_SB_PAD,
  UIUTILS_DROPDOWN_COL_MAX,
};

typedef struct uidropdown {
  char          *title;
  UICallback    *selectcb;
  UIWidget      *parentwin;
  uibutton_t    *button;
  UICallback    buttoncb;
  UIWidget      window;
  UICallback    closecb;
  uitree_t      *uitree;
  GtkTreeSelection  *sel;
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
static void uiDropDownWindowCreate (uidropdown_t *dropdown, UICallback *uicb, void *udata);
static void uiDropDownSelectionSet (uidropdown_t *dropdown, nlistidx_t internalidx);
static void uiDropDownSelectHandler (GtkTreeView *tv, GtkTreePath *path, GtkTreeViewColumn *column, gpointer udata);
static nlistidx_t uiDropDownSelectionGet (uidropdown_t *dropdown, GtkTreePath *path);

uidropdown_t *
uiDropDownInit (void)
{
  uidropdown_t  *dropdown;

  dropdown = malloc (sizeof (uidropdown_t));

  dropdown->title = NULL;
  dropdown->parentwin = NULL;
  dropdown->button = NULL;
  uiutilsUIWidgetInit (&dropdown->window);
  dropdown->uitree = NULL;
  dropdown->sel = NULL;
  dropdown->closeHandlerId = 0;
  dropdown->strSelection = NULL;
  dropdown->strIndexMap = NULL;
  dropdown->keylist = NULL;
  dropdown->open = false;
  dropdown->iscombobox = false;
  dropdown->maxwidth = 10;

  return dropdown;
}


void
uiDropDownFree (uidropdown_t *dropdown)
{
  if (dropdown != NULL) {
    uiButtonFree (dropdown->button);
    dataFree (dropdown->title);
    dataFree (dropdown->strSelection);
    slistFree (dropdown->strIndexMap);
    nlistFree (dropdown->keylist);
    uiTreeViewFree (dropdown->uitree);
    free (dropdown);
  }
}

UIWidget *
uiDropDownCreate (UIWidget *parentwin,
    const char *title, UICallback *uicb,
    uidropdown_t *dropdown, void *udata)
{
  dropdown->parentwin = parentwin;
  dropdown->title = strdup (title);
  uiDropDownButtonCreate (dropdown);
  uiDropDownWindowCreate (dropdown, uicb, udata);
  return uiButtonGetUIWidget (dropdown->button);
}

UIWidget *
uiComboboxCreate (UIWidget *parentwin,
    const char *title, UICallback *uicb,
    uidropdown_t *dropdown, void *udata)
{
  dropdown->iscombobox = true;
  return uiDropDownCreate (parentwin, title, uicb, dropdown, udata);
}

void
uiDropDownSetList (uidropdown_t *dropdown, slist_t *list,
    const char *selectLabel)
{
  char              *strval;
  char              *dispval;
  GtkTreeIter       iter;
  GtkListStore      *store = NULL;
  GtkCellRenderer   *renderer = NULL;
  GtkTreeViewColumn *column = NULL;
  ilistidx_t        iteridx;
  nlistidx_t        internalidx;
  UIWidget          *uitreewidgetp;
  char              tbuff [200];


  store = gtk_list_store_new (UIUTILS_DROPDOWN_COL_MAX,
      G_TYPE_LONG, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
  assert (store != NULL);

  dropdown->strIndexMap = slistAlloc ("uiutils-str-index", LIST_ORDERED, NULL);
  internalidx = 0;

  dropdown->maxwidth = slistGetMaxKeyWidth (list);
  uitreewidgetp = uiTreeViewGetUIWidget (dropdown->uitree);

  if (! dropdown->iscombobox) {
    snprintf (tbuff, sizeof (tbuff), "%-*s",
        dropdown->maxwidth, dropdown->title);
    uiButtonSetText (dropdown->button, tbuff);
  }

  if (dropdown->iscombobox && selectLabel != NULL) {
    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter,
        UIUTILS_DROPDOWN_COL_IDX, (glong) -1,
        UIUTILS_DROPDOWN_COL_STR, "",
        UIUTILS_DROPDOWN_COL_DISP, selectLabel,
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

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("",
      renderer, "text", UIUTILS_DROPDOWN_COL_DISP, NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  gtk_tree_view_append_column (GTK_TREE_VIEW (uitreewidgetp->widget), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("",
      renderer, "text", UIUTILS_DROPDOWN_COL_SB_PAD, NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
  gtk_tree_view_append_column (GTK_TREE_VIEW (uitreewidgetp->widget), column);

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
  GtkCellRenderer   *renderer = NULL;
  GtkTreeViewColumn *column = NULL;
  char              tbuff [200];
  ilistidx_t        iteridx;
  nlistidx_t        internalidx;
  nlistidx_t        idx;
  UIWidget          *uitreewidgetp;

  store = gtk_list_store_new (UIUTILS_DROPDOWN_COL_MAX,
      G_TYPE_LONG, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
  assert (store != NULL);

  dropdown->keylist = nlistAlloc ("uiutils-keylist", LIST_ORDERED, NULL);
  internalidx = 0;

  dropdown->maxwidth = slistGetMaxKeyWidth (list);
  uitreewidgetp = uiTreeViewGetUIWidget (dropdown->uitree);

  if (! dropdown->iscombobox) {
    snprintf (tbuff, sizeof (tbuff), "%-*s",
        dropdown->maxwidth, dropdown->title);
    uiButtonSetText (dropdown->button, tbuff);
  }

  if (dropdown->iscombobox && selectLabel != NULL) {
    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter,
        UIUTILS_DROPDOWN_COL_IDX, (glong) -1,
        UIUTILS_DROPDOWN_COL_STR, "",
        UIUTILS_DROPDOWN_COL_DISP, selectLabel,
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

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("",
      renderer, "text", UIUTILS_DROPDOWN_COL_DISP, NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  gtk_tree_view_append_column (GTK_TREE_VIEW (uitreewidgetp->widget), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("",
      renderer, "text", UIUTILS_DROPDOWN_COL_SB_PAD, NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
  gtk_tree_view_append_column (GTK_TREE_VIEW (uitreewidgetp->widget), column);

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
uiDropDownDisable (uidropdown_t *dropdown)
{
  UIWidget  *uiwidgetp;

  if (dropdown == NULL) {
    return;
  }
  uiwidgetp = uiButtonGetUIWidget (dropdown->button);
  uiWidgetDisable (uiwidgetp);
}

void
uiDropDownEnable (uidropdown_t *dropdown)
{
  UIWidget  *uiwidgetp;

  if (dropdown == NULL) {
    return;
  }
  uiwidgetp = uiButtonGetUIWidget (dropdown->button);
  uiWidgetEnable (uiwidgetp);
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
  UIWidget      *uiwidgetp;
  int           x, y, ws;
  int           bx, by;


  if (dropdown == NULL) {
    return UICB_STOP;
  }

  bx = 0;
  by = 0;
  uiWindowGetPosition (dropdown->parentwin, &x, &y, &ws);
  uiwidgetp = uiButtonGetUIWidget (dropdown->button);
  if (uiutilsUIWidgetSet (uiwidgetp)) {
    uiWidgetGetPosition (uiwidgetp, &bx, &by);
  }
  uiWidgetShowAll (&dropdown->window);
  uiWindowMove (&dropdown->window, bx + x + 4, by + y + 4 + 30, -1);
  dropdown->open = true;
  return UICB_CONT;
}

static bool
uiDropDownClose (void *udata)
{
  uidropdown_t *dropdown = udata;

  if (dropdown->open) {
    uiWidgetHide (&dropdown->window);
    dropdown->open = false;
  }
  uiWindowPresent (dropdown->parentwin);

  return UICB_CONT;
}

static void
uiDropDownButtonCreate (uidropdown_t *dropdown)
{
  UIWidget    *uiwidgetp;

  uiutilsUICallbackInit (&dropdown->buttoncb, uiDropDownWindowShow, dropdown, NULL);
  dropdown->button = uiCreateButton (&dropdown->buttoncb, NULL,
      "button_down_small");
  uiButtonAlignLeft (dropdown->button);
  uiButtonSetImagePosRight (dropdown->button);
  uiwidgetp = uiButtonGetUIWidget (dropdown->button);
  uiWidgetSetMarginTop (uiwidgetp, 1);
  uiWidgetSetMarginStart (uiwidgetp, 1);
}


static void
uiDropDownWindowCreate (uidropdown_t *dropdown,
    UICallback *uicb, void *udata)
{
  UIWidget          uiwidget;
  UIWidget          *uiwidgetp;
  UIWidget          vbox;
  UIWidget          uiscwin;


  uiutilsUICallbackInit (&dropdown->closecb, uiDropDownClose, dropdown, NULL);
  uiCreateDialogWindow (&dropdown->window, dropdown->parentwin,
      uiButtonGetUIWidget (dropdown->button), &dropdown->closecb, "");

  uiCreateVertBox (&uiwidget);
  uiWidgetExpandHoriz (&uiwidget);
  uiWidgetExpandVert (&uiwidget);
  uiBoxPackInWindow (&dropdown->window, &uiwidget);

  uiCreateVertBox (&vbox);
  uiBoxPackStartExpand (&uiwidget, &vbox);

  uiCreateScrolledWindow (&uiscwin, 300);
  uiWidgetExpandHoriz (&uiscwin);
  uiBoxPackStartExpand (&vbox, &uiscwin);

  dropdown->uitree = uiCreateTreeView ();
  uiwidgetp = uiTreeViewGetUIWidget (dropdown->uitree);
  if (G_IS_OBJECT (uiwidgetp->widget)) {
    g_object_ref_sink (G_OBJECT (uiwidgetp->widget));
  }
  dropdown->sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (uiwidgetp->widget));
  uiTreeViewDisableHeaders (dropdown->uitree);
  gtk_tree_selection_set_mode (dropdown->sel, GTK_SELECTION_SINGLE);
  uiWidgetExpandHoriz (uiwidgetp);
  uiWidgetExpandVert (uiwidgetp);
  uiBoxPackInWindow (&uiscwin, uiwidgetp);
  if (uicb != NULL) {
    dropdown->selectcb = uicb;
    g_signal_connect (uiwidgetp->widget, "row-activated",
        G_CALLBACK (uiDropDownSelectHandler), dropdown);
  }
}

static void
uiDropDownSelectionSet (uidropdown_t *dropdown, nlistidx_t internalidx)
{
  GtkTreePath   *path;
  GtkTreeModel  *model;
  GtkTreeIter   iter;
  char          tbuff [200];
  char          *p;
  UIWidget      *uitreewidgetp;


  if (dropdown == NULL || dropdown->uitree == NULL) {
    return;
  }
  uitreewidgetp = uiTreeViewGetUIWidget (dropdown->uitree);
  if (uitreewidgetp->widget == NULL) {
    return;
  }

  if (internalidx < 0) {
    internalidx = 0;
  }
  snprintf (tbuff, sizeof (tbuff), "%d", internalidx);
  path = gtk_tree_path_new_from_string (tbuff);
  if (path != NULL) {
    gtk_tree_selection_select_path (dropdown->sel, path);
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
  uiutilsCallbackLongHandler (dropdown->selectcb, idx);
}

static nlistidx_t
uiDropDownSelectionGet (uidropdown_t *dropdown, GtkTreePath *path)
{
  GtkTreeIter   iter;
  GtkTreeModel  *model = NULL;
  glong         idx = 0;
  nlistidx_t    retval;
  char          tbuff [200];
  UIWidget      *uitreewidgetp;

  uitreewidgetp = uiTreeViewGetUIWidget (dropdown->uitree);

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (uitreewidgetp->widget));
  if (gtk_tree_model_get_iter (model, &iter, path)) {
    gtk_tree_model_get (model, &iter, UIUTILS_DROPDOWN_COL_IDX, &idx, -1);
    retval = idx;
    dataFree (dropdown->strSelection);
    gtk_tree_model_get (model, &iter, UIUTILS_DROPDOWN_COL_STR,
        &dropdown->strSelection, -1);
    if (dropdown->iscombobox) {
      char  *p;

      gtk_tree_model_get (model, &iter, UIUTILS_DROPDOWN_COL_DISP, &p, -1);
      snprintf (tbuff, sizeof (tbuff), "%-*s",
          dropdown->maxwidth, p);
      uiButtonSetText (dropdown->button, tbuff);
      free (p);
    }
    uiWidgetHide (&dropdown->window);
    dropdown->open = false;
  } else {
    return -1;
  }

  return retval;
}

