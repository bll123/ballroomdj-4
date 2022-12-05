/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_PLAYER_H
#define INC_PLAYER_H

typedef enum {
  PL_STATE_UNKNOWN,
  PL_STATE_STOPPED,
  PL_STATE_LOADING,
  PL_STATE_PLAYING,
  PL_STATE_PAUSED,
  PL_STATE_IN_FADEOUT,
  PL_STATE_IN_GAP,
  PL_STATE_MAX,
} playerstate_t;

enum {
  PL_UNIQUE_ANN = -2,
};

#endif /* INC_PLAYER_H */
