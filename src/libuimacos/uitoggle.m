/*
 * Copyright 2024-2026 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

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
#include "ui/uitoggle.h"

typedef struct uitoggleint {
  callback_t      *cb;
  NSImage         *image;
  NSImage         *altimage;
} uitoggleint_t;

static void uiToggleButtonSetImage (uiwcont_t *uiwidget);
static uitoggleint_t * uiToggleButtonInit (void);

uiwcont_t *
uiCreateCheckButton (const char *txt, int value)
{
  uiwcont_t   *uiwidget;
  uitoggleint_t  *uitoggle;
  IButton     *widget = nil;

  uitoggle = uiToggleButtonInit ();

  widget = [[IButton alloc] init];
  widget.state = NSControlStateValueOff;
  if (value) {
    widget.state = NSControlStateValueOn;
  }
  widget.buttonType = NSButtonTypeSwitch;
  if (txt != NULL) {
    widget.attributedTitle = uiAttrString (txt);
  }
  widget.translatesAutoresizingMaskIntoConstraints = NO;

  uiwidget = uiwcontAlloc (WCONT_T_BUTTON_TOGGLE, WCONT_T_BUTTON_CHKBOX);
  uiwcontSetWidget (uiwidget, widget, NULL);
  uiwidget->uiint.uitoggle = uitoggle;

  [widget setUIWidget : uiwidget];
  uiWidgetSetIdentifier (uiwidget, "chkbox");

  return uiwidget;
}

uiwcont_t *
uiCreateRadioButton (uiwcont_t *widgetgrp, const char *txt, int value)
{
  uiwcont_t   *uiwidget;
  IButton     *widget = nil;
  uitoggleint_t  *uitoggle;

  uitoggle = uiToggleButtonInit ();

  widget = [[IButton alloc] init];
  widget.buttonType = NSButtonTypeRadio;
  widget.state = NSControlStateValueOff;
  if (value) {
    widget.state = NSControlStateValueOn;
  }
  if (txt != NULL) {
    widget.attributedTitle = uiAttrString (txt);
  }
  widget.translatesAutoresizingMaskIntoConstraints= NO;

  uiwidget = uiwcontAlloc (WCONT_T_BUTTON_TOGGLE, WCONT_T_BUTTON_RADIO);
  uiwcontSetWidget (uiwidget, widget, NULL);
  uiwidget->uiint.uitoggle = uitoggle;

  [widget setUIWidget : uiwidget];
  uiWidgetSetIdentifier (uiwidget, "radio");

  return uiwidget;
}

uiwcont_t *
uiCreateToggleButton (const char *txt,
    const char *imagenm, const char *tooltiptxt, int value)
{
  uiwcont_t   *uiwidget;
  IButton     *widget = nil;
  uitoggleint_t  *uitoggle;

  uitoggle = uiToggleButtonInit ();

  widget = [[IButton alloc] init];
  widget.buttonType = NSButtonTypePushOnPushOff;
  widget.state = NSControlStateValueOff;
  if (value) {
    widget.state = NSControlStateValueOn;
  }
  if (txt != NULL) {
    widget.attributedTitle = uiAttrString (txt);
    widget.imagePosition = NSImageTrailing;
  }
  widget.translatesAutoresizingMaskIntoConstraints = NO;

  if (tooltiptxt != NULL) {
    widget.toolTip = [NSString stringWithUTF8String : tooltiptxt];
  }

  if (imagenm != NULL) {
    NSString    *ns;
    NSImage     *nsimage;
    char        tbuff [BDJ4_PATH_MAX];

    /* relative path */
    pathbldMakePath (tbuff, sizeof (tbuff), imagenm, BDJ4_IMG_SVG_EXT,
        PATHBLD_MP_DREL_IMG | PATHBLD_MP_USEIDX);
    ns = [NSString stringWithUTF8String : imagenm];
    nsimage = [[NSImage alloc] initWithContentsOfFile : ns];
    widget.image = nsimage;
  }

  uiwidget = uiwcontAlloc (WCONT_T_BUTTON_TOGGLE, WCONT_T_BUTTON_TOGGLE);
  uiwcontSetWidget (uiwidget, widget, NULL);
  uiwidget->uiint.uitoggle = uitoggle;

  [widget setUIWidget : uiwidget];
  uiWidgetSetIdentifier (uiwidget, "toggle");

  return uiwidget;
}

void
uiToggleButtonFree (uiwcont_t *uiwidget)
{
  uitoggleint_t    *uitoggle;

  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON_TOGGLE, "toggle-free")) {
    return;
  }

  uitoggle = uiwidget->uiint.uitoggle;
  mdfree (uitoggle);
  return;
}

void
uiToggleButtonSetAltImage (uiwcont_t *uiwidget, const char *imagenm)
{
  uitoggleint_t  *uitoggle;

  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON_TOGGLE, "toggle-set-alt-image")) {
    return;
  }

  uitoggle = uiwidget->uiint.uitoggle;

  if (imagenm != NULL) {
    NSButton    *widget = nil;
    NSString    *ns;
    NSImage     *nsimage;
    char        tbuff [BDJ4_PATH_MAX];

    widget = uiwidget->widget;

    /* relative path */
    pathbldMakePath (tbuff, sizeof (tbuff), imagenm, BDJ4_IMG_SVG_EXT,
        PATHBLD_MP_DREL_IMG | PATHBLD_MP_USEIDX);
    ns = [NSString stringWithUTF8String: imagenm];
    nsimage = [[NSImage alloc] initWithContentsOfFile: ns];
    widget.alternateImage= nsimage;
  }

  uiToggleButtonSetImage (uiwidget);
}

void
uiToggleButtonSetCallback (uiwcont_t *uiwidget, callback_t *uicb)
{
  return;
}

void
uiToggleButtonSetFocusCallback (uiwcont_t *uiwidget, callback_t *uicb)
{
  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON_TOGGLE, "tb-set-cb")) {
    return;
  }
}

void
uiToggleButtonSetText (uiwcont_t *uiwidget, const char *txt)
{
  return;
}

bool
uiToggleButtonIsActive (uiwcont_t *uiwidget)
{
  NSButton  *widget = nil;
  bool      active;

  widget = uiwidget->widget;
  active = false;
  if (widget.state == NSControlStateValueOn) {
    active = true;
  }
  return active;
}

void
uiToggleButtonSetValue (uiwcont_t *uiwidget, int state)
{
  NSButton    *widget = nil;

  widget = uiwidget->widget;
  widget.state = NSControlStateValueOff;
  if (state) {
    widget.state = NSControlStateValueOn;
  }

  return;
}

void
uiToggleButtonEllipsize (uiwcont_t *uiwidget)
{
  return;
}

static void
uiToggleButtonSetImage (uiwcont_t *uiwidget)
{
  NSButton    *widget = nil;
  uitoggleint_t  *uitoggle;
  bool        active;

  widget = uiwidget->widget;
  uitoggle = uiwidget->uiint.uitoggle;
  if (uitoggle == NULL) {
    return;
  }

  active = false;
  if (widget.state == NSControlStateValueOn) {
    active = true;
  }

  if ((! active || uitoggle->altimage == NULL) &&
      uitoggle->image != NULL) {
    widget.image= uitoggle->image;
  }
  if (active && uitoggle->altimage != NULL) {
    widget.image= uitoggle->altimage;
  }
}

static uitoggleint_t *
uiToggleButtonInit (void)
{
  uitoggleint_t    *uitoggle;

  uitoggle = mdmalloc (sizeof (uitoggleint_t));
  uitoggle->image = NULL;
  uitoggle->altimage = NULL;

  return uitoggle;
}
