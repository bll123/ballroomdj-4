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

#include "ui/uiwcont-int.h"

#include "ui/uiimage.h"
#include "ui/uiui.h"
#include "ui/uiwidget.h"
#include "ui/uiswitch.h"

typedef struct uiswitch {
  int       junk;
} uiswitch_t;

uiwcont_t *
uiCreateSwitch (int value)
{
  uiwcont_t   *uiwidget;
  NSButton    *widget = nil;
  NSImage     *nsimage;
  char        tbuff [BDJ4_PATH_MAX];

  widget = [[NSButton alloc] init];
  [widget setButtonType : NSButtonTypePushOnPushOff];
  [widget setState : NSControlStateValueOff];
  if (value) {
    [widget setState : NSControlStateValueOn];
  }
  [widget setTranslatesAutoresizingMaskIntoConstraints : NO];

  /* relative path */
  pathbldMakePath (tbuff, sizeof (tbuff), "switch-off", BDJ4_IMG_SVG_EXT,
      PATHBLD_MP_DREL_IMG | PATHBLD_MP_USEIDX);
  nsimage = [[NSImage alloc] initWithContentsOfFile :
      [NSString stringWithUTF8String : tbuff]];
  [widget setImage : nsimage];

  pathbldMakePath (tbuff, sizeof (tbuff), "switch-on", BDJ4_IMG_SVG_EXT,
      PATHBLD_MP_DREL_IMG | PATHBLD_MP_USEIDX);
  nsimage = [[NSImage alloc] initWithContentsOfFile :
      [NSString stringWithUTF8String : tbuff]];
  [widget setAlternateImage : nsimage];

  uiwidget = uiwcontAlloc (WCONT_T_TOGGLE_BUTTON, WCONT_T_RADIO_BUTTON);
  uiwcontSetWidget (uiwidget, widget, NULL);

  [widget setIdentifier :
      [[NSNumber numberWithUnsignedInt : uiwidget->id] stringValue]];

  return uiwidget;
}

void
uiSwitchFree (uiwcont_t *uiwidget)
{
  return;
}

void
uiSwitchSetValue (uiwcont_t *uiwidget, int value)
{
  return;
}

int
uiSwitchGetValue (uiwcont_t *uiwidget)
{
  return 0;
}

void
uiSwitchSetCallback (uiwcont_t *uiwidget, callback_t *uicb)
{
  return;
}
