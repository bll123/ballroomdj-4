/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "callback.h"
#include "uiwcont.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

/* uibox.c */

typedef enum {
  WCONT_FREE,
  WCONT_KEEP,
} uiwcontrls_t;

uiwcont_t * uiCreateVertBox (void);
uiwcont_t * uiCreateHorizBox (void);
void uiBoxPostProcess (uiwcont_t *uiwidget);
void uiBoxFree (uiwcont_t *uibox);
void uiBoxPackStart (uiwcont_t *uibox, uiwcont_t *uiwidget, uiwcontrls_t release);
void uiBoxPackEnd (uiwcont_t *uibox, uiwcont_t *uiwidget, uiwcontrls_t release);
void uiBoxPackStartExpandChildren (uiwcont_t *uibox, uiwcont_t *uiwidget, uiwcontrls_t release);
void uiBoxPackEndExpandChildren (uiwcont_t *uibox, uiwcont_t *uiwidget, uiwcontrls_t release);

/* uibutton.c */
uiwcont_t *uiCreateButton (callback_t *uicb, const char *title, const char *imagenm, const char *tooltiptxt);
void uiButtonFree (uiwcont_t *uiwidget);
bool uiButtonCheckRepeat (uiwcont_t *uiwidget);
bool uiButtonPressCallback (void *udata);
bool uiButtonReleaseCallback (void *udata);
uibuttonstate_t uiButtonGetState (uiwcont_t *uiwidget);

/* uigrid.c */
uiwcont_t *uiCreateGrid (void);
void uiGridFree (uiwcont_t *uigrid);
void uiGridPack (uiwcont_t *uigrid, uiwcont_t *uiwidget, uiwcontrls_t release);
void uiGridPackColumn (uiwcont_t *uigrid, uiwcont_t *uiwidget, int numcols, uiwcontrls_t release);
void uiGridPackRow (uiwcont_t *uigrid, uiwcont_t *uiwidget, int numcols, uiwcontrls_t release);
void uiGridPostProcess (uiwcont_t *uigrid);
void uiGridNewRow (uiwcont_t *uigrid);
void uiGridNewColumn (uiwcont_t *uigrid);
void uiGridSkipColumn (uiwcont_t *uigrid, int numcols);

/* uilabel.c */
uiwcont_t * uiCreateColonLabel (const char *txt);

/* uientry.c */

enum {
  UIENTRY_VAL_TIMER = 400,
};

uiwcont_t *uiEntryInit (int entrySize, int maxSize);
void  uiEntryFree (uiwcont_t *uientry);
void  uiEntrySetValidate (uiwcont_t *uiwidget, const char *label, uientryval_t valfunc, void *udata, int valdelay);
int   uiEntryValidate (uiwcont_t *uiwidget, bool forceflag);
void  uiEntryValidateClear (uiwcont_t *uiwidget);
int   uiEntryValidateDir (uiwcont_t *uiwidget, const char *label, void *udata);
int   uiEntryValidateFile (uiwcont_t *uiwidget, const char *label, void *udata);
bool  uiEntryIsNotValid (uiwcont_t *uiwidget);

/* uifont.c */
void uiFontInfo (const char *font, char *buff, size_t sz, int *fontsz);

/* uicolor.c */
void uiConvertColor (const char *color, double *r, double *g, double *b);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

