/*
 * Copyright 2023-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "callback.h"
#include "uiwcont.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

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
void uiEntrySetInternalValidate (uiwcont_t *uiwidget);
void uiEntrySetFocusCallback (uiwcont_t *uiwidget, callback_t *uicb);
void uiEntrySetState (uiwcont_t *entry, int state);
bool  uiEntryChanged (uiwcont_t *uiwidget);
void  uiEntryClearChanged (uiwcont_t *uiwidget);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

