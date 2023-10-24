/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
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

#include "fileop.h"
#include "dirop.h"
#include "sysvars.h"

int
main (int argc, const char *argv [])
{
  const char  *fn;
  int         count;

  sysvarsInit (argv [0]);

  fn = "tmp";

  count = 0;
  while (fileopIsDirectory (fn)) {
    diropDeleteDir (fn);
    ++count;
    if (count > 5) {
      return 1;
    }
  }

  diropMakeDir (fn);

  return 0;
}
