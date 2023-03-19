#ifndef INC_UIINTERNAL_H
#define INC_UIINTERNAL_H

/* gtk3 */

#if BDJ4_USE_GTK

#include <gtk/gtk.h>

typedef union uiwidget {
  GtkWidget         *widget;
  GtkSizeGroup      *sg;
  GdkPixbuf         *pixbuf;
  GtkTreeSelection  *sel;
  GtkTextBuffer     *buffer;
  GtkAdjustment     *adjustment;
} uiwcont_t;

#endif /* BDJ4_USE_GTK */

#endif /* INC_UIINTERNAL_H */
