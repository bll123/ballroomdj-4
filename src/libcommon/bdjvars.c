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

    for (int i = 0; i < BDJV_MAX; ++i) {
      bdjvars [i] = NULL;
    }
    for (int i = BDJVL_MAIN_PORT; i < BDJVL_NUM_PORTS; ++i) {
      bdjvarsl [i] = port++;
    }
    bdjvarsl [BDJVL_NUM_PORTS] = BDJVL_NUM_PORTS;

    bdjvarsAdjustPorts ();
    initialized = true;
  }
}

void
bdjvarsCleanup (void)
{
  for (int i = 0; i < BDJV_MAX; ++i) {
    if (bdjvars [i] != NULL) {
      free (bdjvars [i]);
      bdjvars [i] = NULL;
    }
  }
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
    return -1;
  }

  return bdjvarsl [idx];
}


void
bdjvarsSetStr (bdjvarkey_t idx, const char *str)
{
  if (idx >= BDJV_MAX) {
    return;
  }

  bdjvars [idx] = strdup (str);
}

/* internal routines */

static void
bdjvarsAdjustPorts (void)
{
  ssize_t     idx = sysvarsGetNum (SVL_BDJIDX);
  uint16_t    port;

  port = sysvarsGetNum (SVL_BASEPORT) +
      bdjvarsGetNum (BDJVL_NUM_PORTS) * idx;
  for (int i = BDJVL_MAIN_PORT; i < BDJVL_NUM_PORTS; ++i) {
    bdjvarsl [i] = port++;
  }
}

inline bool
bdjvarsIsInitialized (void)
{
  return initialized;
}
