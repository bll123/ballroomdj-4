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

void
uiWidgetExpandHoriz (uiwcont_t *uiwidget)
{
#if 0
  NSView          *super;
  NSView          *container;
  macoslayout_t   *layout;

  if (uiwidget == NULL) {
    return;
  }

  container = uiwidget->uidata.packwidget;

fprintf (stderr, "  add expand-horiz constraint %d\n", uiwidget->packed);
  if (uiwidget->packed == true) {
// ### how to do this?
//    [super.leadingAnchor
//        constraintLessThanOrEqualToAnchor: container.leadingAnchor
//        constant: layout->margins.left].active = YES;
//    [super.trailingAnchor
//        constraintLessThanOrEqualToAnchor: container.trailingAnchor
//        constant: layout->margins.right].active = YES;
  }
  [container setAutoresizingMask : NSViewWidthSizable];
  container.needsDisplay = true;
#endif
  return;
}

void
uiWidgetExpandVert (uiwcont_t *uiwidget)
{
#if 0
  NSView          *widget;
  macoslayout_t   *layout;
  NSView          *container;

  if (uiwidget == NULL) {
    return;
  }

  widget = uiwidget->uidata.widget;

  layout = uiwidget->uidata.layout;
//  container = layout->container;
  layout->expand = true;
fprintf (stderr, "  add expand-vert constraint %d\n", uiwidget->packed);
  if (uiwidget->packed == true) {
//    [widget.topAnchor
//        constraintLessThanOrEqualToAnchor: container.topAnchor
//        constant: 600.0].active = YES;
//    [widget.bottomAnchor
//        constraintLessThanOrEqualToAnchor: container.bottomAnchor
//        constant: 600.0].active = YES;
//    [widget.layoutMarginsGuide.topAnchor
//        constraintEqualToAnchor: container.layoutMarginsGuide.topAnchor].active = YES;
//    [widget.layoutMarginsGuide.bottomAnchor
//        constraintEqualToAnchor: container.layoutMarginsGuide.bottomAnchor].active = YES;
  }
  [widget setAutoresizingMask : NSViewHeightSizable];
  widget.needsDisplay = true;
#endif
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

  widget = uiwidget->uidata.widget;

  val = uiBaseMarginSz * mult;

  layout = uiwidget->uidata.layout;
  layout->margins.right = (CGFloat) val;
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
  widget.needsDisplay = true;
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
  widget.needsDisplay = true;
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
  widget.needsDisplay = true;
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

  layout = uiwidget->uidata.layout;
  layout->centered = true;

  uiWidgetUpdateLayout (uiwidget);

  return;
}

void
uiWidgetAlignVertFill (uiwcont_t *uiwidget)
{
  NSView        *view;
  macoslayout_t *layout;
  NSStackView   *stview;

  if (uiwidget == NULL) {
    return;
  }

  view = uiwidget->uidata.widget;

  layout = uiwidget->uidata.layout;
//  uiwidget->uidata.widget.needsDisplay = true;

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
  widget.needsDisplay = true;
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
  NSView        *view = NULL;
  NSStackView   *container = NULL;

  layout = uiwidget->uidata.layout;
  view = uiwidget->uidata.widget;
  container = layout->container;

  if (container == NULL) {
    return;
  }

  container.edgeInsets = layout->margins;
  view.needsDisplay = YES;
}

