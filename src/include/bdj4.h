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

#define ITUNES_NAME     "iTunes"
#define BDJ4_LONG_NAME  "BallroomDJ 4"
#define BDJ4_NAME       "BDJ4"
#define BDJ3_NAME       "BallroomDJ 3"

#define BDJ4_DB_EXT       ".dat"
#define BDJ4_LOCK_EXT     ".lck"
#define BDJ4_LINK_EXT     ".link"
#define BDJ4_PLAYLIST_EXT ".pl"
#define BDJ4_PL_DANCE_EXT ".pldances"
#define BDJ4_SEQUENCE_EXT ".sequence"
#define BDJ4_SONGLIST_EXT ".songlist"
#define BDJ4_IMG_SVG_EXT  ".svg"
#define BDJ4_CONFIG_EXT   ".txt"

/* option data files */
#define MANAGEUI_OPT_FN     "ui-manage"
#define PLAYERUI_OPT_FN     "ui-player"
#define STARTERUI_OPT_FN    "ui-starter"
#define BPMCOUNTER_OPT_FN   "ui-bpmcounter"
/* volume registration */
#define VOLREG_FN           "volreg"
#define VOLREG_LOCK         "volreglock"
#define VOLREG_BDJ4_EXT_FN  "volbdj4"
#define VOLREG_BDJ3_EXT_FN  "volbdj3"
/* alternates/base port */
#define ALT_COUNT_FN        "altcount"
#define BASE_PORT_FN        "baseport"

#endif /* INC_BDJ4_H */
