/*
 * Copyright 2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 // #include <signal.h>

#include "bdjstring.h"

#if ! _lib_stpecpy

/* the following code is in the public domain */
/* modified from the linux stpecpy manual page */

char *
stpecpy (char *dst, char end[0], const char *restrict src)
{
  char  *p;

  if (dst == NULL) {
    fprintf (stderr, "ERR: stpecpy: null destination\n");
    return end;
  }
  if (src == NULL) {
    fprintf (stderr, "ERR: stpecpy: null source\n");
    return end;
  }

  if (src [strlen (src)] != '\0') {
    fprintf (stderr, "ERR: stpecpy: source string is not null terminated\n");
    // raise (SIGSEGV);
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
