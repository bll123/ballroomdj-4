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
#if _hdr_libintl
# include <libintl.h>
#endif

#include "bdj4.h"
#include "bdj4intl.h"
#include "bdjstring.h"
#include "datafile.h"
#include "fileop.h"
#include "ilist.h"
#include "istring.h"
#include "localeutil.h"
#include "log.h"
#include "mdebug.h"
#include "oslocale.h"
#include "osutils.h"
#include "pathbld.h"
#include "slist.h"
#include "sysvars.h"

/* must be sorted in ascii order */
static datafilekey_t localedfkeys [LOCALE_KEY_MAX] = {
  { "AUTO",       LOCALE_KEY_AUTO,      VALUE_STR, NULL, DF_NORM },
  { "DISPLAY",    LOCALE_KEY_DISPLAY,   VALUE_STR, NULL, DF_NORM },
  { "ISO639-3",   LOCALE_KEY_ISO639_3,  VALUE_STR, NULL, DF_NORM },
  { "LONG",       LOCALE_KEY_LONG,      VALUE_STR, NULL, DF_NORM },
  { "QDANCE",     LOCALE_KEY_QDANCE,    VALUE_STR, NULL, DF_NORM },
  { "SHORT",      LOCALE_KEY_SHORT,     VALUE_STR, NULL, DF_NORM },
  { "STDROUNDS",  LOCALE_KEY_STDROUNDS, VALUE_STR, NULL, DF_NORM },
};

typedef struct bdj4localedata {
  datafile_t      *df;
  ilist_t         *locales;
  ilistidx_t      currlocale;
  slist_t         *displayList;
} bdj4localedata_t;

static bdj4localedata_t *localedata = NULL;

static void localePostSetup (void);

void
localeInit (void)
{
  char              fname [MAXPATHLEN];
  ilistidx_t        iteridx;
  ilistidx_t        key;
  ilistidx_t        gbidx = -1;
  const char        *svlocale;

  localeSetup ();
  istringInit (sysvarsGetStr (SV_LOCALE));

  pathbldMakePath (fname, sizeof (fname), LOCALIZATION_FN,
      BDJ4_CONFIG_EXT, PATHBLD_MP_DIR_TEMPLATE);
  if (! fileopFileExists (fname)) {
    logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: localeutil: missing %s", fname);
    return;
  }

  localedata = mdmalloc (sizeof (bdj4localedata_t));

  localedata->df = datafileAllocParse (LOCALIZATION_FN,
      DFTYPE_INDIRECT, fname,
      localedfkeys, LOCALE_KEY_MAX, DF_NO_OFFSET, NULL);
  localedata->locales = datafileGetList (localedata->df);
  localedata->currlocale = -1;   /* for the moment, mark as invalid */

  localedata->displayList = slistAlloc ("locale-disp", LIST_UNORDERED, NULL);
  slistSetSize (localedata->displayList, ilistGetCount (localedata->locales));

  svlocale = sysvarsGetStr (SV_LOCALE);

  ilistStartIterator (localedata->locales, &iteridx);
  while ((key = ilistIterateKey (localedata->locales, &iteridx)) >= 0) {
    const char    *disp;
    const char    *llocale;
    const char    *slocale;

    disp = ilistGetStr (localedata->locales, key, LOCALE_KEY_DISPLAY);
    llocale = ilistGetStr (localedata->locales, key, LOCALE_KEY_LONG);
    slocale = ilistGetStr (localedata->locales, key, LOCALE_KEY_SHORT);
    if (strcmp (llocale, "en_GB") == 0) {
      gbidx = key;
    }
    if (strcmp (llocale, svlocale) == 0) {
      /* specific match */
      localedata->currlocale = key;
    }
    if (localedata->currlocale == -1 &&
        strncmp (slocale, svlocale, 2) == 0) {
      /* basic match */
      localedata->currlocale = key;
    }
    slistSetStr (localedata->displayList, disp, llocale);
  }
  slistSort (localedata->displayList);

  if (localedata->currlocale == -1) {
    localedata->currlocale = gbidx;
  }

  sysvarsSetStr (SV_LOCALE_ISO639_3, localeGetStr (LOCALE_KEY_ISO639_3));

  return;
}

