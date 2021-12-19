#include "bdjconfig.h"

#include <stdlib.h>
#include <wchar.h>
#include <string.h>

#include "bdjstring.h"

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

#if 0

static size_t istrlen (const bdjstring_t *);

bdjstring_t *
stringAlloc (const char *data, const int isstatic)
{
  bdjstring_t       *str;

  str = malloc (sizeof (bdjstring_t));
  if (str == NULL) {
    return NULL;
  }
  str->str = NULL;
  str->isstatic = isstatic;
  str->alloclen = 0;
  str->len = 0;
  str->bytelen = 0;

  str->str = strdup (data);
  if (str->str == NULL) {
    free (str);
    return NULL;
  }

  str->bytelen = strlen (data);
  str->alloclen = str->bytelen + 1;
  /* do not calculate str->len unless needed */
  return str;
}

void
stringFree (bdjstring_t *str)
{
  if (str != NULL) {
    if (str->str != NULL) {
      free (str->str);
    }
    free (str);
  }
}

/* returns the number of graphemes */
size_t
stringLen (bdjstring_t *str) {
  size_t      len;

  len = 0;
  if (str != NULL) {
    if (str->str != NULL) {
      if (str->len == 0) {
        str->len = istrlen (str);
      }
      len = str->len;
    }
  }
  return len;
}

static size_t
istrlen (const bdjstring_t *str)
{
  size_t            len;
  size_t            mlen;
  size_t            slen;
  mbstate_t         ps;
  const char        *tstr;

  len = 0;
  memset (&ps, 0, sizeof (mbstate_t));
  slen = str->bytelen;
  tstr = str->str;
  while (slen > 0) {
    mlen = mbrlen (tstr, slen, &ps);
    if ((int) mlen <= 0) {
      return str->bytelen;
    }
    ++len;
    tstr += mlen;
    slen -= mlen;
  }
  return len;
}

#endif
