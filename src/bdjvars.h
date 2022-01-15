#ifndef INC_BDJVARS_H
#define INC_BDJVARS_H

typedef enum {
  PL_STATE_STOPPED,
  PL_STATE_LOADING,
  PL_STATE_PLAYING,
  PL_STATE_PAUSED,
} playerstate_t;

typedef enum {
  STATE_NOT_RUNNING,
  STATE_INITIALIZING,
  STATE_LISTENING,
  STATE_CONNECTING,
  STATE_WAIT_HANDSHAKE,
  STATE_RUNNING,
  STATE_CLOSING,
} programstate_t;

typedef enum {
  BDJV_UNKNOWN,
  BDJV_MAX,
} bdjvarkey_t;

typedef enum {
  BDJVL_MAIN_PORT,
  BDJVL_PLAYER_PORT,
  BDJVL_GUI_PORT,
  BDJVL_NUM_PORTS,
  BDJVL_MAX,
} bdjvarkeyl_t;

extern char *     bdjvars [BDJV_MAX];
extern ssize_t    bdjvarsl [BDJVL_MAX];

void    bdjvarsInit (void);
void    bdjvarsCleanup (void);
char    *songFullFileName (char *sfname);

#endif /* INC_BDJVARS_H */
