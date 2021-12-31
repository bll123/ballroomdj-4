#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sysvars.h"
#include "bdjvars.h"

static void bdjvarsAdjustPorts (void);

char *     bdjvars [BDJV_MAX];
long       lbdjvars [BDJVL_MAX];

void
bdjvarsInit (void)
{
  lbdjvars [BDJVL_MAIN_PORT] = 35548;
  lbdjvars [BDJVL_PLAYER_PORT] = lbdjvars [BDJVL_MAIN_PORT] + 1;
  lbdjvars [BDJVL_NUM_PORTS] = 2;

  bdjvarsAdjustPorts ();
}

/* internal routines */


void
bdjvarsAdjustPorts (void)
{
  long idx = lsysvars [SVL_BDJIDX];

  lbdjvars [BDJVL_MAIN_PORT] += lbdjvars [BDJVL_NUM_PORTS] * idx;
  lbdjvars [BDJVL_PLAYER_PORT] += lbdjvars [BDJVL_NUM_PORTS] * idx;
}
