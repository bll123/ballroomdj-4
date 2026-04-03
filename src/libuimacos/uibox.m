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

typedef struct uibox {
  long        ident;
  int         count;
  int         endcount;
  bool        expandchildren;
} uibox_t;

static long     gident = 0;

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

uiwcont_t *
ruiCreateVertBox (void)
{
  return uiCreateBox (NSUserInterfaceLayoutOrientationVertical);
}

uiwcont_t *
ruiCreateHorizBox (void)
{
  return uiCreateBox (NSUserInterfaceLayoutOrientationHorizontal);
}

void
uiBoxPostProcess (uiwcont_t *uibox)
{
  uiboxbase_t *boxbase = NULL;
  uibox_t     *uiboxint = NULL;
  nlist_t     *wlist = NULL;
  nlistidx_t  iteridx;
  nlistidx_t  key;
  uiwcont_t   *prioruiwidget = NULL;
  NSView      *box = NULL;

  if (! uiwcontValid (uibox, WCONT_T_BOX, "box-post-process")) {
    return;
  }

  uiboxint = uibox->uiint.uibox;
  boxbase = &uibox->uiint.uiboxbase;
fprintf (stderr, "    post-process %d count: %d %d/%s\n", uiboxint->ident, (int) nlistGetCount (wlist), uibox->wtype, uiwcontDesc (uibox->wtype));

  if (boxbase->startcount == 0 && boxbase->endcount == 0) {
    return;
  }

  if (boxbase->postprocess) {
    fprintf (stderr, "ERR: box post-process twice\n");
    return;
  }

  wlist = boxbase->startlist;

  box = uibox->uidata.widget;

  nlistStartIterator (wlist, &iteridx);
  while ((key = nlistIterateKey (wlist, &iteridx)) >= 0) {
    uiwcont_t       *uiwidget;
    NSView          *widget;
    NSView          *container;
    macoslayout_t   *layout;
    NSView          *pwidget = NULL;
    NSView          *pcont = NULL;
    macoslayout_t   *playout = NULL;

fprintf (stderr, "    pp: key: %d\n", key);
    uiwidget = nlistGetData (wlist, key);
    if (uiwidget == NULL) {
fprintf (stderr, "    pp: null\n");
      continue;
    }

    widget = uiwidget->uidata.widget;
    container = [widget superview];
    layout = uiwidget->uidata.layout;
fprintf (stderr, "    pp: %d/%s\n", uiwidget->wtype, uiwcontDesc (uiwidget->wtype));

    if (prioruiwidget != NULL) {
fprintf (stderr, "    pp: prior: %d/%s\n", prioruiwidget->wtype, uiwcontDesc (prioruiwidget->wtype));
      pwidget = prioruiwidget->uidata.widget;
      playout = prioruiwidget->uidata.layout;
      pcont = playout->container;
    }

    if (uibox->wtype == WCONT_T_VBOX) {
fprintf (stderr, "    pp: in-vbox\n");
fprintf (stderr, "    pp: v: lead/trail\n");
      if (uiboxint->expandchildren) {
        widget.autoresizingMask |= NSViewHeightSizable;
      }
      if (layout->expandhoriz) {
        [layout->container.leadingAnchor
            constraintEqualToAnchor: box.leadingAnchor].active = YES;
        [layout->container.trailingAnchor
            constraintEqualToAnchor: box.trailingAnchor].active = YES;
      }
      if (layout->centered) {
fprintf (stderr, "    pp: v: centered\n");
        widget.autoresizingMask |= NSViewMinYMargin;
        widget.autoresizingMask |= NSViewMaxYMargin;
      }
      if (layout->expandvert) {
fprintf (stderr, "    pp: v: expand-vert\n");
        widget.autoresizingMask |= NSViewHeightSizable;
        if (pcont != NULL) {
          [layout->container.topAnchor
              constraintEqualToAnchor: pcont.bottomAnchor].active = YES;
        } else {
          /* attach to the box */
          [layout->container.topAnchor
              constraintEqualToAnchor: box.topAnchor].active = YES;
        }
      }
      if (playout != NULL && playout->expandvert) {
fprintf (stderr, "    pp: v: prior expand-vert\n");
        [playout->container.bottomAnchor
            constraintEqualToAnchor: container.topAnchor].active = YES;
      }
    }
    if (uibox->wtype == WCONT_T_HBOX) {
fprintf (stderr, "    pp: in-hbox\n");
fprintf (stderr, "    pp: h: top/bottom\n");
      if (uiboxint->expandchildren) {
        widget.autoresizingMask |= NSViewWidthSizable;
      }
      if (layout->expandvert) {
        [layout->container.topAnchor
            constraintEqualToAnchor: box.topAnchor].active = YES;
        [layout->container.bottomAnchor
            constraintEqualToAnchor: box.bottomAnchor].active = YES;
      }

      if (layout->centered) {
fprintf (stderr, "    pp: h: centered\n");
        widget.autoresizingMask |= NSViewMinXMargin;
        widget.autoresizingMask |= NSViewMaxXMargin;
      }
      if (layout->expandhoriz) {
fprintf (stderr, "    pp: h: expand-horiz\n");
        widget.autoresizingMask |= NSViewWidthSizable;
        if (pcont != NULL) {
          [layout->container.leadingAnchor
              constraintEqualToAnchor: pcont.trailingAnchor].active = YES;
        } else {
          /* attach to the box */
          [layout->container.leadingAnchor
              constraintEqualToAnchor: box.leadingAnchor].active = YES;
        }
      }
      if (playout != NULL && playout->expandhoriz) {
fprintf (stderr, "    pp: h: prior: expand-horiz\n");
        [playout->container.trailingAnchor
            constraintEqualToAnchor: container.leadingAnchor].active = YES;
      }
    }

    prioruiwidget = uiwidget;
  }

  if (prioruiwidget != NULL) {
    NSView        *pwidget = NULL;
    macoslayout_t *playout = NULL;

fprintf (stderr, "    pp: last\n");
    pwidget = prioruiwidget->uidata.widget;
    playout = prioruiwidget->uidata.layout;

    if (playout->expandhoriz) {
      /* attach to the box */
fprintf (stderr, "    pp: l: expand-horiz\n");
      [playout->container.trailingAnchor
          constraintEqualToAnchor: box.trailingAnchor].active = YES;
    }
    if (playout->expandvert) {
fprintf (stderr, "    pp: l: expand-vert\n");
      /* attach to the box */
      [playout->container.bottomAnchor
          constraintEqualToAnchor: box.bottomAnchor].active = YES;
    }
  }

  boxbase->postprocess = true;

  return;
}

