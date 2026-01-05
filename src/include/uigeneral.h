/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#define MACOS_UI_DEBUG 1

#if BDJ4_UI_GTK3
# include "ui-gtk3.h"
#endif
#if BDJ4_UI_NULL
# include "ui-null.h"
#endif
#if BDJ4_UI_MACOS
# include "ui-macos.h"
#endif
#if BDJ4_UI_NCURSES
# include "ui-ncurses.h"
#endif

#include "uiwcont.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

/* uibutton.c */
bool uiButtonCheckRepeat (uiwcont_t *uiwidget);
bool uiButtonPressCallback (void *udata);
bool uiButtonReleaseCallback (void *udata);

/* uilabel.c */
uiwcont_t * uiCreateColonLabel (const char *txt);

/* uientry.c */

enum {
  UIENTRY_VAL_TIMER = 400,
};

typedef int (*uientryval_t)(uiwcont_t *uiwidget, const char *label, void *udata);

void  uiEntrySetValidate (uiwcont_t *uiwidget, const char *label, uientryval_t valfunc, void *udata, int valdelay);
int   uiEntryValidate (uiwcont_t *uiwidget, bool forceflag);
void  uiEntryValidateClear (uiwcont_t *uiwidget);
int   uiEntryValidateDir (uiwcont_t *uiwidget, const char *label, void *udata);
int   uiEntryValidateFile (uiwcont_t *uiwidget, const char *label, void *udata);
bool  uiEntryIsNotValid (uiwcont_t *uiwidget);

/* uientryval.c */
void  uiEntryValidateHandler (uiwcont_t *uiwidget);

/* uifont.c */
void uiFontInfo (const char *font, char *buff, size_t sz, int *fontsz);

/* uicolor.c */
void uiConvertColor (const char *color, double *r, double *g, double *b);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

