/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
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
#if __has_include (<libintl.h>)
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
#include "pathdisp.h"
#include "slist.h"
#include "sysvars.h"
#include "uidd.h"

#include "osdirutil.h"

#define LOCALE_DEBUG 0

static const char *BDJ4_TEXT_DOMAIN = "bdj4";

/* must be sorted in ascii order */
static datafilekey_t localedfkeys [LOCALE_KEY_MAX] = {
  { "DISPLAY",    LOCALE_KEY_DISPLAY,   VALUE_STR, NULL, DF_NORM },
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
  char              fname [BDJ4_PATH_MAX];
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

#if LOCALE_DEBUG
  localeDebug ("localeutil");
#endif

  return;
}

void
localeSetup (void)
{
  char          locpath [BDJ4_PATH_MAX];
  char          lbuff [BDJ4_PATH_MAX];
  char          tbuff [BDJ4_PATH_MAX];
  char          sbuff [40];
  bool          useutf8ext = false;
  struct lconv  *lconv;

  *lbuff = '\0';
  *locpath = '\0';

  /* get the locale from the environment */
  /* on Linux, this must be done first */
  if (setlocale (LC_ALL, "") == NULL) {
    fprintf (stderr, "set of locale from env failed\n");
  }

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

  /* lbuff contains the locale tag without any trailing character set */
  /* tbuff now contains the full locale tag */

  /* windows doesn't work without this */
  /* note that LC_MESSAGES is an msys2 extension */
  /* windows normally has no LC_MESSAGES setting */

  /* setlocale of LC_MESSAGES, etc. is needed for bdj4starterui */
  setlocale (LC_MESSAGES, tbuff);
  setlocale (LC_COLLATE, tbuff);
  setlocale (LC_CTYPE, tbuff);
  osSetEnv ("LC_MESSAGES", tbuff);
  osSetEnv ("LC_COLLATE", tbuff);
  osSetEnv ("LC_CTYPE", tbuff);

  pathbldMakePath (locpath, sizeof (locpath), "", "", PATHBLD_MP_DIR_LOCALE);
#if _lib_wbindtextdomain
  {
    wchar_t   *wlocale;

    pathDisplayPath (locpath, sizeof (locpath));
    wlocale = osToWideChar (locpath);
    if (wbindtextdomain (BDJ4_TEXT_DOMAIN, wlocale) == NULL) {
      logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: localeutil: wbindtextdomain failed %s", locpath);
    }
    dataFree (wlocale);
  }
#else
  if (bindtextdomain (BDJ4_TEXT_DOMAIN, locpath) == NULL) {
    logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: localeutil: bindtextdomain failed %s", locpath);
  }
#endif

  if (textdomain (BDJ4_TEXT_DOMAIN) == NULL) {
    logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: localeutil: textdomain failed %s", BDJ4_TEXT_DOMAIN);
  }

#if _lib_bind_textdomain_codeset
  bind_textdomain_codeset (BDJ4_TEXT_DOMAIN, "UTF-8");
#endif

  lconv = localeconv ();
  sysvarsSetStr (SV_LOCALE_RADIX, lconv->decimal_point);

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

  if (localedata == NULL) {
    return;
  }

  datafileFree (localedata->df);
  slistFree (localedata->displayList);
  localedata->df = NULL;
  localedata->locales = NULL;
  localedata->displayList = NULL;
  localedata->direction = TEXT_DIR_DEFAULT;
  mdfree (localedata);
  localedata = NULL;
}

