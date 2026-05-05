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
#include "oslocale.h"
#include "uiwcont.h"

#include "ui/uiwcont-int.h"
#include "ui/uimacos-int.h"

#include "ui/uiui.h"
#include "ui/uiwidget.h"

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
  macoslayout_t   *layout;

  widget = uiwidget->packwidget;
//  [widget.widthAnchor
//      constraintLessThanOrEqualToConstant : 600.0].active = YES;

  layout = uiwidget->uidata.layout;
  layout->expandhoriz = true;
  return;
}

/* this uses the GTK terminology */
/* set the widget to take any horizontal space */
void
uiWidgetExpandVert (uiwcont_t *uiwidget)
{
  NSView          *widget;
  macoslayout_t   *layout;

  widget = uiwidget->packwidget;
//  [widget.heightAnchor
//      constraintLessThanOrEqualToConstant : 600.0].active = YES;

  layout = uiwidget->uidata.layout;
  layout->expandvert = true;
  return;
}

void
uiWidgetSetAllMargins (uiwcont_t *uiwidget, int mult)
{
  NSView        *widget;
  double        val;
  macoslayout_t *layout;

  if (uiwidget == NULL || uiwidget->widget == NULL) {
    return;
  }

  widget = uiwidget->widget;

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

  if (uiwidget == NULL || uiwidget->widget == NULL) {
    return;
  }

  widget = uiwidget->widget;

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

  if (uiwidget == NULL || uiwidget->widget == NULL) {
    return;
  }

  widget = uiwidget->widget;

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

  if (uiwidget == NULL || uiwidget->widget == NULL) {
    return;
  }

  widget = uiwidget->widget;

  val = (double) (uiBaseMarginSz * mult);

  layout = uiwidget->uidata.layout;
  if (guisetup.direction == TEXT_DIR_RTL) {
    layout->margins.right = (CGFloat) val;
  } else {
    layout->margins.left = (CGFloat) val;
  }
  uiWidgetUpdateLayout (uiwidget);

  return;
}

void
uiWidgetSetMarginEnd (uiwcont_t *uiwidget, int mult)
{
  NSView        *widget;
  int           val;
  macoslayout_t *layout;

  if (uiwidget == NULL || uiwidget->widget == NULL) {
    return;
  }

  widget = uiwidget->widget;

  val = uiBaseMarginSz * mult;

  layout = uiwidget->uidata.layout;
  if (guisetup.direction == TEXT_DIR_RTL) {
    layout->margins.left = (CGFloat) val;
  } else {
    layout->margins.right = (CGFloat) val;
  }
  uiWidgetUpdateLayout (uiwidget);

  return;
}

/* this uses the GTK terminology */
/* controls how the widget handles extra space allocated to it */
void
uiWidgetAlignHorizFill (uiwcont_t *uiwidget)
{
  macoslayout_t *layout;

  if (uiwidget == NULL) {
    return;
  }

  layout = uiwidget->uidata.layout;
  layout->horizfill = true;

  return;
}

void
uiWidgetAlignHorizStart (uiwcont_t *uiwidget)
{
  macoslayout_t *layout;

  if (uiwidget == NULL) {
    return;
  }

  layout = uiwidget->uidata.layout;
  layout->horizstart = true;

  return;
}

void
uiWidgetAlignHorizEnd (uiwcont_t *uiwidget)
{
  macoslayout_t *layout;

  if (uiwidget == NULL) {
    return;
  }

  layout = uiwidget->uidata.layout;
  layout->horizend = true;

  return;
}

void
uiWidgetAlignHorizCenter (uiwcont_t *uiwidget)
{
  macoslayout_t   *layout;

  if (uiwidget == NULL) {
    return;
  }

  layout = uiwidget->uidata.layout;
  layout->horizcenter = true;

  return;
}

void
uiWidgetAlignVertFill (uiwcont_t *uiwidget)
{
  macoslayout_t *layout;

  if (uiwidget == NULL) {
    return;
  }

  layout = uiwidget->uidata.layout;
  layout->vertfill = true;

  return;
}

void
uiWidgetAlignVertStart (uiwcont_t *uiwidget)
{
  macoslayout_t *layout;

  if (uiwidget == NULL) {
    return;
  }

  layout = uiwidget->uidata.layout;
  layout->vertstart = true;

  return;
}

void
uiWidgetAlignVertCenter (uiwcont_t *uiwidget)
{
  macoslayout_t   *layout;

  if (uiwidget == NULL) {
    return;
  }

  layout = uiwidget->uidata.layout;
  layout->vertcenter = true;

  return;
}

void
uiWidgetAlignVertBaseline (uiwcont_t *uiwidget)
{
  macoslayout_t   *layout;

  if (uiwidget == NULL) {
    return;
  }

  layout = uiwidget->uidata.layout;
  layout->vertbaseline = true;

  return;
}

void
uiWidgetAlignVertEnd (uiwcont_t *uiwidget)
{
  macoslayout_t   *layout;

  if (uiwidget == NULL) {
    return;
  }

  layout = uiwidget->uidata.layout;
  layout->vertend = true;
}

