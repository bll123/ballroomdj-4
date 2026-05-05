/*
 * Copyright 2023-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>

#include "ui/uidialog.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

enum {
  /* used so that the validity check will work (keys) */
  WCONT_EMPTY_WIDGET = 0x0001,
};

/* ---------------------- */
/* GTK3 interface */

# if BDJ4_UI_GTK3
#  include "ui/uigtk3-wcont.h"

/* uigeneric.c */
GtkWidget * uiImageWidget (const char *imagenm);

# endif /* BDJ4_UI_GTK3 */

/* ---------------------- */
/* NULL interface */

# if BDJ4_UI_NULL
#  include "ui/uinull-wcont.h"
# endif /* BDJ4_UI_NULL */

/* ---------------------- */
/* native MACOS interface */
# if BDJ4_UI_MACOS
#  include "ui/uimacos-wcont.h"
# endif /* BDJ4_UI_MACOS */

/* ---------------------- */

/* ui specific widget container */
typedef struct uiwcontdata {
  /* widget data specific to the interface */
  uispecific_t    uidata;
} uiwcontdata_t;

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif
