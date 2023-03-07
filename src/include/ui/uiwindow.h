/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIWINDOW_H
#define INC_UIWINDOW_H

#if BDJ4_USE_GTK
# include "ui-gtk3.h"
#endif

#include "callback.h"

void uiCreateMainWindow (UIWidget *uiwidget, callback_t *uicb,
    const char *title, const char *imagenm);
void uiWindowSetTitle (UIWidget *uiwidget, const char *title);
void uiCloseWindow (UIWidget *uiwindow);
bool uiWindowIsMaximized (UIWidget *uiwindow);
void uiWindowIconify (UIWidget *uiwindow);
void uiWindowDeIconify (UIWidget *uiwindow);
void uiWindowMaximize (UIWidget *uiwindow);
void uiWindowUnMaximize (UIWidget *uiwindow);
void uiWindowDisableDecorations (UIWidget *uiwindow);
void uiWindowEnableDecorations (UIWidget *uiwindow);
void uiWindowGetSize (UIWidget *uiwindow, int *x, int *y);
void uiWindowSetDefaultSize (UIWidget *uiwindow, int x, int y);
void uiWindowGetPosition (UIWidget *uiwindow, int *x, int *y, int *ws);
void uiWindowMove (UIWidget *uiwindow, int x, int y, int ws);
void uiWindowMoveToCurrentWorkspace (UIWidget *uiwindow);
void uiWindowNoFocusOnStartup (UIWidget *uiwindow);
void uiCreateScrolledWindow (UIWidget *uiwindow, int minheight);
void uiWindowSetPolicyExternal (UIWidget *uisw);
void uiCreateDialogWindow (UIWidget *uiwidget, UIWidget *parentwin, UIWidget *attachment, callback_t *uicb, const char *title);
void uiWindowSetDoubleClickCallback (UIWidget *uiwindow, callback_t *uicb);
void uiWindowSetWinStateCallback (UIWidget *uiwindow, callback_t *uicb);
void uiWindowNoDim (UIWidget *uiwidget);
void uiWindowSetMappedCallback (UIWidget *uiwidget, callback_t *uicb);
void uiWindowPresent (UIWidget *uiwidget);
void uiWindowRaise (UIWidget *uiwidget);
void uiWindowFind (UIWidget *window);

#endif /* INC_UIWINDOW_H */
