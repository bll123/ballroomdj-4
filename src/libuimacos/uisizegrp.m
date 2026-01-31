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

  first = uisg->uidata.packwidget;
  widget = uiwidget->uidata.packwidget;

  if (first == NULL) {
    uisg->uidata.widget = uiwidget->uidata.widget;
    uisg->uidata.packwidget = widget;
    uisg->packed = uiwidget->packed;
    return;
  }

  if (! uiwcontValid (uisg, WCONT_T_SIZE_GROUP, "sg-add")) {
    return;
  }

  if (! uisg->packed) {
    fprintf (stderr, "ERR : first widget %d/%s not packed\n", uisg->wtype, uiwcontDesc (uisg->wtype));
  }
  if (! uiwidget->packed) {
    fprintf (stderr, "ERR : widget %d/%s not packed\n", uiwidget->wtype, uiwcontDesc (uiwidget->wtype));
  }
  if (! uisg->packed || ! uiwidget->packed) {
    return;
  }

//  [widget.widthAnchor
//      constraintEqualToAnchor : first.widthAnchor].active = YES;

  first = uisg->uidata.widget;
  widget = uiwidget->uidata.widget;

  [widget.widthAnchor
      constraintEqualToAnchor : first.widthAnchor].active = YES;

  return;
}

