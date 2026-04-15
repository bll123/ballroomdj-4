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

#include "uiwcont.h"

#include "ui/uimacos-int.h"
#include "ui/uiwcont-int.h"

#include "ui/uiimage.h"
#include "ui/uiwidget.h"

uiwcont_t *
uiImageNew (void)
{
  uiwcont_t     *uiwidget;
  NSImage       *image;
  NSImageView   *imgv;

  image = [[NSImage alloc] init];

  imgv = [[NSImageView alloc] init];
  imgv.translatesAutoresizingMaskIntoConstraints = NO;
  imgv.image = image;
  imgv.imageScaling = NSImageScaleProportionallyDown;

  uiwidget = uiwcontAlloc (WCONT_T_IMAGE, WCONT_T_IMAGE);
  uiwcontSetWidget (uiwidget, imgv, NULL);

  uiWidgetAlignHorizCenter (uiwidget);
  uiWidgetAlignVertCenter (uiwidget);

  uiWidgetSetIdentifier (uiwidget, "image");

  return uiwidget;
}

void
uiImageFree (uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uiwidget, WCONT_T_IMAGE, "image-free")) {
    return;
  }

  return;
}

uiwcont_t *
uiImageFromFile (const char *fn)
{
  uiwcont_t   *uiwidget = NULL;
  NSImage     *image;
  NSImageView *imgv;
  NSString    *ns;

  ns = [NSString stringWithUTF8String : fn];
  image = [[NSImage alloc] initWithContentsOfFile : ns];

  imgv = [[NSImageView alloc] init];
  imgv.image = image;
  imgv.translatesAutoresizingMaskIntoConstraints = NO;
  imgv.imageScaling = NSImageScaleProportionallyDown;

  uiwidget = uiwcontAlloc (WCONT_T_IMAGE, WCONT_T_IMAGE);
  uiwcontSetWidget (uiwidget, imgv, NULL);

  uiWidgetSetIdentifier (uiwidget, "image");

  return uiwidget;
}

uiwcont_t *
uiImageScaledFromFile (const char *fn, int scale)
{
  uiwcont_t   *uiwidget = NULL;
  NSImageView *imgv;
  NSImage     *image;
  NSString    *ns;

  ns = [NSString stringWithUTF8String : fn];
  image = [[NSImage alloc] initWithContentsOfFile : ns];

  imgv = [[NSImageView alloc] init];
  imgv.image = image;
  imgv.translatesAutoresizingMaskIntoConstraints = NO;
  imgv.imageScaling = NSImageScaleProportionallyDown;
  [imgv.widthAnchor constraintEqualToConstant : scale + 4].active = YES;
  [imgv.heightAnchor constraintEqualToConstant : scale + 4].active = YES;

  uiwidget = uiwcontAlloc (WCONT_T_IMAGE, WCONT_T_IMAGE);
  uiwcontSetWidget (uiwidget, imgv, NULL);

  uiWidgetSetIdentifier (uiwidget, "image");

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

  image = [[NSImage alloc] initWithSize : NSZeroSize];
  imgv = uiwidget->uidata.widget;
  imgv.image = image;

  return;
}

void
uiImageCopy (uiwcont_t *toimg, uiwcont_t *fromimg)
{
  NSImageView *imgv;
  NSImage     *image;

  imgv = fromimg->uidata.widget;
  image = imgv.image;
  imgv = toimg->uidata.widget;
  imgv.image = image;

  return;
}
