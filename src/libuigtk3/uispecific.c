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

#include "bdj4.h"
#include "mdebug.h"
#include "pathbld.h"
#include "uiwcont.h"

#include "ui/uiwcont-int.h"

GtkWidget *
uiImageWidget (const char *imagenm)
{
  GtkWidget   *image = NULL;

  if (imagenm != NULL) {
    char        tbuff [BDJ4_PATH_MAX];
    const char  *timgnm;

    if (strchr (imagenm, '/') == NULL) {
      /* relative path */
      pathbldMakePath (tbuff, sizeof (tbuff), imagenm, BDJ4_IMG_SVG_EXT,
          PATHBLD_MP_DREL_IMG | PATHBLD_MP_USEIDX);
      timgnm = tbuff;
    } else {
      timgnm = imagenm;
    }
    image = gtk_image_new_from_file (timgnm);
  }

  return image;
}
