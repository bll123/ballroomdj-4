/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>

#include "bdjstring.h"

  // static const char *versionNext (const char *tv1);

/* not for use on localized strings */
char *
stringAsciiToLower (char * s)
{
  if (s == NULL) {
    return s;
  }

  for (char *p = s; *p; p++) {
    *p = tolower (*p);
  }
  return s;
}

/* not for use on localized strings */
char *
stringAsciiToUpper (char * s)
{
  if (s == NULL) {
    return s;
  }

  for (char *p = s; *p; p++) {
    *p = toupper (*p);
  }
  return s;
}

void
stringTrim (char *s)
{
  ssize_t     len;

  if (s == NULL) {
    return;
  }

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

  if (s == NULL) {
    return;
  }

  len = strlen (s);
  --len;
  while (len >= 0 && s [len] == c) {
    s [len] = '\0';
    --len;
  }

  return;
}

#if 0
int
versionCompare (const char *v1, const char *v2) /* UNUSED */
{
  const char  *tv1, *tv2;
  int         iv1, iv2;
  int         rc = 0;

  if (v1 == NULL && v2 == NULL) {
    return rc;
  }
  if (v1 != NULL && v2 == NULL) {
    return 1;
  }
  if (v1 == NULL && v2 != NULL) {
    return -1;
  }

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
#endif

/* internal routines */

#if 0
static const char *
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
#endif

