/*
 * Copyright 2024-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "uiwcont.h"

#include "ui/uiwcont-int.h"

#include "ui/uiui.h"
#include "ui/uipanedwindow.h"

static uiwcont_t * uiPanedWindowCreate (int orientation);

uiwcont_t *
uiPanedWindowCreateVert (void)
{
  uiwcont_t *uiwidget;

fprintf (stderr, "c-pw-v\n");
  uiwidget = uiPanedWindowCreate (0);
  return uiwidget;
}

void
uiPanedWindowPackStart (uiwcont_t *panedwin, uiwcont_t *box)
{
  if (panedwin == NULL || box == NULL) {
    return;
  }
  if (panedwin->wtype != WCONT_T_PANED_WINDOW) {
    return;
  }

  return;
}

void
uiPanedWindowPackEnd (uiwcont_t *panedwin, uiwcont_t *box)
{
  if (panedwin == NULL || box == NULL) {
    return;
  }

  if (panedwin->wtype != WCONT_T_PANED_WINDOW) {
    return;
  }

  return;
}

int
uiPanedWindowGetPosition (uiwcont_t *panedwin)
{
  if (panedwin == NULL) {
    return -1;
  }

  if (panedwin->wtype != WCONT_T_PANED_WINDOW) {
    return -1;
  }

  return -1;
}

void
uiPanedWindowSetPosition (uiwcont_t *panedwin, int position)
{
  if (panedwin == NULL) {
    return;
  }

  if (panedwin->wtype != WCONT_T_PANED_WINDOW) {
    return;
  }

  return;
}

/* internal routines */

static uiwcont_t *
uiPanedWindowCreate (int orientation)
{
  return NULL;
}