void
localeSetup (void)
{
  char          lbuff [MAXPATHLEN];
  char          tbuff [MAXPATHLEN];
  bool          useutf8ext = false;
  struct lconv  *lconv;

  /* get the locale from the environment */
  /* works on windows, but windows returns the old style locale name */
  if (setlocale (LC_ALL, "") == NULL) {
    fprintf (stderr, "set of locale from env failed\n");
  }

  /* on windows, returns the locale set for the user, not what's set */
  /* in the environment. GTK apparently uses the appropriate locale */
  if (sysvarsGetNum (SVL_LOCALE_SET) == SYSVARS_LOCALE_NOT_SET) {
    osGetLocale (lbuff, sizeof (lbuff));
    sysvarsSetStr (SV_LOCALE_SYSTEM, lbuff);
  } else {
    strlcpy (lbuff, sysvarsGetStr (SV_LOCALE_SYSTEM), sizeof (lbuff));
  }
  snprintf (tbuff, sizeof (tbuff), "%-.5s", lbuff);
  /* windows uses en-US rather than en_US */
  tbuff [2] = '_';
  sysvarsSetStr (SV_LOCALE_ORIG, tbuff);

  /* if sysvars has already read the locale.txt file, do not override */
  /* the locale setting */
  if (sysvarsGetNum (SVL_LOCALE_SET) == SYSVARS_LOCALE_NOT_SET) {
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

  /* windows doesn't work without this */
  /* note that LC_MESSAGES is an msys2 extension */
  /* windows normally has no LC_MESSAGES setting */
  /* MacOS seems to need this also, setlocale() apparently is not enough */
  osSetEnv ("LC_MESSAGES", tbuff);
  osSetEnv ("LC_COLLATE", tbuff);

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

  localePostSetup ();
}

const char *
localeGetStr (int key)
{
  if (localedata == NULL || localedata->locales == NULL) {
    return NULL;
  }

  return ilistGetStr (localedata->locales, localedata->currlocale, key);
}

slist_t *
localeGetDisplayList (void)
{
  if (localedata == NULL) {
    return NULL;
  }

  return localedata->displayList;
}

void
localeCleanup (void)
{
  istringCleanup ();
  if (localedata != NULL) {
    datafileFree (localedata->df);
    slistFree (localedata->displayList);
    localedata->df = NULL;
    localedata->locales = NULL;
    localedata->displayList = NULL;
    mdfree (localedata);
    localedata = NULL;
  }
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
  fprintf (stderr, "  locale-orig:%s\n", sysvarsGetStr (SV_LOCALE_ORIG));
  fprintf (stderr, "  locale:%s\n", sysvarsGetStr (SV_LOCALE));
  fprintf (stderr, "  locale-short:%s\n", sysvarsGetStr (SV_LOCALE_SHORT));
  fprintf (stderr, "  locale-set:%d\n", (int) sysvarsGetNum (SVL_LOCALE_SET));
  fprintf (stderr, "  locale-sys-set:%d\n", (int) sysvarsGetNum (SVL_LOCALE_SYS_SET));
  fprintf (stderr, "  env-all:%s\n", getenv ("LC_ALL"));
  fprintf (stderr, "  env-mess:%s\n", getenv ("LC_MESSAGES"));
  fprintf (stderr, "  bindtextdomain:%s\n", bindtextdomain (GETTEXT_DOMAIN, NULL));
  fprintf (stderr, "  textdomain:%s\n", textdomain (NULL));
}

/* internal routines */

void
localePostSetup (void)
{
  ilistidx_t    iteridx;
  ilistidx_t    key;
  ilistidx_t    gbidx = -1;
  ilistidx_t    tlocale = -1;
  const char    *svlocale;

  if (localedata == NULL) {
    return;
  }

  svlocale = sysvarsGetStr (SV_LOCALE);

  ilistStartIterator (localedata->locales, &iteridx);
  while ((key = ilistIterateKey (localedata->locales, &iteridx)) >= 0) {
    const char  *llocale;
    const char  *slocale;

    llocale = ilistGetStr (localedata->locales, key, LOCALE_KEY_LONG);
    slocale = ilistGetStr (localedata->locales, key, LOCALE_KEY_SHORT);
    if (strcmp (llocale, "en_GB") == 0) {
      gbidx = key;
    }
    if (strcmp (llocale, svlocale) == 0) {
      tlocale = key;
      break;
    }
    if (tlocale == -1 &&
        strncmp (slocale, svlocale, 2) == 0) {
      tlocale = key;
    }
  }

  if (tlocale == -1) {
    tlocale = gbidx;
  }
  localedata->currlocale = tlocale;

  sysvarsSetStr (SV_LOCALE_ISO639_3, localeGetStr (LOCALE_KEY_ISO639_3));
}
