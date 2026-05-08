/*
 * Copyright 2024-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include <Cocoa/Cocoa.h>

#include "ui/uiui.h"      // for uisetup_t

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

extern uisetup_t   guisetup;

/* to hold the margin information for the widget */
typedef struct macoslayout macoslayout_t;

typedef struct macoslayout {
  NSStackView   *container;
  NSEdgeInsets  margins;
  bool          expandhoriz;
  bool          expandvert;
  bool          horizcenter;
  bool          horizend;
  bool          horizfill;
  bool          horizright;
  bool          horizstart;
  bool          vertbaseline;
  bool          vertcenter;
  bool          vertend;
  bool          vertfill;
  bool          vertstart;
  bool          expandchildren;
} macoslayout_t;

typedef struct uispecific {
  macoslayout_t   layout;
} uispecific_t;

static inline NSAttributedString *
uiAttrString (const char *txt)
{
  NSAttributedString  *astr = NULL;
  NSString            *tstr;

NSLog(@"%@",[[[NSFontManager sharedFontManager] availableFontFamilies] description]);
  if (txt == NULL) {
    txt = "";
  }
  tstr = [NSString stringWithUTF8String : txt];
  if (guisetup.uifontattr != NULL) {
    astr = [[NSAttributedString alloc] initWithString : tstr
          attributes : guisetup.uifontattr];
  } else {
    astr = [[NSAttributedString alloc] initWithString : tstr];
  }
  return astr;
}

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

