/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#include <stdio.h>
#include <stdlib.h>

#include "osrandom.h"
#include "vsencdec.h"

int
main (int argc, char *argv [])
{
  char    buff [100];

  if (argc != 2) {
    fprintf (stderr, "usage %s <str>\n", argv [0]);
    exit (1);
  }

  sRandom ();
  vsencdec (argv [1], buff, sizeof (buff));
  fprintf (stderr, "%s\n", buff);
  return 0;
}
