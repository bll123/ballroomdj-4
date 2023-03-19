/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIWINDOW_H
#define INC_UIWINDOW_H

#if BDJ4_USE_GTK
# include "ui-gtk3.h"
#endif

#include "callback.h"

void uiCreateMainWindow (uiwidget_t *uiwidget, callback_t *uicb,
    const char *title, const char *imagenm);
void uiWindowSetTitle (uiwidget_t *uiwidget, const char *title);
void uiCloseWindow (uiwidget_t *uiwindow);
bool uiWindowIsMaximized (uiwidget_t *uiwindow);
void uiWindowIconify (uiwidget_t *uiwindow);
void uiWindowDeIconify (uiwidget_t *uiwindow);
void uiWindowMaximize (uiwidget_t *uiwindow);
void uiWindowUnMaximize (uiwidget_t *uiwindow);
void uiWindowDisableDecorations (uiwidget_t *uiwindow);
void uiWindowEnableDecorations (uiwidget_t *uiwindow);
void uiWindowGetSize (uiwidget_t *uiwindow, int *x, int *y);
void uiWindowSetDefaultSize (uiwidget_t *uiwindow, int x, int y);
void uiWindowGetPosition (uiwidget_t *uiwindow, int *x, int *y, int *ws);
void uiWindowMove (uiwidget_t *uiwindow, int x, int y, int ws);
void uiWindowMoveToCurrentWorkspace (uiwidget_t *uiwindow);
void uiWindowNoFocusOnStartup (uiwidget_t *uiwindow);
void uiCreateScrolledWindow (uiwidget_t *uiwindow, int minheight);
void uiWindowSetPolicyExternal (uiwidget_t *uisw);
void uiCreateDialogWindow (uiwidget_t *uiwidget, uiwidget_t *parentwin, uiwidget_t *attachment, callback_t *uicb, const char *title);
void uiWindowSetDoubleClickCallback (uiwidget_t *uiwindow, callback_t *uicb);
void uiWindowSetWinStateCallback (uiwidget_t *uiwindow, callback_t *uicb);
void uiWindowNoDim (uiwidget_t *uiwidget);
void uiWindowSetMappedCallback (uiwidget_t *uiwidget, callback_t *uicb);
void uiWindowPresent (uiwidget_t *uiwidget);
void uiWindowRaise (uiwidget_t *uiwidget);
void uiWindowFind (uiwidget_t *window);

#endif /* INC_UIWINDOW_H */
