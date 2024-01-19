/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>

#include "bdj4arg.h"
#include "fileop.h"
#include "dirop.h"
#include "sysvars.h"

int
main (int argc, char *argv [])
{
  const char  *fn;
  int         count;
  bdj4arg_t   *bdj4arg;
  const char  *targ;

  bdj4arg = bdj4argInit (argc, argv);

  targ = bdj4argGet (bdj4arg, 0, argv [0]);
  sysvarsInit (targ);

  fn = "tmp";

  count = 0;
  while (fileopIsDirectory (fn)) {
    diropDeleteDir (fn, DIROP_ALL);
    ++count;
    if (count > 5) {
      return 1;
    }
  }

  diropMakeDir (fn);

  bdj4argCleanup (bdj4arg);
  return 0;
}
