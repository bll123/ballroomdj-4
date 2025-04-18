/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIUTILS_H
#define INC_UIUTILS_H

#include "uiwcont.h"

#include "ui/uiui.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef struct {
  uiwcont_t   *hbox;
  uiwcont_t   *cbox;
} uiutilsaccent_t;

/* uiutils.c */
void uiutilsAddProfileColorDisplay (uiwcont_t *vbox, uiutilsaccent_t *accent);
void uiutilsSetProfileColor (uiwcont_t *uiwidgetp, const char *oldcolor);
const char * uiutilsGetCurrentFont (void);
const char * uiutilsGetListingFont (void);
int uiutilsValidatePlaylistNameClr (uiwcont_t *entry, const char *label, void *udata);
int uiutilsValidatePlaylistName (uiwcont_t *entry, const char *label, void *udata);
void uiutilsProgressStatus (uiwcont_t *statusMsg, int count, int tot);
void uiutilsFontInfo (const char *font, char *buff, size_t sz, int *fontsz);
void uiutilsNewFontSize (char *buff, size_t sz, const char *font, const char *style, int newsz);
void uiutilsAddFavoriteClasses (void);
void uiutilsInitSetup (uisetup_t *uisetup);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_UIUTILS_H */
