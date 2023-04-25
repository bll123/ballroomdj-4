/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIUTILS_H
#define INC_UIUTILS_H

#include "ui.h"
#include "uiwcont.h"

typedef struct {
  uiwcont_t   *hbox;
  uiwcont_t   *label;
} uiutilsaccent_t;

void uiutilsAddAccentColorDisplay (uiwcont_t *vbox, uiutilsaccent_t *accent);
void uiutilsSetAccentColor (uiwcont_t *uiwidgetp);
const char * uiutilsGetCurrentFont (void);
int uiutilsValidateSongListName (uientry_t *entry, void *udata);
void uiutilsProgressStatus (uiwcont_t *statusMsg, int count, int tot);

#endif /* INC_UIUTILS_H */
