/*
 * Copyright 2023-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIWCONT_INT_H
#define INC_UIWCONT_INT_H

#include "tmutil.h"

#include "ui/uidialog.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

/* partially in use. */
/* The widget container will be more generic and */
/* hold uibutton_t, etc. */
typedef enum {
  WCONT_T_ADJUSTMENT,       // gtk widget
  WCONT_T_BOX,              // base type
  WCONT_T_VBOX,
  WCONT_T_HBOX,
  /* base type for color-button, font-button */
  WCONT_T_BUTTON,
  WCONT_T_CHECK_BOX,
  WCONT_T_CHGIND,
  WCONT_T_COLOR_BUTTON,
  WCONT_T_DIALOG_WINDOW,
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
  WCONT_T_EVENT_BOX,
  WCONT_T_UNKNOWN,
  /* base type for scroll-window, dialog-window, paned-window */
  WCONT_T_WINDOW,
  WCONT_T_MAX,
} uiwconttype_t;

enum {
  /* used so that the validity check will work (keys) */
  WCONT_EMPTY_WIDGET = 0x0001,
};

/* used in all ui interfaces */
typedef struct uibuttonbase {
  callback_t  *cb;
  callback_t  *presscb;
  callback_t  *releasecb;
  mstime_t    repeatTimer;
  int         repeatMS;
  bool        repeatOn : 1;
  bool        repeating : 1;
} uibuttonbase_t;

typedef struct uibutton uibutton_t;
typedef struct uientry uientry_t;
typedef struct uievent uievent_t;
typedef struct uimenu uimenu_t;
typedef struct uiscrollbar uiscrollbar_t;
typedef struct uispinbox uispinbox_t;
typedef struct uiswitch uiswitch_t;
typedef struct uitextbox uitextbox_t;
typedef struct uivirtlist uivirtlist_t;
typedef struct uibox uibox_t;

typedef union {
    void          *voidwidget;
    struct {
      uibuttonbase_t  uibuttonbase;
      uibutton_t      *uibutton;
    };
    uientry_t     *uientry;
    uievent_t     *uievent;
    uimenu_t      *uimenu;
    uiscrollbar_t *uiscrollbar;
    uispinbox_t   *uispinbox;
    uiswitch_t    *uiswitch;
    uitextbox_t   *uitextbox;
    uivirtlist_t  *uivirtlist;
} uiwcontint_t;

# if BDJ4_UI_GTK3 /* gtk3 */
#  include "ui/uigtk3-wcont.h"
# endif /* BDJ4_UI_GTK3 */

# if BDJ4_UI_NULL
#  include "ui/uinull-wcont.h"
# endif /* BDJ4_UI_NULL */

# if BDJ4_UI_MACOS
#  include "ui/uimacos-wcont.h"
# endif /* BDJ4_UI_MACOS */

typedef struct uiwcont {
  uiwconttype_t   wbasetype;
  uiwconttype_t   wtype;
  uiwcontint_t    uiint;
  uispecific_t    uidata;
  bool            packed : 1;
} uiwcont_t;

static inline bool
uiwcontValid (uiwcont_t *uiwidget, int exptype, const char *tag)
{
  if (uiwidget == NULL) {
    return false;
  }
  if (uiwidget->uidata.widget == NULL) {
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

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_UIWCONT_INT_H */
