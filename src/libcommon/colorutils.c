/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
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

/* takes a standard #rrggbb text string */
void
colorValues (const char *color, double *r, double *g, double *b)
{
  char    temp [20];

  *r = 0.0;
  *g = 0.0;
  *b = 0.0;

  if (color == NULL || *color != '#') {
    return;
  }

  ++color;
  strncpy (temp, color, 2);
  temp [2] = '\0';
  *r = atof (temp) / 255.0;

  color += 2;
  strncpy (temp, color, 2);
  temp [2] = '\0';
  *g = atof (temp) / 255.0;

  color += 2;
  strncpy (temp, color, 2);
  temp [2] = '\0';
  *b = atof (temp) / 255.0;
}

#if 0
double
colorLuminance (const char *color)  /* KEEP */
{
  int     r, g, b;
  double  dr, dg, db;

  sscanf (color, "#%2x%2x%2x", &r, &g, &b);
  dr = 0.299 * (double) r;
  dg = 0.587 * (double) g;
  db = 0.114 * (double) b;
  return dr + dg + db;
}
#endif
