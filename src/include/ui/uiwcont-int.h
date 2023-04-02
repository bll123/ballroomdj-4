#ifndef INC_UIWCONT_INT_H
#define INC_UIWCONT_INT_H

/* for future use.  the widget container may become more generic and */
/* hold uitree_t, uibutton_t, etc. */
typedef enum {
  WCONT_T_WIDGET,
  WCONT_T_SIZE_GROUP,
  WCONT_T_PIXBUF,
  WCONT_T_TREE_SELECT,
  WCONT_T_TEXT_BUFFER,
  WCONT_T_ADJUSTMENT,
} uiwconttype_t;

# if BDJ4_USE_GTK /* gtk3 */

#  include <gtk/gtk.h>

typedef struct uiwidget {
  uiwconttype_t       wtype;
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
  uiwconttype_t       wtype;
  union {
    void      *widget;
  };
} uiwcont_t;

# endif /* BDJ4_USE_NULLUI */

#endif /* INC_UIWCONT_INT_H */
