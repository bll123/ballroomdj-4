/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIUTILS_H
#define INC_UIUTILS_H

#include "ui.h"

typedef struct {
  uiwcont_t   *hbox;
  uiwcont_t   *label;
} uiutilsaccent_t;

void uiutilsAddProfileColorDisplay (uiwcont_t *vbox, uiutilsaccent_t *accent);
void uiutilsSetProfileColor (uiwcont_t *uiwidgetp);
const char * uiutilsGetCurrentFont (void);
int uiutilsValidatePlaylistName (uiwcont_t *entry, void *udata);
void uiutilsProgressStatus (uiwcont_t *statusMsg, int count, int tot);
void uiutilsNewFontSize (char *buff, size_t sz, const char *font, const char *style, int newsz);

#endif /* INC_UIUTILS_H */
