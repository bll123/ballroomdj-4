/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UILABEL_H
#define INC_UILABEL_H

#if BDJ4_USE_GTK
# include "ui-gtk3.h"
#endif

void  uiCreateLabel (uiwidget_t *uiwidget, const char *label);
void  uiCreateColonLabel (uiwidget_t *uiwidget, const char *label);
void  uiLabelAddClass (const char *classnm, const char *color);
void  uiLabelSetFont (uiwidget_t *uilabel, const char *font);
void  uiLabelSetText (uiwidget_t *uilabel, const char *text);
const char * uiLabelGetText (uiwidget_t *uiwidget);
void  uiLabelEllipsizeOn (uiwidget_t *uiwidget);
void  uiLabelSetSelectable (uiwidget_t *uiwidget);
void  uiLabelSetMaxWidth (uiwidget_t *uiwidget, int width);
void  uiLabelAlignEnd (uiwidget_t *uiwidget);

#endif /* INC_UILABEL_H */
