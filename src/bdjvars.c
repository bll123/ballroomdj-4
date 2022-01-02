#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bdjvars.h"
#include "sysvars.h"

static void bdjvarsAdjustPorts (void);

char *     bdjvars [BDJV_MAX];
long       bdjvarsl [BDJVL_MAX];

void
bdjvarsInit (void)
{
  bdjvarsl [BDJVL_MAIN_PORT] = 35548;
  bdjvarsl [BDJVL_PLAYER_PORT] = bdjvarsl [BDJVL_MAIN_PORT] + 1;
  bdjvarsl [BDJVL_NUM_PORTS] = 2;

  bdjvarsAdjustPorts ();
}

/* internal routines */

void
bdjvarsAdjustPorts (void)
{
  long idx = lsysvars [SVL_BDJIDX];

  bdjvarsl [BDJVL_MAIN_PORT] += bdjvarsl [BDJVL_NUM_PORTS] * idx;
  bdjvarsl [BDJVL_PLAYER_PORT] += bdjvarsl [BDJVL_NUM_PORTS] * idx;
}
