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
};

enum {
  STOP_WAIT_COUNT_MAX = 60,
  EXIT_WAIT_COUNT = 60,
  INSERT_AT_SELECTION = -2,
  /* when changing the queue max, be sure to update include/musicq.h */
  BDJ4_QUEUE_MAX = 4,
};

#define ITUNES_NAME       "iTunes"
#define ITUNES_MEDIA_NAME "iTunes Media"
#define ITUNES_XML_NAME   "iTunes Music Library.xml"

#define BDJ4_LONG_NAME    "BallroomDJ 4"
#define BDJ4_NAME         "BDJ4"
#define BDJ3_NAME         "BallroomDJ 3"

#define BDJ4_DB_EXT       ".dat"
#define BDJ4_LOCK_EXT     ".lck"
#define BDJ4_LINK_EXT     ".link"
#define BDJ4_PLAYLIST_EXT ".pl"
#define BDJ4_PL_DANCE_EXT ".pldances"
#define BDJ4_SEQUENCE_EXT ".sequence"
#define BDJ4_SONGLIST_EXT ".songlist"
#define BDJ4_IMG_SVG_EXT  ".svg"
#define BDJ4_CONFIG_EXT   ".txt"
/* when an audio file is modified, the original is saved with the */
/* .original extension in the same directory */
#define BDJ4_ORIGINAL_EXT ".original"
/* when an audio file is marked for deletion, it is renamed with the */
/* the .delete extension in the same directory */
#define BDJ4_DELETE_EXT ".delete"

/* data files */
#define ITUNES_STARS_FN     "itunes-stars"
#define ITUNES_FIELDS_FN    "itunes-fields"
/* option data files */
#define CONFIGUI_OPT_FN     "ui-config"
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
