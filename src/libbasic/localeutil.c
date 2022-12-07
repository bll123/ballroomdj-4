/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <math.h>

#include <locale.h>
/* libintl.h is included by localeutil.h */

#include "bdj4.h"
#include "bdj4intl.h"
#include "bdjstring.h"
#include "istring.h"
#include "localeutil.h"
#include "oslocale.h"
#include "osutils.h"
#include "pathbld.h"
#include "sysvars.h"

void
localeInit (void)
{
  char        lbuff [MAXPATHLEN];
  char        tbuff [MAXPATHLEN];
  bool        useutf8ext = false;
  struct lconv *lconv;


  /* get the locale from the environment */
  /* this does work on windows */
  if (setlocale (LC_ALL, "") == NULL) {
    fprintf (stderr, "set of locale from env failed\n");
  }

  /* these will be incorrect for windows */

  sysvarsSetStr (SV_LOCALE_SYSTEM, osGetLocale (tbuff, sizeof (tbuff)));
  snprintf (tbuff, sizeof (tbuff), "%-.5s", sysvarsGetStr (SV_LOCALE_SYSTEM));

  /* if sysvars has already read the locale.txt file, do not override */
  /* the locale setting */
  if (sysvarsGetNum (SVL_LOCALE_SET) == 0) {
    /* windows uses en-US rather than en_US */
    tbuff [2] = '_';
    sysvarsSetStr (SV_LOCALE, tbuff);
    snprintf (tbuff, sizeof (tbuff), "%-.2s", sysvarsGetStr (SV_LOCALE));
    sysvarsSetStr (SV_LOCALE_SHORT, tbuff);
  }

  strlcpy (lbuff, sysvarsGetStr (SV_LOCALE), sizeof (lbuff));

  if (isWindows ()) {
    if (atof (sysvarsGetStr (SV_OSVERS)) >= 10.0) {
      if (atoi (sysvarsGetStr (SV_OSBUILD)) >= 1803) {
        useutf8ext = true;
      }
    }
  } else {
    useutf8ext = true;
  }

  if (useutf8ext) {
    snprintf (tbuff, sizeof (tbuff), "%s.UTF-8", lbuff);
  } else {
    strlcpy (tbuff, lbuff, sizeof (tbuff));
  }

  if (isWindows ()) {
    /* windows doesn't work without this */
    /* note that LC_MESSAGES is an msys2 extension */
    /* windows normally has no LC_MESSAGES setting */
    osSetEnv ("LC_MESSAGES", tbuff);
    osSetEnv ("LC_COLLATE", tbuff);
  }

  pathbldMakePath (lbuff, sizeof (lbuff), "", "", PATHBLD_MP_DIR_LOCALE);
  bindtextdomain (GETTEXT_DOMAIN, lbuff);
  textdomain (GETTEXT_DOMAIN);
#if _lib_bind_textdomain_codeset
  bind_textdomain_codeset (GETTEXT_DOMAIN, "UTF-8");
#endif

  lconv = localeconv ();
  sysvarsSetStr (SV_LOCALE_RADIX, lconv->decimal_point);

  /* setlocale on windows cannot handle utf-8 strings */
  /* nor will it handle the sv_SE style format */
  if (! isWindows ()) {
    if (setlocale (LC_MESSAGES, tbuff) == NULL) {
      fprintf (stderr, "set of locale failed; unknown locale %s\n", tbuff);
    }
    if (setlocale (LC_COLLATE, tbuff) == NULL) {
      fprintf (stderr, "set of locale failed; unknown locale %s\n", tbuff);
    }
  }

  istringInit (sysvarsGetStr (SV_LOCALE));
  return;
}

void
localeCleanup (void)
{
  istringCleanup ();
}

void
localeDebug (void)
{
  char    tbuff [200];

  fprintf (stderr, "-- locale\n");
  fprintf (stderr, "  set-locale-all:%s\n", setlocale (LC_ALL, NULL));
  fprintf (stderr, "  set-locale-collate:%s\n", setlocale (LC_COLLATE, NULL));
  fprintf (stderr, "  set-locale-messages:%s\n", setlocale (LC_MESSAGES, NULL));
  osGetLocale (tbuff, sizeof (tbuff));
  fprintf (stderr, "  os-locale:%s\n", tbuff);
  fprintf (stderr, "  locale-system:%s\n", sysvarsGetStr (SV_LOCALE_SYSTEM));
  fprintf (stderr, "  locale:%s\n", sysvarsGetStr (SV_LOCALE));
  fprintf (stderr, "  locale-short:%s\n", sysvarsGetStr (SV_LOCALE_SHORT));
  fprintf (stderr, "  env-all:%s\n", getenv ("LC_ALL"));
  fprintf (stderr, "  env-mess:%s\n", getenv ("LC_MESSAGES"));
}
