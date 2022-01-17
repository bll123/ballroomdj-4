#ifndef INC_BDJVARS_H
#define INC_BDJVARS_H

typedef enum {
  BDJV_UNKNOWN,
  BDJV_MAX,
} bdjvarkey_t;

typedef enum {
  BDJVL_MAIN_PORT,
  BDJVL_PLAYER_PORT,
  BDJVL_GUI_PORT,
  BDJVL_MOBILEMQ_PORT,
  BDJVL_REMCONTROL_PORT,
  BDJVL_NUM_PORTS,
  BDJVL_MAX,
} bdjvarkeyl_t;

extern char *     bdjvars [BDJV_MAX];
extern ssize_t    bdjvarsl [BDJVL_MAX];

void    bdjvarsInit (void);
void    bdjvarsCleanup (void);
char    *songFullFileName (char *sfname);

#endif /* INC_BDJVARS_H */
