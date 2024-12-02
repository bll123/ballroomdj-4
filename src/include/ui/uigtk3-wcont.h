/*
 * Copyright 2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIGTK3_WCONT_H
#define INC_UIGTK3_WCONT_H

#include <gtk/gtk.h>

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

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

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_UIGTK3_WCONT_H */
