#ifndef INC_BDJVARS_H
#define INC_BDJVARS_H

typedef enum {
  BDJV_DB_TOP_DIR,    // temporary
  BDJV_MAX,
} bdjvarkey_t;

typedef enum {
  /* the ports must precede any other bdjvars keys */
  BDJVL_MAIN_PORT,
  BDJVL_PLAYER_PORT,
  BDJVL_PLAYERUI_PORT,
  BDJVL_CONFIGUI_PORT,
  BDJVL_HELPERUI_PORT,
  BDJVL_MANAGEUI_PORT,
  BDJVL_MOBILEMQ_PORT,
  BDJVL_REMCTRL_PORT,
  BDJVL_MARQUEE_PORT,
  BDJVL_STARTERUI_PORT,
  BDJVL_DBUPDATE_PORT,
  BDJVL_DBTAG_PORT,
  BDJVL_BPM_COUNTER_PORT,
  BDJVL_NUM_PORTS,
  /* insert non-port keys here */
  BDJVL_MAX,
} bdjvarkeyl_t;

void    bdjvarsInit (void);
bool    bdjvarsIsInitialized (void);
void    bdjvarsCleanup (void);
char *  bdjvarsGetStr (bdjvarkey_t idx);
void    bdjvarsSetNum (bdjvarkeyl_t idx, ssize_t value);
void    bdjvarsSetStr (bdjvarkey_t idx, const char *str);
ssize_t bdjvarsGetNum (bdjvarkeyl_t idx);
void    bdjvarsAdjustPorts (void);

#endif /* INC_BDJVARS_H */
