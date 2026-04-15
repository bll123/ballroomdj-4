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

#include "ui/uiwcont-int.h"
#include "ui/uimacos-int.h"

#include "ui/uibox.h"
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
  MACOS_UI_DBG_EXP_WIDTH,
  MACOS_UI_DBG_EXP_HEIGHT,
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
  {   0.0 / 255.0,   0.0 / 255.0, 150.0 / 255.0 },
  { 150.0 / 255.0, 150.0 / 255.0,   0.0 / 255.0 },
  {   0.0 / 255.0, 150.0 / 255.0, 150.0 / 255.0 },
};

#endif

static void uiBoxProcessExpandHoriz (NSView *box, NSView *prevcont, NSView *currcont, NSView *nextcont);
static void uiBoxProcessExpandVert (NSView *box, NSView *prevcont, NSView *currcont, NSView *nextcont);
static void uiBoxProcessCenter (macoslayout_t *layout, NSView *box, NSView *prevcont, NSView *currcont, NSView *nextcont);

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
    NSView          *widget;
    NSView          *currcont;
    macoslayout_t   *layout;
    NSView          *pwidget = NULL;
    NSView          *prevcont = NULL;
    macoslayout_t   *playout = NULL;
    NSView          *nwidget = NULL;
    macoslayout_t   *nlayout = NULL;
    NSView          *nextcont = NULL;

fprintf (stderr, "    pp: curr: %d/%s\n", curr->wtype, uiwcontDesc (curr->wtype));
    widget = curr->uidata.widget;
    layout = curr->uidata.layout;
    currcont = layout->container;

    if (prev != NULL) {
fprintf (stderr, "    pp: prev: %d/%s\n", prev->wtype, uiwcontDesc (prev->wtype));
      pwidget = prev->uidata.widget;
      playout = prev->uidata.layout;
      prevcont = playout->container;
    }

    if (next != NULL) {
fprintf (stderr, "    pp: next: %d/%s\n", next->wtype, uiwcontDesc (next->wtype));
      nwidget = next->uidata.widget;
      nlayout = next->uidata.layout;
      nextcont = nlayout->container;
    }

    if (layout->expandhoriz || boxlayout->expandchildren) {
fprintf (stderr, "    pp: expand-horiz\n");
      uiBoxProcessExpandHoriz (box, prevcont, currcont, nextcont);
    }
    if (layout->expandvert || boxlayout->expandchildren) {
fprintf (stderr, "    pp: expand-vert\n");
      uiBoxProcessExpandVert (box, prevcont, currcont, nextcont);
    }
    if (layout->centerhoriz || layout->centervert) {
fprintf (stderr, "    pp: center\n");
      uiBoxProcessCenter (layout, box, prevcont, currcont, nextcont);
    }
  }

  return;
}

