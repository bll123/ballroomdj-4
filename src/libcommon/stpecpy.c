/*
 * Copyright 2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "bdjstring.h"

#if ! _lib_stpecpy

/* the following code is in the public domain */
/* from the linux stpecpy manual page */

char *
stpecpy (char *dst, char end[0], const char *restrict src)
{
  char  *p;

  if (src [strlen (src)] != '\0') {
    raise (SIGSEGV);
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
