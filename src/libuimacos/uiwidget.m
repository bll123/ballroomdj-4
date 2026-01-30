/*
 * Copyright 2024-2026 Brad Lanam Pleasant Hill CA
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

/* widget interface */

void
uiWidgetSetState (uiwcont_t *uiwidget, int state)
{
  return;
}

/* this uses the GTK terminology */
/* set the widget to take any horizontal space */
void
uiWidgetExpandHoriz (uiwcont_t *uiwidget)
{
  NSView          *widget;

  if (! uiwidget->packed) {
    fprintf (stderr, "ERR: w-exp-h widget not packed %d %s %s\n",
        uiwidget->id, uiwcontDesc (uiwidget->wbasetype),
        uiwcontDesc (uiwidget->wtype));
    return;
  }

  widget = uiwidget->uidata.packwidget;
  [widget.widthAnchor
      constraintLessThanOrEqualToConstant : 600.0].active = YES;
  widget.autoresizingMask |= NSViewWidthSizable;

  return;
}

/* this uses the GTK terminology */
/* set the widget to take any horizontal space */
void
uiWidgetExpandVert (uiwcont_t *uiwidget)
{
  NSView          *widget;

  if (! uiwidget->packed) {
    fprintf (stderr, "ERR: w-exp-v widget not packed %d %s %s\n",
        uiwidget->id, uiwcontDesc (uiwidget->wbasetype),
        uiwcontDesc (uiwidget->wtype));
    return;
  }

  widget = uiwidget->uidata.packwidget;
  [widget.heightAnchor
      constraintLessThanOrEqualToConstant : 600.0].active = YES;
  widget.autoresizingMask |= NSViewHeightSizable;

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

  if (! uiwidget->packed) {
    fprintf (stderr, "ERR: w-set-all-m widget not packed %d %s %s\n",
        uiwidget->id, uiwcontDesc (uiwidget->wbasetype),
        uiwcontDesc (uiwidget->wtype));
    return;
  }

  widget = uiwidget->uidata.widget;

  val = (double) (uiBaseMarginSz * mult);

  layout = uiwidget->uidata.layout;
  layout->margins.left = (CGFloat) val;
  layout->margins.right = (CGFloat) val;
  layout->margins.top = (CGFloat) val;
  layout->margins.bottom = (CGFloat) val;
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

  if (! uiwidget->packed) {
    fprintf (stderr, "ERR: w-set-m-top widget not packed %d %s %s\n",
        uiwidget->id, uiwcontDesc (uiwidget->wbasetype),
        uiwcontDesc (uiwidget->wtype));
    return;
  }

  widget = uiwidget->uidata.widget;

  val = (double) (uiBaseMarginSz * mult);

  layout = uiwidget->uidata.layout;
  layout->margins.top = (CGFloat) val;
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

  if (! uiwidget->packed) {
    fprintf (stderr, "ERR: w-set-m-bottom widget not packed %d %s %s\n",
        uiwidget->id, uiwcontDesc (uiwidget->wbasetype),
        uiwcontDesc (uiwidget->wtype));
    return;
  }

  widget = uiwidget->uidata.widget;

  val = (double) (uiBaseMarginSz * mult);

  layout = uiwidget->uidata.layout;
  layout->margins.bottom = (CGFloat) val;
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

  if (! uiwidget->packed) {
    fprintf (stderr, "ERR: w-set-m-start widget not packed %d %s %s\n",
        uiwidget->id, uiwcontDesc (uiwidget->wbasetype),
        uiwcontDesc (uiwidget->wtype));
    return;
  }

  widget = uiwidget->uidata.widget;

  val = (double) (uiBaseMarginSz * mult);

  layout = uiwidget->uidata.layout;
  layout->margins.left = (CGFloat) val;
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

  if (! uiwidget->packed) {
    fprintf (stderr, "ERR: w-set-m-end widget not packed %d %s %s\n",
        uiwidget->id, uiwcontDesc (uiwidget->wbasetype),
        uiwcontDesc (uiwidget->wtype));
    return;
  }

  widget = uiwidget->uidata.widget;

  val = uiBaseMarginSz * mult;

  layout = uiwidget->uidata.layout;
  layout->margins.right = (CGFloat) val;
  uiWidgetUpdateLayout (uiwidget);

  return;
}

