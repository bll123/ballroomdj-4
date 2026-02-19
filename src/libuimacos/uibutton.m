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

#include "bdj4.h"
#include "callback.h"
#include "mdebug.h"
#include "pathbld.h"
#include "uigeneral.h"

#include "ui/uiwcont-int.h"
#include "ui/uimacos-int.h"

#include "ui/uiui.h"
#include "ui/uiwidget.h"
#include "ui/uibutton.h"

typedef struct uibutton {
  NSImage         *image;
  NSImage         *altimage;
} uibutton_t;

@implementation IButton

- (void) setUIWidget: (uiwcont_t *) tuiwidget {
  uiwidget = tuiwidget;
}

- (IBAction) OnButton1Click: (id) sender {
  uibuttonbase_t  *bbase;

fprintf (stderr, "b: button-1 click\n");
  bbase = &uiwidget->uiint.uibuttonbase;
  if (bbase->cb != NULL) {
    callbackHandler (bbase->cb);
  }
}

- (BOOL) isFlipped {
  return YES;
}

@end

static long   gident = 0;


uiwcont_t *
uiCreateButton (callback_t *uicb, const char *title,
    const char *imagenm, const char *tooltiptxt)
{
  uiwcont_t       *uiwidget;
  uibutton_t      *uibutton;
  uibuttonbase_t  *bbase;
  IButton         *widget = nil;
  char            tmp [40];

fprintf (stderr, "c-bt\n");
  uibutton = mdmalloc (sizeof (uibutton_t));
  uibutton->image = NULL;
  uibutton->altimage = NULL;

  widget = [[IButton alloc] init];

  if (tooltiptxt != NULL) {
    ;
  }
  if (title != NULL) {
    [widget setTitle: [NSString stringWithUTF8String: title]];
  }
  if (imagenm != NULL) {
    NSString    *ns;
    NSImage     *image;
    char        tbuff [BDJ4_PATH_MAX];

    /* relative path */
    pathbldMakePath (tbuff, sizeof (tbuff), imagenm, BDJ4_IMG_SVG_EXT,
        PATHBLD_MP_DREL_IMG | PATHBLD_MP_USEIDX);
    ns = [NSString stringWithUTF8String: imagenm];
    image = [[NSImage alloc] initWithContentsOfFile: ns];
    uibutton->image = image;
    [widget setImage: image];
    widget.imagePosition = NSImageTrailing;
    if (title == NULL) {
      widget.imagePosition = NSImageOnly;
    }
    [widget setTitle:@""];
  }

  uiwidget = uiwcontAlloc (WCONT_T_BUTTON, WCONT_T_BUTTON);
  uiwcontSetWidget (uiwidget, widget, NULL);
  uiwidget->uiint.uibutton = uibutton;

  snprintf (tmp, sizeof (tmp), "button-%ld\n", gident);
  [widget setIdentifier: [NSString stringWithUTF8String: tmp]];
  ++gident;

  [widget setBezelStyle: NSBezelStyleRounded];
  [widget setTarget: widget];
  [widget setUIWidget: uiwidget];
  [widget setAction: @selector(OnButton1Click:)];
//  [widget setTranslatesAutoresizingMaskIntoConstraints: NO];

#if MACOS_UI_DEBUG
  [widget setFocusRingType: NSFocusRingTypeExterior];
  [widget setWantsLayer: YES];
  [[widget layer] setBorderWidth: 2.0];
#endif

  bbase = &uiwidget->uiint.uibuttonbase;
  bbase->cb = uicb;
  bbase->presscb = callbackInit (uiButtonPressCallback,
      uiwidget, "button-repeat-press");
  bbase->releasecb = callbackInit (uiButtonReleaseCallback,
      uiwidget, "button-repeat-release");
  bbase->repeating = false;
  bbase->repeatOn = false;
  bbase->repeatMS = 250;
  bbase->state = BUTTON_OFF;

  if (imagenm != NULL) {
    bbase->state = BUTTON_ON;    // force set of image
    uiButtonSetState (uiwidget, BUTTON_OFF);
  }

  return uiwidget;
}

void
uiButtonFree (uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON, "button-free")) {
    return;
  }

  return;
}

void
uiButtonSetImageMarginTop (uiwcont_t *uiwidget, int margin)
{
  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON, "button-set-image-margin-top")) {
    return;
  }

  return;
}

void
uiButtonSetImageIcon (uiwcont_t *uiwidget, const char *nm)
{
  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON, "button-set-image-icon")) {
    return;
  }

  return;
}

void
uiButtonSetAltImage (uiwcont_t *uiwidget, const char *imagenm)
{
  uibutton_t      *uibutton;

  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON, "button-set-alt-image")) {
    return;
  }

  uibutton = uiwidget->uiint.uibutton;
  if (uibutton == NULL) {
    return;
  }

  if (imagenm != NULL) {
    NSString    *ns;
    NSImage     *image;
    char        tbuff [BDJ4_PATH_MAX];

    /* relative path */
    pathbldMakePath (tbuff, sizeof (tbuff), imagenm, BDJ4_IMG_SVG_EXT,
        PATHBLD_MP_DREL_IMG | PATHBLD_MP_USEIDX);
    ns = [NSString stringWithUTF8String: imagenm];
    image = [[NSImage alloc] initWithContentsOfFile: ns];
    uibutton->altimage = image;
  }
}

void
uiButtonSetState (uiwcont_t *uiwidget, uibuttonstate_t newstate)
{
  uibutton_t      *uibutton;
  IButton         *widget = nil;
  uibuttonbase_t  *bbase;

  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON, "button-set-alt-image")) {
    return;
  }

  uibutton = uiwidget->uiint.uibutton;
  if (uibutton == NULL) {
    return;
  }
  if (uibutton->image == NULL) {
    return;
  }

  bbase = &uiwidget->uiint.uibuttonbase;
  if (bbase == NULL) {
    return;
  }

  if (bbase->state == newstate) {
    return;
  }

  bbase->state = newstate;
  widget = uiwidget->uidata.widget;

  if (bbase->state == BUTTON_OFF && uibutton->image != NULL) {
    [widget setImage: uibutton->image];
  }
  if (bbase->state == BUTTON_ON && uibutton->altimage != NULL) {
    [widget setImage: uibutton->altimage];
  }
}

void
uiButtonAlignLeft (uiwcont_t *uiwidget)
{
  IButton   *button;

  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON, "button-align-left")) {
    return;
  }

  button = uiwidget->uidata.widget;
  [button setAlignment: NSTextAlignmentNatural];
  return;
}

void
uiButtonSetReliefNone (uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON, "button-set-relief-none")) {
    return;
  }

  return;
}

void
uiButtonSetFlat (uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON, "button-set-flat")) {
    return;
  }

  return;
}

void
uiButtonSetText (uiwcont_t *uiwidget, const char *txt)
{
  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON, "button-set-text")) {
    return;
  }

  return;
}

void
uiButtonSetRepeat (uiwcont_t *uiwidget, int repeatms)
{
  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON, "button-set-repeat")) {
    return;
  }

  return;
}