void
uiBoxPackStart_r (uiwcont_t *uibox, uiwcont_t *uiwidget)
{
  IBox          *box;
  NSView        *widget = NULL;
  int           grav = NSStackViewGravityLeading;

  if (! uiwcontValid (uibox, WCONT_T_BOX, "box-pack-start")) {
    return;
  }
  if (uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

fprintf (stderr, "box: %d-%d p-st: %d-%d %d/%s\n", uibox->ui_id, uibox->id, uiwidget->ui_id, uiwidget->id, uiwidget->wtype, uiwcontDesc (uiwidget->wtype));
  box = uibox->uidata.widget;
  widget = uiwidget->uidata.packwidget;
fprintf (stderr, "  bbb %d %d\n", box != NULL, widget != NULL);

  if (uibox->wtype == WCONT_T_VBOX) {
    grav = NSStackViewGravityTop;
  }
  [box addView: widget inGravity: grav];
fprintf (stderr, "  ccc\n");

  uiwidget->packed = true;
  return;
}

void
uiBoxPackStartExpandChildren_r (uiwcont_t *uibox, uiwcont_t *uiwidget)
{
  IBox            *box;
  NSView          *widget = NULL;
  macoslayout_t   *layout;

  if (! uiwcontValid (uibox, WCONT_T_BOX, "box-pack-start-exp")) {
    return;
  }
  if (uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

  uiBoxPackStart_r (uibox, uiwidget);

  box = uibox->uidata.widget;
  widget = uiwidget->uidata.packwidget;
  layout = uiwidget->uidata.layout;

  layout->expandchildren = true;
  uiWidgetUpdateLayout (uiwidget);

  uiBoxMacOSDebug (uiwidget);

  return;
}

void
uiBoxPackEnd_r (uiwcont_t *uibox, uiwcont_t *uiwidget)
{
  IBox          *box;
  NSView        *widget = NULL;
  int           grav = NSStackViewGravityTrailing;

  if (! uiwcontValid (uibox, WCONT_T_BOX, "box-pack-start-exp")) {
    return;
  }
  if (uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

  box = uibox->uidata.widget;
  widget = uiwidget->uidata.packwidget;
  if (uibox->wtype == WCONT_T_VBOX) {
    grav = NSStackViewGravityBottom;
  }
  [box insertView : widget atIndex : 0 inGravity : grav];

//  if (uiwidget->wbasetype == WCONT_T_BOX) {
//    uiwidget->uidata.layout->expandchildren = true;
//  }

fprintf (stderr, "box: %d-%d p-end: %d-%d %d/%s\n", uibox->ui_id, uibox->id, uiwidget->ui_id, uiwidget->id, uiwidget->wtype, uiwcontDesc (uiwidget->wtype));
  uiwidget->packed = true;
  uiWidgetUpdateLayout (uiwidget);

  uiBoxMacOSDebug (uiwidget);

  return;
}

/* this uses the GTK terminology */
/* expand allows any children to expand to fill the space */
void
uiBoxPackEndExpandChildren_r (uiwcont_t *uibox, uiwcont_t *uiwidget)
{
  IBox          *box;
  NSView        *widget = NULL;
  macoslayout_t *layout;

  if (! uiwcontValid (uibox, WCONT_T_BOX, "box-pack-end-exp")) {
    return;
  }
  if (uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

  uiBoxPackEnd_r (uibox, uiwidget);
  box = uibox->uidata.widget;
  widget = uiwidget->uidata.packwidget;
  layout = uiwidget->uidata.layout;

  layout->expandchildren = true;
  uiWidgetUpdateLayout (uiwidget);

  uiBoxMacOSDebug (uiwidget);

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

  box = [[IBox alloc] init];
  if (wtype == WCONT_T_VBOX) {
    box.orientation = NSUserInterfaceLayoutOrientationVertical;
    box.alignment = NSLayoutAttributeTop;
  }
  if (wtype == WCONT_T_HBOX) {
    box.orientation = NSUserInterfaceLayoutOrientationHorizontal;
    box.alignment = NSLayoutAttributeLeading;
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

  uiBoxMacOSDebug (uiwidget);

  return uiwidget;
}

static void
uiBoxProcessExpandHoriz (NSView *box,
    NSView *prevcont, NSView *currcont, NSView *nextcont)
{
  if (prevcont != NULL) {
    [currcont.leadingAnchor
        constraintEqualToAnchor: prevcont.trailingAnchor].active = YES;
  } else {
    /* attach to the box */
    [currcont.leadingAnchor
        constraintEqualToAnchor: box.leadingAnchor].active = YES;
  }

  if (nextcont != NULL) {
    [currcont.trailingAnchor
        constraintEqualToAnchor: nextcont.leadingAnchor].active = YES;
  } else {
    /* attach to the box */
    [currcont.trailingAnchor
        constraintEqualToAnchor: box.trailingAnchor].active = YES;
  }
}

static void
uiBoxProcessExpandVert (NSView *box,
    NSView *prevcont, NSView *currcont, NSView *nextcont)
{
  if (prevcont != NULL) {
    [currcont.topAnchor
        constraintEqualToAnchor: prevcont.bottomAnchor].active = YES;
  } else {
    /* attach to the box */
    [currcont.topAnchor
        constraintEqualToAnchor: box.topAnchor].active = YES;
  }

  if (nextcont != NULL) {
    [currcont.bottomAnchor
        constraintEqualToAnchor: nextcont.topAnchor].active = YES;
  } else {
    /* attach to the box */
    [currcont.bottomAnchor
        constraintEqualToAnchor: box.bottomAnchor].active = YES;
  }
}

static void
uiBoxProcessCenter (macoslayout_t *layout, NSView *box,
    NSView *prevcont, NSView *currcont, NSView *nextcont)
{
// ### this is not correct...
  if (layout->centervert) {
    [currcont.centerYAnchor
        constraintEqualToAnchor: box.centerYAnchor].active = YES;
  }
  if (layout->centerhoriz) {
    [currcont.centerXAnchor
        constraintEqualToAnchor: box.centerXAnchor].active = YES;
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
    idx = MACOS_UI_DBG_EXP_WIDTH;
  }
  if (layout->expandvert) {
    idx = MACOS_UI_DBG_EXP_HEIGHT;
  }

  [widget layer].borderColor =
      [NSColor colorWithRed : dbgcols [idx].r
          green : dbgcols [idx].g
          blue : dbgcols [idx].b
          alpha : 1.0].CGColor;
  widget.needsDisplay = YES;
#endif
}
