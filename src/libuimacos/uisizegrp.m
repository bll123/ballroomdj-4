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

#include "mdebug.h"
#include "uiwcont.h"

#include "ui/uimacos-int.h"
#include "ui/uiwcont-int.h"

#include "ui/uisizegrp.h"

uiwcont_t *
uiCreateSizeGroupHoriz (uiwcont_t *box)
{
  uiwcont_t           *uiwidget;
  macoslayout_t       *layout;
  NSView              *boxview;
  NSLayoutConstraint  *c;

  boxview = box->uidata.packwidget;

  uiwidget = uiwcontAlloc (WCONT_T_SIZE_GROUP, WCONT_T_SIZE_GROUP);
  uiwidget->uidata.widget = box;

  /* there is no actual widget associated with a size group on macos */
  /* only an nslayoutguide is needed */
  layout = uiwidget->uidata.layout;
  layout->sizegroup = [[NSLayoutGuide alloc] init];

fprintf (stderr, "sg: create\n");
  [boxview addLayoutGuide : layout->sizegroup];
  c = [layout->sizegroup.topAnchor
      constraintEqualToAnchor : boxview.topAnchor];
  uiWidgetUpdateConstraint ("sg_a", c, boxview, boxview);
  c = [layout->sizegroup.bottomAnchor
      constraintEqualToAnchor : boxview.bottomAnchor];
  uiWidgetUpdateConstraint ("sg_b", c, boxview, boxview);
  c = [layout->sizegroup.leadingAnchor
      constraintEqualToAnchor : boxview.leadingAnchor];
  uiWidgetUpdateConstraint ("sg_c", c, boxview, boxview);
  c = [layout->sizegroup.trailingAnchor
      constraintLessThanOrEqualToAnchor : boxview.trailingAnchor];
  uiWidgetUpdateConstraint ("sg_d", c, boxview, boxview);

  return uiwidget;
}

void
uiSizeGroupAdd (uiwcont_t *uisg, uiwcont_t *uiwidget)
{
  NSView              *container;
  macoslayout_t       *layout;
  NSLayoutConstraint  *c;

  if (uisg == NULL || uiwidget == NULL) {
    return;
  }

  if (! uiwcontValid (uisg, WCONT_T_SIZE_GROUP, "sg-add")) {
    return;
  }

  if (! uiwidget->packed) {
    fprintf (stderr, "ERR : widget %d/%s not packed\n", uiwidget->wtype, uiwcontDesc (uiwidget->wtype));
    return;
  }

fprintf (stderr, "sg: set constraints\n");
  layout = uiwidget->uidata.layout;
  container = layout->container;
fprintf (stderr, "  l:%d c:%d\n", layout != NULL, container != NULL);

  layout = uisg->uidata.layout;
fprintf (stderr, "  l:%d\n", layout != NULL);
fprintf (stderr, "  sg:%d\n", uisg != NULL);

// ### this only works for widgets that are distributed vertically
  c = [container.leadingAnchor
      constraintEqualToAnchor : layout->sizegroup.leadingAnchor];
  uiWidgetUpdateConstraint ("sg_e", c, container, uisg->uidata.widget);
  c = [container.trailingAnchor
      constraintEqualToAnchor : layout->sizegroup.trailingAnchor];
  uiWidgetUpdateConstraint ("sg_f", c, container, uisg->uidata.widget);

  return;
}

