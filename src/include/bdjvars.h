/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_BDJVARS_H
#define INC_BDJVARS_H

#include <stdbool.h>
#include <stdint.h>

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef enum {
  BDJV_DELETE_PFX,
  BDJV_INSTANCE_NAME,       // BDJ4(.alt)?(.profile)?
  BDJV_INST_DATATOP,      // installers, argument
  BDJV_INST_NAME,         // installers, argument
  BDJV_INST_TARGET,       // installers, argument
  BDJV_MUSIC_DIR,         // dbupdate/updater, argument
  BDJV_ORIGINAL_EXT,
  BDJV_TS_SECTION,        // testsuite, argument
  BDJV_TS_TEST,           // testsutie, argument
  BDJV_MAX,
} bdjvarkey_t;

typedef enum {
  /* the ports must precede any other bdjvars keys */
  BDJVL_PORT_MAIN,
  BDJVL_PORT_PLAYER,
  BDJVL_PORT_PLAYERUI,
  BDJVL_PORT_CONFIGUI,
  BDJVL_PORT_HELPERUI,
  BDJVL_PORT_MANAGEUI,
  BDJVL_PORT_MOBILEMQ,
  BDJVL_PORT_REMCTRL,
  BDJVL_PORT_MARQUEE,
  BDJVL_PORT_STARTERUI,
  BDJVL_PORT_DBUPDATE,
  BDJVL_PORT_BPM_COUNTER,
  BDJVL_PORT_TEST_SUITE,
  BDJVL_PORT_SERVER,
  BDJVL_NUM_PORTS,
  /* insert non-port keys here */
  BDJVL_DELETE_PFX_LEN,
  BDJVL_MAX,
} bdjvarkeyl_t;

void    bdjvarsInit (void);
bool    bdjvarsIsInitialized (void);
void    bdjvarsCleanup (void);
void    bdjvarsUpdateData (void);
char *  bdjvarsGetStr (bdjvarkey_t idx);
void    bdjvarsSetNum (bdjvarkeyl_t idx, int64_t value);
void    bdjvarsSetStr (bdjvarkey_t idx, const char *str);
int64_t bdjvarsGetNum (bdjvarkeyl_t idx);
const char  *bdjvarsDesc (bdjvarkey_t idx);
const char  *bdjvarslDesc (bdjvarkeyl_t idx);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_BDJVARS_H */
