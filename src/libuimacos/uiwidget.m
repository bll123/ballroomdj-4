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

#include "ui/uiui.h"
#include "ui/uiwidget.h"

typedef struct {
  /* to hold the margin information for the widget */
  NSEdgeInsets  margins;
  NSDictionary  *viewBindings;
  NSDictionary  *metrics;
} macosmargin_t;

static void uiWidgetInitMargins (uiwcont_t *uiwidget);

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

  uiWidgetInitMargins (uiwidget);
  margins = uiwidget->uidata.margins;
  margins->margins.left = val;
  margins->margins.right = val;
  margins->margins.top = val;
  margins->margins.bottom = val;

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

  uiWidgetInitMargins (uiwidget);
  margins = uiwidget->uidata.margins;
  margins->margins.top = val;

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

  uiWidgetInitMargins (uiwidget);
  margins = uiwidget->uidata.margins;
  margins->margins.bottom = val;

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

  uiWidgetInitMargins (uiwidget);
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

  uiWidgetInitMargins (uiwidget);
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
uiWidgetInitMargins (uiwcont_t *uiwidget)
{
  NSView        *view = uiwidget->uidata.widget;
  macosmargin_t *margins;

  if (uiwidget->uidata.margins != NULL) {
    return;
  }

  margins = mdmalloc (sizeof (macosmargin_t));
  uiwidget->uidata.margins = margins;

  margins->margins = NSEdgeInsetsMake (0, 0, 0, 0);
  margins->viewBindings = NSDictionaryOfVariableBindings (view);
  // ### TODO left and right need to be swapped for RTL languages
  margins->metrics = @{
      @"marginLeft" : @(margins->margins.left),
      @"marginRight" : @(margins->margins.right),
      @"marginBottom" : @(margins->margins.bottom),
      @"marginTop" : @(margins->margins.top)
      };

if (uiwidget->wbasetype != WCONT_T_BUTTON) {
return;
}

fprintf (stderr, "ml: %p \n", &margins->margins.left);
fprintf (stderr, "view: %p \n", view);
  [view addConstraints: [NSLayoutConstraint
      constraintsWithVisualFormat:@"H:|-(marginLeft)-[view]-(marginRight)-|"
      options:0
      metrics:margins->metrics
      views:margins->viewBindings]];
  [view addConstraints: [NSLayoutConstraint
      constraintsWithVisualFormat:@"V:|-(marginTop)-[view]-(marginBottom)-|"
      options:0
      metrics:margins->metrics
      views:margins->viewBindings]];
}
