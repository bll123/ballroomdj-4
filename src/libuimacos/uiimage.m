/*
 * Copyright 2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <Cocoa/Cocoa.h>
#import <Foundation/NSObject.h>

#include "uiwcont.h"

#include "ui/uiwcont-int.h"

#include "ui/uiimage.h"
#include "ui/uiwidget.h"

uiwcont_t *
uiImageNew (void)
{
  return NULL;
}

uiwcont_t *
uiImageFromFile (const char *fn)
{
  uiwcont_t   *uiwidget = NULL;
  NSImageView *imgv;
  NSImage     *image;
  NSString    *ns;

  ns = [NSString stringWithUTF8String: fn];
  image = [[NSImage alloc] initWithContentsOfFile: ns];
  imgv = [[NSImageView alloc] init];
  [imgv setImage: image];

  uiwidget = uiwcontAlloc (WCONT_T_IMAGE, WCONT_T_IMAGE);
  uiwcontSetWidget (uiwidget, imgv, NULL);

  uiWidgetAlignHorizCenter (uiwidget);
  uiWidgetAlignVertCenter (uiwidget);

  return uiwidget;
}

uiwcont_t *
uiImageScaledFromFile (const char *fn, int scale)
{
  uiwcont_t   *uiwidget = NULL;
  NSImageView *imgv;
  NSImage     *image;
  NSString    *ns;

  ns = [NSString stringWithUTF8String: fn];
  image = [[NSImage alloc] initWithContentsOfFile: ns];
  imgv = [[NSImageView alloc] init];
  [imgv setImage: image];
  imgv.imageScaling = NSImageScaleProportionallyDown;
  [imgv.widthAnchor constraintEqualToConstant: scale + 4].active = YES;
  [imgv.heightAnchor constraintEqualToConstant: scale + 4].active = YES;

  uiwidget = uiwcontAlloc (WCONT_T_IMAGE, WCONT_T_IMAGE);
  uiwcontSetWidget (uiwidget, imgv, NULL);

  uiWidgetAlignHorizCenter (uiwidget);
  uiWidgetAlignVertCenter (uiwidget);

  return uiwidget;
}

void
uiImageClear (uiwcont_t *uiwidget)
{
  NSImageView *imgv;
  NSImage     *image;

  if (! uiwcontValid (uiwidget, WCONT_T_IMAGE, "image-clr")) {
    return;
  }

  image = [[NSImage alloc] initWithSize: NSZeroSize];
  imgv = [[NSImageView alloc] init];
  [imgv setImage: image];
  uiwcontSetWidget (uiwidget, imgv, NULL);
  return;
}

void
uiImageConvertToPixbuf (uiwcont_t *uiwidget)
{
  return;
}

void
uiImageSetFromPixbuf (uiwcont_t *uiwidget, uiwcont_t *uipixbuf)
{
  return;
}
