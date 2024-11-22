/*
 * Copyright 2023-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIENTRY_H
#define INC_UIENTRY_H

#include "callback.h"
#include "uiwcont.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef int (*uientryval_t)(uiwcont_t *uiwidget, const char *label, void *udata);

enum {
  UIENTRY_IMMEDIATE,
  UIENTRY_DELAYED,
};

enum {
  UIENTRY_RESET,
  UIENTRY_ERROR,
  UIENTRY_OK,
};

uiwcont_t *uiEntryInit (int entrySize, int maxSize);
void uiEntryFree (uiwcont_t *entry);
void uiEntrySetIcon (uiwcont_t *entry, const char *name);
void uiEntryClearIcon (uiwcont_t *entry);
const char * uiEntryGetValue (uiwcont_t *entry);
void uiEntrySetValue (uiwcont_t *entry, const char *value);
void uiEntrySetValidate (uiwcont_t *entry, const char *label, uientryval_t valfunc, void *udata, int valdelay);
int uiEntryValidate (uiwcont_t *entry, bool forceflag);
void uiEntryValidateClear (uiwcont_t *entry);
int uiEntryValidateDir (uiwcont_t *edata, const char *label, void *udata);
int uiEntryValidateFile (uiwcont_t *edata, const char *label, void *udata);
bool uiEntryIsNotValid (uiwcont_t *uiwidget);
void uiEntrySetFocusCallback (uiwcont_t *uiwidget, callback_t *uicb);
void uiEntrySetState (uiwcont_t *entry, int state);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_UIENTRY_H */
