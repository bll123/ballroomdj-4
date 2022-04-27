#ifndef INC_BDJ4_H
#define INC_BDJ4_H

typedef enum {
  PL_STATE_UNKNOWN,
  PL_STATE_STOPPED,
  PL_STATE_LOADING,
  PL_STATE_PLAYING,
  PL_STATE_PAUSED,
  PL_STATE_IN_FADEOUT,
  PL_STATE_IN_GAP,
} playerstate_t;

#if ! defined (MAXPATHLEN)
# define MAXPATHLEN         512
#endif

#define STOP_WAIT_COUNT_MAX   60

#define BDJ4_LONG_NAME  "BallroomDJ 4"
#define BDJ4_NAME       "BDJ4"
#define BDJ3_NAME       "BallroomDJ 3"

#endif /* INC_BDJ4_H */
