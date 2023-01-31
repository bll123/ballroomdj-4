/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "bdj4.h"
#include "bdj4intl.h"
#include "bdjstring.h"
#include "bdjvars.h"
#include "mdebug.h"
#include "sysvars.h"

static char *   bdjvars [BDJV_MAX];
static int64_t  bdjvarsl [BDJVL_MAX];
static bool     initialized = false;

void
bdjvarsInit (void)
{
  char    tbuff [MAXPATHLEN];

  if (! initialized) {
    for (int i = 0; i < BDJV_MAX; ++i) {
      bdjvars [i] = NULL;
    }

    /* when an audio file is modified, the original is saved with the */
    /* .original (localized) extension in the same directory */
    /* CONTEXT: The suffix for an original audio file (may be abbreviated) */
    snprintf (tbuff, sizeof (tbuff), ".%s", _("original"));
    bdjvars [BDJV_ORIGINAL_EXT] = strdup (tbuff);

    /* when an audio file is marked for deletion, it is renamed with the */
    /* the 'delete-' prefix in the same directory */
    /* CONTEXT: The prefix name for an audio file marked for deletion (may be abbreviated) */
    snprintf (tbuff, sizeof (tbuff), "%s-", _("delete"));
    bdjvars [BDJV_DELETE_PFX] = strdup (tbuff);
    bdjvarsl [BDJVL_DELETE_PFX_LEN] = strlen (bdjvars [BDJV_DELETE_PFX]);

    bdjvarsl [BDJVL_NUM_PORTS] = BDJVL_NUM_PORTS;
    bdjvarsAdjustPorts ();
    initialized = true;
  }
}

void
bdjvarsCleanup (void)
{
  if (initialized) {
    for (int i = 0; i < BDJV_MAX; ++i) {
      dataFree (bdjvars [i]);
      bdjvars [i] = NULL;
    }
    initialized = false;
  }
  return;
}

char *
bdjvarsGetStr (bdjvarkey_t idx)
{
  if (idx >= BDJV_MAX) {
    return NULL;
  }

  return bdjvars [idx];
}

int64_t
bdjvarsGetNum (bdjvarkeyl_t idx)
{
  if (idx >= BDJVL_MAX) {
    return -1;
  }

  return bdjvarsl [idx];
}

void
bdjvarsSetNum (bdjvarkeyl_t idx, int64_t value)
{
  if (idx >= BDJVL_MAX) {
    return;
  }

  bdjvarsl [idx] = value;
}

void
bdjvarsSetStr (bdjvarkey_t idx, const char *str)
{
  if (idx >= BDJV_MAX) {
    return;
  }

  bdjvars [idx] = mdstrdup (str);
}

void
bdjvarsAdjustPorts (void)
{
  int       idx = sysvarsGetNum (SVL_BDJIDX);
  uint16_t  port;

  port = sysvarsGetNum (SVL_BASEPORT) +
      bdjvarsGetNum (BDJVL_NUM_PORTS) * idx;
  for (int i = BDJVL_MAIN_PORT; i < BDJVL_NUM_PORTS; ++i) {
    bdjvarsl [i] = port++;
  }
}

bool
bdjvarsIsInitialized (void)
{
  return initialized;
}
