/*
 * Copyright 2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include <Cocoa/Cocoa.h>
#import <Foundation/NSObject.h>

#include "uiwcont.h"

#include "ui/uiwcont-int.h"

#include "ui/uibox.h"

static uiwcont_t * uiCreateBox (int orientation);

uiwcont_t *
uiCreateVertBox (void)
{
  return uiCreateBox (NSUserInterfaceLayoutOrientationVertical);
}

uiwcont_t *
uiCreateHorizBox (void)
{
  return uiCreateBox (NSUserInterfaceLayoutOrientationHorizontal);
}

void
uiBoxPackStart (uiwcont_t *uibox, uiwcont_t *uiwidget)
{
  NSStackView   *box = uibox->uidata.widget;

  if (uiwidget == NULL) {
    return;
  }
fprintf (stderr, "box: ps\n");
  [box addSubview:uiwidget->uidata.widget];
  return;
}

void
uiBoxPackStartExpand (uiwcont_t *uibox, uiwcont_t *uiwidget)
{
  NSStackView   *box = uibox->uidata.widget;

  if (uiwidget == NULL) {
    return;
  }
fprintf (stderr, "box: pse\n");
  [box addSubview:uiwidget->uidata.widget];
  return;
}

void
uiBoxPackEnd (uiwcont_t *uibox, uiwcont_t *uiwidget)
{
  NSStackView   *box = uibox->uidata.widget;

  if (uiwidget == NULL) {
    return;
  }
fprintf (stderr, "box: pe\n");
  [box addSubview:uiwidget->uidata.widget];
  return;
}

void
uiBoxPackEndExpand (uiwcont_t *uibox, uiwcont_t *uiwidget)
{
  NSStackView   *box = uibox->uidata.widget;

  if (uiwidget == NULL) {
    return;
  }
fprintf (stderr, "box: pee\n");
  [box addSubview:uiwidget->uidata.widget];
  return;
}

void
uiBoxSetSizeChgCallback (uiwcont_t *uiwindow, callback_t *uicb)
{
  return;
}

/* internal routines */

static uiwcont_t *
uiCreateBox (int orientation)
{
  uiwcont_t   *uiwidget;
  NSStackView *box;

fprintf (stderr, "box: create %d\n", orientation);
  box = [[NSStackView alloc] autorelease];
  [box setOrientation:orientation];

  uiwidget = uiwcontAlloc ();
  uiwidget->wbasetype = WCONT_T_BOX;
  uiwidget->wtype = WCONT_T_BOX;
  uiwidget->uidata.widget = box;
  uiwidget->uidata.packwidget = box;
  return uiwidget;
}

