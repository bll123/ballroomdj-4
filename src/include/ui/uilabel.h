/*
 * Copyright 2023-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "uiwcont.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

uiwcont_t *uiCreateLabel (const char *label);
void  uiLabelAddClass (const char *classnm, const char *color);
void uiLabelSetTooltip (uiwcont_t *uiwidget, const char *txt);
void  uiLabelSetFont (uiwcont_t *uilabel, const char *font);
void  uiLabelSetText (uiwcont_t *uilabel, const char *text);
const char * uiLabelGetText (uiwcont_t *uiwidget);
void  uiLabelEllipsizeOn (uiwcont_t *uiwidget);
void  uiLabelWrapOn (uiwcont_t *uiwidget);
void  uiLabelSetSelectable (uiwcont_t *uiwidget);
void  uiLabelSetMinWidth (uiwcont_t *uiwidget, int width);
void  uiLabelSetMaxWidth (uiwcont_t *uiwidget, int width);
void  uiLabelAlignEnd (uiwcont_t *uiwidget);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

