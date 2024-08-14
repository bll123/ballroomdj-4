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

#include "ui/uiwcont-int.h"

#include "ui/uiui.h"
#include "ui/uiwidget.h"
#include "ui/uibutton.h"

typedef struct uibutton {
  NSImage     *image;
  callback_t  *cb;
  callback_t  *presscb;
  callback_t  *releasecb;
} uibutton_t;

uiwcont_t *
uiCreateButton (callback_t *uicb, char *title, char *imagenm)
{
  uiwcont_t   *uiwidget;
  uibutton_t  *uibutton;
  NSButton    *widget = nil;

  uibutton = mdmalloc (sizeof (uibutton_t));
  uibutton->image = NULL;

  uiwidget = uiwcontAlloc ();
  uiwidget->wbasetype = WCONT_T_BUTTON;
  uiwidget->wtype = WCONT_T_BUTTON;

//  gtk_widget_set_margin_top (widget, uiBaseMarginSz);
//  gtk_widget_set_margin_start (widget, uiBaseMarginSz);
  if (imagenm != NULL) {
    NSString    *ns;
    NSImage     *image;
    char        tbuff [MAXPATHLEN];

    /* relative path */
    pathbldMakePath (tbuff, sizeof (tbuff), imagenm, BDJ4_IMG_SVG_EXT,
        PATHBLD_MP_DREL_IMG | PATHBLD_MP_USEIDX);
    ns = [NSString stringWithUTF8String: imagenm];
    image = [[NSImage alloc] initWithContentsOfFile: ns];
//    gtk_widget_set_tooltip_text (widget, title);
    uibutton->image = image;
    widget = [[NSButton alloc] init];
    [[widget init] setImage: image];
    [widget setTitle:@""];
  } else {
    widget = [[NSButton alloc] init];
    [widget setTitle: [NSString stringWithUTF8String: title]];
  }
//  if (uicb != NULL) {
//    g_signal_connect (widget, "clicked",
//        G_CALLBACK (uiButtonSignalHandler), uibutton);
//  }

  [widget setAutoresizingMask:NSViewMinXMargin | NSViewMinYMargin];
  [widget setBezelStyle:NSBezelStyleRounded];
  [widget setTarget:widget];
  [widget setAction:@selector(OnButton1Click:)];

  uibutton->cb = uicb;
//  uibutton->presscb = callbackInit (uiButtonPressCallback,
//      uiwidget, "button-repeat-press");
//  uibutton->releasecb = callbackInit (uiButtonReleaseCallback,
//      uiwidget, "button-repeat-release");
//  uibutton->repeating = false;
//  uibutton->repeatOn = false;
//  uibutton->repeatMS = 250;

  uiwidget->uidata.widget = widget;
  uiwidget->uidata.packwidget = widget;
  uiwidget->uiint.uibutton = uibutton;

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

bool
uiButtonCheckRepeat (uiwcont_t *uiwidget)
{
  return false;
}
