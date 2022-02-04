#include "config.h"


#include <stdlib.h>
#include <stdbool.h>
#include <wchar.h>
#include <string.h>
#include <ctype.h>

#include "bdjstring.h"

char *
strtolower (char * s) {
  for (char *p = s; *p; p++) {
    *p = tolower (*p);
  }
  return s;
}

char *
strtoupper (char * s) {
  for (char *p = s; *p; p++) {
    *p = toupper (*p);
  }
  return s;
}

/* standard C comparison */
int
stringCompare (void *str1, void *str2)
{
  int       rc;

  rc = strcmp ((char *) str1, (char *) str2);
  return rc;
}

/* collated comparison */
int
istringCompare (void *str1, void *str2)
{
  int       rc;

  rc = strcoll ((char *) str1, (char *) str2);
  return rc;
}

size_t
istrlen (const char *str)
{
  size_t            len;
  size_t            mlen;
  size_t            slen;
  size_t            bytelen;
  mbstate_t         ps;
  const char        *tstr;

  len = 0;
  memset (&ps, 0, sizeof (mbstate_t));
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
  return len;
}

