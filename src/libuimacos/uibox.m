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
#include "ui/uiwidget.h"

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
  MACOS_UI_DBG_EXP_BOTH,
  MACOS_UI_DBG_COLS,
};

typedef struct dbgcol {
  double  r;
  double  g;
  double  b;
} dbgcol_t;

static dbgcol_t dbgcols [MACOS_UI_DBG_COLS] = {
  { 150.0 / 255.0,   0.0 / 255.0,   0.0 / 255.0 },  // r  win
  {   0.0 / 255.0, 150.0 / 255.0,   0.0 / 255.0 },  // g  children
  {  20.0 / 255.0,  20.0 / 255.0, 150.0 / 255.0 },  // b  norm
  { 150.0 / 255.0, 150.0 / 255.0,   0.0 / 255.0 },  // y  horiz
  {   0.0 / 255.0, 150.0 / 255.0, 150.0 / 255.0 },  // lb  vert
  { 150.0 / 255.0,   0.0 / 255.0, 150.0 / 255.0 },  // magenta both
};

#endif

static void uiBoxProcessExpandHoriz (uiwcont_t *uibox, NSView *prevcont, NSView *currcont, NSView *nextcont);
static void uiBoxProcessExpandVert (uiwcont_t *uibox, NSView *prevcont, NSView *currcont, NSView *nextcont);
static void uiBoxProcessCenter (uiwconttype_t wtype, macoslayout_t *layout, NSView *box, NSView *prevcont, NSView *currcont, NSView *nextcont);

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
  return;
}

void
uiBoxPostProcess_r (uiwcont_t *uibox,
    uiwcont_t *prev, uiwcont_t *curr, uiwcont_t *next, bool last)
{
  uiboxgeneric_t    *boxint = NULL;
  NSView        *box = NULL;
  macoslayout_t *boxlayout;

  if (! uiwcontValid (uibox, WCONT_T_BOX, "box-post-process")) {
    return;
  }
  if (curr == NULL) {
    return;
  }

  boxint = uibox->uiboxgen;

  if (boxint->startcount == 0 && boxint->endcount == 0) {
    return;
  }

  if (boxint->postprocess) {
    fprintf (stderr, "ERR: box post-process twice\n");
    return;
  }

  box = uibox->widget;
  boxlayout = uibox->uidata->layout;

  {
    NSView          *cwidget;
    NSView          *currcont;
    macoslayout_t   *clayout;
    NSView          *pwidget = NULL;
    NSView          *prevcont = NULL;
    macoslayout_t   *playout = NULL;
    NSView          *nwidget = NULL;
    macoslayout_t   *nlayout = NULL;
    NSView          *nextcont = NULL;

    cwidget = curr->packwidget;
    clayout = curr->uidata->layout;
    currcont = clayout->container;

    if (clayout == NULL) {
      return;
    }

    if (prev != NULL) {
      pwidget = prev->packwidget;
      playout = prev->uidata->layout;
      prevcont = playout->container;
fprintf (stderr, "%d-%d pp: prev: %d-%d %d/%s\n", uibox->ui_id, uibox->id, prev->ui_id, prev->id, prev->wtype, uiwcontDesc (prev->wtype));
    }

fprintf (stderr, "%d-%d pp: curr: %d-%d %d/%s\n", uibox->ui_id, uibox->id, curr->ui_id, curr->id, curr->wtype, uiwcontDesc (curr->wtype));

    if (next != NULL) {
fprintf (stderr, "%d-%d pp: next: %d-%d %d/%s\n", uibox->ui_id, uibox->id, next->ui_id, next->id, next->wtype, uiwcontDesc (next->wtype));
      nwidget = next->packwidget;
      nlayout = next->uidata->layout;
      nextcont = nlayout->container;
    }

    if (clayout->expandhoriz || boxlayout->expandchildren) {
//fprintf (stderr, "    pp: expand-horiz\n");
      uiBoxProcessExpandHoriz (uibox, prevcont, currcont, nextcont);
      if (curr->wbasetype == WCONT_T_BOX) {
//        NSStackView     *currbox;

//        currbox = (NSStackView *) widget;
//        widget.autoresizingMask |= NSViewWidthSizable;
      }
    }

    if (clayout->expandvert || boxlayout->expandchildren) {
//fprintf (stderr, "    pp: expand-vert\n");
      uiBoxProcessExpandVert (uibox, prevcont, currcont, nextcont);
      if (curr->wbasetype == WCONT_T_BOX) {
//        NSStackView     *currbox;

//        currbox = (NSStackView *) widget;
//        widget.autoresizingMask |= NSViewHeightSizable;
      }
    }

    if (clayout->horizcenter || clayout->vertcenter) {
//fprintf (stderr, "    pp: center\n");
      uiBoxProcessCenter (uibox->wtype, clayout, box, prevcont, currcont, nextcont);
    }

    uiBoxMacOSDebug (uibox);
  }

  return;
}

