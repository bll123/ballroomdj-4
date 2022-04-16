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

#include "localeutil.h"
#include "sysvars.h"

int
main (int argc, char *argv [])
{
  char      *locale;

  setlocale (LC_ALL, "");
  locale = setlocale (LC_ALL, NULL);
  printf ("current: %s\n", locale);
  locale = setlocale (LC_MESSAGES, NULL);
  printf ("msgs: %s\n", locale);

  sysvarsInit (argv [0]);
  localeInit ();

  printf ("locale: %s\n", sysvarsGetStr (SV_LOCALE));
  printf ("short locale: %s\n", sysvarsGetStr (SV_LOCALE_SHORT));
  fflush (stdout);
}
