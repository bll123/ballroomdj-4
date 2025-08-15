/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_BDJ4_H
#define INC_BDJ4_H

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

#if ! defined (MAXPATHLEN)
enum {
  MAXPATHLEN = 1024,
};
#endif

enum {
  /* used for queueing playlists */
  EDIT_FALSE,
  EDIT_TRUE,
};

/* processing states */
enum {
  BDJ4_STATE_OFF,
  BDJ4_STATE_START,
  BDJ4_STATE_WAIT,
  BDJ4_STATE_PROCESS,
  BDJ4_STATE_FINISH,
  BDJ4_STATE_MAX,
};

enum {
  STOP_WAIT_COUNT_MAX = 60,
  EXIT_WAIT_COUNT = 60,
  INSERT_AT_SELECTION = -2,
  /* when changing the queue max, be sure to update include/musicq.h */
  BDJ4_QUEUE_MAX = 5,
  QUEUE_LOC_LAST = 999,
  SONG_LIST_LIMIT = 900,
  REPEAT_TIME = 175,
  BDJ4_DFLT_TAG_WEIGHT = 90,
  BDJ4_MAX_ALT = 10,
  MAX_PL_NM_LEN = 200,
};

enum {
  BDJ4_IDENT_FREE = 0xccaa006465657266,
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
#define BDJ4_PODCAST_EXT  ".podcast"
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

/* audio content recognition services */
#define ACOUSTID_NAME     "AcoustID"
#define ACRCLOUD_NAME     "ACRCloud"
#define AUDIOTAG_NAME     "AudioTag"

/* data files */
#define ASCONF_FN           "audiosrc"
#define BDJ_URIFN           "bdjuri"
#define AUDIOADJ_FN         "audioadjust"
#define AUDIOTAGINTFC_FN    "audiotagintfc"
#define AUTOSEL_FN          "autoselection"
#define DANCE_FN            "dances"
#define ITUNES_FIELDS_FN    "itunes-fields"
#define ITUNES_STARS_FN     "itunes-stars"
#define NEWINSTALL_FN       "newinstall"
#define LOCALIZATION_FN     "localization"
#define ORGOPT_FN           "orgopt"
#define RELOAD_FN           "reload"
#define RELOAD_CURR_FN      "reload-curr"
#define SORTOPT_FN          "sortopt"
/* display settings files */
#define DS_FILTER_FN        "ds-songfilter"
#define DS_QUICKEDIT_FN     "ds-quickedit"
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
#define ALT_IDX_FN          "altidx"
#define BASE_PORT_FN        "baseport"
#define INST_PATH_FN        "installdir"
#define NEWINST_FN          "newinstall"
#define READONLY_FN         "readonly"
/* installation */
#define BDJ4_INST_DIR  "bdj4-install"

/* macos stuff */
#define MACOS_APP_EXT       ".app"
#define MACOS_APP_PREFIX    "/Contents/MacOS"
#define MACOS_DIR_LIBDATA   "/Library/Application Support"

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_BDJ4_H */
