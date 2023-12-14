/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#if __linux__

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

char *
osRegistryGet (char *key, char *name)
{
  return NULL;
}

char *
osGetSystemFont (const char *gsettingspath)
{
  char    *tptr;
  char    *rptr = NULL;

  tptr = osRunProgram (gsettingspath, "get",
      "org.gnome.desktop.interface",  "font-name", NULL);
  if (tptr != NULL) {
    /* gsettings puts quotes around the data */
    stringTrim (tptr);
    stringTrimChar (tptr, '\'');
    rptr = mdstrdup (tptr + 1);
  }
  mdfree (tptr);

  return rptr;
}

void
osSuspendSleep (void)
{
  return;
}

void
osResumeSleep (void)
{
  return;
}

#endif
