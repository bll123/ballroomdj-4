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

static uiwcont_t * uiCreateBox (int orientation);

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

static void uiBoxProcessExpandHoriz (NSView *box, NSView *prevcont, NSView *container, NSView *nextcont);
static void uiBoxProcessExpandVert (NSView *box, NSView *prevcont, NSView *container, NSView *nextcont);
static void uiBoxProcessCenter (NSView *box, NSView *prevcont, NSView *container, NSView *nextcont);

uiwcont_t *
uiCreateVertBox_r (void)
{
  return uiCreateBox (NSUserInterfaceLayoutOrientationVertical);
}

uiwcont_t *
uiCreateHorizBox_r (void)
{
  return uiCreateBox (NSUserInterfaceLayoutOrientationHorizontal);
}

void
uiBoxPostProcess_r (uiwcont_t *uibox,
    uiwcont_t *prev, uiwcont_t *curr, uiwcont_t *next)
{
  uiboxbase_t *boxbase = NULL;
  NSView      *box = NULL;

  if (! uiwcontValid (uibox, WCONT_T_BOX, "box-post-process")) {
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

  {
    NSView          *widget;
    NSView          *container;
    macoslayout_t   *layout;
    NSView          *pwidget = NULL;
    NSView          *prevcont = NULL;
    macoslayout_t   *playout = NULL;
    NSView          *nwidget = NULL;
    macoslayout_t   *nlayout = NULL;
    NSView          *nextcont = NULL;

fprintf (stderr, "    pp: %d/%s\n", curr->wtype, uiwcontDesc (curr->wtype));
    widget = curr->uidata.widget;
    layout = curr->uidata.layout;
    container = layout->container;

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

    if (layout->expandhoriz) {
fprintf (stderr, "    pp: expand-horiz\n");
      uiBoxProcessExpandHoriz (box, prevcont, container, nextcont);
    }
    if (layout->expandvert) {
fprintf (stderr, "    pp: expand-vert\n");
      uiBoxProcessExpandVert (box, prevcont, container, nextcont);
    }
    if (layout->centered) {
fprintf (stderr, "    pp: center\n");
      uiBoxProcessCenter (box, prevcont, container, nextcont);
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

  box = uibox->uidata.widget;
  widget = uiwidget->uidata.packwidget;

  if (uibox->wtype == WCONT_T_VBOX) {
    grav = NSStackViewGravityTop;
  }
  [box addView: widget inGravity: grav];

fprintf (stderr, "box: %d p-st: %d/%s\n", uibox->id, uiwidget->wtype, uiwcontDesc (uiwidget->wtype));
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

  if (uibox->wtype == WCONT_T_VBOX) {
    [box.widthAnchor constraintEqualToAnchor :
        widget.widthAnchor].active = YES;
  }
  if (uibox->wtype == WCONT_T_HBOX) {
    [box.heightAnchor constraintEqualToAnchor :
        widget.heightAnchor].active = YES;
  }
#if MACOS_UI_DEBUG
  if (uiwidget->wbasetype == WCONT_T_BOX) {
fprintf (stderr, "c-box: %d pack into w\n", uiwidget->id);
    widget = uiwidget->uidata.widget;
    [[widget layer] setBorderColor :
        [NSColor colorWithRed : dbgcols [MACOS_UI_DBG_NORM].r
        green : dbgcols [MACOS_UI_DBG_NORM].g
        blue : dbgcols [MACOS_UI_DBG_NORM].b
        alpha:1.0].CGColor];
    widget.needsDisplay = YES;
  }
#endif

  uiwidget->packed = true;
  layout->expandchildren = true;
  uiWidgetUpdateLayout (uiwidget);

  return;
}

void
uiBoxPackEnd_r (uiwcont_t *uibox, uiwcont_t *uiwidget)
{
  IBox          *box;
  NSView        *widget = NULL;
  int           grav = NSStackViewGravityLeading;

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
//  [box addView : widget inGravity : grav];

  if (uibox->uidata.layout->expandchildren &&
      uibox->wtype == WCONT_T_VBOX) {
    [box.widthAnchor constraintEqualToAnchor :
        widget.widthAnchor].active = YES;
//    widget.autoresizingMask |= NSViewWidthSizable;
#if MACOS_UI_DEBUG
    if (uiwidget->wbasetype == WCONT_T_BOX) {
fprintf (stderr, "c-box: %d pack into w-e\n", uiwidget->id);
      widget = uiwidget->uidata.widget;
      [[widget layer] setBorderColor :
          [NSColor colorWithRed : dbgcols [MACOS_UI_DBG_EXP_WIDTH].r
          green : dbgcols [MACOS_UI_DBG_EXP_WIDTH].g
          blue : dbgcols [MACOS_UI_DBG_EXP_WIDTH].b
          alpha:1.0].CGColor];
      widget.needsDisplay = YES;
    }
#endif
  }

//  if (uiwidget->wbasetype == WCONT_T_BOX) {
//    uiwidget->uidata.layout->expandchildren = true;
//  }

  uiwidget->packed = true;
  uiWidgetUpdateLayout (uiwidget);

  return;
}

#if 0
void
uiBoxPackEnd (uiwcont_t *uibox, uiwcont_t *uiwidget)
{
  IBox          *box;
  NSView        *widget = NULL;
  int           grav = NSStackViewGravityTrailing;

  if (! uiwcontValid (uibox, WCONT_T_BOX, "box-pack-end")) {
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

  if (uibox->uidata.layout->expandchildren &&
      uibox->wtype == WCONT_T_HBOX) {
    [box.heightAnchor constraintEqualToAnchor :
        widget.heightAnchor].active = YES;
//    widget.autoresizingMask |= NSViewHeightSizable;
#if MACOS_UI_DEBUG
    if (uiwidget->wbasetype == WCONT_T_BOX) {
fprintf (stderr, "c-box: %d pack into h\n", uiwidget->id);
      widget = uiwidget->uidata.widget;
      [[widget layer] setBorderColor :
          [NSColor colorWithRed : dbgcols [MACOS_UI_DBG_NORM].r
          green : dbgcols [MACOS_UI_DBG_NORM].g
          blue : dbgcols [MACOS_UI_DBG_NORM].b
          alpha:1.0].CGColor];
      widget.needsDisplay = YES;
    }
#endif
  }

  uiwidget->packed = true;
  uiWidgetUpdateLayout (uiwidget);

  return;
}
#endif

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

  if (uibox->uidata.layout->expandchildren &&
      uibox->wtype == WCONT_T_HBOX) {
    [box.heightAnchor constraintEqualToAnchor :
        widget.heightAnchor].active = YES;
  }

#if MACOS_UI_DEBUG
  if (uiwidget->wbasetype == WCONT_T_BOX) {
fprintf (stderr, "c-box: %d pack into h-e\n", uiwidget->id);
    widget = uiwidget->uidata.widget;
    [[widget layer] setBorderColor :
        [NSColor colorWithRed : dbgcols [MACOS_UI_DBG_EXP_HEIGHT].r
        green : dbgcols [MACOS_UI_DBG_EXP_HEIGHT].g
        blue : dbgcols [MACOS_UI_DBG_EXP_HEIGHT].b
        alpha:1.0].CGColor];
    widget.needsDisplay = YES;
  }
#endif

  uiwidget->packed = true;
  layout->expandchildren = true;
  uiWidgetUpdateLayout (uiwidget);

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
uiCreateBox (int orientation)
{
  uiwcont_t   *uiwidget = NULL;
  IBox        *box = NULL;
  char        tmp [40];

  box = [[IBox alloc] init];
  [box setOrientation : orientation];
  [box setTranslatesAutoresizingMaskIntoConstraints : NO];
  [box setDistribution : NSStackViewDistributionGravityAreas];
//  box.autoresizingMask |= NSViewWidthSizable | NSViewHeightSizable;
  box.needsDisplay = true;
  box.spacing = 1.0;
  box.layerContentsRedrawPolicy = NSViewLayerContentsRedrawBeforeViewResize;

#if MACOS_UI_DEBUG
  [box setFocusRingType : NSFocusRingTypeExterior];
  [box setWantsLayer : YES];
  [[box layer] setBorderColor :
      [NSColor colorWithRed : dbgcols [MACOS_UI_DBG_WINDOW].r
      green : dbgcols [MACOS_UI_DBG_WINDOW].g
      blue : dbgcols [MACOS_UI_DBG_WINDOW].b
      alpha:1.0].CGColor];
  [[box layer] setBorderWidth : 2.0];
#endif

  if (orientation == NSUserInterfaceLayoutOrientationHorizontal) {
    uiwidget = uiwcontAlloc (WCONT_T_BOX, WCONT_T_HBOX);
    box.alignment = NSLayoutAttributeTop;
  }
  if (orientation == NSUserInterfaceLayoutOrientationVertical) {
    uiwidget = uiwcontAlloc (WCONT_T_BOX, WCONT_T_VBOX);
    box.alignment = NSLayoutAttributeTop;
  }

  uiwcontSetWidget (uiwidget, box, NULL);

  snprintf (tmp, sizeof (tmp), "box-%" PRIu32 "-%" PRIu32 "\n",
      uiwidget->ui_id, uiwidget->id);
  box.identifier = [NSString stringWithUTF8String: tmp];

  return uiwidget;
}

static void
uiBoxProcessExpandHoriz (NSView *box,
    NSView *prevcont, NSView *container, NSView *nextcont)
{
  if (prevcont != NULL) {
    [container.leadingAnchor
        constraintEqualToAnchor: prevcont.trailingAnchor].active = YES;
  } else {
    /* attach to the box */
    [container.leadingAnchor
        constraintEqualToAnchor: box.leadingAnchor].active = YES;
  }
  if (nextcont != NULL) {
    [container.trailingAnchor
        constraintEqualToAnchor: nextcont.leadingAnchor].active = YES;
  } else {
    /* attach to the box */
    [container.trailingAnchor
        constraintEqualToAnchor: box.trailingAnchor].active = YES;
  }
}

static void
uiBoxProcessExpandVert (NSView *box,
    NSView *prevcont, NSView *container, NSView *nextcont)
{
  if (prevcont != NULL) {
    [container.topAnchor
        constraintEqualToAnchor: prevcont.bottomAnchor].active = YES;
  } else {
    /* attach to the box */
    [container.topAnchor
        constraintEqualToAnchor: box.topAnchor].active = YES;
  }

  if (nextcont != NULL) {
    [container.bottomAnchor
        constraintEqualToAnchor: nextcont.topAnchor].active = YES;
  } else {
    /* attach to the box */
    [container.bottomAnchor
        constraintEqualToAnchor: box.bottomAnchor].active = YES;
  }
}

static void
uiBoxProcessCenter (NSView *box,
    NSView *prevcont, NSView *container, NSView *nextcont)
{
  if (prevcont != NULL) {
    ;
  } else {
    ;
  }

  if (nextcont != NULL) {
    ;
  } else {
    ;
  }
}

