#ifndef INC_UIWCONT_INT_H
#define INC_UIWCONT_INT_H

#include "ui/uibutton.h"
#include "ui/uidialog.h"
#include "ui/uidropdown.h"
#include "ui/uientry.h"
#include "ui/uispinbox.h"
#include "ui/uitreeview.h"

/* partially in use. */
/* The widget container will be more generic and */
/* hold uitree_t, uibutton_t, etc. */
typedef enum {
  WCONT_T_ADJUSTMENT,       // gtk widget
  WCONT_T_BOX,
  /* base type for color-button, font-button */
  WCONT_T_BUTTON,
  WCONT_T_CHECK_BOX,
  WCONT_T_CHGIND,
  WCONT_T_COLOR_BUTTON,
  WCONT_T_DIALOG_WINDOW,
  WCONT_T_DROPDOWN,
  WCONT_T_ENTRY,
  WCONT_T_FONT_BUTTON,
  WCONT_T_IMAGE,
  WCONT_T_KEY,
  WCONT_T_LABEL,
  WCONT_T_LINK,
  /* base type for menubar, menu-dropdown, menu-sub */
  WCONT_T_MENU,
  WCONT_T_MENUBAR,
  WCONT_T_MENU_CHECK_BOX,
  /* base type for menubar-item */
  WCONT_T_MENU_ITEM,
  WCONT_T_MENU_MENUBAR_ITEM,
  WCONT_T_MENU_SUB,
  WCONT_T_NOTEBOOK,
  WCONT_T_PANED_WINDOW,
  WCONT_T_PIXBUF,           // gtk widget
  WCONT_T_PROGRESS_BAR,
  WCONT_T_RADIO_BUTTON,
  WCONT_T_SCALE,
  WCONT_T_SCROLLBAR,
  WCONT_T_SCROLL_WINDOW,
  WCONT_T_SELECT,
  WCONT_T_SEPARATOR,
  WCONT_T_SIZE_GROUP,       // gtk widget
  /* base type for all spinbox types */
  WCONT_T_SPINBOX,
  WCONT_T_SPINBOX_DOUBLE,
  WCONT_T_SPINBOX_DOUBLE_DFLT,
  WCONT_T_SPINBOX_NUM,
  WCONT_T_SPINBOX_TEXT,
  WCONT_T_SPINBOX_TIME,
  WCONT_T_SWITCH,
  WCONT_T_TEXT_BOX,
  WCONT_T_TEXT_BUFFER,      // gtk widget
  /* base type for check-box, radio-button */
  WCONT_T_TOGGLE_BUTTON,
  WCONT_T_TREE,
  WCONT_T_UNKNOWN,
  /* base type for scroll-window, dialog-window, paned-window */
  WCONT_T_WINDOW,
  WCONT_T_MAX,
} uiwconttype_t;

enum {
  /* used so that the validity check will work (keys) */
  WCONT_EMPTY_WIDGET = 0x0001,
};

typedef struct uikey uikey_t;
typedef struct uimenu uimenu_t;
typedef struct uiscrollbar uiscrollbar_t;
typedef struct uiswitch uiswitch_t;
typedef struct uitextbox uitextbox_t;

/* for future use */
/* the widget pointer will be moved out of the below structure */
/* and into the uiwcont_t union */
/* done: 2023-12 scrollbar, chgind */
/* done: 2023-12-15: switch */
typedef union {
    void          *voidwidget;
    uibutton_t    *uibutton;
    uidropdown_t  *uidropdown;
    uientry_t     *uientry;
    uikey_t       *uikey;
    uimenu_t      *uimenu;
    uiscrollbar_t *uiscrollbar;
    uiselect_t    *uiselect;
    uispinbox_t   *uispinbox;
    uiswitch_t    *uiswitch;
    uitextbox_t   *uitextbox;
    uitree_t      *uitree;
} uiwcontint_t;

/* UISPECIFIC is defined as the contents of the anonymous union. */
/* UISPECIFIC must contain a value named 'widget' */

# if BDJ4_USE_GTK3 /* gtk3 */

#  include <gtk/gtk.h>

#define UISPECIFIC \
  GtkWidget     *widget; \
  GtkSizeGroup  *sg; \
  GdkPixbuf     *pixbuf; \
  GtkTextBuffer *buffer; \
  GtkAdjustment *adjustment;

# endif /* BDJ4_USE_GTK3 */

# if BDJ4_USE_NULLUI

#define UISPECIFIC \
  void      *widget;

# endif /* BDJ4_USE_NULLUI */

typedef struct uiwcont {
  uiwconttype_t   wbasetype;
  uiwconttype_t   wtype;
  uiwcontint_t    uiint;
  union {
    UISPECIFIC
  };
} uiwcont_t;

static inline bool
uiwcontValid (uiwcont_t *uiwidget, int exptype, const char *tag)
{
  if (uiwidget == NULL || uiwidget->widget == NULL) {
    return false;
  }
  if ((int) uiwidget->wbasetype != exptype &&
     (int) uiwidget->wtype != exptype) {
    fprintf (stderr, "ERR: %s incorrect type exp:%d/%s actual:%d/%s %d/%s\n",
        tag,
        exptype, uiwcontDesc (exptype),
        uiwidget->wbasetype, uiwcontDesc (uiwidget->wbasetype),
        uiwidget->wtype, uiwcontDesc (uiwidget->wtype));
    return false;
  }

  return true;
}

#endif /* INC_UIWCONT_INT_H */
