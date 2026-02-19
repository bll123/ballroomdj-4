/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <getopt.h>

#include "bdj4arg.h"
#include "fileop.h"
#include "dirop.h"
#include "mdebug.h"
#include "tmutil.h"

int
main (int argc, char *argv [])
{
  bdj4arg_t   *bdj4arg;
  const char  *targ = NULL;
  const char  *tmp = NULL;
  int         c;
  int         option_index = 0;

  static struct option bdj_options [] = {
    { "bdj4",           no_argument,        NULL,   0 },
    { "bdj4cleaninst",  required_argument,  NULL,   1 },
    /* launcher options */
    { "debug",          required_argument,  NULL,   0 },
    { "debugself",      no_argument,        NULL,   0 },
    { "nodetach",       no_argument,        NULL,   0 },
    { "origcwd",        required_argument,  NULL,   0 },
    { "scale",          required_argument,  NULL,   0 },
    { "theme",          required_argument,  NULL,   0 },
    { "pli",            required_argument,  NULL,   0 },
    { "wait",           no_argument,        NULL,   0 },
  };

  bdj4arg = bdj4argInit (argc, argv);

  while ((c = getopt_long_only (argc, bdj4argGetArgv (bdj4arg),
      "", bdj_options, &option_index)) != -1) {
    switch (c) {
      case 1: {
        targ = bdj4argGet (bdj4arg, optind - 1, optarg);
        break;
      }
    }
  }

  /* bdj4cleantmp is started by bdj4, and will be in the datatopdir */

  /* make sure that this process can only be used to */
  /* clean bdj4-install directories */
  tmp = strstr (targ, "bdj4-install");
  if (targ == NULL || tmp == NULL || ! fileopIsDirectory (targ)) {
    bdj4argCleanup (bdj4arg);
    return 0;
  }

  /* wait for the installer to finish up */
  mssleep (3000);

  diropDeleteDir (targ, DIROP_ALL);

  bdj4argCleanup (bdj4arg);
  return 0;
}
