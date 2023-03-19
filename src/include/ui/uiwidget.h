/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIWIDGET_H
#define INC_UIWIDGET_H

#if BDJ4_USE_GTK
# include "ui-gtk3.h"
#endif

/* widget interface */
void  uiWidgetSetState (uiwidget_t *uiwidget, int state);
void  uiWidgetEnable (uiwidget_t *uiwidget);
void  uiWidgetExpandHoriz (uiwidget_t *uiwidget);
void  uiWidgetExpandVert (uiwidget_t *uiwidget);
void  uiWidgetSetAllMargins (uiwidget_t *uiwidget, int mult);
void  uiWidgetSetMarginTop (uiwidget_t *uiwidget, int mult);
void  uiWidgetSetMarginBottom (uiwidget_t *uiwidget, int mult);
void  uiWidgetSetMarginStart (uiwidget_t *uiwidget, int mult);
void  uiWidgetSetMarginEnd (uiwidget_t *uiwidget, int mult);
void  uiWidgetAlignHorizFill (uiwidget_t *uiwidget);
void  uiWidgetAlignHorizStart (uiwidget_t *uiwidget);
void  uiWidgetAlignHorizEnd (uiwidget_t *uiwidget);
void  uiWidgetAlignHorizCenter (uiwidget_t *uiwidget);
void  uiWidgetAlignVertFill (uiwidget_t *uiwidget);
void  uiWidgetAlignVertStart (uiwidget_t *uiwidget);
void  uiWidgetAlignVertCenter (uiwidget_t *uiwidget);
void  uiWidgetDisableFocus (uiwidget_t *uiwidget);
void  uiWidgetHide (uiwidget_t *uiwidget);
void  uiWidgetShow (uiwidget_t *uiwidget);
void  uiWidgetShowAll (uiwidget_t *uiwidget);
void  uiWidgetMakePersistent (uiwidget_t *uiuiwidget);
void  uiWidgetClearPersistent (uiwidget_t *uiuiwidget);
void  uiWidgetSetSizeRequest (uiwidget_t *uiuiwidget, int width, int height);
bool  uiWidgetIsValid (uiwidget_t *uiwidget);
void  uiWidgetGetPosition (uiwidget_t *widget, int *x, int *y);
void  uiWidgetSetClass (uiwidget_t *uiwidget, const char *class);
void  uiWidgetRemoveClass (uiwidget_t *uiwidget, const char *class);

#endif /* INC_UIWIDGET_H */
