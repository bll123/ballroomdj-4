/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "aesencdec.h"
#include "bdjopt.h"
#include "osrandom.h"
#include "sysvars.h"

int
main (int argc, char *argv [])
{
  int     c = 0;
  int     option_index = 0;
  char    buff [4096];

  static struct option bdj_options [] = {
    { "aesed",        no_argument,        NULL,   0 },
    { "bdj4",         no_argument,        NULL,   0 },
    /* launcher options */
    { "debugself",    no_argument,        NULL,   0 },
    { "debug",        required_argument,  NULL,   'd' },
    { "nodetach",     no_argument,        NULL,   0, },
    { "origcwd",      required_argument,  NULL,   0 },
    { "scale",        required_argument,  NULL,   0 },
    { "theme",        required_argument,  NULL,   0 },
  };

  while ((c = getopt_long_only (argc, argv,
      "", bdj_options, &option_index)) != -1) {
    ;
  }

  if (argc - optind != 2) {
    fprintf (stderr, "usage %s {e|d} <str>\n", argv [0]);
    exit (1);
  }

  sRandom ();
  sysvarsInit (argv [0], SYSVARS_FLAG_ALL);
  // logStartAppend ("aesed", "taes", LOG_IMPORTANT | LOG_BASIC | LOG_INFO);
  bdjoptInit ();

  *buff = '\0';
  if (strcmp (argv [optind], "e") == 0) {
    aesencrypt (argv [optind + 1], buff, sizeof (buff));
  }
  if (strcmp (argv [optind], "d") == 0) {
    aesdecrypt (argv [optind + 1], buff, sizeof (buff));
  }
  fprintf (stderr, "%s\n", buff);

  bdjoptCleanup ();
  // logEnd ();

  return 0;
}
