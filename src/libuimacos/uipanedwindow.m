/*
 * Copyright 2024-2026 Brad Lanam Pleasant Hill CA
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

uiwcont_t *
uiPanedWindowCreateVert (void)
{
  uiwcont_t   *uiwidget;
  NSSplitView *widget;

  widget = [[NSSplitView alloc] init];
  widget.vertical = true;
  widget.dividerStyle = NSSplitViewDividerStyleThick;
  // dividerColor

  uiwidget = uiwcontAlloc (WCONT_T_WINDOW, WCONT_T_PANED_WINDOW);
  uiwcontSetWidget (uiwidget, widget, NULL);
  return uiwidget;
}

void
uiPanedWindowPackStart (uiwcont_t *panedwin, uiwcont_t *box)
{
  NSSplitView *widget;

  if (! uiwcontValid (panedwin, WCONT_T_PANED_WINDOW, "paned-pack-start")) {
    return;
  }
  if (box == NULL) {
    return;
  }

  widget = panedwin->widget;
  [widget addArrangedSubview : box->widget];
  return;
}

void
uiPanedWindowPackEnd (uiwcont_t *panedwin, uiwcont_t *box)
{
  NSSplitView *widget;

  if (! uiwcontValid (panedwin, WCONT_T_PANED_WINDOW, "paned-pack-start")) {
    return;
  }
  if (box == NULL) {
    return;
  }

  if (panedwin->wtype != WCONT_T_PANED_WINDOW) {
    return;
  }

  widget = panedwin->widget;
  [widget addArrangedSubview : box->widget];
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
