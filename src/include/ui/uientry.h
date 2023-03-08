/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIENTRY_H
#define INC_UIENTRY_H

#if BDJ4_USE_GTK
# include "ui-gtk3.h"
#endif

typedef struct uientry uientry_t;
typedef int (*uientryval_t)(uientry_t *entry, void *udata);

enum {
  UIENTRY_IMMEDIATE,
  UIENTRY_DELAYED,
};

enum {
  UIENTRY_RESET,
  UIENTRY_ERROR,
  UIENTRY_OK,
};

uientry_t *uiEntryInit (int entrySize, int maxSize);
void uiEntryFree (uientry_t *entry);
void uiEntryCreate (uientry_t *entry);
void uiEntrySetIcon (uientry_t *entry, const char *name);
void uiEntryClearIcon (uientry_t *entry);
UIWidget * uiEntryGetUIWidget (uientry_t *entry);
void uiEntryPeerBuffer (uientry_t *targetentry, uientry_t *sourceentry);
const char * uiEntryGetValue (uientry_t *entry);
void uiEntrySetValue (uientry_t *entry, const char *value);
void uiEntrySetValidate (uientry_t *entry,
    uientryval_t valfunc, void *udata, int valdelay);
int uiEntryValidate (uientry_t *entry, bool forceflag);
int uiEntryValidateDir (uientry_t *edata, void *udata);
int uiEntryValidateFile (uientry_t *edata, void *udata);
void uiEntrySetState (uientry_t *entry, int state);

#endif /* INC_UIENTRY_H */
