/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <assert.h>

#if _hdr_windows
/* required for bcrypt.h */
# define WIN32_LEAN_AND_MEAN 1
# include <windows.h>
#endif
#if _hdr_bcrypt
# include <bcrypt.h>
#endif

#include "fileop.h"
#include "osrandom.h"
#include "tmutil.h"

static const char * URANDOM_FN = "/dev/urandom";
static bool initialized = false;

double
dRandom (void)
{
  double        dval = 0.0;
  unsigned long tval;

  if (! initialized) {
    sRandom ();
    initialized = true;
    fprintf (stderr, "WARN: osrandom: not initialized\n");
  }

#if _lib_BCryptGenRandom
  BCryptGenRandom (NULL, (PUCHAR) &tval, sizeof (tval),
      BCRYPT_USE_SYSTEM_PREFERRED_RNG);
  dval = (double) tval / (double) ULONG_MAX;
#endif
#if _lib_random
  tval = random ();
  /* random() returns a long, but the range is for an int */
  dval = (double) tval / (double) INT_MAX;
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

#if _lib_srandom
  srandom (seed);
  initialized = true;
  assert (sizeof (int) == 4);   /* make sure INT_MAX is 2^32-1 */
#endif
#if _lib_BCryptGenRandom
  initialized = true;
#endif
}
