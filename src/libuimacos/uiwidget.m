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

static void uiWidgetUpdateLayout (uiwcont_t *uiwidget);
static NSView * uiwidgetGetPrior (NSStackView *stview, NSView *view);

/* widget interface */

void
uiWidgetSetState (uiwcont_t *uiwidget, int state)
{
  return;
}

void
uiWidgetExpandHoriz (uiwcont_t *uiwidget)
{
  NSView        *widget;
  NSView        *container;
  macoslayout_t *layout;

  if (uiwidget == NULL) {
    return;
  }

  widget = uiwidget->uidata.widget;
  container = [widget superview];
  if (container == nil) {
    fprintf (stderr, "ERR: exp-horiz: widget is not packed\n");
    return;
  }

  layout = uiwidget->uidata.layout;
  layout->expand = true;
  uiWidgetUpdateLayout (uiwidget);
  return;
}

void
uiWidgetExpandVert (uiwcont_t *uiwidget)
{
  NSView        *widget;
  macoslayout_t *layout;
  NSView        *container;

  if (uiwidget == NULL) {
    return;
  }

  widget = uiwidget->uidata.widget;
  container = [widget superview];
  if (container == nil) {
    fprintf (stderr, "ERR: exp-vert: widget is not packed\n");
    return;
  }

  layout = uiwidget->uidata.layout;
  layout->expand = true;
  uiWidgetUpdateLayout (uiwidget);
  return;
}

void
uiWidgetSetAllMargins (uiwcont_t *uiwidget, int mult)
{
  NSView        *widget;
  double        val;
  macoslayout_t *layout;

  if (uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

  widget = uiwidget->uidata.widget;
  if ([widget superview] == nil) {
    fprintf (stderr, "ERR: mg-all: widget is not packed\n");
    return;
  }
  val = (double) (uiBaseMarginSz * mult);

  layout = uiwidget->uidata.layout;
  layout->margins.left = val;
  layout->margins.right = val;
  layout->margins.top = val;
  layout->margins.bottom = val;
  uiWidgetUpdateLayout (uiwidget);

  return;
}

void
uiWidgetSetMarginTop (uiwcont_t *uiwidget, int mult)
{
  NSView        *widget;
  double        val;
  macoslayout_t *layout;

  if (uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

  widget = uiwidget->uidata.widget;
  if ([widget superview] == nil) {
    fprintf (stderr, "ERR: mg-top: widget is not packed\n");
    return;
  }
  val = (double) (uiBaseMarginSz * mult);

  layout = uiwidget->uidata.layout;
  layout->margins.top = val;
  uiWidgetUpdateLayout (uiwidget);

  return;
}

void
uiWidgetSetMarginBottom (uiwcont_t *uiwidget, int mult)
{
  NSView        *widget;
  double        val;
  macoslayout_t *layout;

  if (uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

  widget = uiwidget->uidata.widget;
  if ([widget superview] == nil) {
    fprintf (stderr, "ERR: mg-bottom: widget is not packed\n");
    return;
  }
  val = (double) (uiBaseMarginSz * mult);

  layout = uiwidget->uidata.layout;
  layout->margins.bottom = val;
  uiWidgetUpdateLayout (uiwidget);

  return;
}

void
uiWidgetSetMarginStart (uiwcont_t *uiwidget, int mult)
{
  NSView        *widget;
  double        val;
  macoslayout_t *layout;

  if (uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

  widget = uiwidget->uidata.widget;
  if ([widget superview] == nil) {
    fprintf (stderr, "ERR: mg-start: widget is not packed\n");
    return;
  }
  val = (double) (uiBaseMarginSz * mult);

  layout = uiwidget->uidata.layout;
  layout->margins.left = val;
  uiWidgetUpdateLayout (uiwidget);

  return;
}

void
uiWidgetSetMarginEnd (uiwcont_t *uiwidget, int mult)
{
  NSView        *widget;
  int           val;
  macoslayout_t *layout;

  if (uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

  widget = uiwidget->uidata.widget;
  if ([widget superview] == nil) {
    fprintf (stderr, "ERR: mg-end: widget is not packed\n");
    return;
  }
  val = uiBaseMarginSz * mult;

  layout = uiwidget->uidata.layout;
  layout->margins.right = val;
  uiWidgetUpdateLayout (uiwidget);

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
  [widget setAutoresizingMask: NSViewWidthSizable];
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
  NSView        *view;
  macoslayout_t *layout;
  NSStackView   *stview;

  if (uiwidget == NULL) {
    return;
  }

  view = uiwidget->uidata.widget;
  stview = (NSStackView *) [view superview];
  if (stview == nil) {
    fprintf (stderr, "ERR: align-h-center: widget is not packed\n");
    return;
  }

  layout = uiwidget->uidata.layout;
  layout->centered = true;
  uiWidgetUpdateLayout (uiwidget);

  return;
}

void
uiWidgetAlignVertFill (uiwcont_t *uiwidget)
{
  NSView        *view;
//  macoslayout_t *layout;
  NSStackView   *stview;

  if (uiwidget == NULL) {
    return;
  }

  view = uiwidget->uidata.widget;
  stview = (NSStackView *) [view superview];
  if (stview == nil) {
    fprintf (stderr, "ERR: vert-fill: widget is not packed\n");
    return;
  }

//  layout = uiwidget->uidata.layout;

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
  NSView        *view;
  macoslayout_t *layout;
  NSStackView   *stview;

  if (uiwidget == NULL) {
    return;
  }

  view = uiwidget->uidata.widget;
  stview = (NSStackView *) [view superview];
  if (stview == nil) {
    fprintf (stderr, "ERR: align-v-center: widget is not packed\n");
    return;
  }

  layout = uiwidget->uidata.layout;
  layout->centered = true;
  uiWidgetUpdateLayout (uiwidget);

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
uiWidgetUpdateLayout (uiwcont_t *uiwidget)
{
  macoslayout_t *layout = NULL;
  NSStackView   *stvcontainer = NULL;

  layout = uiwidget->uidata.layout;
  stvcontainer = layout->container;

  stvcontainer.edgeInsets = layout->margins;
  stvcontainer.needsDisplay = YES;
}

static NSView *
uiwidgetGetPrior (NSStackView *stview, NSView *view)
{
  NSView    *prior;
  NSArray   *subviews;
  int       svcount;

  subviews = [stview subviews];
  svcount = [subviews count];

  prior = NULL;
  for (int i = 0; i < svcount; ++i) {
    NSView        *tview;

    tview = [subviews objectAtIndex: i];
    if (tview == view) {
      break;
    }
    prior = tview;
  }

  return prior;
}
