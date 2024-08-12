/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include "bdj4.h"
#include "bdj4intl.h"
#include "bdjstring.h"
#include "bdjvars.h"
#include "mdebug.h"
#include "sysvars.h"

/* for debugging */
static const char *bdjvarsdesc [BDJV_MAX] = {
  [BDJV_DELETE_PFX] = "DELETE_PFX",
  [BDJV_INSTANCE_NAME] = "INSTANCE_NAME",
  [BDJV_INST_DATATOP] = "INST_DATATOP",
  [BDJV_INST_NAME] = "INST_NAME",
  [BDJV_INST_TARGET] = "INST_TARGET",
  [BDJV_MUSIC_DIR] = "MUSIC_DIR",
  [BDJV_ORIGINAL_EXT] = "ORIGINAL_EXT",
  [BDJV_TS_SECTION] = "TS_SECTION",
  [BDJV_TS_TEST] = "TS_TEST",
};

static const char *bdjvarsldesc [BDJVL_MAX] = {
  [BDJVL_DELETE_PFX_LEN] = "DELETE_PFX_LEN",
  [BDJVL_NUM_PORTS] = "NUM_PORTS",
  [BDJVL_PORT_BPM_COUNTER] = "PORT_BPM_COUNTER",
  [BDJVL_PORT_CONFIGUI] = "PORT_CONFIGUI",
  [BDJVL_PORT_DBUPDATE] = "PORT_DBUPDATE",
  [BDJVL_PORT_HELPERUI] = "PORT_HELPERUI",
  [BDJVL_PORT_MAIN] = "PORT_MAIN",
  [BDJVL_PORT_MANAGEUI] = "PORT_MANAGEUI",
  [BDJVL_PORT_MARQUEE] = "PORT_MARQUEE",
  [BDJVL_PORT_MOBILEMQ] = "PORT_MOBILEMQ",
  [BDJVL_PORT_PLAYER] = "PORT_PLAYER",
  [BDJVL_PORT_PLAYERUI] = "PORT_PLAYERUI",
  [BDJVL_PORT_REMCTRL] = "PORT_REMCTRL",
  [BDJVL_PORT_STARTERUI] = "PORT_STARTERUI",
  [BDJVL_PORT_TEST_SUITE] = "PORT_TEST_SUITE",
};

static char *   bdjvars [BDJV_MAX];
static int64_t  bdjvarsl [BDJVL_MAX];
static bool     initialized = false;

static void    bdjvarsAdjustPorts (void);
static void    bdjvarsSetUniqueName (void);

/* must be called after sysvarsInit() */
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
    bdjvars [BDJV_ORIGINAL_EXT] = mdstrdup (tbuff);

    /* when an audio file is marked for deletion, it is renamed with the */
    /* the 'delete-' prefix in the same directory */
    /* CONTEXT: The prefix name for an audio file marked for deletion (may be abbreviated) */
    snprintf (tbuff, sizeof (tbuff), "%s-", _("delete"));
    bdjvars [BDJV_DELETE_PFX] = mdstrdup (tbuff);
    bdjvarsl [BDJVL_DELETE_PFX_LEN] = strlen (bdjvars [BDJV_DELETE_PFX]);

    bdjvarsl [BDJVL_NUM_PORTS] = BDJVL_NUM_PORTS;
    bdjvarsAdjustPorts ();
    bdjvarsSetUniqueName ();
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

void
bdjvarsUpdateData (void)
{
  bdjvarsAdjustPorts ();
  bdjvarsSetUniqueName ();
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
bdjvarsSetNum (bdjvarkeyl_t idx, int64_t value)   /* KEEP */
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

bool
bdjvarsIsInitialized (void)
{
  return initialized;
}

const char *
bdjvarsDesc (bdjvarkey_t idx)
{
  return bdjvarsdesc [idx];
}

const char *
bdjvarslDesc (bdjvarkeyl_t idx)
{
  return bdjvarsldesc [idx];
}

/* internal routines */

static void
bdjvarsAdjustPorts (void)
{
  int       idx = sysvarsGetNum (SVL_PROFILE_IDX);
  uint16_t  port;

  port = sysvarsGetNum (SVL_BASEPORT) +
      bdjvarsGetNum (BDJVL_NUM_PORTS) * idx;
  for (int i = BDJVL_PORT_MAIN; i < BDJVL_NUM_PORTS; ++i) {
    bdjvarsl [i] = port++;
  }
}

static void
bdjvarsSetUniqueName (void)
{
  int   profidx = 0;
  int   altidx = 0;
  char  tbuff [MAXPATHLEN];

  snprintf (tbuff, sizeof (tbuff), "%s", BDJ4_NAME);
  altidx = sysvarsGetNum (SVL_ALTIDX);
  profidx = sysvarsGetNum (SVL_PROFILE_IDX);
  if (altidx != 0) {
    snprintf (tbuff, sizeof (tbuff), "%s.a%02d-p%02d", BDJ4_NAME, altidx, profidx);
  } else if (profidx != 0) {
    snprintf (tbuff, sizeof (tbuff), "%s.p%02d", BDJ4_NAME, profidx);
  }
  bdjvars [BDJV_INSTANCE_NAME] = mdstrdup (tbuff);
}

