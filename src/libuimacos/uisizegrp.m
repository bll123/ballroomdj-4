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

#include "uiwcont.h"

#include "ui/uiwcont-int.h"

#include "ui/uisizegrp.h"

uiwcont_t *
uiCreateSizeGroupHoriz (void)
{
  uiwcont_t   *uiwidget;

fprintf (stderr, "c-sg-h\n");
  uiwidget = uiwcontAlloc (WCONT_T_SIZE_GROUP, WCONT_T_SIZE_GROUP);
  return uiwidget;
}

void
uiSizeGroupAdd (uiwcont_t *uisg, uiwcont_t *uiwidget)
{
  NSView    *widget;
  NSView    *first;

  if (uisg == NULL || uiwidget == NULL) {
    return;
  }

  first = uisg->uidata.widget;
  widget = uiwidget->uidata.widget;

  if (first == NULL) {
    uisg->uidata.widget = widget;
    uisg->packed = uiwidget->packed;
    return;
  }

  if (! uiwcontValid (uisg, WCONT_T_SIZE_GROUP, "sg-add")) {
    return;
  }

  if (! uisg->packed) {
    fprintf (stderr, "ERR: first widget %d/%s not packed\n", uisg->wtype, uiwcontDesc (uisg->wtype));
  }
  if (! uiwidget->packed) {
    fprintf (stderr, "ERR: widget %d/%s not packed\n", uiwidget->wtype, uiwcontDesc (uiwidget->wtype));
  }
  if (! uisg->packed || ! uiwidget->packed) {
    return;
  }

  [widget.layoutMarginsGuide.widthAnchor
      constraintEqualToAnchor: first.layoutMarginsGuide.widthAnchor].active = YES;
  return;
}

