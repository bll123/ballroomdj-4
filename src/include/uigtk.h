/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIGTK_H
#define INC_UIGTK_H

#include <gtk/gtk.h>

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
