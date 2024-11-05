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
#include <stdarg.h>

#include <Cocoa/Cocoa.h>
#import <Foundation/NSObject.h>

#include "uiwcont.h"

#include "ui/uiwcont-int.h"

#include "ui/uibox.h"
#include "ui/uidialog.h"
#include "ui/uibox.h"
#include "ui/uiwidget.h"
#include "ui/uiwindow.h"

typedef struct uiselect {
  uiwcont_t   *window;
  const char  *label;
  const char  *startpath;
  const char  *dfltname;
  const char  *mimefiltername;
  const char  *mimetype;
} uiselect_t;

@interface IDWindow : NSWindow { }
@property uiwcont_t *uibox;
- (instancetype) init;
- (void) awakeFromNib;
@end

@implementation IDWindow

- (instancetype)init {

  [super initWithContentRect:NSMakeRect(10, 10, 100, 100)
      styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
          NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable
      backing:NSBackingStoreBuffered
      defer:NO];
  [self setIsVisible:YES];
  return self;
}

- (void)awakeFromNib {
  IDWindow*  w = self;
  NSRect    f;
  NSStackView *box;
  NSView    *clg;
  NSSize    nssz;

  box = [w contentView];
// ### this doesn't seem to work.
// but leave it here just in case.
  nssz = [box fittingSize];
  f.size.height = nssz.height;
  f.size.width  = nssz.width;
  [w setFrame:f display:YES];

// ### this doesn't seem to be working either
  clg = w.contentLayoutGuide;
//  [clg.leadingAnchor constraintEqualToAnchor: box.leadingAnchor].active = YES;
//  [clg.trailingAnchor constraintEqualToAnchor: box.trailingAnchor].active = YES;
//  [clg.topAnchor constraintEqualToAnchor: box.topAnchor].active = YES;
//  [clg.bottomAnchor constraintEqualToAnchor: box.bottomAnchor].active = YES;
  [clg.heightAnchor constraintEqualToAnchor: box.heightAnchor].active = YES;
  [clg.widthAnchor constraintEqualToAnchor: box.widthAnchor].active = YES;
}

@end

uiselect_t *
uiSelectInit (uiwcont_t *window, const char *label,
    const char *startpath, const char *dfltname,
    const char *mimefiltername, const char *mimetype)
{
  return NULL;
}

void
uiSelectFree (uiselect_t *selectdata)
{
  return;
}

char *
uiSelectDirDialog (uiselect_t *selectdata)
{
  return NULL;
}

char *
uiSelectFileDialog (uiselect_t *selectdata)
{
  return NULL;
}


uiwcont_t *
uiCreateDialog (uiwcont_t *window,
    callback_t *uicb, const char *title, ...)
{
  uiwcont_t     *uiwin;
  IDWindow      *win = NULL;
  uiwcont_t     *uibox;
  NSStackView   *box;
//  id            windowDelegate;

  win = [[IDWindow alloc] init];
  uibox = uiCreateVertBox ();
  if (title != NULL) {
    NSString  *nstitle;

    nstitle = [NSString stringWithUTF8String: title];
    [win setTitle: nstitle];
  }

  box = uibox->uidata.widget;
  [win setContentView: box];

  uibox->packed = true;

//  windowDelegate = [[IDWindowDelegate alloc] init];
//  [win setDelegate:windowDelegate];

  uiwin = uiwcontAlloc (WCONT_T_WINDOW, WCONT_T_DIALOG_WINDOW);
  uiwcontSetWidget (uiwin, win, NULL);
  uiwin->packed = true;

  uiWidgetSetAllMargins (uibox, 2);
  uiWidgetExpandHoriz (uibox);
  uiWidgetExpandVert (uibox);

  return uiwin;
}

void
uiDialogShow (uiwcont_t *uiwidgetp)
{
  return;
}

void
uiDialogAddButtons (uiwcont_t *uidialog, ...)
{
  return;
}

void
uiDialogPackInDialog (uiwcont_t *uidialog, uiwcont_t *boxp)
{
  return;
}

void
uiDialogDestroy (uiwcont_t *uidialog)
{
  return;
}

