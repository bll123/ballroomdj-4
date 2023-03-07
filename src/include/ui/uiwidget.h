/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIWIDGET_H
#define INC_UIWIDGET_H

#if BDJ4_USE_GTK
# include "ui-gtk3.h"
#endif

/* widget interface */
void  uiWidgetDisable (UIWidget *uiwidget);
void  uiWidgetEnable (UIWidget *uiwidget);
void  uiWidgetExpandHoriz (UIWidget *uiwidget);
void  uiWidgetExpandVert (UIWidget *uiwidget);
void  uiWidgetSetAllMargins (UIWidget *uiwidget, int mult);
void  uiWidgetSetMarginTop (UIWidget *uiwidget, int mult);
void  uiWidgetSetMarginBottom (UIWidget *uiwidget, int mult);
void  uiWidgetSetMarginStart (UIWidget *uiwidget, int mult);
void  uiWidgetSetMarginEnd (UIWidget *uiwidget, int mult);
void  uiWidgetAlignHorizFill (UIWidget *uiwidget);
void  uiWidgetAlignHorizStart (UIWidget *uiwidget);
void  uiWidgetAlignHorizEnd (UIWidget *uiwidget);
void  uiWidgetAlignHorizCenter (UIWidget *uiwidget);
void  uiWidgetAlignVertFill (UIWidget *uiwidget);
void  uiWidgetAlignVertStart (UIWidget *uiwidget);
void  uiWidgetAlignVertCenter (UIWidget *uiwidget);
void  uiWidgetDisableFocus (UIWidget *uiwidget);
void  uiWidgetHide (UIWidget *uiwidget);
void  uiWidgetShow (UIWidget *uiwidget);
void  uiWidgetShowAll (UIWidget *uiwidget);
void  uiWidgetMakePersistent (UIWidget *uiuiwidget);
void  uiWidgetClearPersistent (UIWidget *uiuiwidget);
void  uiWidgetSetSizeRequest (UIWidget *uiuiwidget, int width, int height);
bool  uiWidgetIsValid (UIWidget *uiwidget);
void  uiWidgetGetPosition (UIWidget *widget, int *x, int *y);
void  uiWidgetSetClass (UIWidget *uiwidget, const char *class);
void  uiWidgetRemoveClass (UIWidget *uiwidget, const char *class);

#endif /* INC_UIWIDGET_H */
