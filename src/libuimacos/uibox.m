/*
 * Copyright 2024-2026 Brad Lanam Pleasant Hill CA
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

#include "mdebug.h"
#include "nlist.h"
#include "uigeneral.h"    // debug flag
#include "uiwcont.h"

#include "ui/uimacos-int.h"
#include "ui/uiwcont-int.h"

#include "ui/uibox.h"
#include "ui/uigrid.h"
#include "ui/uiwidget.h"

@interface IBox : NSStackView {}
- (BOOL) isFlipped;
@end

@implementation IBox
- (BOOL) isFlipped {
  return YES;
}
@end

static uiwcont_t * uiCreateBox (uiwconttype_t wtype);
static void uiBoxMacOSDebug (uiwcont_t *uiwidget);

#if MACOS_UI_DEBUG

enum {
  MACOS_UI_DBG_WINDOW,
  MACOS_UI_DBG_EXP_CHILDREN,
  MACOS_UI_DBG_NORM,
  MACOS_UI_DBG_EXP_HORIZ,
  MACOS_UI_DBG_EXP_VERT,
  MACOS_UI_DBG_COLS,
};

typedef struct dbgcol {
  double  r;
  double  g;
  double  b;
} dbgcol_t;

static dbgcol_t dbgcols [MACOS_UI_DBG_COLS] = {
  { 150.0 / 255.0,   0.0 / 255.0,   0.0 / 255.0 },
  {   0.0 / 255.0, 150.0 / 255.0,   0.0 / 255.0 },
  {  20.0 / 255.0,  20.0 / 255.0, 150.0 / 255.0 },
  { 150.0 / 255.0, 150.0 / 255.0,   0.0 / 255.0 },
  {   0.0 / 255.0, 150.0 / 255.0, 150.0 / 255.0 },
};

#endif

typedef struct uibox {
  uiwcont_t *grid_s;
  uiwcont_t *grid_t;
} uibox_t;

static void uiBoxProcessExpandHoriz (uiwconttype_t wtype, NSView *box, NSView *pcont, NSView *cview, NSView *ncont);
static void uiBoxProcessExpandVert (uiwconttype_t wtype, NSView *box, NSView *pcont, NSView *cview, NSView *ncont);
static void uiBoxProcessCenter (uiwconttype_t wtype, macoslayout_t *layout, NSView *box, NSView *pcont, NSView *cview, NSView *ncont);

uiwcont_t *
uiCreateVertBox_r (void)
{
  return uiCreateBox (WCONT_T_VBOX);
}

uiwcont_t *
uiCreateHorizBox_r (void)
{
  return uiCreateBox (WCONT_T_HBOX);
}

void
uiBoxFree_r (uiwcont_t *uiwidget)
{
  uibox_t   *uibox;

  uibox = uiwidget->uiint.uibox;
  mdfree (uibox);
}

void
uiBoxPostProcess_r (uiwcont_t *uibox,
    uiwcont_t *prev, uiwcont_t *curr, uiwcont_t *next)
{
  uiboxbase_t   *boxbase = NULL;
  NSView        *box = NULL;
  macoslayout_t *boxlayout;

  if (! uiwcontValid (uibox, WCONT_T_BOX, "box-post-process")) {
    return;
  }
  if (curr == NULL) {
fprintf (stderr, "    pp: null-curr\n");
    return;
  }

  boxbase = &uibox->uiint.uiboxbase;

  if (boxbase->startcount == 0 && boxbase->endcount == 0) {
fprintf (stderr, "    pp: no-children\n");
    return;
  }

  if (boxbase->postprocess) {
    fprintf (stderr, "ERR: box post-process twice\n");
    return;
  }

  box = uibox->uidata.widget;
  boxlayout = uibox->uidata.layout;

  {
    NSView          *ccont;
    macoslayout_t   *clayout;
    NSView          *pcont = NULL;
    macoslayout_t   *playout = NULL;
    macoslayout_t   *nlayout = NULL;
    NSView          *ncont = NULL;

    clayout = curr->uidata.layout;
    ccont = clayout->container;

    if (clayout == NULL) {
fprintf (stderr, "    pp: null-layout\n");
      return;
    }

    if (prev != NULL) {
fprintf (stderr, "    pp: prev: %d/%s\n", prev->wtype, uiwcontDesc (prev->wtype));
      playout = prev->uidata.layout;
      pcont = playout->container;
    }

fprintf (stderr, "    pp: curr: %d/%s\n", curr->wtype, uiwcontDesc (curr->wtype));

    if (next != NULL) {
fprintf (stderr, "    pp: next: %d/%s\n", next->wtype, uiwcontDesc (next->wtype));
      nlayout = next->uidata.layout;
      ncont = nlayout->container;
    }

    if (clayout->expandhoriz || boxlayout->expandchildren) {
fprintf (stderr, "    pp: expand-horiz\n");
      uiBoxProcessExpandHoriz (uibox->wtype, box, pcont, ccont, ncont);
    }

    if (clayout->expandvert || boxlayout->expandchildren) {
fprintf (stderr, "    pp: expand-vert\n");
      uiBoxProcessExpandVert (uibox->wtype, box, pcont, ccont, ncont);
    }

    if (clayout->horizcenter || clayout->vertcenter) {
fprintf (stderr, "    pp: center\n");
      uiBoxProcessCenter (uibox->wtype, clayout, box, pcont, ccont, ncont);
    }

    uiBoxMacOSDebug (uibox);
  }

  return;
}

void
uiBoxPackStart_r (uiwcont_t *uibox, uiwcont_t *uiwidget)
{
  IBox          *box;
  uibox_t       *uiboxint;

  if (! uiwcontValid (uibox, WCONT_T_BOX, "box-pack-start")) {
    return;
  }
  if (uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

fprintf (stderr, "box: %d-%d p-st: %d-%d %d/%s\n", uibox->ui_id, uibox->id, uiwidget->ui_id, uiwidget->id, uiwidget->wtype, uiwcontDesc (uiwidget->wtype));
  box = uibox->uidata.widget;

  uiboxint = uibox->uiint.uibox;
  if (uibox->wtype == WCONT_T_VBOX) {
    uiGridNewRow (uiboxint->grid_s);
  }
  uiGridPack (uiboxint->grid_s, uiwidget);

  uiwidget->packed = true;
  return;
}

void
uiBoxPackStartExpandChildren_r (uiwcont_t *uibox, uiwcont_t *uiwidget)
{
  macoslayout_t   *layout;

  if (! uiwcontValid (uibox, WCONT_T_BOX, "box-pack-start-exp")) {
    return;
  }
  if (uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

  uiBoxPackStart_r (uibox, uiwidget);

  layout = uiwidget->uidata.layout;
  layout->expandchildren = true;

  return;
}

void
uiBoxPackEnd_r (uiwcont_t *uibox, uiwcont_t *uiwidget)
{
  int           grav = NSStackViewGravityTrailing;
  uibox_t       *uiboxint;
  IBox          *box;

  if (! uiwcontValid (uibox, WCONT_T_BOX, "box-pack-start-exp")) {
    return;
  }
  if (uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

  box = uibox->uidata.widget;
  uiboxint = uibox->uiint.uibox;

  if (uiboxint->grid_t == NULL) {
    uiboxint->grid_t = uiCreateGrid ();
    if (uibox->wtype == WCONT_T_VBOX) {
      grav = NSStackViewGravityBottom;
    }
    [box insertView : uiboxint->grid_t->uidata.packwidget
        atIndex : 0 inGravity : grav];
  }

fprintf (stderr, "box: %d-%d p-end: %d-%d %d/%s\n", uibox->ui_id, uibox->id, uiwidget->ui_id, uiwidget->id, uiwidget->wtype, uiwcontDesc (uiwidget->wtype));
  if (uibox->wtype == WCONT_T_VBOX) {
    grav = NSStackViewGravityBottom;
  }
  if (uibox->wtype == WCONT_T_VBOX) {
    uiGridNewRow (uiboxint->grid_t);
  }
  uiGridPack (uiboxint->grid_t, uiwidget);

  uiwidget->packed = true;

  return;
}

/* this uses the GTK terminology */
/* expand allows any children to expand to fill the space */
void
uiBoxPackEndExpandChildren_r (uiwcont_t *uibox, uiwcont_t *uiwidget)
{
  macoslayout_t *layout;

  if (! uiwcontValid (uibox, WCONT_T_BOX, "box-pack-end-exp")) {
    return;
  }
  if (uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

  uiBoxPackEnd_r (uibox, uiwidget);

  layout = uiwidget->uidata.layout;
  layout->expandchildren = true;

  return;
}

void
uiBoxSetSizeChgCallback (uiwcont_t *uiwindow, callback_t *uicb)
{
  if (! uiwcontValid (uiwindow, WCONT_T_BOX, "box-set-size-chg-cb")) {
    return;
  }

  return;
}

/* internal routines */

static uiwcont_t *
uiCreateBox (uiwconttype_t wtype)
{
  uiwcont_t   *uiwidget = NULL;
  IBox        *box = NULL;
  int         grav = NSStackViewGravityLeading;
  uibox_t     *uiboxint;

  box = [[IBox alloc] init];
  if (wtype == WCONT_T_VBOX) {
    box.orientation = NSUserInterfaceLayoutOrientationVertical;
    box.alignment = NSLayoutAttributeLeading;
    grav = NSStackViewGravityTop;
  }
  if (wtype == WCONT_T_HBOX) {
    box.orientation = NSUserInterfaceLayoutOrientationHorizontal;
    box.alignment = NSLayoutAttributeTop;
  }
  box.translatesAutoresizingMaskIntoConstraints = NO;
  box.distribution = NSStackViewDistributionGravityAreas;
  box.needsDisplay = true;
  box.spacing = 1.0;
  box.layerContentsRedrawPolicy = NSViewLayerContentsRedrawBeforeViewResize;

#if MACOS_UI_DEBUG
  box.focusRingType = NSFocusRingTypeExterior;
  box.wantsLayer = YES;
  [box layer].borderWidth = 2.0;
#endif

  uiwidget = uiwcontAlloc (WCONT_T_BOX, wtype);
  uiwcontSetWidget (uiwidget, box, NULL);

  if (wtype == WCONT_T_HBOX) {
    uiWidgetSetIdentifier (uiwidget, "hbox");
  }
  if (wtype == WCONT_T_VBOX) {
    uiWidgetSetIdentifier (uiwidget, "vbox");
  }

  uiwidget->uiint.uibox = mdmalloc (sizeof (uibox_t));
  uiboxint = uiwidget->uiint.uibox;
  uiboxint->grid_s = uiCreateGrid ();
  uiboxint->grid_t = NULL;
  [box addView: uiboxint->grid_s->uidata.widget inGravity: grav];

  return uiwidget;
}

static void
uiBoxProcessExpandHoriz (uiwconttype_t wtype, NSView *box,
    NSView *pcont, NSView *ccont, NSView *ncont)
{
  NSLayoutConstraint    *c;

  /* a widget that is set to expand horizontally within a vbox */
  /* is attached to both edges of the box */
  /* a widget that is set to expand horizontally within a hbox */
  /* is attached to its neighboring widgets */

  if (wtype == WCONT_T_HBOX) {
fprintf (stderr, "    pp: eh: hbox\n");
    if (pcont != NULL) {
//      c = [pcont.trailingAnchor
//          constraintEqualToAnchor: ccont.leadingAnchor
//          constant : 1.0];
//      uiWidgetUpdateConstraint ("eh_hbox_a", c, pcont, ccont);
    } else {
      /* attach to the box */
      c = [box.leadingAnchor
          constraintEqualToAnchor: ccont.leadingAnchor];
      uiWidgetUpdateConstraint ("eh_hbox_b", c, box, ccont);
    }

    if (ncont != NULL) {
//      c = [ccont.trailingAnchor
//          constraintEqualToAnchor: ncont.leadingAnchor];
//      uiWidgetUpdateConstraint ("eh_hbox_c", c, ccont, ncont);
    } else {
      /* attach to the box */
      c = [ccont.trailingAnchor
          constraintEqualToAnchor: box.trailingAnchor];
      uiWidgetUpdateConstraint ("eh_hbox_d", c, ccont, box);
    }
  }

  if (wtype == WCONT_T_VBOX) {
fprintf (stderr, "    pp: eh: vbox\n");
    c = [box.leadingAnchor
        constraintEqualToAnchor: ccont.leadingAnchor];
    uiWidgetUpdateConstraint ("eh_vbox_a", c, box, ccont);
    c = [ccont.trailingAnchor
        constraintEqualToAnchor: box.trailingAnchor];
    uiWidgetUpdateConstraint ("eh_vbox_b", c, ccont, box);
  }

  pcont.needsDisplay = YES;
  ncont.needsDisplay = YES;
  box.needsDisplay = YES;
  ccont.needsDisplay = YES;
}

static void
uiBoxProcessExpandVert (uiwconttype_t wtype, NSView *box,
    NSView *pcont, NSView *ccont, NSView *ncont)
{
  NSLayoutConstraint    *c;

  /* a widget that is set to expand vertically within a vbox */
  /* is attached to its neighboring widgets */
  /* a widget that is set to expand vertically within a hbox */
  /* is attached to both edges of the box */

  if (wtype == WCONT_T_VBOX) {
fprintf (stderr, "    pp: ev: vbox\n");
    if (pcont != NULL) {
//      c = [pcont.bottomAnchor
//          constraintEqualToAnchor: ccont.topAnchor
//          constant : 1.0];
//      uiWidgetUpdateConstraint ("ev_vbox_a", c, pcont, ccont);
    } else {
      /* attach to the box */
      c = [box.topAnchor
          constraintEqualToAnchor: ccont.topAnchor];
      uiWidgetUpdateConstraint ("ev_vbox_b", c, box, ccont);
    }

    if (ncont != NULL) {
//      c = [ccont.bottomAnchor
//          constraintEqualToAnchor: ncont.topAnchor
//          constant : -1.0];
//      uiWidgetUpdateConstraint ("ev_vbox_c", c, ccont, ncont);
    } else {
      /* attach to the box */
      c = [ccont.bottomAnchor
          constraintEqualToAnchor: box.bottomAnchor];
      uiWidgetUpdateConstraint ("ev_vbox_d", c, ccont, box);
    }
  }

  if (wtype == WCONT_T_HBOX) {
fprintf (stderr, "    pp: ev: hbox\n");
    c = [box.topAnchor
        constraintEqualToAnchor: ccont.topAnchor];
    uiWidgetUpdateConstraint ("ev_hbox_a", c, box, ccont);
    c = [ccont.bottomAnchor
        constraintEqualToAnchor: box.bottomAnchor];
    uiWidgetUpdateConstraint ("ev_hbox_b", c, ccont, box);
  }

  pcont.needsDisplay = YES;
  ncont.needsDisplay = YES;
  box.needsDisplay = YES;
  ccont.needsDisplay = YES;
}

static void
uiBoxProcessCenter (uiwconttype_t wtype, macoslayout_t *layout, NSView *box,
    NSView *pcont, NSView *ccont, NSView *ncont)
{
// ### this is not correct...
// this probably should be in widget-update-layout
  if (layout->vertcenter) {
//    [ccont.centerYAnchor
//        constraintEqualToAnchor: box.centerYAnchor].active = YES;
  }
  if (layout->horizcenter) {
//    [ccont.centerXAnchor
//        constraintEqualToAnchor: box.centerXAnchor].active = YES;
  }
}

static void
uiBoxMacOSDebug (uiwcont_t *uiwidget)
{
  NSView          *widget;
  macoslayout_t   *layout;
  int             idx = MACOS_UI_DBG_NORM;

#if MACOS_UI_DEBUG
  if (uiwidget->wbasetype != WCONT_T_BOX) {
    return;
  }

  widget = uiwidget->uidata.widget;
  layout = uiwidget->uidata.layout;
  if (layout->expandchildren) {
    idx = MACOS_UI_DBG_EXP_CHILDREN;
  }
  if (layout->expandhoriz) {
    idx = MACOS_UI_DBG_EXP_HORIZ;
  }
  if (layout->expandvert) {
    idx = MACOS_UI_DBG_EXP_VERT;
  }

  [widget layer].borderColor =
      [NSColor colorWithRed : dbgcols [idx].r
          green : dbgcols [idx].g
          blue : dbgcols [idx].b
          alpha : 1.0].CGColor;
  widget.needsDisplay = YES;
#endif
}