void
ruiBoxPackStart (uiwcont_t *uibox, uiwcont_t *uiwidget)
{
  IBox          *box;
  NSView        *widget = NULL;
  int           grav = NSStackViewGravityLeading;
  uibox_t       *uiboxint = NULL;

  if (! uiwcontValid (uibox, WCONT_T_BOX, "box-pack-start")) {
    return;
  }
  if (uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

  box = uibox->uidata.widget;
  widget = uiwidget->uidata.packwidget;
  uiboxint = uibox->uiint.uibox;

  if (uibox->wtype == WCONT_T_VBOX) {
    grav = NSStackViewGravityTop;
  }
  [box addView: widget inGravity: grav];

  uiWidgetSetMarginTop (uiwidget, 1);
  uiWidgetSetMarginStart (uiwidget, 1);
fprintf (stderr, "box: %ld p-st: %d %d/%s\n", uiboxint->ident, uiboxint->count, uiwidget->wtype, uiwcontDesc (uiwidget->wtype));
  nlistSetData (uiboxint->widgetlist, uiboxint->count, uiwidget);
  uiboxint->count += 1;
  uiwidget->packed = true;
  return;
}

void
ruiBoxPackStartExpandChildren (uiwcont_t *uibox, uiwcont_t *uiwidget)
{
  IBox          *box;
  NSView        *widget = NULL;
  int           grav = NSStackViewGravityLeading;
  uibox_t       *uiboxint;

  if (! uiwcontValid (uibox, WCONT_T_BOX, "box-pack-start-exp")) {
    return;
  }
  if (uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

  uiboxint = uibox->uiint.uibox;
  box = uibox->uidata.widget;
  widget = uiwidget->uidata.packwidget;

  if (uibox->wtype == WCONT_T_VBOX) {
    grav = NSStackViewGravityTop;
  }
  [box addView : widget inGravity : grav];

  if (uibox->uidata.layout->expandchildren &&
      uibox->wtype == WCONT_T_VBOX) {
    [box.widthAnchor constraintEqualToAnchor :
        widget.widthAnchor].active = YES;
    widget.autoresizingMask |= NSViewWidthSizable;
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
  }

  uiwidget->packed = true;
  uiWidgetUpdateLayout (uiwidget);

  return;
}

/* this uses the GTK terminology */
/* expand allows any children to expand to fill the space */
void
ruiBoxPackEnd (uiwcont_t *uibox, uiwcont_t *uiwidget)
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
  [box addView : widget inGravity : grav];

  if (uibox->uidata.layout->expandchildren &&
      uibox->wtype == WCONT_T_VBOX) {
    [box.widthAnchor constraintEqualToAnchor :
        widget.widthAnchor].active = YES;
    widget.autoresizingMask |= NSViewWidthSizable;
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

  if (uiwidget->wbasetype == WCONT_T_BOX) {
    uiwidget->uidata.layout->expandchildren = true;
  }

  uiwidget->packed = true;
  uiWidgetUpdateLayout (uiwidget);

  return;
}

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

  uiboxint = uibox->uiint.uibox;
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
    widget.autoresizingMask |= NSViewHeightSizable;
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

/* this uses the GTK terminology */
/* expand allows any children to expand to fill the space */
void
ruiBoxPackEndExpandChildren (uiwcont_t *uibox, uiwcont_t *uiwidget)
{
  IBox        *box;
  NSView      *widget = NULL;
  int         grav = NSStackViewGravityTrailing;
  uibox_t     *uiboxint = NULL;

  if (! uiwcontValid (uibox, WCONT_T_BOX, "box-pack-end-exp")) {
    return;
  }
  if (uiwidget == NULL || uiwidget->uidata.widget == NULL) {
    return;
  }

  uiboxint = uibox->uiint.uibox;
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
    widget.autoresizingMask |= NSViewHeightSizable;
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
  }

  if (uiwidget->wbasetype == WCONT_T_BOX) {
    uiwidget->uidata.layout->expandchildren = true;
  }

  uiwidget->packed = true;
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
  uibox_t     *uiboxint = NULL;
  IBox        *box = NULL;
  char        tmp [40];

  uiboxint = mdmalloc (sizeof (uibox_t));
  uiboxint->widgetlist = nlistAlloc ("box", LIST_UNORDERED, NULL);
  uiboxint->expandchildren = false;
  uiboxint->postprocess = false;
  uiboxint->count = 0;
  /* the number of widgets in a box is fairly small */
  /* this needs to a be larger value than the widgets packed into the start */
  uiboxint->endcount = 100;

  box = [[IBox alloc] init];
  [box setOrientation : orientation];
//  [box setTranslatesAutoresizingMaskIntoConstraints : NO];
  [box setDistribution : NSStackViewDistributionGravityAreas];
  box.autoresizingMask |= NSViewWidthSizable | NSViewHeightSizable;
  box.needsDisplay = true;
  box.spacing = 1.0;
  box.layerContentsRedrawPolicy = NSViewLayerContentsRedrawBeforeViewResize;

  snprintf (tmp, sizeof (tmp), "box-%ld\n", gident);
  box.identifier = [NSString stringWithUTF8String: tmp];
  uiboxint->ident = gident;
fprintf (stderr, "  c-box %ld\n", gident);
  ++gident;

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
  uiwidget->uiint.uibox = uiboxint;

  [box setIdentifier :
      [[NSNumber numberWithUnsignedInt : uiwidget->id] stringValue]];

  return uiwidget;
}

