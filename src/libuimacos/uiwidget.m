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
#include "uiwcont.h"

#include "ui/uiwcont-int.h"

#include "ui/uiui.h"
#include "ui/uiwidget.h"

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
  [widget.widthAnchor
      constraintEqualToAnchor: [widget superview].widthAnchor].active = YES;
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
  [widget.heightAnchor
      constraintEqualToAnchor: [widget superview].heightAnchor].active = YES;
  return;
}

void
uiWidgetSetAllMargins (uiwcont_t *uiwidget, int mult)
{
  NSView    *widget;
  NSView    *cont;
  int       val;

  if (uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

  widget = uiwidget->uidata.widget;
  if ([widget superview] == nil) {
//    fprintf (stderr, "ERR: all: widget is not packed\n");
    return;
  }
  cont = [widget superview];
  val = uiBaseMarginSz * mult;
  [widget.leadingAnchor
      constraintEqualToAnchor: widget.layoutMarginsGuide.leadingAnchor
      constant: val].active = YES;
  [widget.trailingAnchor
      constraintEqualToAnchor: widget.layoutMarginsGuide.trailingAnchor
      constant: (- val)].active = YES;
  [widget.topAnchor
      constraintEqualToAnchor: widget.layoutMarginsGuide.topAnchor
      constant: val].active = YES;
  [widget.bottomAnchor
      constraintEqualToAnchor: widget.layoutMarginsGuide.bottomAnchor
      constant: (- val)].active = YES;

  return;
}

void
uiWidgetSetMarginTop (uiwcont_t *uiwidget, int mult)
{
  NSView    *widget;
  NSView    *cont;
  int       val;

  if (uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

  widget = uiwidget->uidata.widget;
  if ([widget superview] == nil) {
//    fprintf (stderr, "ERR: top: widget is not packed\n");
    return;
  }
  cont = [widget superview];
  val = uiBaseMarginSz * mult;
  [widget.topAnchor
      constraintEqualToAnchor: widget.layoutMarginsGuide.topAnchor
      constant: val].active = YES;

  return;
}

void
uiWidgetSetMarginBottom (uiwcont_t *uiwidget, int mult)
{
  NSView    *widget;
  NSView    *cont;
  int       val;

  if (uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

  widget = uiwidget->uidata.widget;
  if ([widget superview] == nil) {
//    fprintf (stderr, "ERR: bottom: widget is not packed\n");
    return;
  }
  cont = [widget superview];
  val = uiBaseMarginSz * mult;
  [widget.bottomAnchor
      constraintEqualToAnchor: widget.layoutMarginsGuide.bottomAnchor
      constant: (- val)].active = YES;

  return;
}

void
uiWidgetSetMarginStart (uiwcont_t *uiwidget, int mult)
{
  NSView    *widget;
  NSView    *cont;
  int       val;

  if (uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

  widget = uiwidget->uidata.widget;
  if ([widget superview] == nil) {
//    fprintf (stderr, "ERR: start: widget is not packed\n");
    return;
  }
  cont = [widget superview];
  val = uiBaseMarginSz * mult;
  [widget.leadingAnchor
      constraintEqualToAnchor: widget.layoutMarginsGuide.leadingAnchor
      constant: val].active = YES;

  return;
}

void
uiWidgetSetMarginEnd (uiwcont_t *uiwidget, int mult)
{
  NSView    *widget;
  NSView    *cont;
  int       val;

  if (uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

  widget = uiwidget->uidata.widget;
  if ([widget superview] == nil) {
//    fprintf (stderr, "ERR: end: widget is not packed\n");
    return;
  }
  cont = [widget superview];
  val = uiBaseMarginSz * mult;
  [widget.trailingAnchor
      constraintEqualToAnchor: widget.layoutMarginsGuide.trailingAnchor
      constant: (- val)].active = YES;

  return;
}

void
uiWidgetAlignHorizFill (uiwcont_t *uiwidget)
{
  return;
}

void
uiWidgetAlignHorizStart (uiwcont_t *uiwidget)
{
  return;
}

void
uiWidgetAlignHorizEnd (uiwcont_t *uiwidget)
{
  return;
}

void
uiWidgetAlignHorizCenter (uiwcont_t *uiwidget)
{
  return;
}

void
uiWidgetAlignVertFill (uiwcont_t *uiwidget)
{
  return;
}

void
uiWidgetAlignVertStart (uiwcont_t *uiwidget)
{
  return;
}

void
uiWidgetAlignVertCenter (uiwcont_t *uiwidget)
{
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