void
uiBoxPackStart_r (uiwcont_t *uibox, uiwcont_t *uiwidget)
{
  IBox          *box;
  macoslayout_t *layout;
  int           grav = NSStackViewGravityLeading;

  if (! uiwcontValid (uibox, WCONT_T_BOX, "box-pack-start")) {
    return;
  }
  if (uiwidget == NULL || uiwidget->widget == NULL) {
    return;
  }

//fprintf (stderr, "box: %d-%d p-st: %d-%d %d/%s\n", uibox->ui_id, uibox->id, uiwidget->ui_id, uiwidget->id, uiwidget->wtype, uiwcontDesc (uiwidget->wtype));
  box = uibox->widget;
  layout = uiwidget->uidata->layout;
  if (layout == NULL || layout->container == NULL) {
    fprintf (stderr, "ERR: null container %d-%d\n", uibox->ui_id, uibox->id);
    return;
  }

  if (uibox->wtype == WCONT_T_VBOX) {
    grav = NSStackViewGravityTop;
  }
  [box addView: layout->container inGravity: grav];

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
  if (uiwidget == NULL || uiwidget->widget == NULL) {
    return;
  }

  uiBoxPackStart_r (uibox, uiwidget);

  layout = uiwidget->uidata->layout;
  layout->expandchildren = true;

  return;
}

void
uiBoxPackEnd_r (uiwcont_t *uibox, uiwcont_t *uiwidget)
{
  IBox          *box;
  macoslayout_t *layout;
  int           grav = NSStackViewGravityTrailing;

  if (! uiwcontValid (uibox, WCONT_T_BOX, "box-pack-start-exp")) {
    return;
  }
  if (uiwidget == NULL || uiwidget->widget == NULL) {
    return;
  }

  box = uibox->widget;
  layout = uiwidget->uidata->layout;

  if (uibox->wtype == WCONT_T_VBOX) {
    grav = NSStackViewGravityBottom;
  }
  [box insertView : layout->container atIndex : 0 inGravity : grav];

//fprintf (stderr, "box: %d-%d p-end: %d-%d %d/%s\n", uibox->ui_id, uibox->id, uiwidget->ui_id, uiwidget->id, uiwidget->wtype, uiwcontDesc (uiwidget->wtype));
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
  if (uiwidget == NULL || uiwidget->widget == NULL) {
    return;
  }

  uiBoxPackEnd_r (uibox, uiwidget);

  layout = uiwidget->uidata->layout;
  layout->expandchildren = true;

  return;
}

/* internal routines */

