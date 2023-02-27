/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIGTK_H
#define INC_UIGTK_H

#include <gtk/gtk.h>

/* tree view storage types */
enum {
  TREE_TYPE_STRING = G_TYPE_STRING,
  TREE_TYPE_NUM = G_TYPE_LONG,
  TREE_TYPE_IMAGE = G_TYPE_OBJECT,
  TREE_TYPE_BOOLEAN = G_TYPE_BOOLEAN,
  TREE_TYPE_ELLIPSIZE = G_TYPE_INT,
  TREE_COL_DISP_NORM = GTK_TREE_VIEW_COLUMN_AUTOSIZE,
  TREE_COL_DISP_GROW = GTK_TREE_VIEW_COLUMN_GROW_ONLY,
};

#define TREE_COL_MODE_TEXT "text"

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
  };
} UIWidget;

#endif /* INC_UIGTK_H */
