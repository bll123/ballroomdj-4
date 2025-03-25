/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_CONFIGUI_H
#define INC_CONFIGUI_H

#include <stdint.h>

#include "callback.h"
#include "dispsel.h"
#include "ilist.h"
#include "nlist.h"
#include "orgutil.h"
#include "uidd.h"
#include "uiduallist.h"
#include "uinbutil.h"
#include "uiselectfile.h"
#include "uivirtlist.h"
#include "uiwcont.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

/* base type */
typedef enum {
  CONFUI_NONE,
  CONFUI_ENTRY,
  CONFUI_ENTRY_ENCRYPT,
  CONFUI_FONT,
  CONFUI_COLOR,
  CONFUI_DD,
  CONFUI_SPINBOX_TEXT,
  CONFUI_SPINBOX_NUM,
  CONFUI_SPINBOX_DOUBLE,
  CONFUI_SPINBOX_TIME,
  CONFUI_CHECK_BUTTON,
  CONFUI_SWITCH,
} confuibasetype_t;

/* out type */
typedef enum {
  CONFUI_OUT_NONE,
  CONFUI_OUT_STR,
  CONFUI_OUT_DOUBLE,
  CONFUI_OUT_NUM,
  CONFUI_OUT_BOOL,
  CONFUI_OUT_DEBUG,
  CONFUI_OUT_CB,
} confuiouttype_t;

