#ifndef INC_UIWCONT_INT_H
#define INC_UIWCONT_INT_H

# if BDJ4_USE_GTK /* gtk3 */

#  include <gtk/gtk.h>

typedef struct uiwidget {
  union {
    GtkWidget         *widget;
    GtkSizeGroup      *sg;
    GdkPixbuf         *pixbuf;
    GtkTreeSelection  *sel;
    GtkTextBuffer     *buffer;
    GtkAdjustment     *adjustment;
  };
} uiwcont_t;

# endif /* BDJ4_USE_GTK */

# if BDJ4_USE_NULLUI

typedef struct uiwidget {
  union {
    void      *widget;
  };
} uiwcont_t;

# endif /* BDJ4_USE_NULLUI */

#endif /* INC_UIWCONT_INT_H */
