/*
 * Copyright 2023-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>

#include "callback.h"
#include "nlist.h"
#include "tmutil.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef struct uiwcont uiwcont_t;

typedef enum {
  WCONT_T_UNKNOWN,
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
  WCONT_T_GRID,
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
  WCONT_T_SWITCH,
  WCONT_T_TEXT_BOX,
  WCONT_T_TEXT_BUFFER,      // gtk widget
  WCONT_T_VBOX,
  WCONT_T_WINDOW,   // base type for scroll-window, dialog-window, paned-window
  WCONT_T_WINDOW_MAIN,
  WCONT_T_MAX,
} uiwconttype_t;

typedef enum {
  BUTTON_ON = true,
  BUTTON_OFF = false,
} uibuttonstate_t;

/* used in all ui interfaces */
typedef struct uiboxintgen {
  nlist_t         *widgetlist;
  nlist_t         *releaselist;
  int             startcount;
  int             endcount;
  bool            postprocess;
} uiboxgeneric_t;

/* used in all ui interfaces */
typedef struct uibuttonintgen {
  callback_t      *cb;
  callback_t      *presscb;
  callback_t      *releasecb;
  mstime_t        repeatTimer;
  int             repeatMS;
  uibuttonstate_t state;
  bool            repeatOn;
  bool            repeating;
} uibuttongeneric_t;

typedef int (*uientryval_t)(uiwcont_t *uiwidget, const char *label, void *udata);

/* used in all ui interfaces */
typedef struct uientryintgen {
  const char      *label;
  void            *udata;
  uientryval_t    validateFunc;
  mstime_t        validateTimer;
  int32_t         valdelaytime;
  bool            valdelay;
  bool            valid;
  bool            useicon;
} uientrygeneric_t;

/* used in all ui interfaces */
typedef struct uigridgen {
  nlist_t         *widgetlist;
  nlist_t         *releaselist;
  int             count;
  int             row;
  int             col;
  int             nrows;
  int             ncols;
  bool            postprocess;
  bool            new;
} uigridgeneric_t;

typedef struct uiboxint uiboxint_t;
typedef struct uibuttonint uibuttonint_t;
typedef struct uientryint uientryint_t;
typedef struct uieventint uieventint_t;
typedef struct uiimageint uiimageint_t;
typedef struct uimenuint uimenuint_t;
typedef struct uiscrollbarint uiscrollbarint_t;
typedef struct uispinboxint uispinboxint_t;
typedef struct uiswitchint uiswitchint_t;
typedef struct uitextboxint uitextboxint_t;
typedef struct uitoggleint uitoggleint_t;
typedef struct uiwindowint uiwindowint_t;

/* data internal to each widget type */
typedef union {
    void              *voidwidget;
    uiboxint_t        *uibox;
    uibuttonint_t     *uibutton;
    uientryint_t      *uientry;
    uieventint_t      *uievent;
    uiimageint_t      *uiimage;
    uimenuint_t       *uimenu;
    uiscrollbarint_t  *uiscrollbar;
    uispinboxint_t    *uispinbox;
    uiswitchint_t     *uiswitch;
    uitextboxint_t    *uitextbox;
    uitoggleint_t     *uitoggle;
    uiwindowint_t     *uiwindow;
} uiwcontint_t;

typedef struct uiwcontdata uiwcontdata_t;
typedef struct uiwcont {
  union {
    void              *voidgen;
    uiboxgeneric_t    *uiboxgen;
    uibuttongeneric_t *uibuttongen;
    uientrygeneric_t  *uientrygen;
    uigridgeneric_t   *uigridgen;
  };
  void            *packwidget;
  void            *widget;
  uiwcontdata_t   *uidata;
  /* data internal to each widget type */
  /* this is ui toolkit specific data */
  uiwcontint_t    uiint;
  uint32_t        ui_id;
  uint32_t        id;
  uint32_t        parentid;
  uiwconttype_t   wbasetype;
  uiwconttype_t   wtype;
  bool            packed;
} uiwcont_t;

/* general routines that are called by the ui specific code */

/* uiwcont.c */

#define uiwcontAlloc(bt,t) uiwcontAlloc_r (bt,t, __FILE__, __LINE__)

void uiwcontInitID (int32_t id);
void uiwcontPushID (int32_t id);
void uiwcontPopID (void);

uiwcont_t *uiwcontAlloc_r (int basetype, int type, const char *, int);
/* basefree only frees the uiwidget, not any internals */
void uiwcontBaseFree (uiwcont_t *uiwidget);
void uiwcontSetWidget (uiwcont_t *uiwidget, void *widget, void *packwidget);
void uiwcontSetID (uiwcont_t *parent, uiwcont_t *uiwidget, int count);

/* for debugging */
void uiwcontDebugDisp (uiwcont_t *uiwidget, const char *tag);
const char * uiwcontDesc (int wtype);

/* uiwcontfree.c */

/* uiwcontFree() will also call any widget-specific free procedures */
void  uiwcontFree (uiwcont_t *uiwidget);

static inline bool
uiwcontValid (uiwcont_t *uiwidget, int exptype, const char *tag)
{
  if (uiwidget == NULL) {
    return false;
  }
  if (uiwidget->widget == NULL) {
    return false;
  }
  if ((int) uiwidget->wbasetype != exptype &&
     (int) uiwidget->wtype != exptype) {
    fprintf (stderr, "ERR: %s (id:%" PRIu32 "-%" PRIu32 ") incorrect type exp:%d/%s actual:%d/%s %d/%s\n",
        tag, uiwidget->ui_id, uiwidget->id,
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

