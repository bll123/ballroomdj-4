/*
 * Copyright 2023-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "tmutil.h"
#include "uigeneral.h"

#include "ui/uidialog.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef enum {
  WCONT_T_ADJUSTMENT,       // gtk widget
  WCONT_T_BOX,              // base type
  WCONT_T_BUTTON,           /* base type for color-button, font-button */
  WCONT_T_BUTTON_CHKBOX,
  WCONT_T_BUTTON_COLOR,
  WCONT_T_BUTTON_FONT,
  WCONT_T_BUTTON_RADIO,
  WCONT_T_BUTTON_TOGGLE,    /* base type for check-box, radio-button */
  WCONT_T_CHGIND,
  WCONT_T_DIALOG_WINDOW,
  WCONT_T_ENTRY,
  WCONT_T_EVENT_BOX,
  WCONT_T_HBOX,
  WCONT_T_IMAGE,
  WCONT_T_KEY,
  WCONT_T_LABEL,
  WCONT_T_LINK,
  WCONT_T_MENU,          /* base type for menu, menubar, menu-sub */
  WCONT_T_MENUBAR,
  WCONT_T_MENU_CHECK_BOX,
  WCONT_T_MENU_ITEM,     /* base type for menubar-item */
  WCONT_T_MENU_MENUBAR_ITEM,
  WCONT_T_MENU_SUB,
  WCONT_T_NOTEBOOK,
  WCONT_T_PANED_WINDOW,
  WCONT_T_PROGRESS_BAR,
  WCONT_T_SCALE,
  WCONT_T_SCROLLBAR,
  WCONT_T_WINDOW_SCROLL,
  WCONT_T_SEPARATOR,
  WCONT_T_SIZE_GROUP,
  WCONT_T_SPINBOX,          /* base type for all spinbox types */
  WCONT_T_SPINBOX_DOUBLE,
  WCONT_T_SPINBOX_DOUBLE_DFLT,
  WCONT_T_SPINBOX_NUM,
  WCONT_T_SPINBOX_TEXT,
  WCONT_T_SPINBOX_TIME,
  WCONT_T_SWITCH,
  WCONT_T_TEXT_BOX,
  WCONT_T_TEXT_BUFFER,      // gtk widget
  WCONT_T_UNKNOWN,
  WCONT_T_VBOX,
  WCONT_T_WINDOW,   // base type for scroll-window, dialog-window, paned-window
  WCONT_T_WINDOW_MAIN,
  WCONT_T_MAX,
} uiwconttype_t;

enum {
  /* used so that the validity check will work (keys) */
  WCONT_EMPTY_WIDGET = 0x0001,
};

/* used in all ui interfaces */
typedef struct uibuttonbase {
  callback_t      *cb;
  callback_t      *presscb;
  callback_t      *releasecb;
  mstime_t        repeatTimer;
  int             repeatMS;
  uibuttonstate_t state;
  bool            repeatOn : 1;
  bool            repeating : 1;
} uibuttonbase_t;

/* used in all ui interfaces */
typedef struct uientrybase {
  const char      *label;
  void            *udata;
  uientryval_t    validateFunc;
  mstime_t        validateTimer;
  int             entrySize;
  int             maxSize;
  bool            valdelay : 1;
  bool            valid : 1;
} uientrybase_t;

typedef struct uibox uibox_t;
typedef struct uibutton uibutton_t;
typedef struct uientry uientry_t;
typedef struct uievent uievent_t;
typedef struct uiimage uiimage_t;
typedef struct uimenu uimenu_t;
typedef struct uiscrollbar uiscrollbar_t;
typedef struct uispinbox uispinbox_t;
typedef struct uiswitch uiswitch_t;
typedef struct uitextbox uitextbox_t;
typedef struct uitoggle uitoggle_t;
typedef struct uivirtlist uivirtlist_t;
typedef struct uibox uibox_t;
typedef struct uiwindow uiwindow_t;

/* data internal to each widget type */
typedef union {
    void          *voidwidget;
    uibox_t       *uibox;
    struct {
      uibuttonbase_t  uibuttonbase;
      uibutton_t      *uibutton;
    };
    struct {
      uientrybase_t   uientrybase;
      uientry_t       *uientry;
    };
    uievent_t     *uievent;
    uiimage_t     *uiimage;
    uimenu_t      *uimenu;
    uiscrollbar_t *uiscrollbar;
    uispinbox_t   *uispinbox;
    uiswitch_t    *uiswitch;
    uitextbox_t   *uitextbox;
    uitoggle_t    *uitoggle;
    uivirtlist_t  *uivirtlist;
    uiwindow_t    *uiwindow;
} uiwcontint_t;

# if BDJ4_UI_GTK3 /* gtk3 */
#  include "ui/uigtk3-wcont.h"

/* uigeneric.c */
GtkWidget * uiImageWidget (const char *imagenm);

# endif /* BDJ4_UI_GTK3 */

# if BDJ4_UI_NULL
#  include "ui/uinull-wcont.h"
# endif /* BDJ4_UI_NULL */

# if BDJ4_UI_MACOS
#  include "ui/uimacos-wcont.h"
# endif /* BDJ4_UI_MACOS */

/* general widget container */
typedef struct uiwcont {
  uiwconttype_t   wbasetype;
  uiwconttype_t   wtype;
  /* data internal to each widget type */
  uiwcontint_t    uiint;
  /* widget data specific to the interface */
  /* uidata must contain .widget and .packwidget */
  uispecific_t    uidata;
  bool            packed;
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
