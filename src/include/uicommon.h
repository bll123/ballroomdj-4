/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "uiwcont.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

int uicommonValidatePlaylistNameClr (uiwcont_t *entry, const char *label, void *udata);
int uicommonValidatePlaylistName (uiwcont_t *entry, const char *label, void *udata);
void uicommonProgressStatus (uiwcont_t *statusMsg, int count, int tot);
void uicommonAddFavoriteClasses (void);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

