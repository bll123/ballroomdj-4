/*
 * Copyright 2024-2025 Brad Lanam Pleasant Hill CA
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

uiwcont_t *
uiCreateButton (const char *ident, callback_t *uicb, char *title, char *imagenm)
{
  uiwcont_t       *uiwidget;
  uibutton_t      *uibutton;
  uibuttonbase_t  *bbase;
  IButton         *widget = nil;

fprintf (stderr, "c-bt\n");
  uibutton = mdmalloc (sizeof (uibutton_t));
  uibutton->image = NULL;

  widget = [[IButton alloc] init];

  if (imagenm != NULL) {
    NSString    *ns;
    NSImage     *image;
    char        tbuff [MAXPATHLEN];

    /* relative path */
    pathbldMakePath (tbuff, sizeof (tbuff), imagenm, BDJ4_IMG_SVG_EXT,
        PATHBLD_MP_DREL_IMG | PATHBLD_MP_USEIDX);
    ns = [NSString stringWithUTF8String: imagenm];
    image = [[NSImage alloc] initWithContentsOfFile: ns];
    uibutton->image = image;
    [widget setImage: image];
    [widget setTitle:@""];
// ### not working
//    [widget addToolTipRect: widget.frame
//        owner: [NSString stringWithUTF8String: title]];
  } else {
    [widget setTitle: [NSString stringWithUTF8String: title]];
  }

  uiwidget = uiwcontAlloc (WCONT_T_BUTTON, WCONT_T_BUTTON);
  uiwcontSetWidget (uiwidget, widget, NULL);
  uiwidget->uiint.uibutton = uibutton;

  [widget setIdentifier: [NSString stringWithUTF8String: ident]];
  [widget setBezelStyle: NSBezelStyleRounded];
  [widget setTarget: widget];
  [widget setUIWidget: uiwidget];
  [widget setAction: @selector(OnButton1Click:)];
  [widget setTranslatesAutoresizingMaskIntoConstraints: NO];

#if MACOS_UI_DEBUG
  [widget setFocusRingType: NSFocusRingTypeExterior];
  [widget setWantsLayer: YES];
  [[widget layer] setBorderWidth: 2.0];
#endif

  bbase = &uiwidget->uiint.uibuttonbase;
  bbase->ident = ident;
  bbase->cb = uicb;
  bbase->presscb = callbackInit (uiButtonPressCallback,
      uiwidget, "button-repeat-press");
  bbase->releasecb = callbackInit (uiButtonReleaseCallback,
      uiwidget, "button-repeat-release");
  bbase->repeating = false;
  bbase->repeatOn = false;
  bbase->repeatMS = 250;

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
uiButtonSetImagePosRight (uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON, "button-set-image-pos-r")) {
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

