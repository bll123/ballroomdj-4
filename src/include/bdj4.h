/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_BDJ4_H
#define INC_BDJ4_H

#if ! defined (MAXPATHLEN)
enum {
  MAXPATHLEN = 512,
};
#endif

enum {
  /* used for queueing playlists */
  EDIT_TRUE,
  EDIT_FALSE,
  /* processing states */
  BDJ4_STATE_OFF,
  BDJ4_STATE_START,
  BDJ4_STATE_WAIT,
  BDJ4_STATE_PROCESS,
  BDJ4_STATE_FINISH,
};

enum {
  STOP_WAIT_COUNT_MAX = 60,
  EXIT_WAIT_COUNT = 60,
  INSERT_AT_SELECTION = -2,
  /* when changing the queue max, be sure to update include/musicq.h */
  BDJ4_QUEUE_MAX = 4,
  QUEUE_LOC_LAST = 999,
  SONG_LIST_LIMIT = 900,
};

#define BDJ4_LONG_NAME    "BallroomDJ 4"
#define BDJ4_NAME         "BDJ4"
#define BDJ3_NAME         "BallroomDJ 3"
#define BDJ4_MP3_LABEL    "MP3"

#define BDJ4_DB_EXT       ".dat"
#define BDJ4_HTML_EXT     ".html"
#define BDJ4_LOCK_EXT     ".lck"
#define BDJ4_PLAYLIST_EXT ".pl"
#define BDJ4_PL_DANCE_EXT ".pldances"
#define BDJ4_SEQUENCE_EXT ".sequence"
#define BDJ4_SONGLIST_EXT ".songlist"
#define BDJ4_IMG_SVG_EXT  ".svg"
#define BDJ4_IMG_PNG_EXT  ".png"
#define BDJ4_CONFIG_EXT   ".txt"
#define BDJ4_CSS_EXT      ".css"
#define BDJ4_GENERIC_ORIG_EXT ".original"

/* some itunes stuff */
#define ITUNES_NAME       "iTunes"
#define ITUNES_MEDIA_NAME "iTunes Media"
#define ITUNES_XML_NAME   "iTunes Music Library.xml"

/* data files */
#define AUDIOADJ_FN         "audioadjust"
#define AUDIOTAGINTFC_FN    "audiotagintfc"
#define AUTOSEL_FN          "autoselection"
#define DANCE_FN            "dances"
#define ITUNES_FIELDS_FN    "itunes-fields"
#define ITUNES_STARS_FN     "itunes-stars"
#define NEWINSTALL_FN       "newinstall"
/* option data files */
#define BPMCOUNTER_OPT_FN   "ui-bpmcounter"
#define CONFIGUI_OPT_FN     "ui-config"
#define MANAGEUI_OPT_FN     "ui-manage"
#define PLAYERUI_OPT_FN     "ui-player"
#define STARTERUI_OPT_FN    "ui-starter"
/* volume registration */
#define VOLREG_FN           "volreg"
#define VOLREG_BDJ4_EXT_FN  "volbdj4"
#define VOLREG_BDJ3_EXT_FN  "volbdj3"
/* installation/base port */
#define ALT_COUNT_FN        "altcount"
#define ALT_INST_PATH_FN    "altinstdir"
#define BASE_PORT_FN        "baseport"
#define INST_PATH_FN        "installdir"
#define NEWINST_FN          "newinstall"
#define READONLY_FN         "readonly"
/* cache files */
#define SYSVARS_PY_DOT_VERS_FN  "pydotvers"
#define SYSVARS_PY_VERS_FN      "pyvers"

#endif /* INC_BDJ4_H */
