/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "uiwcont.h"

#include "ui/uiui.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef struct uihdrline uihdrline_t;

/* uiutils.c */
uihdrline_t *uiutilsHeaderLineSetup (uiwcont_t *boxp);
uiwcont_t *uiutilsHeaderLineAddMenubar (uihdrline_t *hdrline);
uiwcont_t *uiutilsHeaderLineAddLabel (uihdrline_t *hdrline, const char *class);
void uiutilsHeaderLinePostProcess (uihdrline_t *hdrline);
void uiutilsHeaderLineSetColor (uihdrline_t *hdrline, const char *oldcolor);
void uiutilsHeaderLineFree (uihdrline_t *hdrline);

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

