/*
 * Copyright 2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIGTK3_INT_H
#define INC_UIGTK3_INT_H

#include <gtk/gtk.h>

typedef struct uispecific {
  GtkWidget     *packwidget;
  union {
    GtkWidget     *widget;
    GtkSizeGroup  *sg;
    GdkPixbuf     *pixbuf;
    GtkTextBuffer *buffer;
    GtkAdjustment *adjustment;
  };
} uispecific_t;

#endif /* INC_UIGTK3_INT_H */
