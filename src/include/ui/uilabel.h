/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UILABEL_H
#define INC_UILABEL_H

#if BDJ4_USE_GTK
# include "ui-gtk3.h"
#endif

void  uiCreateLabel (UIWidget *uiwidget, const char *label);
void  uiCreateColonLabel (UIWidget *uiwidget, const char *label);
void  uiLabelAddClass (const char *classnm, const char *color);
void  uiLabelSetFont (UIWidget *uilabel, const char *font);
void  uiLabelSetText (UIWidget *uilabel, const char *text);
const char * uiLabelGetText (UIWidget *uiwidget);
void  uiLabelEllipsizeOn (UIWidget *uiwidget);
void  uiLabelSetSelectable (UIWidget *uiwidget);
void  uiLabelSetMaxWidth (UIWidget *uiwidget, int width);
void  uiLabelAlignEnd (UIWidget *uiwidget);

#endif /* INC_UILABEL_H */
