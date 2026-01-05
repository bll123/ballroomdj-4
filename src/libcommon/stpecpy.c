/*
 * Copyright 2024-2026 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 // #include <signal.h>

#include "bdjstring.h"

#if ! _lib_stpecpy

# define STPECPY_DEBUG 0

/* the following code is in the public domain */
/* modified from the linux stpecpy manual page */

char *
stpecpy (char *dst, char *end, const char *restrict src)
{
  char  *p;

#if defined STEPCPY_DEBUG
  if (end - dst == sizeof (char *)) {
    fprintf (stderr, "WARN: stpecpy: length set to sizeof (char *)\n");
  }
#endif
#if defined (BDJ4_MEM_DEBUG)
  if (dst == NULL) {
    fprintf (stderr, "ERR: stpecpy: null destination\n");
  }
  if (end == NULL) {
    fprintf (stderr, "ERR: stpecpy: null end pointer\n");
  }
  if (src == NULL) {
    fprintf (stderr, "ERR: stpecpy: null source\n");
  }
#endif

  if (src == NULL || dst == NULL || end == NULL) {
    return end;
  }
  if (dst == end) {
    return end;
  }

  p = memccpy (dst, src, '\0', end - dst);
  if (p != NULL) {
    return p - 1;
  }

  /* truncation detected */
  end [-1] = '\0';
  return end;
}

#endif /* ! _lib_stpecpy */
