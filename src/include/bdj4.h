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
  PL_STATE_MAX,
} playerstate_t;

#if ! defined (MAXPATHLEN)
enum {
  MAXPATHLEN = 512,
};
#endif

enum {
  STOP_WAIT_COUNT_MAX = 60,
  EXIT_WAIT_COUNT = 60,
  INSERT_AT_SELECTION = -2,
};

#define BDJ4_LONG_NAME  "BallroomDJ 4"
#define BDJ4_NAME       "BDJ4"
#define BDJ3_NAME       "BallroomDJ 3"

#define BDJ4_PLAYLIST_EXT ".pl"
#define BDJ4_PL_DANCE_EXT ".pldances"
#define BDJ4_CONFIG_EXT   ".txt"
#define BDJ4_DB_EXT       ".dat"
#define BDJ4_SONGLIST_EXT ".songlist"
#define BDJ4_SEQUENCE_EXT ".sequence"

#define MANAGEUI_OPT_FN   "ui-manage"
#define PLAYERUI_OPT_FN   "ui-player"
#define STARTERUI_OPT_FN  "ui-starter"
#define BPMCOUNTER_OPT_FN "ui-bpmcounter"

#endif /* INC_BDJ4_H */
