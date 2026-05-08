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
#include "uiwcont.h"

#include "ui/uimacos-int.h"
#include "ui/uiwcont-int.h"

#include "ui/uiui.h"
#include "ui/uiwidget.h"
#include "ui/uibutton.h"

typedef struct uibuttonint {
  NSImage         *image;
  NSImage         *altimage;
} uibuttonint_t;

@implementation IButton

- (void) setUIWidget : (uiwcont_t *) tuiwidget {
  uiwidget = tuiwidget;
}

- (void) setCallback : (callback_t *) tcb {
  cb = tcb;
}

- (IBAction) OnButton1Click : (id) sender {
  if (cb != NULL) {
    callbackHandler (cb);
  }
}

- (BOOL) isFlipped {
  return YES;
}

@end

uiwcont_t *
uiCreateButton_r (callback_t *uicb, const char *txt,
    const char *imagenm, const char *tooltiptxt)
{
  uiwcont_t       *uiwidget;
  uibuttonint_t    *uibutton;
  IButton         *widget = nil;

  uibutton = mdmalloc (sizeof (uibuttonint_t));
  uibutton->image = NULL;
  uibutton->altimage = NULL;

  widget = [[IButton alloc] init];

  if (txt != NULL) {
    widget.title= [NSString stringWithUTF8String: txt];
  }
  if (tooltiptxt != NULL) {
    widget.toolTip= [NSString stringWithUTF8String : tooltiptxt];
  }
  if (imagenm != NULL) {
    NSString    *ns;
    NSImage     *image;
    char        tbuff [BDJ4_PATH_MAX];

    /* relative path */
    pathbldMakePath (tbuff, sizeof (tbuff), imagenm, BDJ4_IMG_SVG_EXT,
        PATHBLD_MP_DREL_IMG | PATHBLD_MP_USEIDX);
// why is this not working?
// the toggle button handles a .svg
//fprintf (stderr, "button-image: %s\n", tbuff);
    ns = [NSString stringWithUTF8String : imagenm];
    image = [[NSImage alloc] initWithContentsOfFile : ns];
    uibutton->image = image;
    widget.image = image;
    if (txt == NULL) {
      widget.imagePosition = NSImageOnly;
    } else {
      widget.imagePosition = NSImageTrailing;
    }
  } else {
    widget.imagePosition = NSNoImage;
  }

  uiwidget = uiwcontAlloc (WCONT_T_BUTTON, WCONT_T_BUTTON);
  uiwcontSetWidget (uiwidget, widget, NULL);
  uiwidget->uiint.uibutton = uibutton;

  uiWidgetSetIdentifier (uiwidget, "button");

//  widget.bezelStyle = NSBezelStyleRounded;
  widget.target = widget;
  widget.uIWidget = uiwidget;
  widget.callback = uicb;
  [widget setAction : @selector(OnButton1Click : )];
  widget.translatesAutoresizingMaskIntoConstraints = NO;
  widget.bordered = YES;
  widget.needsDisplay = true;

  return uiwidget;
}

void
uiButtonFree_r (uiwcont_t *uiwidget)
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
  uibuttonint_t      *uibutton;

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
//fprintf (stderr, "button-alt-image: %s\n", tbuff);
    ns = [NSString stringWithUTF8String: imagenm];
    image = [[NSImage alloc] initWithContentsOfFile: ns];
    uibutton->altimage = image;
  }
}

void
uiButtonSetFocusCallback (uiwcont_t *uiwidget, callback_t *uicb)
{
  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON, "button-set-focus-cb")) {
    return;
  }
}

void
uiButtonSetState (uiwcont_t *uiwidget, uibuttonstate_t newstate)
{
  uibuttonint_t   *uibutton;
  uibuttongeneric_t   *bgen;
  IButton         *widget = nil;

  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON, "button-set-alt-image")) {
    return;
  }

  uibutton = uiwidget->uiint.uibutton;
  bgen = uiwidget->uibuttongen;
  if (uibutton == NULL) {
    return;
  }
  if (uibutton->image == NULL) {
    return;
  }

  if (bgen->state == newstate) {
    return;
  }

  bgen->state = newstate;
  widget = uiwidget->widget;

  if (bgen->state == BUTTON_OFF && uibutton->image != NULL) {
    widget.image= uibutton->image;
  }
  if (bgen->state == BUTTON_ON && uibutton->altimage != NULL) {
    widget.image= uibutton->altimage;
  }
  widget.needsDisplay = true;
}

void
uiButtonAlignLeft (uiwcont_t *uiwidget)
{
  IButton   *button;

  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON, "button-align-left")) {
    return;
  }

  button = uiwidget->widget;
  button.alignment = NSTextAlignmentNatural;
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
  IButton   *button = nil;

  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON, "button-set-flat")) {
    return;
  }

  button = uiwidget->widget;

  button.bordered = NO;
  button.bezelStyle = NSBezelStyleToolbar;
  button.needsDisplay = YES;

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
  uibuttongeneric_t    *bgen;

  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON, "button-set-repeat")) {
    return;
  }

  bgen = uiwidget->uibuttongen;

  bgen->repeatMS = repeatms;
  bgen->repeatOn = true;

  return;
}

