#ifndef INC_BDJVARS_H
#define INC_BDJVARS_H

typedef enum {
  STATE_NOT_RUNNING,
  STATE_INITIALIZING,
  STATE_RUNNING,
  STATE_CLOSING,
} programstate_t;

typedef enum {
  BDJV_UNKNOWN,
  BDJV_MAX
} bdjvarkey_t;

typedef enum {
  BDJVL_MAIN_PORT,
  BDJVL_PLAYER_PORT,
  BDJVL_NUM_PORTS,
  BDJVL_MAX
} bdjvarkeyl_t;

extern char *     bdjvars [BDJV_MAX];
extern long       lbdjvars [BDJVL_MAX];

void    bdjvarsInit (void);

#endif /* INC_BDJVARS_H */
