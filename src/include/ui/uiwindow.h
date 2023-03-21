/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIWINDOW_H
#define INC_UIWINDOW_H

#include "callback.h"
#include "uiwcont.h"

uiwcont_t *uiCreateMainWindow (callback_t *uicb, const char *title, const char *imagenm);
void uiWindowSetTitle (uiwcont_t *uiwidget, const char *title);
void uiCloseWindow (uiwcont_t *uiwindow);
bool uiWindowIsMaximized (uiwcont_t *uiwindow);
void uiWindowIconify (uiwcont_t *uiwindow);
void uiWindowDeIconify (uiwcont_t *uiwindow);
void uiWindowMaximize (uiwcont_t *uiwindow);
void uiWindowUnMaximize (uiwcont_t *uiwindow);
void uiWindowDisableDecorations (uiwcont_t *uiwindow);
void uiWindowEnableDecorations (uiwcont_t *uiwindow);
void uiWindowGetSize (uiwcont_t *uiwindow, int *x, int *y);
void uiWindowSetDefaultSize (uiwcont_t *uiwindow, int x, int y);
void uiWindowGetPosition (uiwcont_t *uiwindow, int *x, int *y, int *ws);
void uiWindowMove (uiwcont_t *uiwindow, int x, int y, int ws);
void uiWindowMoveToCurrentWorkspace (uiwcont_t *uiwindow);
void uiWindowNoFocusOnStartup (uiwcont_t *uiwindow);
uiwcont_t *uiCreateScrolledWindow (int minheight);
void uiWindowSetPolicyExternal (uiwcont_t *uisw);
uiwcont_t *uiCreateDialogWindow (uiwcont_t *parentwin, uiwcont_t *attachment, callback_t *uicb, const char *title);
void uiWindowSetDoubleClickCallback (uiwcont_t *uiwindow, callback_t *uicb);
void uiWindowSetWinStateCallback (uiwcont_t *uiwindow, callback_t *uicb);
void uiWindowNoDim (uiwcont_t *uiwidget);
void uiWindowSetMappedCallback (uiwcont_t *uiwidget, callback_t *uicb);
void uiWindowPresent (uiwcont_t *uiwidget);
void uiWindowRaise (uiwcont_t *uiwidget);
void uiWindowFind (uiwcont_t *window);

#endif /* INC_UIWINDOW_H */