static uiwcont_t *
uiCreateBox (uiwconttype_t wtype)
{
  uiwcont_t   *uiwidget = NULL;
  IBox        *box = NULL;

  box = [[IBox alloc] init];
  if (wtype == WCONT_T_VBOX) {
    box.orientation = NSUserInterfaceLayoutOrientationVertical;
    box.alignment = NSLayoutAttributeLeading;
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

  return uiwidget;
}

static void
uiBoxProcessExpandHoriz (uiwcont_t *uibox,
    NSView *prevcont, NSView *currcont, NSView *nextcont)
{
  NSLayoutConstraint    *c;
  NSView                *box;
  macoslayout_t         *layout;
  uiwconttype_t         wtype;

  /* a widget that is set to expand horizontally within a vbox */
  /* is attached to both edges of the box */
  /* a widget that is set to expand horizontally within a hbox */
  /* is attached to its neighboring widgets */

  wtype = uibox->wtype;
  box = uibox->widget;
  layout = uibox->uidata->layout;

  [layout->container setHuggingPriority :
      NSLayoutPriorityDefaultLow
      forOrientation : NSLayoutConstraintOrientationHorizontal];

  if (wtype == WCONT_T_HBOX) {
//fprintf (stderr, "    pp: eh: hbox\n");
    if (prevcont != NULL) {
fprintf (stderr, "   eh-h: p-t <= c-l\n");
      c = [prevcont.trailingAnchor
          constraintLessThanOrEqualToAnchor : currcont.leadingAnchor];
      uiWidgetUpdateConstraint ("eh_hbox_a", c, prevcont, currcont);
    } else {
      /* attach to the box */
fprintf (stderr, "   eh-h: b-l = c-l\n");
      c = [box.leadingAnchor
          constraintEqualToAnchor : currcont.leadingAnchor];
      uiWidgetUpdateConstraint ("eh_hbox_b", c, box, currcont);
    }

    if (nextcont != NULL) {
fprintf (stderr, "   eh-h: c-t = n-l\n");
      c = [currcont.trailingAnchor
          constraintLessThanOrEqualToAnchor : nextcont.leadingAnchor];
      uiWidgetUpdateConstraint ("eh_hbox_c", c, currcont, nextcont);
    } else {
      /* attach to the box */
fprintf (stderr, "   eh-h: c-t = b-t\n");
      c = [currcont.trailingAnchor
          constraintEqualToAnchor : box.trailingAnchor];
      uiWidgetUpdateConstraint ("eh_hbox_d", c, currcont, box);
    }
  }

  if (wtype == WCONT_T_VBOX) {
//fprintf (stderr, "    pp: eh: vbox\n");
fprintf (stderr, "   eh-v: c-l = b-l\n");
    c = [box.leadingAnchor
        constraintEqualToAnchor : currcont.leadingAnchor];
    uiWidgetUpdateConstraint ("eh_vbox_a", c, box, currcont);
fprintf (stderr, "   eh-v: c-t = b-t\n");
    c = [currcont.trailingAnchor
        constraintEqualToAnchor : box.trailingAnchor];
    uiWidgetUpdateConstraint ("eh_vbox_b", c, currcont, box);
  }

  prevcont.needsDisplay = YES;
  nextcont.needsDisplay = YES;
  box.needsDisplay = YES;
  currcont.needsDisplay = YES;
}

static void
uiBoxProcessExpandVert (uiwcont_t *uibox,
    NSView *prevcont, NSView *currcont, NSView *nextcont)
{
  NSLayoutConstraint    *c;
  NSView                *box;
  macoslayout_t         *layout;
  uiwconttype_t         wtype;

  /* a widget that is set to expand vertically within a vbox */
  /* is attached to its neighboring widgets */
  /* a widget that is set to expand vertically within a hbox */
  /* is attached to both edges of the box */

  wtype = uibox->wtype;
  box = uibox->widget;
  layout = uibox->uidata->layout;

  [layout->container setHuggingPriority :
      NSLayoutPriorityDefaultLow
      forOrientation : NSLayoutConstraintOrientationVertical];

  if (wtype == WCONT_T_VBOX) {
    [layout->container setHuggingPriority :
        NSLayoutPriorityDefaultLow
        forOrientation : NSLayoutConstraintOrientationVertical];

//fprintf (stderr, "    pp: ev: vbox\n");
    if (prevcont != NULL) {
fprintf (stderr, "   ev-v: p-b <= c-t\n");
      c = [prevcont.bottomAnchor
          constraintLessThanOrEqualToAnchor: currcont.topAnchor];
      uiWidgetUpdateConstraint ("ev_vbox_a", c, prevcont, currcont);
    } else {
      /* attach to the box */
fprintf (stderr, "   ev-v: b-t = c-t\n");
      c = [box.topAnchor
          constraintEqualToAnchor: currcont.topAnchor];
      uiWidgetUpdateConstraint ("ev_vbox_b", c, box, currcont);
    }

    if (nextcont != NULL) {
fprintf (stderr, "   ev-v: c-b <= n-t\n");
      c = [currcont.bottomAnchor
          constraintLessThanOrEqualToAnchor: nextcont.topAnchor];
      uiWidgetUpdateConstraint ("ev_vbox_c", c, currcont, nextcont);
    } else {
      /* attach to the box */
fprintf (stderr, "   ev-v: b-b = c-b\n");
      c = [currcont.bottomAnchor
          constraintEqualToAnchor: box.bottomAnchor];
      uiWidgetUpdateConstraint ("ev_vbox_d", c, currcont, box);
    }
  }

  if (wtype == WCONT_T_HBOX) {
//fprintf (stderr, "    pp: ev: hbox\n");
fprintf (stderr, "   ev-h: b-t = c-t\n");
    c = [box.topAnchor
        constraintEqualToAnchor: currcont.topAnchor];
    uiWidgetUpdateConstraint ("ev_hbox_a", c, box, currcont);
fprintf (stderr, "   ev-h: c-b = b-b\n");
    c = [currcont.bottomAnchor
        constraintEqualToAnchor: box.bottomAnchor];
    uiWidgetUpdateConstraint ("ev_hbox_b", c, currcont, box);
  }

  prevcont.needsDisplay = YES;
  nextcont.needsDisplay = YES;
  box.needsDisplay = YES;
  currcont.needsDisplay = YES;
}

static void
uiBoxProcessCenter (uiwconttype_t wtype, macoslayout_t *layout, NSView *box,
    NSView *prevcont, NSView *currcont, NSView *nextcont)
{
// ### this is not correct...
// this probably should be in widget-update-layout
  if (layout->vertcenter) {
//    [currcont.centerYAnchor
//        constraintEqualToAnchor: box.centerYAnchor].active = YES;
  }
  if (layout->horizcenter) {
//    [currcont.centerXAnchor
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

  widget = uiwidget->widget;
  layout = uiwidget->uidata->layout;
  if (layout->expandchildren) {
fprintf (stderr, "exp-ch: %d-%d %d/%s\n", uiwidget->ui_id, uiwidget->id, uiwidget->wtype, uiwcontDesc (uiwidget->wtype));
    idx = MACOS_UI_DBG_EXP_CHILDREN;
  }
  if (layout->expandhoriz) {
    idx = MACOS_UI_DBG_EXP_HORIZ;
  }
  if (layout->expandvert) {
    idx = MACOS_UI_DBG_EXP_VERT;
  }
  if (layout->expandhoriz && layout->expandvert) {
    idx = MACOS_UI_DBG_EXP_BOTH;
  }

  [widget layer].borderColor =
      [NSColor colorWithRed : dbgcols [idx].r
          green : dbgcols [idx].g
          blue : dbgcols [idx].b
          alpha : 1.0].CGColor;
  widget.needsDisplay = YES;
#endif
}
