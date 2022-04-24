#include "config.h"


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <assert.h>

#include "bdj4.h"
#include "bdjopt.h"
#include "bdjstring.h"
#include "songutil.h"

char *
songFullFileName (char *sfname)
{
  char      *tname;

  tname = malloc (MAXPATHLEN);
  assert (tname != NULL);

  if (sfname [0] == '/' || (sfname [1] == ':' && sfname [2] == '/')) {
    strlcpy (tname, sfname, MAXPATHLEN);
  } else {
    snprintf (tname, MAXPATHLEN, "%s/%s",
        bdjoptGetStr (OPT_M_DIR_MUSIC), sfname);
  }
  return tname;
}
