/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_PLAYER_H
#define INC_PLAYER_H

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef enum {
  PL_STATE_UNKNOWN,
  PL_STATE_STOPPED,
  PL_STATE_LOADING,
  PL_STATE_PLAYING,
  PL_STATE_PAUSED,
  PL_STATE_IN_FADEOUT,
  PL_STATE_IN_GAP,
  PL_PROGSTATE_MAX,
} playerstate_t;

enum {
  PL_UNIQUE_ANN = -2,
};

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_PLAYER_H */