enum {
  CONFUI_BEGIN,
  CONFUI_DD_BEGIN,
  CONFUI_DD_ORGPATH,
  CONFUI_DD_MAX,
  CONFUI_ENTRY_BEGIN,
  CONFUI_ENTRY_COMPLETE_MSG,
  CONFUI_ENTRY_DANCE_TAGS,
  CONFUI_ENTRY_DANCE_DANCE,
  CONFUI_ENTRY_MOBMQ_TITLE,
  CONFUI_ENTRY_MOBMQ_TAG,
  CONFUI_ENTRY_MOBMQ_KEY,
  CONFUI_ENTRY_PROFILE_NAME,
  CONFUI_ENTRY_QUEUE_NM,
  CONFUI_ENTRY_RC_PASS,
  CONFUI_ENTRY_RC_USER_ID,
  CONFUI_ENTRY_ACRCLOUD_API_KEY,
  CONFUI_ENTRY_ACRCLOUD_API_SECRET,
  CONFUI_ENTRY_ACRCLOUD_API_HOST,
  CONFUI_ENTRY_BDJ4_SERVER,
  CONFUI_ENTRY_BDJ4_SERVER_PASS,
  CONFUI_ENTRY_BDJ4_SERVER_USER,
  CONFUI_ENTRY_MAX,
  CONFUI_ENTRY_CHOOSE_BEGIN,
  CONFUI_ENTRY_CHOOSE_DANCE_ANNOUNCEMENT,
  CONFUI_ENTRY_CHOOSE_ITUNES_DIR,
  CONFUI_ENTRY_CHOOSE_ITUNES_XML,
  CONFUI_ENTRY_CHOOSE_MUSIC_DIR,
  CONFUI_ENTRY_CHOOSE_SHUTDOWN,
  CONFUI_ENTRY_CHOOSE_STARTUP,
  CONFUI_ENTRY_CHOOSE_MAX,
  CONFUI_SPINBOX_BEGIN,
  CONFUI_SPINBOX_AUDIO_OUTPUT,
  CONFUI_SPINBOX_BPM,
  CONFUI_SPINBOX_CONTROLLER,
  CONFUI_SPINBOX_DANCE_SPEED,
  CONFUI_SPINBOX_DANCE_TIME_SIG,
  CONFUI_SPINBOX_DANCE_TYPE,
  CONFUI_SPINBOX_DISP_SEL,
  CONFUI_SPINBOX_FADE_TYPE,
  CONFUI_SPINBOX_ITUNES_STARS_10,
  CONFUI_SPINBOX_ITUNES_STARS_100,
  CONFUI_SPINBOX_ITUNES_STARS_20,
  CONFUI_SPINBOX_ITUNES_STARS_30,
  CONFUI_SPINBOX_ITUNES_STARS_40,
  CONFUI_SPINBOX_ITUNES_STARS_50,
  CONFUI_SPINBOX_ITUNES_STARS_60,
  CONFUI_SPINBOX_ITUNES_STARS_70,
  CONFUI_SPINBOX_ITUNES_STARS_80,
  CONFUI_SPINBOX_ITUNES_STARS_90,
  CONFUI_SPINBOX_LOCALE,
  CONFUI_SPINBOX_MARQUEE_SHOW,
  CONFUI_SPINBOX_Q_MAX_PLAY_TIME,
  CONFUI_SPINBOX_MQ_THEME,
  CONFUI_SPINBOX_PLI,
  CONFUI_SPINBOX_MOBMQ_TYPE,
  CONFUI_SPINBOX_MUSIC_QUEUE,
  CONFUI_SPINBOX_PLAYER_FADE_TYPE,
  CONFUI_SPINBOX_PLAYER_CLOCK,
  CONFUI_SPINBOX_RC_HTML_TEMPLATE,
  CONFUI_SPINBOX_Q_STOP_AT_TIME,
  CONFUI_SPINBOX_UI_THEME,
  CONFUI_SPINBOX_VOL_INTFC,
  CONFUI_SPINBOX_WRITE_AUDIO_FILE_TAGS,
  CONFUI_SPINBOX_MAX,
  CONFUI_WIDGET_BEGIN,
  CONFUI_SWITCH_AUTO_ORGANIZE,
  CONFUI_SWITCH_BDJ4_SERVER_DISP,
  CONFUI_SWITCH_DB_LOAD_FROM_GENRE,
  CONFUI_SWITCH_ENABLE_ITUNES,
  CONFUI_SWITCH_MOBILE_MQ,
  CONFUI_SWITCH_MQ_SHOW_SONG_INFO,
  CONFUI_SWITCH_USE_WORK_MOVEMENT,
  CONFUI_SWITCH_Q_ACTIVE,
  CONFUI_SWITCH_Q_DISPLAY,
  CONFUI_SWITCH_Q_PAUSE_EACH_SONG,
  CONFUI_SWITCH_Q_PLAY_ANNOUNCE,
  CONFUI_SWITCH_Q_PLAY_WHEN_QUEUED,
  CONFUI_SWITCH_Q_SHOW_QUEUE_DANCE,
  CONFUI_SWITCH_RC_ENABLE,
  CONFUI_SWITCH_SHOW_SPD_CONTROL,
  CONFUI_WIDGET_AO_EXAMPLE_1,
  CONFUI_WIDGET_AO_EXAMPLE_2,
  CONFUI_WIDGET_AO_EXAMPLE_3,
  CONFUI_WIDGET_AO_EXAMPLE_4,
  CONFUI_WIDGET_AO_EXAMPLE_MAX,
  CONFUI_WIDGET_DANCE_MPM_HIGH,
  CONFUI_WIDGET_DANCE_MPM_LOW,
  /* the debug enums must be in the same order as log.h */
  CONFUI_DBG_IMPORTANT,
  CONFUI_DBG_BASIC,
  CONFUI_DBG_MSGS,
  CONFUI_DBG_INFO,
  CONFUI_DBG_ACTIONS,
  CONFUI_DBG_LIST,
  CONFUI_DBG_SONGSEL,
  CONFUI_DBG_DANCESEL,
  CONFUI_DBG_VOLUME,
  CONFUI_DBG_SOCKET,
  CONFUI_DBG_DB,
  CONFUI_DBG_RAFILE,
  CONFUI_DBG_PROC,
  CONFUI_DBG_PLAYER,
  CONFUI_DBG_DATAFILE,
  CONFUI_DBG_PROCESS,
  CONFUI_DBG_WEBSRV,
  CONFUI_DBG_WEBCLIENT,
  CONFUI_DBG_DBUPDATE,
  CONFUI_DBG_PROGSTATE,
  CONFUI_DBG_ITUNES,
  CONFUI_DBG_AUDIO_ADJUST,
  CONFUI_DBG_AUDIO_TAG,
  CONFUI_DBG_AUDIO_ID,
  CONFUI_DBG_AUDIOID_DUMP,
  CONFUI_DBG_VIRTLIST,
  CONFUI_DBG_GROUPING,
  CONFUI_DBG_MAX,
  CONFUI_WIDGET_DEFAULT_VOL,
  /* should be in the same order as in songfilter.h */
  CONFUI_WIDGET_FILTER_START,
  CONFUI_WIDGET_FILTER_GENRE,
  CONFUI_WIDGET_FILTER_DANCE,
  CONFUI_WIDGET_FILTER_DANCELEVEL,
  CONFUI_WIDGET_FILTER_DANCERATING,
  CONFUI_WIDGET_FILTER_STATUS,
  CONFUI_WIDGET_FILTER_FAVORITE,
  CONFUI_WIDGET_FILTER_STATUS_PLAYABLE,
  CONFUI_WIDGET_FILTER_MAX,
  /* should be in the same order as quickedit.h */
  CONFUI_WIDGET_QUICKEDIT_START,
  CONFUI_WIDGET_QUICKEDIT_SPEED,
  CONFUI_WIDGET_QUICKEDIT_VOLUME,
  CONFUI_WIDGET_QUICKEDIT_DANCELEVEL,
  CONFUI_WIDGET_QUICKEDIT_DANCERATING,
  CONFUI_WIDGET_QUICKEDIT_FAVORITE,
  CONFUI_WIDGET_QUICKEDIT_MAX,
  /* there needs to be more itunes fields than what is possible */
  CONFUI_WIDGET_ITUNES_FIELD_1,
  CONFUI_WIDGET_ITUNES_FIELD_2,
  CONFUI_WIDGET_ITUNES_FIELD_3,
  CONFUI_WIDGET_ITUNES_FIELD_4,
  CONFUI_WIDGET_ITUNES_FIELD_5,
  CONFUI_WIDGET_ITUNES_FIELD_6,
  CONFUI_WIDGET_ITUNES_FIELD_7,
  CONFUI_WIDGET_ITUNES_FIELD_8,
  CONFUI_WIDGET_ITUNES_FIELD_9,
  CONFUI_WIDGET_ITUNES_FIELD_10,
  CONFUI_WIDGET_ITUNES_FIELD_11,
  CONFUI_WIDGET_ITUNES_FIELD_12,
  CONFUI_WIDGET_ITUNES_FIELD_13,
  CONFUI_WIDGET_ITUNES_FIELD_14,
  CONFUI_WIDGET_ITUNES_FIELD_15,
  CONFUI_WIDGET_ITUNES_FIELD_16,
  CONFUI_WIDGET_ITUNES_FIELD_17,
  CONFUI_WIDGET_ITUNES_FIELD_18,
  CONFUI_WIDGET_ITUNES_FIELD_19,
  CONFUI_WIDGET_ITUNES_FIELD_20,
  CONFUI_WIDGET_ITUNES_FIELD_21,
  CONFUI_WIDGET_ITUNES_FIELD_22,
  CONFUI_WIDGET_ITUNES_FIELD_23,
  CONFUI_WIDGET_ITUNES_FIELD_24,
  CONFUI_WIDGET_ITUNES_FIELD_25,
  CONFUI_WIDGET_ITUNES_FIELD_26,
  CONFUI_WIDGET_ITUNES_FIELD_27,
  CONFUI_WIDGET_ITUNES_FIELD_28,
  CONFUI_WIDGET_ITUNES_FIELD_29,
  CONFUI_WIDGET_ITUNES_FIELD_30,
  CONFUI_WIDGET_ITUNES_FIELD_31,
  CONFUI_WIDGET_ITUNES_FIELD_32,
  CONFUI_WIDGET_ITUNES_FIELD_33,
  CONFUI_WIDGET_ITUNES_FIELD_34,
  CONFUI_WIDGET_BDJ4_SERVER_PORT,
  CONFUI_WIDGET_MOBMQ_PORT,
  CONFUI_WIDGET_MOBMQ_QR_CODE,
  CONFUI_WIDGET_MQ_BG_COLOR,
  CONFUI_WIDGET_MQ_ACCENT_COLOR,
  CONFUI_WIDGET_MQ_INFO_COLOR,
  CONFUI_WIDGET_MQ_TEXT_COLOR,
  CONFUI_WIDGET_MQ_FONT,
  CONFUI_WIDGET_MQ_FONT_FS,
  CONFUI_WIDGET_MQ_QUEUE_LEN,
  CONFUI_WIDGET_PL_QUEUE_LEN,
  CONFUI_WIDGET_Q_START_WAIT_TIME,    // per queue
  CONFUI_WIDGET_Q_FADE_IN_TIME,       // per queue
  CONFUI_WIDGET_Q_FADE_OUT_TIME,      // per queue
  CONFUI_WIDGET_Q_GAP,                // per queue
  CONFUI_WIDGET_RC_PORT,
  CONFUI_WIDGET_RC_QR_CODE,
  CONFUI_WIDGET_RC_QR_CODE_B,
  CONFUI_WIDGET_UI_ACCENT_COLOR,
  CONFUI_WIDGET_UI_ERROR_COLOR,
  CONFUI_WIDGET_UI_FONT,
  CONFUI_WIDGET_UI_LISTING_FONT,
  CONFUI_WIDGET_UI_MARK_COLOR,
  CONFUI_WIDGET_UI_PROFILE_COLOR,
  CONFUI_WIDGET_UI_SCALE,
  CONFUI_WIDGET_MAX,
  CONFUI_ITEM_MAX,
};

