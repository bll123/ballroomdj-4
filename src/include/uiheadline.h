/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "uiwcont.h"

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

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

