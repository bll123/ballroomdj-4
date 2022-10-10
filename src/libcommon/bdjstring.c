#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <wchar.h>
#include <string.h>
#include <ctype.h>

#include "bdjstring.h"

static const char *versionNext (const char *tv1);

/* not for use on localized strings */
char *
stringAsciiToLower (char * s)
{
  for (char *p = s; *p; p++) {
    *p = tolower (*p);
  }
  return s;
}

/* not for use on localized strings */
char *
stringAsciiToUpper (char * s)
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

/* a more efficient way of appending strings, the length must be tracked */
/* used by musicdb.c */
size_t
stringAppend (char *str, size_t maxsz, size_t currsz, const char *data)
{
  const char  *d = data;
  char        *s = str;

  s += currsz;
  --maxsz;
  while (*d != '\0' && currsz < maxsz) {
    *s++ = *d++;
    ++currsz;
  }
  *s = '\0';
  return currsz;
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
