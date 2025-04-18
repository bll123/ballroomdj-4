/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
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
#include "osenv.h"
#include "oslocale.h"
#include "osutils.h"
#include "pathbld.h"
#include "slist.h"
#include "sysvars.h"

/* must be sorted in ascii order */
static datafilekey_t localedfkeys [LOCALE_KEY_MAX] = {
  { "DISPLAY",    LOCALE_KEY_DISPLAY,   VALUE_STR, NULL, DF_NORM },
  { "ISO639_2",   LOCALE_KEY_ISO639_2,  VALUE_STR, NULL, DF_NORM },
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
  int             direction;
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

  pathbldMakePath (fname, sizeof (fname), LOCALIZATION_FN,
      BDJ4_CONFIG_EXT, PATHBLD_MP_DIR_TEMPLATE);
  if (! fileopFileExists (fname)) {
    logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: localeutil: missing %s", fname);
    return;
  }

  localeSetup ();
  istringInit (sysvarsGetStr (SV_LOCALE));

  localedata = mdmalloc (sizeof (bdj4localedata_t));
  localedata->df = NULL;
  localedata->displayList = NULL;
  localedata->locales = NULL;
  localedata->currlocale = -1;   /* for the moment, mark as invalid */
  localedata->direction = TEXT_DIR_DEFAULT;

  localedata->df = datafileAllocParse (LOCALIZATION_FN,
      DFTYPE_INDIRECT, fname,
      localedfkeys, LOCALE_KEY_MAX, DF_NO_OFFSET, NULL);
  localedata->locales = datafileGetList (localedata->df);

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

  localePostSetup ();

  return;
}

void
localeSetup (void)
{
  char          lbuff [MAXPATHLEN];
  char          tbuff [MAXPATHLEN];
  char          sbuff [40];
  bool          useutf8ext = false;
  struct lconv  *lconv;

  *lbuff = '\0';

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
    stpecpy (lbuff, lbuff + sizeof (lbuff), sysvarsGetStr (SV_LOCALE_SYSTEM));
  }
  snprintf (tbuff, sizeof (tbuff), "%-.5s", lbuff);
  /* windows uses en-US rather than en_US */
  if (strlen (tbuff) >= 3 && tbuff [2] == '-') {
    tbuff [2] = '_';
  }
  sysvarsSetStr (SV_LOCALE_ORIG, tbuff);
  snprintf (sbuff, sizeof (sbuff), "%-.2s", tbuff);
  sysvarsSetStr (SV_LOCALE_ORIG_SHORT, sbuff);

  /* if sysvars has already read the locale.txt file, do not override */
  /* the locale setting */
  if (sysvarsGetNum (SVL_LOCALE_SET) == SYSVARS_LOCALE_NOT_SET) {
    sysvarsSetStr (SV_LOCALE, tbuff);
    snprintf (sbuff, sizeof (sbuff), "%-.2s", sysvarsGetStr (SV_LOCALE));
    sysvarsSetStr (SV_LOCALE_SHORT, sbuff);
  }

  stpecpy (lbuff, lbuff + sizeof (lbuff), sysvarsGetStr (SV_LOCALE));

  if (isWindows ()) {
    if (atof (sysvarsGetStr (SV_OS_VERS)) >= 10.0) {
      if (atol (sysvarsGetStr (SV_OS_BUILD)) >= 1803) {
        useutf8ext = true;
      }
    }
  } else {
    if (strcmp (lbuff, "C") != 0) {
      useutf8ext = true;
    }
  }

  if (useutf8ext) {
    snprintf (tbuff, sizeof (tbuff), "%s.UTF-8", lbuff);
  } else {
    stpecpy (tbuff, tbuff + sizeof (tbuff), lbuff);
  }

  /* windows doesn't work without this */
  /* note that LC_MESSAGES is an msys2 extension */
  /* windows normally has no LC_MESSAGES setting */
  /* MacOS seems to need this also, setlocale() apparently is not enough */
  osSetEnv ("LC_MESSAGES", tbuff);
  osSetEnv ("LC_COLLATE", tbuff);

  pathbldMakePath (lbuff, sizeof (lbuff), "", "", PATHBLD_MP_DIR_LOCALE);
#if _lib_wbindtextdomain
  {
    wchar_t   *wlocale;

    wlocale = osToWideChar (lbuff);
    wbindtextdomain (GETTEXT_DOMAIN, wlocale);
    dataFree (wlocale);
  }
#else
  bindtextdomain (GETTEXT_DOMAIN, lbuff);
#endif
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
    localedata->direction = TEXT_DIR_DEFAULT;
    mdfree (localedata);
    localedata = NULL;
  }
}

#if 0   /* for debugging */
void
localeDebug (const char *tag)   /* KEEP */
{
  char    tbuff [200];

  fprintf (stderr, "-- locale : %s\n", tag);
  fprintf (stderr, "  set-locale-all:%s\n", setlocale (LC_ALL, NULL));
  fprintf (stderr, "  set-locale-collate:%s\n", setlocale (LC_COLLATE, NULL));
  fprintf (stderr, "  set-locale-messages:%s\n", setlocale (LC_MESSAGES, NULL));
  osGetLocale (tbuff, sizeof (tbuff));
  fprintf (stderr, "  os-locale:%s\n", tbuff);
  fprintf (stderr, "  locale-system:%s\n", sysvarsGetStr (SV_LOCALE_SYSTEM));
  fprintf (stderr, "  locale-orig:%s\n", sysvarsGetStr (SV_LOCALE_ORIG));
  fprintf (stderr, "  locale:%s\n", sysvarsGetStr (SV_LOCALE));
  fprintf (stderr, "  locale-short:%s\n", sysvarsGetStr (SV_LOCALE_SHORT));
  fprintf (stderr, "  locale-639-2:%s\n", sysvarsGetStr (SV_LOCALE_639_2));
  fprintf (stderr, "  locale-set:%d\n", (int) sysvarsGetNum (SVL_LOCALE_SET));
  fprintf (stderr, "  locale-sys-set:%d\n", (int) sysvarsGetNum (SVL_LOCALE_SYS_SET));
  fprintf (stderr, "  env-lang:%s\n", getenv ("LANG"));
  fprintf (stderr, "  env-all:%s\n", getenv ("LC_ALL"));
  fprintf (stderr, "  env-mess:%s\n", getenv ("LC_MESSAGES"));
  fprintf (stderr, "  bindtextdomain:%s\n", bindtextdomain (GETTEXT_DOMAIN, NULL));
  fprintf (stderr, "  textdomain:%s\n", textdomain (NULL));
}
#endif

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

  localedata->direction = osLocaleDirection (svlocale);
  sysvarsSetNum (SVL_LOCALE_DIR, localedata->direction);

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
      /* this will override any short locale that has been located */
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
}
