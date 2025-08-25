/*
 * Copyright 2024-2025 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "ui/uiui.h"      // for uisetup_t

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

extern uisetup_t   guisetup;

/* to hold the margin information for the widget */
typedef struct macoslayout macoslayout_t;

typedef struct uispecific {
  void              *packwidget;
  union {
    void            *widget;
  };
  /* to hold the margin information and margin constraints for the widget */
  macoslayout_t     *layout;
} uispecific_t;

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

