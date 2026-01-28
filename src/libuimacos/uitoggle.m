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
#include "pathbld.h"
#include "uiwcont.h"

#include "ui/uiwcont-int.h"

#include "ui/uiui.h"
#include "ui/uitoggle.h"

uiwcont_t *
uiCreateCheckButton (const char *txt, int value)
{
  uiwcont_t   *uiwidget;
  NSButton    *widget = nil;

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

  uiwidget = uiwcontAlloc (WCONT_T_TOGGLE_BUTTON, WCONT_T_CHECK_BOX);
  uiwcontSetWidget (uiwidget, widget, NULL);

  return uiwidget;
}

uiwcont_t *
uiCreateRadioButton (uiwcont_t *widgetgrp, const char *txt, int value)
{
  uiwcont_t   *uiwidget;
  NSButton    *widget = nil;

fprintf (stderr, "c-radio-bt\n");
//  gtk_widget_set_margin_top (widget, uiBaseMarginSz);
//  gtk_widget_set_margin_start (widget, uiBaseMarginSz);
  widget = [[NSButton alloc] init];
  [widget setButtonType: NSButtonTypeRadio];
  [widget setState: NSControlStateValueOff];
  if (value) {
    [widget setState: NSControlStateValueOn];
  }
  if (txt != NULL) {
    [widget setTitle: [NSString stringWithUTF8String: txt]];
  }
  [widget setTranslatesAutoresizingMaskIntoConstraints: NO];

  uiwidget = uiwcontAlloc (WCONT_T_TOGGLE_BUTTON, WCONT_T_RADIO_BUTTON);
  uiwcontSetWidget (uiwidget, widget, NULL);

  return uiwidget;
}

uiwcont_t *
uiCreateToggleButton (const char *txt,
    const char *imagenm, const char *tooltiptxt, uiwcont_t *image, int value)
{
  uiwcont_t   *uiwidget;
  NSButton    *widget = nil;

fprintf (stderr, "c-toggle-bt %s\n", imagenm);
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
    widget.imagePosition = NSImageRight;
  }
  [widget setTranslatesAutoresizingMaskIntoConstraints: NO];

  if (tooltiptxt != NULL) {
    [widget setToolTip: [NSString stringWithUTF8String: tooltiptxt]];
  }
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
  }
  if (image != NULL) {
fprintf (stderr, "   image-from-wcont %s %s\n", uiwcontDesc (image->wbasetype), uiwcontDesc (image->wtype));
    [widget setImage: image->uidata.widget];
  }

  uiwidget = uiwcontAlloc (WCONT_T_TOGGLE_BUTTON, WCONT_T_RADIO_BUTTON);
  uiwcontSetWidget (uiwidget, widget, NULL);

  return uiwidget;
}

void
uiToggleButtonSetCallback (uiwcont_t *uiwidget, callback_t *uicb)
{
  return;
}

void
uiToggleButtonSetImage (uiwcont_t *uiwidget, uiwcont_t *image)
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
  return false;
}

void
uiToggleButtonSetValue (uiwcont_t *uiwidget, int state)
{
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
