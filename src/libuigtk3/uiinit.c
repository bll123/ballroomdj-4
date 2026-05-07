/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include "ui/uiui.h"

#define BDJ4_DEBUG_CSS 0

void
uiUIBasicInit (void)
{
  int   argc = 0;

  /* gtk's gtk_get_locale_direction() gets very confused if the locale */
  /* settings are changed before gtk_init() is called. */

  /* the locale has already been set */
  gtk_disable_setlocale ();
  gtk_init (&argc, NULL);
}

