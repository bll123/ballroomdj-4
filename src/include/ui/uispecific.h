/*
 * Copyright 2024-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

# if BDJ4_UI_GTK3 /* gtk3 */

#include <gtk/gtk.h>

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

/* uigeneric.c */
GtkWidget * uiImageWidget (const char *imagenm);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

# endif /* BDJ4_UI_GTK3 */

