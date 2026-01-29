/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "callback.h"
#include "uiwcont.h"

#include "ui/uiwcont-int.h"

#include "ui/uiui.h"
#include "ui/uiwidget.h"
#include "ui/uiwindow.h"

uiwcont_t *
uiCreateMainWindow (callback_t *uicb, const char *title, const char *imagenm)
{
  return NULL;
}

void
uiWindowSetTitle (uiwcont_t *uiwidget, const char *title)
{
  if (! uiwcontValid (uiwidget, WCONT_T_WINDOW, "win-set-title")) {
    return;
  }
  if (title == NULL) {
    return;
  }

  return;
}

void
uiCloseWindow (uiwcont_t *uiwindow)
{
  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-close")) {
    return;
  }

  return;
}

bool
uiWindowIsMaximized (uiwcont_t *uiwindow)
{
  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-is-max")) {
    return false;
  }

  return false;
}

void
uiWindowIconify (uiwcont_t *uiwindow)
{
  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-iconify")) {
    return;
  }

  return;
}

void
uiWindowDeIconify (uiwcont_t *uiwindow)
{
  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-deiconify")) {
    return;
  }

  return;
}

void
uiWindowMaximize (uiwcont_t *uiwindow)
{
  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-maximize")) {
    return;
  }

  return;
}

void
uiWindowUnMaximize (uiwcont_t *uiwindow)
{
  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-unmaximize")) {
    return;
  }

  return;
}

void
uiWindowDisableDecorations (uiwcont_t *uiwindow)
{
  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-disable-dec")) {
    return;
  }

  return;
}

void
uiWindowEnableDecorations (uiwcont_t *uiwindow)
{
  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-enable-dec")) {
    return;
  }

  return;
}

void
uiWindowGetSize (uiwcont_t *uiwindow, int *x, int *y)
{
  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-get-sz")) {
    return;
  }

  return;
}

void
uiWindowSetDefaultSize (uiwcont_t *uiwindow, int x, int y)
{
  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-set-dflt-sz")) {
    return;
  }

  return;
}

void
uiWindowGetPosition (uiwcont_t *uiwindow, int *x, int *y, int *ws)
{
  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-get-pos")) {
    return;
  }

  return;
}

void
uiWindowMove (uiwcont_t *uiwindow, int x, int y, int ws)
{
  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-move")) {
    return;
  }

  return;
}

void
uiWindowMoveToCurrentWorkspace (uiwcont_t *uiwindow)
{
  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-move-curr-ws")) {
    return;
  }

  return;
}

void
uiWindowNoFocusOnStartup (uiwcont_t *uiwindow)
{
  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-no-focus-startup")) {
    return;
  }

  return;
}

uiwcont_t *
uiCreateScrolledWindow (int minheight)
{
  return NULL;
}

void
uiWindowSetPolicyExternal (uiwcont_t *uisw)
{
  if (! uiwcontValid (uisw, WCONT_T_WINDOW, "win-set-policy-ext")) {
    return;
  }

  return;
}

uiwcont_t *
uiCreateDialogWindow (uiwcont_t *parentwin,
    uiwcont_t *attachment, callback_t *uicb, const char *title)
{
  if (! uiwcontValid (parentwin, WCONT_T_WINDOW, "win-create-dialog-win")) {
    return NULL;
  }

  return NULL;
}

void
uiWindowSetDoubleClickCallback (uiwcont_t *uiwindow, callback_t *uicb)
{
  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-set-dclick-cb")) {
    return;
  }

  return;
}

void
uiWindowSetWinStateCallback (uiwcont_t *uiwindow, callback_t *uicb)
{
  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-set-state-cb")) {
    return;
  }

  return;
}

void
uiWindowNoDim (uiwcont_t *uiwindow)
{
  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-no-dim")) {
    return;
  }

  return;
}

void
uiWindowPresent (uiwcont_t *uiwindow)
{
  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-present")) {
    return;
  }

  return;
}

void
uiWindowRaise (uiwcont_t *uiwindow)
{
  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-raise")) {
    return;
  }

  return;
}

void
uiWindowFind (uiwcont_t *uiwindow)
{
  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-find")) {
    return;
  }

  return;
}

void
uiWindowPackInWindow (uiwcont_t *uiwindow, uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-pack-in-win-win")) {
    return;
  }

  return;
}

void
uiWindowSetNoMaximize (uiwcont_t *uiwindow)
{
  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "window-set-no-maximize")) {
    return;
  }

  return;
}

void
uiWindowClearFocus (uiwcont_t *uiwindow)
{
  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "window-clear-focus")) {
    return;
  }

  return;
}

void
uiWindowGetMonitorSize (uiwcont_t *uiwindow, int *width, int *height)
{
  if (! uiwcontValid (uiwindow, WCONT_T_WINDOW, "win-get-mon-sz")) {
    return;
  }

  *width = 0;
  *height = 0;
  return;
}