typedef struct configui configui_t;
typedef struct confuigui confuigui_t;
typedef struct confuitable confuitable_t;
typedef void (*savefunc_t) (confuigui_t *);
typedef bool (*listcreatefunc_t) (void *);
typedef void (*addfunc_t) (confuigui_t *);
typedef void (*removefunc_t) (confuigui_t *, ilistidx_t delidx);
typedef void (*movefunc_t) (confuigui_t *, ilistidx_t idx, int dir);

typedef struct {
  confuigui_t       *gui;
  confuibasetype_t  basetype;
  confuiouttype_t   outtype;
  long              debuglvl;
  int               bdjoptIdx;
  uiwcont_t         *uibutton;      // for entry chooser
  uisfcb_t          sfcb;           // for entry chooser, combobox
  int               listidx;        // for combobox, spinbox
  ilist_t           *ddlist;
  nlist_t           *displist;      // indexed by spinbox/combobox index
                                    //    value: display
  nlist_t           *sbkeylist;     // indexed by spinbox index
                                    //    value: key
  int               danceitemidx;   // for dance edit
  int               entrysz;
  int               entrymaxsz;
  uiwcont_t         *uilabelp;
  uiwcont_t         *uiwidgetp;
  callback_t        *callback;
  uidd_t            *uidd;
  char              *uri;
  bool              changed : 1;
} confuiitem_t;

