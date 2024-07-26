/*
 * Copyright 2023-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UISPINBOX_H
#define INC_UISPINBOX_H

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

#include <stdint.h>

#include "callback.h"
#include "nlist.h"
#include "slist.h"
#include "uiwcont.h"

enum {
  SB_TEXT,
  SB_TIME_BASIC,
  SB_TIME_PRECISE,
};

typedef const char * (*uispinboxdisp_t)(void *, int);

void  uiSpinboxFree (uiwcont_t *uiwidget);

uiwcont_t *uiSpinboxTextCreate (void *udata);
void  uiSpinboxTextSet (uiwcont_t *uiwidget, int min, int count,
    int maxWidth, slist_t *list, nlist_t *keylist, uispinboxdisp_t textGetProc);
int   uiSpinboxTextGetValue (uiwcont_t *uiwidget);
void  uiSpinboxTextSetValue (uiwcont_t *uiwidget, int ivalue);
void uiSpinboxTextSetValueChangedCallback (uiwcont_t *uiwidget, callback_t *uicb);

uiwcont_t *uiSpinboxIntCreate (void);
uiwcont_t *uiSpinboxDoubleCreate (void);

uiwcont_t *uiSpinboxDoubleDefaultCreate (void);

uiwcont_t *uiSpinboxTimeCreate (int sbtype, void *udata, const char *label, callback_t *convcb);
int32_t uiSpinboxTimeGetValue (uiwcont_t *uiwidget);
void  uiSpinboxTimeSetValue (uiwcont_t *uiwidget, int32_t value);
void uiSpinboxTimeSetValueChangedCallback (uiwcont_t *uiwidget, callback_t *uicb);

void uiSpinboxSetState (uiwcont_t *uiwidget, int state);
void uiSpinboxSetValueChangedCallback (uiwcont_t *uiwidget, callback_t *uicb);
void uiSpinboxSetFocusCallback (uiwcont_t *uiwidget, callback_t *uicb);
void uiSpinboxSetRange (uiwcont_t *uiwidget, double min, double max);
void uiSpinboxSetIncrement (uiwcont_t *uiwidget, double incr, double pageincr);
void uiSpinboxWrap (uiwcont_t *uiwidget);
void uiSpinboxSet (uiwcont_t *uispinbox, double min, double max);
double uiSpinboxGetValue (uiwcont_t *uiwidget);
void uiSpinboxSetValue (uiwcont_t *uiwidget, double ivalue);
bool uiSpinboxIsChanged (uiwcont_t *uiwidget);
void uiSpinboxResetChanged (uiwcont_t *uiwidget);
void uiSpinboxAddClass (const char *classnm, const char *color);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_UISPINBOX_H */
