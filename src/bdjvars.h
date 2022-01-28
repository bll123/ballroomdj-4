#ifndef INC_BDJVARS_H
#define INC_BDJVARS_H

typedef enum {
  BDJV_UNKNOWN,
  BDJV_MAX,
} bdjvarkey_t;

typedef enum {
  BDJVL_MAIN_PORT,
  BDJVL_PLAYER_PORT,
  BDJVL_PLAYERGUI_PORT,
  BDJVL_CONFIGGUI_PORT,
  BDJVL_MANAGEGUI_PORT,
  BDJVL_MOBILEMQ_PORT,
  BDJVL_REMCTRL_PORT,
  BDJVL_MARQUEE_PORT,
  BDJVL_NUM_PORTS,
  BDJVL_MAX,
} bdjvarkeyl_t;

extern char *     bdjvars [BDJV_MAX];
extern ssize_t    bdjvarsl [BDJVL_MAX];

void    bdjvarsInit (void);
bool    bdjvarsIsInitialized (void);
void    bdjvarsCleanup (void);

#endif /* INC_BDJVARS_H */
