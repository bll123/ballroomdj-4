#include "config.h"


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "bdjvarsdf.h"

static void *bdjvarsdf [BDJVDF_MAX];

void *
bdjvarsdfGet (bdjvarkeydf_t idx)
{
  if (idx >= BDJVDF_MAX) {
    return NULL;
  }

  return bdjvarsdf [idx];
}

void
bdjvarsdfSet (bdjvarkeydf_t idx, void *data)
{
  if (idx >= BDJVDF_MAX) {
    return;
  }

  bdjvarsdf [idx] = data;
}