/* this uses the GTK terminology */
/* controls how the widget handles extra space allocated to it */
void
uiWidgetAlignHorizFill (uiwcont_t *uiwidget)
{
  NSView    *widget;

  if (uiwidget == NULL) {
    return;
  }

  if (! uiwidget->packed) {
    fprintf (stderr, "ERR: w-al-h-f widget not packed %d %s %s\n",
        uiwidget->id, uiwcontDesc (uiwidget->wbasetype),
        uiwcontDesc (uiwidget->wtype));
    return;
  }

  widget = uiwidget->uidata.packwidget;
  [widget.widthAnchor
      constraintLessThanOrEqualToConstant : 600.0].active = YES;
  widget.autoresizingMask |= NSViewWidthSizable;

  return;
}

void
uiWidgetAlignHorizStart (uiwcont_t *uiwidget)
{
  NSView    *view;

  if (uiwidget == NULL) {
    return;
  }

  if (! uiwidget->packed) {
    fprintf (stderr, "ERR: w-al-h-s widget not packed %d %s %s\n",
        uiwidget->id, uiwcontDesc (uiwidget->wbasetype),
        uiwcontDesc (uiwidget->wtype));
    return;
  }

  view = uiwidget->uidata.widget;
  view.autoresizingMask |= NSViewMaxXMargin;
  view.needsDisplay = true;

  return;
}

void
uiWidgetAlignHorizEnd (uiwcont_t *uiwidget)
{
  NSView    *view;

  if (uiwidget == NULL) {
    return;
  }

  if (! uiwidget->packed) {
    fprintf (stderr, "ERR: w-al-h-e widget not packed %d %s %s\n",
        uiwidget->id, uiwcontDesc (uiwidget->wbasetype),
        uiwcontDesc (uiwidget->wtype));
    return;
  }

  view = uiwidget->uidata.widget;
  view.autoresizingMask |= NSViewMinXMargin;
  view.needsDisplay = YES;

  return;
}

void
uiWidgetAlignHorizCenter (uiwcont_t *uiwidget)
{
  NSView        *view;

  if (uiwidget == NULL) {
    return;
  }

  if (! uiwidget->packed) {
    fprintf (stderr, "ERR: w-al-h-c widget not packed %d %s %s\n",
        uiwidget->id, uiwcontDesc (uiwidget->wbasetype),
        uiwcontDesc (uiwidget->wtype));
    return;
  }

  view = uiwidget->uidata.widget;
  view.autoresizingMask |= NSViewMinXMargin | NSViewMaxXMargin;
  view.needsDisplay = YES;

  return;
}

void
uiWidgetAlignVertFill (uiwcont_t *uiwidget)
{
  NSView        *widget;

  if (uiwidget == NULL) {
    return;
  }

  if (! uiwidget->packed) {
    fprintf (stderr, "ERR: w-al-v-f widget not packed %d %s %s\n",
        uiwidget->id, uiwcontDesc (uiwidget->wbasetype),
        uiwcontDesc (uiwidget->wtype));
    return;
  }

  widget = uiwidget->uidata.packwidget;
  [widget.heightAnchor
      constraintLessThanOrEqualToConstant : 600.0].active = YES;

  return;
}

void
uiWidgetAlignVertStart (uiwcont_t *uiwidget)
{
  NSView    *view;

  if (uiwidget == NULL) {
    return;
  }

  view = uiwidget->uidata.widget;
  view.autoresizingMask |= NSViewMaxYMargin;
  view.needsDisplay = true;

  return;
}

void
uiWidgetAlignVertCenter (uiwcont_t *uiwidget)
{
  NSView        *view;

  if (uiwidget == NULL) {
    return;
  }

  view = uiwidget->uidata.widget;
  view.autoresizingMask |= NSViewMinYMargin | NSViewMaxYMargin;

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
  widget.needsDisplay = true;
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
  NSStackView   *container = NULL;

  layout = uiwidget->uidata.layout;
  container = layout->container;

  if (container == NULL) {
    return;
  }

  container.edgeInsets = layout->margins;
  container.needsDisplay = YES;
}

