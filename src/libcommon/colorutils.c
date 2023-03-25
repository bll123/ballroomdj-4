/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "colorutils.h"
#include "osrandom.h"

char *
createRandomColor (char *tbuff, size_t sz)
{
  int r, g, b;

  r = (int) (dRandom () * 256.0);
  g = (int) (dRandom () * 256.0);
  b = (int) (dRandom () * 256.0);
  snprintf (tbuff, sz, "#%02x%02x%02x", r, g, b);
  return tbuff;
}

double
colorLuminance (const char *color)
{
  int     r, g, b;
  double  dr, dg, db;

  sscanf (color, "#%2x%2x%2x", &r, &g, &b);
  dr = 0.299 * (double) r;
  dg = 0.587 * (double) g;
  db = 0.114 * (double) b;
  return dr + dg + db;
}

