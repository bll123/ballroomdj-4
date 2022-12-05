/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <getopt.h>

int
main (int argc, char *argv[])
{
  unsigned int val = 1;
  int   c = 0;
  int   exitcode = 0;
  int   option_index = 0;
  char  *sval = NULL;

  /* abuse --profile to pass in the sleep time */
  /* abuse --theme to pass in data to output */
  static struct option bdj_options [] = {
    { "bdj4",       no_argument,        NULL,   'B' },
    { "profile",    required_argument,  NULL,   'p' },
    { "debug",      required_argument,  NULL,   'd' },
    { "theme",      required_argument,  NULL,   't' },
    { NULL,         0,                  NULL,   0 }
  };

  while ((c = getopt_long_only (argc, argv, "p:d:", bdj_options, &option_index)) != -1) {
    switch (c) {
      case 'd': {
        exitcode = atoi (optarg);
        break;
      }
      case 'p': {
        if (optarg) {
          val = (unsigned int) atol (optarg);
        }
        break;
      }
      case 't': {
        if (optarg) {
          sval = optarg;
        }
        break;
      }
      default: {
        break;
      }
    }
  }

  if (val > 0) {
    sleep (val);
  }
  if (sval != NULL) {
    fprintf (stdout, "%s\n", sval);
  }
  return exitcode;
}
