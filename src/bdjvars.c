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

static char *   bdjvars [BDJV_MAX];
static ssize_t  bdjvarsl [BDJVL_MAX];
static bool     initialized = false;

void
bdjvarsInit (void)
{
  if (! initialized) {
    uint16_t        port = sysvarsGetNum (SVL_BASEPORT);

    bdjvarsl [BDJVL_MAIN_PORT] = port++;
    bdjvarsl [BDJVL_PLAYER_PORT] = port++;
    bdjvarsl [BDJVL_PLAYERUI_PORT] = port++;
    bdjvarsl [BDJVL_CONFIGUI_PORT] = port++;
    bdjvarsl [BDJVL_MANAGEUI_PORT] = port++;
    bdjvarsl [BDJVL_MOBILEMQ_PORT] = port++;
    bdjvarsl [BDJVL_REMCTRL_PORT] = port++;
    bdjvarsl [BDJVL_MARQUEE_PORT] = port++;
    bdjvarsl [BDJVL_STARTER_PORT] = port++;
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

inline char *
bdjvarsGetStr (bdjvarkey_t idx)
{
  if (idx >= BDJV_MAX) {
    return NULL;
  }

  return bdjvars [idx];
}

inline ssize_t
bdjvarsGetNum (bdjvarkeyl_t idx)
{
  if (idx >= BDJVL_MAX) {
    return NULL;
  }

  return bdjvarsl [idx];
}


/* internal routines */

static void
bdjvarsAdjustPorts (void)
{
  ssize_t     idx = sysvarsGetNum (SVL_BDJIDX);
  uint16_t    port;

  port = sysvarsGetNum (SVL_BASEPORT) +
      sysvarsGetNum (bdjvarsl [BDJVL_NUM_PORTS]) * idx;
  bdjvarsl [BDJVL_MAIN_PORT] = port++;
  bdjvarsl [BDJVL_PLAYER_PORT] = port++;
  bdjvarsl [BDJVL_PLAYERUI_PORT] = port++;
  bdjvarsl [BDJVL_CONFIGUI_PORT] = port++;
  bdjvarsl [BDJVL_MANAGEUI_PORT] = port++;
  bdjvarsl [BDJVL_MOBILEMQ_PORT] = port++;
  bdjvarsl [BDJVL_REMCTRL_PORT] = port++;
  bdjvarsl [BDJVL_MARQUEE_PORT] = port++;
  bdjvarsl [BDJVL_STARTER_PORT] = port++;
}

inline bool
bdjvarsIsInitialized (void)
{
  return initialized;
}
