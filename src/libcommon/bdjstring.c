#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <wchar.h>
#include <string.h>
#include <ctype.h>

#if _hdr_winsock2
# include <winsock2.h>
#endif
#if _hdr_windows
# include <windows.h>
#endif

#include "bdjstring.h"
#include "osutils.h"

static const char *versionNext (const char *tv1);

/* not for use on localized strings */
char *
stringToLower (char * s)
{
  for (char *p = s; *p; p++) {
    *p = tolower (*p);
  }
  return s;
}

/* not for use on localized strings */
char *
stringToUpper (char * s)
{
  for (char *p = s; *p; p++) {
    *p = toupper (*p);
  }
  return s;
}

void
stringTrim (char *s)
{
  ssize_t     len;

  len = strlen (s);
  --len;
  while (len >= 0 && (s [len] == '\r' || s [len] == '\n')) {
    s [len] = '\0';
    --len;
  }

  return;
}

void
stringTrimChar (char *s, unsigned char c)
{
  ssize_t     len;

  len = strlen (s);
  --len;
  while (len >= 0 && s [len] == c) {
    s [len] = '\0';
    --len;
  }

  return;
}

/* collated comparison */
int
istringCompare (void *str1, void *str2)
{
  int       rc;

#if _lib_CompareStringEx
  int     trc;
  int     len;
  wchar_t *wstr1;
  wchar_t *wstr2;

  wstr1 = osToWideChar (str1);
  wstr2 = osToWideChar (str2);

  trc = CompareStringEx (
      sysvarsGetStr (SV_LOCALE),
      wstr1, -1,
      wstr2, -1,
      NORM_LINGUISTIC_CASING
      NULL, NULL, 0);
  switch (trc) {
    case CSTR_EQUAL: {
      rc = 0;
      break;
    }
    case CSTR_LESS_THAN: {
      rc = -1;
      break;
    }
    case CSTR_GREATER_THAN: {
      rc = 1;
      break;
    }
  }
  free (wstr1);
  free (wstr2);
#else
  rc = strcoll ((char *) str1, (char *) str2);
#endif
  return rc;
}

size_t
istrlen (const char *str)
{
  size_t            len;

#if _lib_MultiByteToWideChar
  /* this is not quite correct. */
  /* single-glyph characters that use 4 bytes will probably */
  /* need 2 wide chars */
  len = MultiByteToWideChar (CP_UTF8, 0, str, strlen (str), NULL, 0);
#else
  size_t            mlen;
  size_t            slen;
  size_t            bytelen;
  const char        *tstr;
  mbstate_t         ps;

  memset (&ps, 0, sizeof (mbstate_t));
  len = 0;
  bytelen = strlen (str);
  slen = bytelen;
  tstr = str;
  while (slen > 0) {
    mlen = mbrlen (tstr, slen, &ps);
    if ((int) mlen <= 0) {
      return bytelen;
    }
    ++len;
    tstr += mlen;
    slen -= mlen;
  }
#endif
  return len;
}

int
versionCompare (const char *v1, const char *v2)
{
  const char  *tv1, *tv2;
  int         iv1, iv2, rc;

  tv1 = v1;
  tv2 = v2;
  iv1 = atoi (tv1);
  iv2 = atoi (tv2);
  rc = iv1 - iv2;
  while (rc == 0 && *tv1) {
    tv1 = versionNext (tv1);
    tv2 = versionNext (tv2);
    iv1 = atoi (tv1);
    iv2 = atoi (tv2);
    rc = iv1 - iv2;
  }

  return rc;
}

inline static const char *
versionNext (const char *tv1)
{
  tv1 = strstr (tv1, ".");
  if (tv1 == NULL) {
    tv1 = "";
  } else {
    tv1 += 1;
  }
  return tv1;
}