typedef enum {
  CONFUI_ID_AUDIOSRC,
  CONFUI_ID_DANCE,
  CONFUI_ID_DISP_SEL_LIST,
  CONFUI_ID_DISP_SEL_TABLE,
  CONFUI_ID_GENRES,
  CONFUI_ID_LEVELS,
  CONFUI_ID_MOBILE_MQ,
  CONFUI_ID_NONE,
  CONFUI_ID_ORGANIZATION,
  CONFUI_ID_RATINGS,
  CONFUI_ID_REM_CONTROL,
  CONFUI_ID_STATUS,
  CONFUI_ID_TABLE_MAX,
} confuiident_t;

enum {
  CONFUI_MOVE_PREV,
  CONFUI_MOVE_NEXT,
  CONFUI_TABLE_TEXT,
  CONFUI_TABLE_NUM,
  CONFUI_NO_INDENT,
  CONFUI_INDENT,
};

enum {
  CONFUI_TABLE_NONE       = 0x00,
  CONFUI_TABLE_NO_UP_DOWN = 0x01,
  CONFUI_TABLE_KEEP_FIRST = 0x02,
  CONFUI_TABLE_KEEP_LAST  = 0x04,
};

enum {
  CONFUI_VOL_DESC,
  CONFUI_VOL_INTFC,
  CONFUI_VOL_OS,
  CONFUI_VOL_MAX,
};

