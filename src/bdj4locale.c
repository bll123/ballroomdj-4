#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include <locale.h>
#if _hdr_libintl
# include <libintl.h>
#endif

int
main (int argc, char *argv [])
{
  char      *locale;

  setlocale (LC_ALL, "");
  locale = setlocale (LC_ALL, NULL);
  if (locale != NULL) {
    printf ("%2.2s ", locale);
    if (locale [2] == '_') {
      printf ("%5.5s ", locale);
    } else {
      printf ("'' ");
    }
    printf ("'%s'\n", locale);
  }
}
