#ifndef INC_BDJ4_H
#define INC_BDJ4_H

#include "sock.h"

typedef enum {
  PL_STATE_STOPPED,
  PL_STATE_LOADING,
  PL_STATE_PLAYING,
  PL_STATE_PAUSED,
  PL_STATE_IN_FADEOUT,
  PL_STATE_IN_GAP,
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
  PROCESS_PLAYER,         // must be first for initialization to work.
  PROCESS_MAIN,
  PROCESS_MOBILEMQ,
  PROCESS_REMCTRL,
  PROCESS_GUI,
  PROCESS_MAX,
} processconnidx_t;

typedef struct {
  Sock_t        sock;
  char          *name;
  bool          handshake : 1;
  bool          started : 1;
} processconn_t;

#endif /* INC_BDJ4_H */