enum {
  CONFUI_PLI_DESC,
  CONFUI_PLI_INTFC,
  CONFUI_PLI_OS,
  CONFUI_PLI_MAX,
};

enum {
  CONFUI_ATI_DESC,
  CONFUI_ATI_INTFC,
  CONFUI_ATI_MAX,
};

enum {
  CONFUI_AUDIOSRC_DESC,
  CONFUI_AUDIOSRC_INTFC,
  CONFUI_AUDIOSRC_MAX,
};

enum {
  CONFUI_RATING_CB_WEIGHT,
  CONFUI_LEVEL_CB,
  CONFUI_STATUS_CB,
  CONFUI_GENRE_CB,
  CONFUI_TABLE_CB_UP,
  CONFUI_TABLE_CB_DOWN,
  CONFUI_TABLE_CB_REMOVE,
  CONFUI_TABLE_CB_ADD,
  CONFUI_TABLE_CB_MAX,
};

enum {
  CONFUI_BUTTON_TABLE_UP,
  CONFUI_BUTTON_TABLE_DOWN,
  CONFUI_BUTTON_TABLE_DELETE,
  CONFUI_BUTTON_TABLE_ADD,
  CONFUI_BUTTON_TABLE_MAX,
};

typedef struct confuitable {
  uivirtlist_t      *uivl;
  callback_t        *callbacks [CONFUI_TABLE_CB_MAX];
  uiwcont_t         *buttons [CONFUI_BUTTON_TABLE_MAX];
  int               flags;
  bool              changed;
  int               currcount;
  int               saveidx;
  ilist_t           *savelist;
  listcreatefunc_t  listcreatefunc;
  savefunc_t        savefunc;
  addfunc_t         addfunc;
  removefunc_t      removefunc;
  movefunc_t        movefunc;
} confuitable_t;

enum {
  CONFUI_DANCE_COL_DANCE,
  CONFUI_DANCE_COL_DANCE_KEY,
  CONFUI_DANCE_COL_MAX,
};

enum {
  CONFUI_RATING_COL_RATING,
  CONFUI_RATING_COL_WEIGHT,
  CONFUI_RATING_COL_MAX,
};

enum {
  CONFUI_LEVEL_COL_LEVEL,
  CONFUI_LEVEL_COL_WEIGHT,
  CONFUI_LEVEL_COL_DEFAULT,
  CONFUI_LEVEL_COL_MAX,
};

enum {
  CONFUI_GENRE_COL_GENRE,
  CONFUI_GENRE_COL_CLASSICAL,
  CONFUI_GENRE_COL_MAX,
};

enum {
  CONFUI_STATUS_COL_STATUS,
  CONFUI_STATUS_COL_PLAY_FLAG,
  CONFUI_STATUS_COL_MAX,
};

enum {
  CONFUI_TAG_COL_TAG,
  CONFUI_TAG_COL_SB_PAD,
  CONFUI_TAG_COL_TAG_IDX,
  CONFUI_TAG_COL_MAX,
};

enum {
  CONFUI_POSITION_X,
  CONFUI_POSITION_Y,
  CONFUI_SIZE_X,
  CONFUI_SIZE_Y,
  CONFUI_KEY_MAX,
};

enum {
  CONFUI_OPT_NONE = -1,
};

typedef struct conforg conforg_t;
typedef struct confitunes confitunes_t;

typedef struct confuigui {
  confuiitem_t      uiitem [CONFUI_ITEM_MAX];
  char              *localip;
  bool              inbuild : 1;
  /* main window */
  uiwcont_t         *window;
  callback_t        *closecb;
  /* main notebook */
  uinbtabid_t       *nbtabid;
  uiwcont_t         *notebook;
  callback_t        *nbcb;
  /* widgets */
  uiwcont_t         *vbox;
  uiwcont_t         *statusMsg;
  /* display select */
  dispsel_t         *dispsel;
  uiduallist_t      *dispselduallist;
  slist_t           *edittaglist;
  slist_t           *audioidtaglist;
  slist_t           *listingtaglist;
  slist_t           *marqueetaglist;
  slist_t           *pluitaglist;
  /* filter */
  nlist_t           *filterDisplaySel;
  nlist_t           *filterLookup;
  nlist_t           *quickeditDisplaySel;
  nlist_t           *quickeditLookup;
  /* tables */
  confuiident_t     tablecurr;
  confuitable_t     tables [CONFUI_ID_TABLE_MAX];
  /* organization */
  conforg_t         *org;
  /* itunes */
  confitunes_t      *itunes;
  /* dances */
  int32_t           dancedkey;      // for dance edit
  bool              inchange : 1;
} confuigui_t;

