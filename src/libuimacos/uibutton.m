/*
 * Copyright 2024 Brad Lanam Pleasant Hill CA
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

#include "ui/uiui.h"
#include "ui/uiwidget.h"
#include "ui/uibutton.h"

typedef struct uibutton {
  NSImage         *image;
} uibutton_t;

@interface IButton : NSButton {
  uiwcont_t   *uiwidget;
}
- (void) setUIWidget: (uiwcont_t *) tuiwidget;
- (IBAction) OnButton1Click: (id) sender;
@end

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

@end

uiwcont_t *
uiCreateButton (callback_t *uicb, char *title, char *imagenm)
{
  uiwcont_t       *uiwidget;
  uibutton_t      *uibutton;
  uibuttonbase_t  *bbase;
  IButton         *widget = nil;

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

  uiwidget = uiwcontAlloc ();
  uiwidget->wbasetype = WCONT_T_BUTTON;
  uiwidget->wtype = WCONT_T_BUTTON;
  uiwidget->uidata.widget = widget;
  uiwidget->uidata.packwidget = widget;
  uiwidget->uiint.uibutton = uibutton;

  [widget setBezelStyle: NSBezelStyleRounded];
  [widget setTarget: widget];
  [widget setUIWidget: uiwidget];
  [widget setAction: @selector(OnButton1Click:)];
  [widget setTranslatesAutoresizingMaskIntoConstraints: NO];

  bbase = &uiwidget->uiint.uibuttonbase;
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
  return;
}

void
uiButtonSetImagePosRight (uiwcont_t *uiwidget)
{
  return;
}

void
uiButtonSetImageMarginTop (uiwcont_t *uiwidget, int margin)
{
  return;
}

void
uiButtonSetImageIcon (uiwcont_t *uiwidget, const char *nm)
{
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
  return;
}

void
uiButtonSetFlat (uiwcont_t *uiwidget)
{
  return;
}

void
uiButtonSetText (uiwcont_t *uiwidget, const char *txt)
{
  return;
}

void
uiButtonSetRepeat (uiwcont_t *uiwidget, int repeatms)
{
  return;
}

