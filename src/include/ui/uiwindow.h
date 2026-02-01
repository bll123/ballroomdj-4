/*
 * Copyright 2023-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "callback.h"
#include "uiwcont.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

uiwcont_t *uiCreateMainWindow (callback_t *uicb, const char *title, const char *imagenm);
void uiMainWindowFree (uiwcont_t *uiwidget);
void uiWindowSetTitle (uiwcont_t *uiwidget, const char *title);
void uiCloseWindow (uiwcont_t *uiwidget);
bool uiWindowIsMaximized (uiwcont_t *uiwidget);
void uiWindowIconify (uiwcont_t *uiwidget);
void uiWindowDeIconify (uiwcont_t *uiwidget);
void uiWindowMaximize (uiwcont_t *uiwidget);
void uiWindowUnMaximize (uiwcont_t *uiwidget);
void uiWindowDisableDecorations (uiwcont_t *uiwidget);
void uiWindowEnableDecorations (uiwcont_t *uiwidget);
void uiWindowGetSize (uiwcont_t *uiwidget, int *x, int *y);
void uiWindowSetDefaultSize (uiwcont_t *uiwidget, int x, int y);
void uiWindowGetPosition (uiwcont_t *uiwidget, int *x, int *y, int *ws);
void uiWindowMove (uiwcont_t *uiwidget, int x, int y, int ws);
void uiWindowMoveToCurrentWorkspace (uiwcont_t *uiwidget);
void uiWindowNoFocusOnStartup (uiwcont_t *uiwidget);
uiwcont_t *uiCreateScrolledWindow (int minheight);
void uiWindowSetPolicyExternal (uiwcont_t *uisw);
uiwcont_t *uiCreateDialogWindow (uiwcont_t *parentwin, uiwcont_t *attachment, callback_t *uicb, const char *title);
void uiWindowSetDoubleClickCallback (uiwcont_t *uiwidget, callback_t *uicb);
void uiWindowSetWinStateCallback (uiwcont_t *uiwidget, callback_t *uicb);
void uiWindowNoDim (uiwcont_t *uiwidget);
void uiWindowPresent (uiwcont_t *uiwidget);
void uiWindowRaise (uiwcont_t *uiwidget);
void uiWindowFind (uiwcont_t *window);
void uiWindowSetNoMaximize (uiwcont_t *uiwidget);
void uiWindowPackInWindow (uiwcont_t *uiwin, uiwcont_t *uiwidget);
void uiWindowClearFocus (uiwcont_t *uiwidget);
void uiWindowGetMonitorSize (uiwcont_t *uiwidget, int *width, int *height);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

