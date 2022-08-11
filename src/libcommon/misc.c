#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdarg.h>

#include "misc.h"
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
