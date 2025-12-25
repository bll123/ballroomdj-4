/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include "bdj4.h"
#include "bdjstring.h"
#include "mdebug.h"
#include "osprocess.h"
#include "osutils.h"

void
osRegistryGet (char *key, char *name, char *buff, size_t sz)
{
  *buff = '\0';
}

void
osGetSystemFont (const char *gsettingspath, char *buff, size_t sz)
{
  char    *tptr;

  tptr = osRunProgram (gsettingspath, "get",
      "org.gnome.desktop.interface",  "font-name", NULL);
  if (tptr != NULL) {
    /* gsettings puts quotes around the data */
    stringTrim (tptr);
    stringTrimChar (tptr, '\'');
    stpecpy (buff, buff + sz, tptr + 1);
  }
  mdfree (tptr);
}
