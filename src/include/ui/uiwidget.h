/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIWIDGET_H
#define INC_UIWIDGET_H

#if BDJ4_USE_GTK
# include "ui-gtk3.h"
#endif

/* widget interface */
void  uiWidgetSetState (uiwcont_t *uiwidget, int state);
void  uiWidgetEnable (uiwcont_t *uiwidget);
void  uiWidgetExpandHoriz (uiwcont_t *uiwidget);
void  uiWidgetExpandVert (uiwcont_t *uiwidget);
void  uiWidgetSetAllMargins (uiwcont_t *uiwidget, int mult);
void  uiWidgetSetMarginTop (uiwcont_t *uiwidget, int mult);
void  uiWidgetSetMarginBottom (uiwcont_t *uiwidget, int mult);
void  uiWidgetSetMarginStart (uiwcont_t *uiwidget, int mult);
void  uiWidgetSetMarginEnd (uiwcont_t *uiwidget, int mult);
void  uiWidgetAlignHorizFill (uiwcont_t *uiwidget);
void  uiWidgetAlignHorizStart (uiwcont_t *uiwidget);
void  uiWidgetAlignHorizEnd (uiwcont_t *uiwidget);
void  uiWidgetAlignHorizCenter (uiwcont_t *uiwidget);
void  uiWidgetAlignVertFill (uiwcont_t *uiwidget);
void  uiWidgetAlignVertStart (uiwcont_t *uiwidget);
void  uiWidgetAlignVertCenter (uiwcont_t *uiwidget);
void  uiWidgetDisableFocus (uiwcont_t *uiwidget);
void  uiWidgetHide (uiwcont_t *uiwidget);
void  uiWidgetShow (uiwcont_t *uiwidget);
void  uiWidgetShowAll (uiwcont_t *uiwidget);
void  uiWidgetMakePersistent (uiwcont_t *uiuiwidget);
void  uiWidgetClearPersistent (uiwcont_t *uiuiwidget);
void  uiWidgetSetSizeRequest (uiwcont_t *uiuiwidget, int width, int height);
bool  uiWidgetIsValid (uiwcont_t *uiwidget);
void  uiWidgetGetPosition (uiwcont_t *widget, int *x, int *y);
void  uiWidgetSetClass (uiwcont_t *uiwidget, const char *class);
void  uiWidgetRemoveClass (uiwcont_t *uiwidget, const char *class);

#endif /* INC_UIWIDGET_H */