/* confcommon.c */
void confuiLoadThemeList (confuigui_t *gui);
void confuiUpdateMobmqQrcode (confuigui_t *gui);
void confuiUpdateRemctrlQrcode (confuigui_t *gui);
void confuiSetStatusMsg (confuigui_t *gui, const char *msg);
void confuiSelectDirDialog (uisfcb_t *sfcb, const char *startpath, const char *title);
void confuiSelectFileDialog (uisfcb_t *sfcb, const char *startpath, const char *mimefiltername, const char *mimetype);
void confuiCreateTagListingDisp (confuigui_t *gui);
void confuiCreateTagSelectedDisp (confuigui_t *gui);
void confuiUpdateOrgExamples (confuigui_t *gui, const char *orgpath);
int32_t confuiOrgPathSelect (void *udata, const char *sval);
void confuiLoadIntfcList (confuigui_t *gui, slist_t *interfaces, int optidx, int opnmidx, int spinboxidx, int offset);

/* confas.c */
void confuiInitAudioSource (confuigui_t *gui);
void confuiBuildUIAudioSource (confuigui_t *gui);

/* confdance.c */
void confuiInitEditDances (confuigui_t *gui);
void confuiBuildUIEditDances (confuigui_t *gui);
void confuiDanceSelectLoadValues (confuigui_t *gui, ilistidx_t key);
void confuiDanceSearchSelect (confuigui_t *gui, ilistidx_t dkey);

/* confdebug.c */
void confuiBuildUIDebug (confuigui_t *gui);

/* confdispset.c */
void confuiInitDispSettings (confuigui_t *gui);
void confuiBuildUIDispSettings (confuigui_t *gui);

/* conffilter.c */
void confuiBuildUIDialogDisplay (confuigui_t *gui);

/* confgeneral.c */
void confuiInitGeneral (confuigui_t *gui);
void confuiBuildUIGeneral (confuigui_t *gui);

/* confgenre.c */
void confuiBuildUIEditGenres (confuigui_t *gui);
void confuiCreateGenreTable (confuigui_t *gui);

