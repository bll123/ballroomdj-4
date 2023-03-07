/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIGTK_H
#define INC_UIGTK_H

#include <gtk/gtk.h>

typedef glong treenum_t;

/* tree view storage types */
enum {
  TREE_TYPE_STRING = G_TYPE_STRING,
  TREE_TYPE_NUM = G_TYPE_LONG,
  TREE_TYPE_WIDGET = G_TYPE_OBJECT,
  TREE_TYPE_BOOLEAN = G_TYPE_BOOLEAN,
  TREE_TYPE_ELLIPSIZE = G_TYPE_INT,
  TREE_TYPE_IMAGE = -5,               // will be set to GDK_TYPE_PIXBUF
  TREE_TYPE_END = -1,
  TREE_COL_DISP_NORM = GTK_TREE_VIEW_COLUMN_AUTOSIZE,
  TREE_COL_DISP_GROW = GTK_TREE_VIEW_COLUMN_GROW_ONLY,
  TREE_ELLIPSIZE_END = PANGO_ELLIPSIZE_END,
};

/* these are defined based on the gtk values */
/* would change for a different gui package */
enum {
  UICB_STOP = true,
  UICB_CONT = false,
  UICB_DISPLAYED = true,
  UICB_NO_DISP = false,
  UICB_NO_CONV = false,
  UICB_CONVERTED = true,
  SELECT_SINGLE = GTK_SELECTION_SINGLE,
  SELECT_MULTIPLE = GTK_SELECTION_MULTIPLE,
  UICB_SUPPORTED = true,
  UICB_NOT_SUPPORTED = false,
};

typedef struct {
  union {
    GtkWidget         *widget;
    GtkSizeGroup      *sg;
    GdkPixbuf         *pixbuf;
    GtkTreeSelection  *sel;
    GtkTextBuffer     *buffer;
    GtkAdjustment     *adjustment;
  };
} UIWidget;

#endif /* INC_UIGTK_H */
