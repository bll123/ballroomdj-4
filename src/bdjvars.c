#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "bdjvars.h"
#include "sysvars.h"
#include "bdjstring.h"
#include "bdjopt.h"

static void   bdjvarsAdjustPorts (void);

char *      bdjvars [BDJV_MAX];
ssize_t     bdjvarsl [BDJVL_MAX];
static bool initialized = false;

void
bdjvarsInit (void)
{
  if (! initialized) {
    uint16_t        port = lsysvars [SVL_BASEPORT];

    bdjvarsl [BDJVL_MAIN_PORT] = port++;
    bdjvarsl [BDJVL_PLAYER_PORT] = port++;
    bdjvarsl [BDJVL_PLAYERUI_PORT] = port++;
    bdjvarsl [BDJVL_CONFIGUI_PORT] = port++;
    bdjvarsl [BDJVL_MANAGEUI_PORT] = port++;
    bdjvarsl [BDJVL_MOBILEMQ_PORT] = port++;
    bdjvarsl [BDJVL_REMCTRL_PORT] = port++;
    bdjvarsl [BDJVL_MARQUEE_PORT] = port++;
    bdjvarsl [BDJVL_NUM_PORTS] = BDJVL_NUM_PORTS;

    bdjvarsAdjustPorts ();
    initialized = true;
  }
}

void
bdjvarsCleanup (void)
{
  return;
}

/* internal routines */

static void
bdjvarsAdjustPorts (void)
{
  ssize_t     idx = lsysvars [SVL_BDJIDX];
  uint16_t    port;

  port = lsysvars [SVL_BASEPORT] + bdjvarsl [BDJVL_NUM_PORTS] * idx;
  bdjvarsl [BDJVL_MAIN_PORT] = port++;
  bdjvarsl [BDJVL_PLAYER_PORT] = port++;
  bdjvarsl [BDJVL_PLAYERUI_PORT] = port++;
  bdjvarsl [BDJVL_CONFIGUI_PORT] = port++;
  bdjvarsl [BDJVL_MANAGEUI_PORT] = port++;
  bdjvarsl [BDJVL_MOBILEMQ_PORT] = port++;
  bdjvarsl [BDJVL_REMCTRL_PORT] = port++;
  bdjvarsl [BDJVL_MARQUEE_PORT] = port++;
}

bool
bdjvarsIsInitialized (void)
{
  return initialized;
}