/* confgui.c */
void confuiEntrySetSize (confuigui_t *gui, int widx, int sz, int maxsz);
void confuiMakeNotebookTab (uiwcont_t *boxp, confuigui_t *gui, const char *txt, int);
void confuiMakeItemEntry (confuigui_t *gui, uiwcont_t *boxp, uiwcont_t *sg, const char *txt, int widx, int bdjoptIdx, const char *disp, int indent);
void confuiMakeItemEntryEncrypt (confuigui_t *gui, uiwcont_t *boxp, uiwcont_t *sg, const char *txt, int widx, int bdjoptIdx, const char *disp, int indent);
void confuiMakeItemEntryChooser (confuigui_t *gui, uiwcont_t *boxp, uiwcont_t *sg, const char *txt, int widx, int bdjoptIdx, const char *disp, void *dialogFunc);
void confuiMakeItemDropdown (confuigui_t *gui, uiwcont_t *boxp, uiwcont_t *sg, const char *txt, int widx, int bdjoptIdx, callbackFuncS ddcb, const char *value);
void confuiMakeItemLink (confuigui_t *gui, uiwcont_t *boxp, uiwcont_t *sg, const char *txt, int widx, const char *disp);
void confuiMakeItemFontButton (confuigui_t *gui, uiwcont_t *boxp, uiwcont_t *sg, const char *txt, int widx, int bdjoptIdx, const char *fontname);
void confuiMakeItemColorButton (confuigui_t *gui, uiwcont_t *boxp, uiwcont_t *sg, const char *txt, int widx, int bdjoptIdx, const char *color);
void confuiMakeItemSpinboxText (confuigui_t *gui, uiwcont_t *boxp, uiwcont_t *sg, uiwcont_t *sgB, const char *txt, int widx, int bdjoptIdx, confuiouttype_t outtype, ssize_t value, void *cb);
void confuiMakeItemSpinboxTime (confuigui_t *gui, uiwcont_t *boxp, uiwcont_t *sg, uiwcont_t *sgB, const char *txt, int widx, int bdjoptIdx, ssize_t value, int indent);
void confuiMakeItemSpinboxNum (confuigui_t *gui, uiwcont_t *boxp, uiwcont_t *sg, uiwcont_t *sgB, const char *txt, int widx, int bdjoptIdx, int min, int max, int value, void *cb);
void confuiMakeItemSpinboxDouble (confuigui_t *gui, uiwcont_t *boxp, uiwcont_t *sg, uiwcont_t *sgB, const char *txt, int widx, int bdjoptIdx, double min, double max, double value, int indent);
void confuiMakeItemSwitch (confuigui_t *gui, uiwcont_t *boxp, uiwcont_t *sg, const char *txt, int widx, int bdjoptIdx, int value, void *cb, int indent);
void confuiMakeItemLabelDisp (confuigui_t *gui, uiwcont_t *boxp, uiwcont_t *sg, const char *txt, int widx, int bdjoptIdx);
void confuiMakeItemCheckButton (confuigui_t *gui, uiwcont_t *boxp, uiwcont_t *sg, const char *txt, int widx, int bdjoptIdx, int value);
void confuiMakeItemLabel (confuigui_t *gui, int widx, uiwcont_t *boxp, uiwcont_t *sg, const char *txt, int indent);
void confuiSpinboxTextInitDataNum (confuigui_t *gui, char *tag, int widx, ...);

/* confitunes.c */
void confuiInitiTunes (confuigui_t *gui);
void confuiCleaniTunes (confuigui_t *gui);
void confuiBuildUIiTunes (confuigui_t *gui);
void confuiSaveiTunes (confuigui_t *gui);

/* conflevel.c */
void confuiBuildUIEditLevels (confuigui_t *gui);
void confuiCreateLevelTable (confuigui_t *gui);

/* confmarquee.c */
void confuiInitMarquee (confuigui_t *gui);
void confuiBuildUIMarquee (confuigui_t *gui);

/* confmobmq.c */
void confuiInitMobileMarquee (confuigui_t *gui);
void confuiBuildUIMobileMarquee (confuigui_t *gui);

/* conforg.c */
void confuiInitOrganization (confuigui_t *gui);
void confuiCleanOrganization (confuigui_t *gui);
void confuiBuildUIOrganization (confuigui_t *gui);
void confuiDispSaveTable (confuigui_t *gui, int selidx);

/* confplayer.c */
void confuiInitPlayer (confuigui_t *gui);
void confuiBuildUIPlayer (confuigui_t *gui);

/* confmusicq.c */
void confuiInitMusicQs (confuigui_t *gui);
void confuiBuildUIMusicQs (confuigui_t *gui);

/* confpopulate.c */
void confuiPopulateOptions (confuigui_t *gui);

/* confrc.c */
void confuiInitMobileRemoteControl (confuigui_t *gui);
void confuiBuildUIMobileRemoteControl (confuigui_t *gui);

/* confrating.c */
void confuiBuildUIEditRatings (confuigui_t *gui);
void confuiCreateRatingTable (confuigui_t *gui);

/* confstatus.c */
void confuiBuildUIEditStatus (confuigui_t *gui);
void confuiCreateStatusTable (confuigui_t *gui);

/* conftable.c */
void confuiMakeItemTable (confuigui_t *gui, uiwcont_t *box, confuiident_t id, int flags);
void confuiTableFree (confuigui_t *gui, confuiident_t id);
void confuiTableSave (confuigui_t *gui, confuiident_t id);
bool confuiSwitchTable (void *udata, int32_t pagenum);
bool confuiTableAdd (void *udata);

/* confui.c */
void confuiBuildUIUserInterface (confuigui_t *gui);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_CONFIGUI_H */
