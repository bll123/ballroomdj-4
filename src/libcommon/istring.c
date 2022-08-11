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

#include "istring.h"
#include "osutils.h"
#include "sysvars.h"

/* collated comparison */
int
istringCompare (void *str1, void *str2)
{
  int       rc = 0;

#if _lib_CompareStringEx
  int     trc;
  wchar_t *wlocale;
  wchar_t *wstr1;
  wchar_t *wstr2;

  wlocale = osToWideChar (sysvarsGetStr (SV_LOCALE));
  wstr1 = osToWideChar (str1);
  wstr2 = osToWideChar (str2);

  trc = CompareStringEx (
      wlocale,
      NORM_LINGUISTIC_CASING,
      wstr1, -1,
      wstr2, -1,
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
  free (wlocale);
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

