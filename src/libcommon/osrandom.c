/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#define _CRT_RAND_S
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>

#include "fileop.h"
#include "osrandom.h"
#include "tmutil.h"

static const char * URANDOM_FN = "/dev/urandom";
static bool initialized = false;

double
dRandom (void)
{
  double        dval = 0.0;
#if _lib_random
  unsigned long lval;
#endif
#if _lib_rand_s
  unsigned int  ival;
#endif

  if (! initialized) {
    sRandom ();
    initialized = true;
    fprintf (stderr, "WARN: osrandom: not initialized\n");
  }

#if _lib_random
  lval = random ();
  dval = (double) lval / (double) UINT_MAX;
#endif
#if ! _lib_random && _lib_rand_s
  rand_s (&ival);
  dval = (double) ival / (double) RAND_MAX;
#endif
  return dval;
}

void
sRandom (void)
{
  ssize_t       pid = (long) getpid ();
  unsigned int  seed = (ssize_t) mstime () ^ (pid + (pid << 15));

  /* the /dev/urandom file exists on linux and macos */
  if (fileopFileExists (URANDOM_FN)) {
    int     fd;

    fd = open (URANDOM_FN, O_RDONLY);
    if (fd >= 0) {
      (void) ! read (fd, &seed, sizeof (seed));
      close (fd);
    }
  }

#if _lib_random
  srandom (seed);
  initialized = true;
#endif
#if ! _lib_random && _lib_srand
  srand (seed);
  initialized = true;
#endif
}
