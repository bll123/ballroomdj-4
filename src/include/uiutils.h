/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIUTILS_H
#define INC_UIUTILS_H

#include "uiwcont.h"

typedef struct {
  uiwcont_t   *hbox;
  uiwcont_t   *cbox;
} uiutilsaccent_t;

void uiutilsAddProfileColorDisplay (uiwcont_t *vbox, uiutilsaccent_t *accent);
void uiutilsSetProfileColor (uiwcont_t *uiwidgetp);
const char * uiutilsGetCurrentFont (void);
const char * uiutilsGetListingFont (void);
int uiutilsValidatePlaylistName (uiwcont_t *entry, const char *label, void *udata);
void uiutilsProgressStatus (uiwcont_t *statusMsg, int count, int tot);
void uiutilsNewFontSize (char *buff, size_t sz, const char *font, const char *style, int newsz);
void uiutilsAddFavoriteClasses (void);

#endif /* INC_UIUTILS_H */
