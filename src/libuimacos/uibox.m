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

@implementation IBox
- (BOOL) isFlipped {
  return YES;
}
@end

typedef struct uibox {
  nlist_t     *widgetlist;
  long        ident;
  int         count;
  int         endcount;
  bool        expandchildren;
  bool        postprocess;
} uibox_t;

static long     gident = 0;

static uiwcont_t * uiCreateBox (int orientation);

uiwcont_t *
uiCreateVertBox (void)
{
fprintf (stderr, "c-vbox\n");
  return uiCreateBox (NSUserInterfaceLayoutOrientationVertical);
}

uiwcont_t *
uiCreateHorizBox (void)
{
fprintf (stderr, "c-hbox\n");
  return uiCreateBox (NSUserInterfaceLayoutOrientationHorizontal);
}

void
uiBoxFree (uiwcont_t *uibox)
{
  if (! uiwcontValid (uibox, WCONT_T_BOX, "box-free")) {
    return;
  }
}

void
uiBoxPostProcess (uiwcont_t *uibox)
{
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
  wlist = uiboxint->widgetlist;
  nlistSort (wlist);
fprintf (stderr, "    post-process %d count: %d %d/%s\n", uiboxint->ident, (int) nlistGetCount (wlist), uibox->wtype, uiwcontDesc (uibox->wtype));

  if (nlistGetCount (wlist) == 0) {
    return;
  }

  if (uiboxint->postprocess) {
    fprintf (stderr, "ERR: box post-process twice\n");
    return;
  }

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

  uiboxint->postprocess = true;

  return;
}

void
uiBoxPackStart (uiwcont_t *uibox, uiwcont_t *uiwidget)
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
uiBoxPackStartExpand (uiwcont_t *uibox, uiwcont_t *uiwidget)
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
  [box addView: widget inGravity: grav];

  uiWidgetSetMarginTop (uiwidget, 1);
  uiWidgetSetMarginStart (uiwidget, 1);
fprintf (stderr, "box: %ld p-st-exp: %d %d/%s\n", uiboxint->ident, uiboxint->count, uiwidget->wtype, uiwcontDesc (uiwidget->wtype));
  nlistSetData (uiboxint->widgetlist, uiboxint->count, uiwidget);
  uiboxint->count += 1;
  uiwidget->packed = true;
  uiboxint->expandchildren = true;
  return;
}

void
uiBoxPackEnd (uiwcont_t *uibox, uiwcont_t *uiwidget)
{
  IBox        *box;
  NSView      *widget = NULL;
  int         grav = NSStackViewGravityTrailing;
  uibox_t     *uiboxint = NULL;

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
  [box insertView: widget atIndex: 0 inGravity: grav];

  uiWidgetSetMarginTop (uiwidget, 1);
  uiWidgetSetMarginStart (uiwidget, 1);
fprintf (stderr, "box: %ld p-end: %d %d/%s\n", uiboxint->ident, uiboxint->endcount, uiwidget->wtype, uiwcontDesc (uiwidget->wtype));
  nlistSetData (uiboxint->widgetlist, uiboxint->endcount, uiwidget);
  uiboxint->endcount += 1;
  uiwidget->packed = true;
  return;
}

void
uiBoxPackEndExpand (uiwcont_t *uibox, uiwcont_t *uiwidget)
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
  [box insertView: widget atIndex: 0 inGravity: grav];

  uiWidgetSetMarginTop (uiwidget, 1);
  uiWidgetSetMarginStart (uiwidget, 1);
fprintf (stderr, "box: %ld p-end-exp: %d %d/%s\n", uiboxint->ident, uiboxint->endcount, uiwidget->wtype, uiwcontDesc (uiwidget->wtype));
  nlistSetData (uiboxint->widgetlist, uiboxint->endcount, uiwidget);
  uiboxint->endcount += 1;
  uiwidget->packed = true;
  uiboxint->expandchildren = true;
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
  [box setOrientation: orientation];
//  [box setTranslatesAutoresizingMaskIntoConstraints: NO];
  [box setDistribution: NSStackViewDistributionGravityAreas];
  box.spacing = 1.0;

  snprintf (tmp, sizeof (tmp), "box-%ld\n", gident);
  box.identifier = [NSString stringWithUTF8String: tmp];
  uiboxint->ident = gident;
fprintf (stderr, "  c-box %ld\n", gident);
  ++gident;

#if MACOS_UI_DEBUG
  [box setFocusRingType: NSFocusRingTypeExterior];
  [box setWantsLayer: YES];
  [[box layer] setBorderWidth: 2.0];
#endif

  if (orientation == NSUserInterfaceLayoutOrientationHorizontal) {
    uiwidget = uiwcontAlloc (WCONT_T_BOX, WCONT_T_HBOX);
    box.alignment = NSLayoutAttributeLeft;
  }
  if (orientation == NSUserInterfaceLayoutOrientationVertical) {
    uiwidget = uiwcontAlloc (WCONT_T_BOX, WCONT_T_VBOX);
    box.alignment = NSLayoutAttributeTop;
  }
  uiwcontSetWidget (uiwidget, box, NULL);
  uiwidget->uiint.uibox = uiboxint;

  return uiwidget;
}

