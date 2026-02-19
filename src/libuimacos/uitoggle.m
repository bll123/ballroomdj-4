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

#include "ui/uiwcont-int.h"

#include "ui/uiui.h"
#include "ui/uitoggle.h"

typedef struct uitoggle {
  callback_t      *cb;
  NSImage         *image;
  NSImage         *altimage;
} uitoggle_t;

static void uiToggleButtonSetImage (uiwcont_t *uiwidget);
static uitoggle_t * uiToggleButtonInit (void);

uiwcont_t *
uiCreateCheckButton (const char *txt, int value)
{
  uiwcont_t   *uiwidget;
  NSButton    *widget = nil;
  uitoggle_t  *uitoggle;

  uitoggle = uiToggleButtonInit ();

fprintf (stderr, "c-chk-bt\n");
//  gtk_widget_set_margin_top (widget, uiBaseMarginSz);
//  gtk_widget_set_margin_start (widget, uiBaseMarginSz);
  widget = [[NSButton alloc] init];
  [widget setState: NSControlStateValueOff];
  if (value) {
    [widget setState: NSControlStateValueOn];
  }
  [widget setButtonType: NSButtonTypeSwitch];
  if (txt != NULL) {
    [widget setTitle: [NSString stringWithUTF8String: txt]];
  }
  [widget setTranslatesAutoresizingMaskIntoConstraints: NO];

  uiwidget = uiwcontAlloc (WCONT_T_BUTTON_TOGGLE, WCONT_T_BUTTON_CHKBOX);
  uiwcontSetWidget (uiwidget, widget, NULL);
  uiwidget->uiint.uitoggle = uitoggle;

  return uiwidget;
}

uiwcont_t *
uiCreateRadioButton (uiwcont_t *widgetgrp, const char *txt, int value)
{
  uiwcont_t   *uiwidget;
  NSButton    *widget = nil;
  uitoggle_t  *uitoggle;

  uitoggle = uiToggleButtonInit ();

fprintf (stderr, "c-radio-bt\n");
//  gtk_widget_set_margin_top (widget, uiBaseMarginSz);
//  gtk_widget_set_margin_start (widget, uiBaseMarginSz);
  widget = [[NSButton alloc] init];
  [widget setState: NSControlStateValueOff];
  if (value) {
    [widget setState: NSControlStateValueOn];
  }
  [widget setButtonType: NSButtonTypeRadio];
  if (txt != NULL) {
    [widget setTitle: [NSString stringWithUTF8String: txt]];
  }
  [widget setTranslatesAutoresizingMaskIntoConstraints: NO];

  uiwidget = uiwcontAlloc (WCONT_T_BUTTON_TOGGLE, WCONT_T_BUTTON_RADIO);
  uiwcontSetWidget (uiwidget, widget, NULL);
  uiwidget->uiint.uitoggle = uitoggle;

  return uiwidget;
}

uiwcont_t *
uiCreateToggleButton (const char *txt,
    const char *imagenm, const char *tooltiptxt, int value)
{
  uiwcont_t   *uiwidget;
  NSButton    *widget = nil;
  uitoggle_t  *uitoggle;

  uitoggle = uiToggleButtonInit ();

fprintf (stderr, "c-toggle-bt\n");
//  gtk_widget_set_margin_top (widget, uiBaseMarginSz);
//  gtk_widget_set_margin_start (widget, uiBaseMarginSz);
  widget = [[NSButton alloc] init];
  [widget setButtonType: NSButtonTypePushOnPushOff];
  [widget setState: NSControlStateValueOff];
  if (value) {
    [widget setState: NSControlStateValueOn];
  }
  if (txt != NULL) {
    [widget setTitle: [NSString stringWithUTF8String: txt]];
  }
  [widget setTranslatesAutoresizingMaskIntoConstraints: NO];

//    gtk_widget_set_tooltip_text (widget, title);

  if (imagenm != NULL) {
    NSString    *ns;
    NSImage     *nsimage;
    char        tbuff [BDJ4_PATH_MAX];

    /* relative path */
    pathbldMakePath (tbuff, sizeof (tbuff), imagenm, BDJ4_IMG_SVG_EXT,
        PATHBLD_MP_DREL_IMG | PATHBLD_MP_USEIDX);
    ns = [NSString stringWithUTF8String: imagenm];
    nsimage = [[NSImage alloc] initWithContentsOfFile: ns];
    [widget setImage: nsimage];
    uitoggle->image = nsimage;
  }

  uiwidget = uiwcontAlloc (WCONT_T_BUTTON_TOGGLE, WCONT_T_BUTTON_RADIO);
  uiwcontSetWidget (uiwidget, widget, NULL);
  uiwidget->uiint.uitoggle = uitoggle;

  return uiwidget;
}

void
uiToggleButtonFree (uiwcont_t *uiwidget)
{
  uitoggle_t      *uitoggle;

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
  uitoggle_t  *uitoggle;

  if (! uiwcontValid (uiwidget, WCONT_T_BUTTON_TOGGLE, "toggle-set-alt-image")) {
    return;
  }

  uitoggle = uiwidget->uiint.uitoggle;

  if (imagenm != NULL) {
    NSButton    *widget = nil;
    NSString    *ns;
    NSImage     *nsimage;
    char        tbuff [BDJ4_PATH_MAX];

    widget = uiwidget->uidata.widget;

    /* relative path */
    pathbldMakePath (tbuff, sizeof (tbuff), imagenm, BDJ4_IMG_SVG_EXT,
        PATHBLD_MP_DREL_IMG | PATHBLD_MP_USEIDX);
    ns = [NSString stringWithUTF8String: imagenm];
    nsimage = [[NSImage alloc] initWithContentsOfFile: ns];
    [widget setAlternateImage: nsimage];
  }

  uiToggleButtonSetImage (uiwidget);
}

void
uiToggleButtonSetCallback (uiwcont_t *uiwidget, callback_t *uicb)
{
  return;
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

  widget = uiwidget->uidata.widget;
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

  widget = uiwidget->uidata.widget;
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

void
uiToggleButtonSetFocusCallback (uiwcont_t *uiwidget, callback_t *uicb)
{
  return;
}

static void
uiToggleButtonSetImage (uiwcont_t *uiwidget)
{
  NSButton    *widget = nil;
  uitoggle_t  *uitoggle;
  bool        active;

  widget = uiwidget->uidata.widget;
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
    [widget setImage: uitoggle->image];
  }
  if (active && uitoggle->altimage != NULL) {
    [widget setImage: uitoggle->altimage];
  }
}

static uitoggle_t *
uiToggleButtonInit (void)
{
  uitoggle_t    *uitoggle;

  uitoggle = mdmalloc (sizeof (uitoggle_t));
  uitoggle->image = NULL;
  uitoggle->altimage = NULL;

  return uitoggle;
}