#if LOCALE_DEBUG
void
localeDebug (const char *tag)   /* KEEP */
{
  char    tbuff [200];

  fprintf (stderr, "-- locale : %s\n", tag);
  fprintf (stderr, "  os-setlocale-all:%s\n", setlocale (LC_ALL, NULL));
  fprintf (stderr, "  os-setlocale-messages:%s\n", setlocale (LC_MESSAGES, NULL));
  fprintf (stderr, "  os-setlocale-collate:%s\n", setlocale (LC_COLLATE, NULL));
  fprintf (stderr, "  os-setlocale-ctype:%s\n", setlocale (LC_CTYPE, NULL));
  fprintf (stderr, "  os-setlocale-numeric:%s\n", setlocale (LC_NUMERIC, NULL));
  osGetLocale (tbuff, sizeof (tbuff));
  fprintf (stderr, "  os-get-locale:%s\n", tbuff);
  fprintf (stderr, "  sv-locale-system:%s\n", sysvarsGetStr (SV_LOCALE_SYSTEM));
  fprintf (stderr, "  sv-locale-orig:%s\n", sysvarsGetStr (SV_LOCALE_ORIG));
  fprintf (stderr, "  sv-locale:%s\n", sysvarsGetStr (SV_LOCALE));
  fprintf (stderr, "  sv-locale-short:%s\n", sysvarsGetStr (SV_LOCALE_SHORT));
  fprintf (stderr, "  sv-locale-639-2:%s\n", sysvarsGetStr (SV_LOCALE_639_2));
  fprintf (stderr, "  sv-locale-set:%d\n", (int) sysvarsGetNum (SVL_LOCALE_SET));
  fprintf (stderr, "  sv-locale-sys-set:%d\n", (int) sysvarsGetNum (SVL_LOCALE_SYS_SET));
  osGetEnv ("LANG", tbuff, sizeof (tbuff));
  fprintf (stderr, "  env-lang:%s\n", tbuff);
  osGetEnv ("LC_ALL", tbuff, sizeof (tbuff));
  fprintf (stderr, "  env-lc-all:%s\n", tbuff);
  osGetEnv ("LC_MESSAGES", tbuff, sizeof (tbuff));
  fprintf (stderr, "  env-lc-messages:%s\n", tbuff);
  osGetEnv ("LC_COLLATE", tbuff, sizeof (tbuff));
  fprintf (stderr, "  env-lc-collate:%s\n", tbuff);
  osGetEnv ("LC_CTYPE", tbuff, sizeof (tbuff));
  fprintf (stderr, "  env-lc-ctype:%s\n", tbuff);
  osGetEnv ("LC_NUMERIC", tbuff, sizeof (tbuff));
  fprintf (stderr, "  env-lc-numeric:%s\n", tbuff);
#if _lib_wbindtextdomain
  fprintf (stderr, "  wbindtextdomain:%S\n", wbindtextdomain (BDJ4_TEXT_DOMAIN, NULL));
#else
  fprintf (stderr, "  bindtextdomain:%s\n", bindtextdomain (BDJ4_TEXT_DOMAIN, NULL));
#endif
  fprintf (stderr, "  textdomain:%s\n", textdomain (NULL));
}
#endif

ilist_t *
localeCreateDropDownList (int *idx, bool uselocale)
{
  slist_t       *list = NULL;
  slistidx_t    iteridx;
  const char    *key;
  const char    *disp;
  ilist_t       *ddlist;
  int           count;
  bool          found;
  int           engbidx = 0;
  int           shortidx = 0;
  sysvarkey_t   localekey = SV_LOCALE;
  sysvarkey_t   localeshortkey = SV_LOCALE_SHORT;

  logProcBegin ();

  if (uselocale == LOCALE_USE_DATA) {
    localekey = SV_LOCALE_DATA;
    localeshortkey = SV_LOCALE_DATA_SHORT;
  }

  *idx = 0;
  list = localeGetDisplayList ();

  ddlist = ilistAlloc ("cu-locale-dd", LIST_ORDERED);
  ilistSetSize (ddlist, slistGetCount (list));

  slistStartIterator (list, &iteridx);
  count = 0;
  found = false;
  shortidx = -1;
  while ((disp = slistIterateKey (list, &iteridx)) != NULL) {
    key = slistGetStr (list, disp);
    if (strcmp (disp, "en_GB") == 0) {
      engbidx = count;
    }
    if (strcmp (key, sysvarsGetStr (localekey)) == 0) {
      *idx = count;
      found = true;
    }
    if (strncmp (key, sysvarsGetStr (localeshortkey), 2) == 0) {
      shortidx = count;
    }
    ilistSetStr (ddlist, count, DD_LIST_DISP, disp);
    ilistSetStr (ddlist, count, DD_LIST_KEY_STR, key);
    ilistSetNum (ddlist, count, DD_LIST_KEY_NUM, count);
    ++count;
  }

  if (! found && shortidx >= 0) {
    *idx = shortidx;
  } else if (! found) {
    *idx = engbidx;
  }

  logProcEnd ("");
  return ddlist;
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
  const char    *l639_2 = NULL;

  if (localedata == NULL) {
    return;
  }

  svlocale = sysvarsGetStr (SV_LOCALE);

  localedata->direction = osLocaleDirection (svlocale);
  sysvarsSetNum (SVL_LOCALE_TEXT_DIR, localedata->direction);

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

  l639_2 = istring639_2 (svlocale);
  sysvarsSetStr (SV_LOCALE_639_2, l639_2);
}

