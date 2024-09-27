/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>

#define U_CHARSET_IS_UTF8 1

/* the unicode headers are not clean, they must be ordered properly */
#include <unicode/stringoptions.h>
#include <unicode/ustring.h>
#include <unicode/utypes.h>
#include <unicode/uloc.h>
#include <unicode/unorm.h>
#include <unicode/ucol.h>
#include <unicode/ucasemap.h>
#include <unicode/utf8.h>

#include "bdjstring.h"
#include "istring.h"
#include "mdebug.h"

static UCollator  *ucoll = NULL;
static UCaseMap   *ucsm = NULL;

void
istringInit (const char *locale)
{
  UErrorCode status = U_ZERO_ERROR;

  if (ucoll == NULL) {
    ucoll = ucol_open (locale, &status);
  }
  if (ucsm == NULL) {
    ucsm = ucasemap_open (locale, U_FOLD_CASE_DEFAULT, &status);
  }
}

void
istringCleanup (void)
{
  if (ucoll != NULL) {
    ucol_close (ucoll);
    ucoll = NULL;
  }
  if (ucsm != NULL) {
    ucasemap_close (ucsm);
    ucsm = NULL;
  }
}

/* collated comparison */
int
istringCompare (const char *str1, const char *str2)
{
  UErrorCode status = U_ZERO_ERROR;
  int   rc = 0;

  if (ucoll == NULL) {
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

  rc = ucol_strcollUTF8 (ucoll, str1, -1, str2, -1, &status);
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

  if (ucsm == NULL) {
    return;
  }

  sz = strlen (str);
  ++sz;
  dest = mdmalloc (sz + 1);
  *dest = '\0';
  rsz = ucasemap_utf8ToLower (ucsm, dest, sz + 1, str, sz, &status);
  if (rsz <= sz && status == U_ZERO_ERROR) {
    stpecpy (str, str + sz, dest);
    mdfree (dest);
  }
}

char *
istring16ToUTF8 (wchar_t *instr)
{
  UErrorCode  status = U_ZERO_ERROR;
  char        *result = NULL;
  int32_t     capacity = 0;
  int32_t     resultlen = 0;

  u_strToUTF8 (result, capacity, &resultlen, (const UChar *) instr, -1, &status);
  if (resultlen > 0) {
    capacity = resultlen + 1;
    result = mdmalloc (capacity);
    status = U_ZERO_ERROR;
    u_strToUTF8 (result, capacity, &resultlen, (const UChar *) instr, -1, &status);
  }
  return result;
}
