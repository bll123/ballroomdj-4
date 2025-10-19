/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>

#define U_CHARSET_IS_UTF8 1

/* the icu unicode headers are not clean, they must be ordered properly */
#include <unicode/stringoptions.h>
#include <unicode/ustring.h>
#include <unicode/utypes.h>
#include <unicode/uloc.h>
#include <unicode/unorm.h>
#include <unicode/ucol.h>
#include <unicode/ucasemap.h>
#include <unicode/utf8.h>

#include "bdj4.h"
#include "bdjstring.h"
#include "dylib.h"
#include "istring.h"
#include "log.h"
#include "mdebug.h"
#include "sysvars.h"
#include "tmutil.h"

typedef struct {
  dlhandle_t      *i18ndlh;
  dlhandle_t      *ucdlh;
  UCollator       *ucoll;
  UCaseMap        *ucsm;
  UCollator       *(*ucol_open)(const char *, UErrorCode *);
  UCollationResult (*ucol_strcollUTF8)(const UCollator *, const char *, int32_t, const char *, int32_t, UErrorCode *);
  int             (*u_strToUTF8)(char *, int32_t, int32_t *, const UChar *, int32_t, UErrorCode *);
  const char      *(*u_errorName)(UErrorCode);
  void            (*ucol_close)(UCollator *);
  UCaseMap        *(*ucasemap_open)(const char *, uint32_t, UErrorCode *);
  int             (*ucasemap_utf8ToLower)(const UCaseMap *, char *, int32_t, const char *, int32_t, UErrorCode *);
  void            (*ucasemap_close)(UCaseMap *ucsm);
  int             i18nvers;
  int             ucvers;
  int             icuvers;
  bool            initialized;
} istring_data_t;

static istring_data_t istringdata =
    { NULL, NULL,
      NULL, NULL,
      NULL, NULL, NULL, NULL, NULL,
      NULL, NULL, NULL,
      0, 0, 0, false };

void
istringInit (const char *locale)
{
  UErrorCode    status = U_ZERO_ERROR;

  if (istringdata.initialized == true) {
    return;
  }

  {
    int         version = -1;
    char        tbuff [MAXPATHLEN];
    const char  *i18nnm = "libicui18n";

    if (isWindows ()) {
      i18nnm = "libicuin";
    }

    istringdata.i18ndlh = dylibLoad (i18nnm,
        DYLIB_OPT_MAC_PREFIX | DYLIB_OPT_VERSION | DYLIB_OPT_ICU);
    if (istringdata.i18ndlh == NULL) {
      fprintf (stderr, "ERR: unable to open ICU library i18n\n");
      logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: unable to open ICU library i18n");
      return;
    }

    istringdata.i18nvers = dylibVersion ();

    istringdata.ucdlh = dylibLoad ("libicuuc",
        DYLIB_OPT_MAC_PREFIX | DYLIB_OPT_VERSION | DYLIB_OPT_ICU);
    if (istringdata.ucdlh == NULL) {
      fprintf (stderr, "ERR: unable to open ICU library uc\n");
      logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: unable to open ICU library uc");
      return;
    }

    istringdata.ucvers = dylibVersion ();

    version = dylibCheckVersion (istringdata.i18ndlh, "ucol_open",
        DYLIB_OPT_VERSION | DYLIB_OPT_ICU);

    istringdata.icuvers = version;

    if (version == -1) {
      fprintf (stderr, "ERR: unable to determine ICU version\n");
      logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: unable to determine ICU version");
      dylibClose (istringdata.i18ndlh);
      dylibClose (istringdata.ucdlh);
      return;
    }

    snprintf (tbuff, sizeof (tbuff), "ucol_open_%d", version);
    istringdata.ucol_open = dylibLookup (istringdata.i18ndlh, tbuff);
    snprintf (tbuff, sizeof (tbuff), "ucol_strcollUTF8_%d", version);
    istringdata.ucol_strcollUTF8 = dylibLookup (istringdata.i18ndlh, tbuff);
    snprintf (tbuff, sizeof (tbuff), "ucol_close_%d", version);
    istringdata.ucol_close = dylibLookup (istringdata.i18ndlh, tbuff);

    snprintf (tbuff, sizeof (tbuff), "ucasemap_open_%d", version);
    istringdata.ucasemap_open = dylibLookup (istringdata.ucdlh, tbuff);
    snprintf (tbuff, sizeof (tbuff), "ucasemap_utf8ToLower_%d", version);
    istringdata.ucasemap_utf8ToLower = dylibLookup (istringdata.ucdlh, tbuff);
    snprintf (tbuff, sizeof (tbuff), "ucasemap_close_%d", version);
    istringdata.ucasemap_close = dylibLookup (istringdata.ucdlh, tbuff);

    /* u_strToUTF8 and u_errorName are in both i18n and uc on linux/macos, */
    /* but only uc on windows (fixed in 4.17.3) */
    snprintf (tbuff, sizeof (tbuff), "u_strToUTF8_%d", version);
    istringdata.u_strToUTF8 = dylibLookup (istringdata.ucdlh, tbuff);
    snprintf (tbuff, sizeof (tbuff), "u_errorName_%d", version);
    istringdata.u_errorName = dylibLookup (istringdata.ucdlh, tbuff);
  }

  istringdata.ucoll = istringdata.ucol_open (locale, &status);
  mdextalloc (istringdata.ucoll);
  istringdata.ucsm = istringdata.ucasemap_open (locale, U_FOLD_CASE_DEFAULT, &status);
  mdextalloc (istringdata.ucsm);
  istringdata.initialized = true;
}