void
uiWidgetDisableFocus (uiwcont_t *uiwidget)
{
  return;
}

void
uiWidgetHide (uiwcont_t *uiwidget)
{
  NSView        *widget = NULL;

  if (uiwidget == NULL) {
    return;
  }

  widget = uiwidget->packwidget;
  if (widget == NULL) {
    widget = uiwidget->widget;
  }
  if (widget == NULL) {
    return;
  }
  widget.hidden = true;
}

void
uiWidgetShow (uiwcont_t *uiwidget)
{
  NSView        *widget = NULL;

  if (uiwidget == NULL) {
    return;
  }

// ### see also nsstackview / view detachment and hiding
  widget = uiwidget->packwidget;
  if (widget == NULL) {
    widget = uiwidget->widget;
  }
  if (widget == NULL) {
    return;
  }
  widget.hidden = false;
}

void
uiWidgetShowAll (uiwcont_t *uiwidget)
{
  NSView        *widget = NULL;

  if (uiwidget == NULL) {
    return;
  }
  if (uiwidget->wbasetype == WCONT_T_WINDOW) {
    return;
  }

  widget = uiwidget->packwidget;
  widget.hidden = false;
  widget.needsDisplay = true;
}

void
uiWidgetSetSizeRequest (uiwcont_t *uiwidget, int width, int height)
{
  if (uiwidget == NULL) {
    return;
  }

  return;
}

void
uiWidgetGetPosition (uiwcont_t *uiwidget, int *x, int *y)
{
  if (uiwidget == NULL) {
    return;
  }

  return;
}

void
uiWidgetSetClass (uiwcont_t *uiwidget, const char *class)
{
  if (uiwidget == NULL) {
    return;
  }

  return;
}

void
uiWidgetClearClass (uiwcont_t *uiwidget, const char *class)
{
  if (uiwidget == NULL) {
    return;
  }

  return;
}

void
uiWidgetSetTooltip (uiwcont_t *uiwidget, const char *tooltip)
{
  if (uiwidget == NULL) {
    return;
  }

  return;
}

void
uiWidgetEnableFocus (uiwcont_t *uiwidget)
{
  if (uiwidget == NULL) {
    return;
  }

  return;
}

void
uiWidgetGrabFocus (uiwcont_t *uiwidget)
{
  if (uiwidget == NULL) {
    return;
  }

  return;
}

bool
uiWidgetIsMapped (uiwcont_t *uiwidget)
{
  if (uiwidget == NULL) {
    return false;
  }

  return true;
}

void
uiWidgetSetSizeChgCallback (uiwcont_t *uiwidget, callback_t *uicb)
{
  if (uiwidget == NULL) {
    return;
  }

  return;
}

void
uiWidgetSetEnterCallback (uiwcont_t *uiwidget, callback_t *uicb)
{
  if (uiwidget == NULL) {
    return;
  }

  return;
}

/* support routines */

void
uiWidgetSetIdentifier (uiwcont_t *uiwidget, const char *tag)
{
  NSView          *widget;
  macoslayout_t   *layout;
  char            tmp [40];

  snprintf (tmp, sizeof (tmp), "w-%s-%" PRIu32 "-%" PRIu32,
      tag, uiwidget->ui_id, uiwidget->id);
  widget = uiwidget->widget;
  widget.identifier = [NSString stringWithUTF8String: tmp];

  if (uiwidget->packwidget != uiwidget->widget) {
    widget = uiwidget->packwidget;
    snprintf (tmp, sizeof (tmp), "p-%s-%" PRIu32 "-%" PRIu32,
        tag, uiwidget->ui_id, uiwidget->id);
    widget.identifier = [NSString stringWithUTF8String: tmp];
  }

  if (uiwidget->wtype != WCONT_T_WINDOW) {
    snprintf (tmp, sizeof (tmp), "c-%s-%" PRIu32 "-%" PRIu32,
        tag, uiwidget->ui_id, uiwidget->id);
    layout = uiwidget->uidata.layout;
    widget = layout->container;
    if (widget != NULL) {
      widget.identifier = [NSString stringWithUTF8String: tmp];
    }
  }
}

void
uiWidgetUpdateLayout (uiwcont_t *uiwidget)
{
  macoslayout_t *layout = NULL;
  NSStackView   *container = NULL;

  if (uiwidget == NULL) {
    return;
  }

  layout = uiwidget->uidata.layout;
  container = layout->container;

  if (container == NULL) {
    return;
  }

  container.edgeInsets = layout->margins;
  container.needsDisplay = YES;
}

void
uiWidgetUpdateConstraint (const char *tag,
    NSLayoutConstraint *c, NSView *va, NSView *vb)
{
  char                  tmp [200];

// fprintf (stderr, "upd-c: %s\n", tag);
  c.active = YES;
  snprintf (tmp, sizeof (tmp), "%s:%s:%s", tag,
      [va.identifier UTF8String], [vb.identifier UTF8String]);
  c.identifier = [NSString stringWithUTF8String: tmp];
}
