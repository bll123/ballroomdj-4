/*
 * Copyright 2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <Cocoa/Cocoa.h>
#import <Foundation/NSObject.h>

#include "callback.h"
#include "mdebug.h"
#include "uiwcont.h"

#include "ui/uiwcont-int.h"
#include "ui/uimacos-int.h"

#include "ui/uiui.h"
#include "ui/uiwidget.h"

static void uiWidgetUpdateMargins (uiwcont_t *uiwidget);

/* widget interface */

void
uiWidgetSetState (uiwcont_t *uiwidget, int state)
{
  return;
}

void
uiWidgetExpandHoriz (uiwcont_t *uiwidget)
{
  NSView    *widget;

  if (uiwidget == NULL) {
    return;
  }

  widget = uiwidget->uidata.widget;
  if ([widget superview] == nil) {
    return;
  }
  [widget setAutoresizingMask: NSViewWidthSizable];
  return;
}

void
uiWidgetExpandVert (uiwcont_t *uiwidget)
{
  NSView    *widget;

  if (uiwidget == NULL) {
    return;
  }

  widget = uiwidget->uidata.widget;
  if ([widget superview] == nil) {
    return;
  }
  [widget setAutoresizingMask: NSViewHeightSizable];
  return;
}

void
uiWidgetSetAllMargins (uiwcont_t *uiwidget, int mult)
{
  NSView        *widget;
  double        val;
  macosmargin_t *margins;

  if (uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

  widget = uiwidget->uidata.widget;
  if ([widget superview] == nil) {
    fprintf (stderr, "ERR: all: widget is not packed\n");
    return;
  }
  val = (double) (uiBaseMarginSz * mult);

  margins = uiwidget->uidata.margins;
  margins->margins.left = val;
  margins->margins.right = val;
  margins->margins.top = val;
  margins->margins.bottom = val;
  uiWidgetUpdateMargins (uiwidget);

  return;
}

void
uiWidgetSetMarginTop (uiwcont_t *uiwidget, int mult)
{
  NSView        *widget;
  double        val;
  macosmargin_t *margins;

  if (uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

  widget = uiwidget->uidata.widget;
  if ([widget superview] == nil) {
    fprintf (stderr, "ERR: top: widget is not packed\n");
    return;
  }
  val = (double) (uiBaseMarginSz * mult);

  margins = uiwidget->uidata.margins;
  margins->margins.top = val;
  uiWidgetUpdateMargins (uiwidget);

  return;
}

void
uiWidgetSetMarginBottom (uiwcont_t *uiwidget, int mult)
{
  NSView        *widget;
  double        val;
  macosmargin_t *margins;

  if (uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

  widget = uiwidget->uidata.widget;
  if ([widget superview] == nil) {
    fprintf (stderr, "ERR: bottom: widget is not packed\n");
    return;
  }
  val = (double) (uiBaseMarginSz * mult);

  margins = uiwidget->uidata.margins;
  margins->margins.bottom = val;
  uiWidgetUpdateMargins (uiwidget);

  return;
}

void
uiWidgetSetMarginStart (uiwcont_t *uiwidget, int mult)
{
  NSView        *widget;
  double        val;
  macosmargin_t *margins;

  if (uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

  widget = uiwidget->uidata.widget;
  if ([widget superview] == nil) {
    fprintf (stderr, "ERR: start: widget is not packed\n");
    return;
  }
  val = (double) (uiBaseMarginSz * mult);

  margins = uiwidget->uidata.margins;
  margins->margins.left = val;

  return;
}

void
uiWidgetSetMarginEnd (uiwcont_t *uiwidget, int mult)
{
  NSView        *widget;
  int           val;
  macosmargin_t *margins;

  if (uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

  widget = uiwidget->uidata.widget;
  if ([widget superview] == nil) {
    fprintf (stderr, "ERR: end: widget is not packed\n");
    return;
  }
  val = uiBaseMarginSz * mult;

  margins = uiwidget->uidata.margins;
  margins->margins.right = val;

  return;
}

void
uiWidgetAlignHorizFill (uiwcont_t *uiwidget)
{
  NSView    *widget;

  if (uiwidget == NULL) {
    return;
  }

  widget = uiwidget->uidata.widget;
  if ([widget superview] == nil) {
    return;
  }
  [widget setAutoresizingMask: NSViewWidthSizable | NSViewMinXMargin | NSViewMaxXMargin];
  return;
}

void
uiWidgetAlignHorizStart (uiwcont_t *uiwidget)
{
  NSView    *widget;

  if (uiwidget == NULL) {
    return;
  }

  widget = uiwidget->uidata.widget;
  if ([widget superview] == nil) {
    return;
  }
  [widget setAutoresizingMask: NSViewMaxXMargin];
  return;
}

void
uiWidgetAlignHorizEnd (uiwcont_t *uiwidget)
{
  NSView    *widget;

  if (uiwidget == NULL) {
    return;
  }

  widget = uiwidget->uidata.widget;
  if ([widget superview] == nil) {
    return;
  }
  [widget setAutoresizingMask: NSViewMinXMargin];
  return;
}

void
uiWidgetAlignHorizCenter (uiwcont_t *uiwidget)
{
  NSView    *widget;

  if (uiwidget == NULL) {
    return;
  }

  widget = uiwidget->uidata.widget;
  if ([widget superview] == nil) {
    return;
  }
  [widget setAutoresizingMask: NSViewMinXMargin | NSViewMaxXMargin];
  return;
}

void
uiWidgetAlignVertFill (uiwcont_t *uiwidget)
{
  NSView    *widget;

  if (uiwidget == NULL) {
    return;
  }

  widget = uiwidget->uidata.widget;
  if ([widget superview] == nil) {
    return;
  }
  [widget setAutoresizingMask: NSViewHeightSizable];
  return;
}

void
uiWidgetAlignVertStart (uiwcont_t *uiwidget)
{
  NSView    *widget;

  if (uiwidget == NULL) {
    return;
  }

  widget = uiwidget->uidata.widget;
  if ([widget superview] == nil) {
    return;
  }
  [widget setAutoresizingMask: NSViewMaxYMargin];
  return;
}

void
uiWidgetAlignVertCenter (uiwcont_t *uiwidget)
{
  NSView    *widget;

  if (uiwidget == NULL) {
    return;
  }

  widget = uiwidget->uidata.widget;
  if ([widget superview] == nil) {
    return;
  }
  [widget setAutoresizingMask: NSViewMinYMargin | NSViewMaxYMargin];
  return;
}

void
uiWidgetAlignVertBaseline (uiwcont_t *uiwidget)
{
  return;
}

void
uiWidgetAlignVertEnd (uiwcont_t *uiwidget)
{
  NSView    *widget;

  if (uiwidget == NULL) {
    return;
  }

  widget = uiwidget->uidata.widget;
  if ([widget superview] == nil) {
    return;
  }
  [widget setAutoresizingMask: NSViewMinYMargin];
  return;
}

void
uiWidgetDisableFocus (uiwcont_t *uiwidget)
{
  return;
}

void
uiWidgetHide (uiwcont_t *uiwidget)
{
  return;
}

void
uiWidgetShow (uiwcont_t *uiwidget)
{
  return;
}

void
uiWidgetShowAll (uiwcont_t *uiwidget)
{
  return;
}


void
uiWidgetMakePersistent (uiwcont_t *uiwidget)
{
  return;
}

void
uiWidgetClearPersistent (uiwcont_t *uiwidget)
{
  return;
}

void
uiWidgetSetSizeRequest (uiwcont_t *uiwidget, int width, int height)
{
  return;
}

void
uiWidgetGetPosition (uiwcont_t *uiwidget, int *x, int *y)
{
  return;
}

void
uiWidgetAddClass (uiwcont_t *uiwidget, const char *class)
{
  return;
}

void
uiWidgetRemoveClass (uiwcont_t *uiwidget, const char *class)
{
  return;
}

void
uiWidgetSetTooltip (uiwcont_t *uiwidget, const char *tooltip)
{
  return;
}

void
uiWidgetEnableFocus (uiwcont_t *uiwidget)
{
  return;
}

void
uiWidgetGrabFocus (uiwcont_t *uiwidget)
{
  return;
}

bool
uiWidgetIsMapped (uiwcont_t *uiwidget)
{
  return false;
}

void
uiWidgetSetSizeChgCallback (uiwcont_t *uiwidget, callback_t *uicb)
{
  return;
}

void
uiWidgetSetEnterCallback (uiwcont_t *uiwidget, callback_t *uicb)
{
  return;
}

/* internal routines */

static void
uiWidgetUpdateMargins (uiwcont_t *uiwidget)
{
  NSView        *view = uiwidget->uidata.widget;
  macosmargin_t *margins;

  margins = uiwidget->uidata.margins;

  if (uiwidget->wbasetype == WCONT_T_BOX) {
    NSStackView *stackview = uiwidget->uidata.widget;

    stackview.edgeInsets = margins->margins;
    return;
  }

  [margins->lguide.leadingAnchor
      constraintEqualToAnchor: view.leadingAnchor
      constant: margins->margins.left].active = YES;
  [margins->lguide.trailingAnchor
      constraintEqualToAnchor: view.trailingAnchor
      constant: margins->margins.right].active = YES;
  [margins->lguide.topAnchor
      constraintEqualToAnchor: view.topAnchor
      constant: margins->margins.top].active = YES;
  [margins->lguide.bottomAnchor
      constraintEqualToAnchor: view.bottomAnchor
      constant: margins->margins.bottom].active = YES;

  view.needsDisplay = YES;
}
