/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
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
  return;
}

void
uiCloseWindow (uiwcont_t *uiwindow)
{
  return;
}

bool
uiWindowIsMaximized (uiwcont_t *uiwindow)
{
  return false;
}

void
uiWindowIconify (uiwcont_t *uiwindow)
{
  return;
}

void
uiWindowDeIconify (uiwcont_t *uiwindow)
{
  return;
}

void
uiWindowMaximize (uiwcont_t *uiwindow)
{
  return;
}

void
uiWindowUnMaximize (uiwcont_t *uiwindow)
{
  return;
}

void
uiWindowDisableDecorations (uiwcont_t *uiwindow)
{
  return;
}

void
uiWindowEnableDecorations (uiwcont_t *uiwindow)
{
  return;
}

void
uiWindowGetSize (uiwcont_t *uiwindow, int *x, int *y)
{
  return;
}

void
uiWindowSetDefaultSize (uiwcont_t *uiwindow, int x, int y)
{
  return;
}

void
uiWindowGetPosition (uiwcont_t *uiwindow, int *x, int *y, int *ws)
{
  return;
}

void
uiWindowMove (uiwcont_t *uiwindow, int x, int y, int ws)
{
  return;
}

void
uiWindowMoveToCurrentWorkspace (uiwcont_t *uiwindow)
{
  return;
}

void
uiWindowNoFocusOnStartup (uiwcont_t *uiwindow)
{
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
  return;
}

uiwcont_t *
uiCreateDialogWindow (uiwcont_t *parentwin,
    uiwcont_t *attachment, callback_t *uicb, const char *title)
{
  return NULL;
}

void
uiWindowSetDoubleClickCallback (uiwcont_t *uiwidget, callback_t *uicb)
{
  return;
}

void
uiWindowSetWinStateCallback (uiwcont_t *uiwindow, callback_t *uicb)
{
  return;
}

void
uiWindowNoDim (uiwcont_t *uiwidget)
{
  return;
}

void
uiWindowSetMappedCallback (uiwcont_t *uiwidget, callback_t *uicb)
{
  return;
}

void
uiWindowPresent (uiwcont_t *uiwindow)
{
  return;
}

void
uiWindowRaise (uiwcont_t *uiwindow)
{
  return;
}

void
uiWindowFind (uiwcont_t *window)
{
  return;
}

void
uiWindowSetNoMaximize (uiwcont_t *uiwindow)
{
  return;
}

void
uiWindowPackInWindow (uiwcont_t *uiwindow, uiwcont_t *uiwidget)
{
  return;
}

void
uiWindowClearFocus (uiwcont_t *uiwidget)
{
  return;
}
