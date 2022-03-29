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

#endif /* INC_BDJ4_H */