bool
istringCheck (void)       /* TESTING */
{
  bool    rc = true;

  if (istringdata.initialized == false) {
    fprintf (stderr, "istring: init failed (general)\n");
    rc = false;
  }

  fprintf (stderr, "istring: icu:%d i18n:%d uc:%d\n",
      istringdata.icuvers, istringdata.i18nvers, istringdata.ucvers);

  if (istringdata.ucol_open == NULL) {
    fprintf (stderr, "istring: ucol_open failed\n");
    rc = false;
  }
  if (istringdata.ucol_strcollUTF8 == NULL) {
    fprintf (stderr, "istring: ucol_strcollUTF8 failed\n");
    rc = false;
  }
  if (istringdata.u_strToUTF8 == NULL) {
    fprintf (stderr, "istring: u_strToUTF8 failed\n");
    rc = false;
  }
  if (istringdata.ucol_close == NULL) {
    fprintf (stderr, "istring: ucol_close failed\n");
    rc = false;
  }
  if (istringdata.u_errorName == NULL) {
    fprintf (stderr, "istring: u_errorName failed\n");
    rc = false;
  }
  if (istringdata.ucasemap_open == NULL) {
    fprintf (stderr, "istring: ucasemap_open failed\n");
    rc = false;
  }
  if (istringdata.ucasemap_utf8ToLower == NULL) {
    fprintf (stderr, "istring: ucasemap_utf8ToLower failed\n");
    rc = false;
  }
  if (istringdata.ucasemap_close == NULL) {
    fprintf (stderr, "istring: ucasemap_close failed\n");
    rc = false;
  }

  if (istringdata.ucoll == NULL) {
    fprintf (stderr, "istring: ucoll failed\n");
    rc = false;
  }
  if (istringdata.ucsm == NULL) {
    fprintf (stderr, "istring: ucsm failed\n");
    rc = false;
  }

  return rc;
}


void
istringCleanup (void)
{
  if (istringdata.initialized == false) {
    return;
  }

  if (istringdata.ucoll != NULL) {
    mdextfree (istringdata.ucoll);
    istringdata.ucol_close (istringdata.ucoll);
    istringdata.ucoll = NULL;
  }
  if (istringdata.ucsm != NULL) {
    mdextfree (istringdata.ucsm);
    istringdata.ucasemap_close (istringdata.ucsm);
    istringdata.ucsm = NULL;
  }

  if (istringdata.i18ndlh != NULL) {
    dylibClose (istringdata.i18ndlh);
    istringdata.i18ndlh = NULL;
  }
  if (istringdata.ucdlh != NULL) {
    dylibClose (istringdata.ucdlh);
    istringdata.ucdlh = NULL;
  }
  istringdata.initialized = false;
}

/* collated comparison */
int
istringCompare (const char *str1, const char *str2)
{
  UErrorCode status = U_ZERO_ERROR;
  int   rc = 0;

  if (istringdata.ucoll == NULL) {
    fprintf (stderr, "ERR: compare istring not initialized\n");
    logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: compare istring not initialized");
    return -1;
  }

  /* the unicode routine does not handle a null string */
  if (str1 == NULL && str2 != NULL) {
    return -1;
  }
  if (str1 != NULL && str2 == NULL) {
    return 1;
  }
  if (str1 == NULL && str2 == NULL) {
    return 0;
  }

  rc = istringdata.ucol_strcollUTF8 (istringdata.ucoll, str1, -1, str2, -1, &status);
  return rc;
}

/* this counts code points, not glyphs */
size_t
istrlen (const char *str)
{
  int32_t   offset;
  int32_t   slen = 0;
  int32_t   c = 0;
  size_t    len = 0;

  if (str == NULL) {
    return 0;
  }

  slen = strlen (str);
  offset = 0;
  while (offset < slen) {
    U8_NEXT (str, offset, slen, c);
    ++len;
  }

  return len;
}

void
istringToLower (char *str)
{
  UErrorCode  status = U_ZERO_ERROR;
  char        *dest = NULL;
  size_t      sz;
  size_t      rsz;

  if (istringdata.ucsm == NULL) {
    fprintf (stderr, "ERR: to-lower istring not initialized\n");
    logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: to-lower istring not initialized");
    return;
  }

  sz = strlen (str);
  ++sz;
  dest = mdmalloc (sz + 1);
  *dest = '\0';
  rsz = istringdata.ucasemap_utf8ToLower (istringdata.ucsm, dest, sz + 1, str, sz, &status);
  if (rsz <= sz && status == U_ZERO_ERROR) {
    stpecpy (str, str + sz, dest);
    mdfree (dest);
  }
}

/* instr is just a buffer pointing to char16_t */
char *
istring16ToUTF8 (const unsigned char *instr)
{
  UErrorCode  status = U_ZERO_ERROR;
  char        *result = NULL;
  int32_t     capacity = 0;
  int32_t     resultlen = 0;

  if (istringdata.initialized == false || istringdata.u_strToUTF8 == NULL) {
    fprintf (stderr, "ERR: to-utf8 istring not initialized\n");
    logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: to-utf8 istring not initialized");
    return NULL;
  }

  status = U_ZERO_ERROR;
  istringdata.u_strToUTF8 (result, capacity, &resultlen, (const UChar *) instr, -1, &status);
  if (resultlen > 0) {
    capacity = resultlen + 1;
    result = mdmalloc (capacity);
    status = U_ZERO_ERROR;
    istringdata.u_strToUTF8 (result, capacity, &resultlen, (const UChar *) instr, -1, &status);
  }
  return result;
}
