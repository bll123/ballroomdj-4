#ifndef INC_CONFIGUI_H
#define INC_CONFIGUI_H

#include "dispsel.h"
#include "ilist.h"
#include "nlist.h"
#include "orgutil.h"
#include "ui.h"
#include "uiduallist.h"
#include "uinbutil.h"

/* base type */
typedef enum {
  CONFUI_NONE,
  CONFUI_ENTRY,
  CONFUI_FONT,
  CONFUI_COLOR,
  CONFUI_COMBOBOX,
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
  CONFUI_COMBOBOX_AO_PATHFMT,
  CONFUI_COMBOBOX_MAX,
  CONFUI_ENTRY_DANCE_TAGS,
  CONFUI_ENTRY_DANCE_ANNOUNCEMENT,
  CONFUI_ENTRY_DANCE_DANCE,
  CONFUI_ENTRY_MM_NAME,
  CONFUI_ENTRY_MM_TITLE,
  CONFUI_ENTRY_MUSIC_DIR,
  CONFUI_ENTRY_PROFILE_NAME,
  CONFUI_ENTRY_COMPLETE_MSG,
  /* the queue name identifiers must be in sequence */
  /* the number of queue names must match MUSICQ_PB_MAX */
  CONFUI_ENTRY_QUEUE_NM_A,
  CONFUI_ENTRY_QUEUE_NM_B,
  CONFUI_ENTRY_RC_PASS,
  CONFUI_ENTRY_RC_USER_ID,
  CONFUI_ENTRY_SHUTDOWN,
  CONFUI_ENTRY_STARTUP,
  CONFUI_ENTRY_MAX,
  CONFUI_SPINBOX_VOL_INTFC,
  CONFUI_SPINBOX_AUDIO_OUTPUT,
  CONFUI_SPINBOX_BPM,
  CONFUI_SPINBOX_DANCE_TYPE,
  CONFUI_SPINBOX_DANCE_SPEED,
  CONFUI_SPINBOX_DANCE_LOW_BPM,
  CONFUI_SPINBOX_DANCE_HIGH_BPM,
  CONFUI_SPINBOX_DANCE_TIME_SIG,
  CONFUI_SPINBOX_DISP_SEL,
  CONFUI_SPINBOX_FADE_TYPE,
  CONFUI_SPINBOX_LOCALE,
  CONFUI_SPINBOX_MAX_PLAY_TIME,
  CONFUI_SPINBOX_MOBILE_MQ,
  CONFUI_SPINBOX_MQ_THEME,
  CONFUI_SPINBOX_PLAYER,
  CONFUI_SPINBOX_RC_HTML_TEMPLATE,
  CONFUI_SPINBOX_UI_THEME,
  CONFUI_SPINBOX_WRITE_AUDIO_FILE_TAGS,
  CONFUI_SPINBOX_MAX,
  CONFUI_SWITCH_BDJ3_COMPAT_TAGS,
  CONFUI_SWITCH_DB_LOAD_FROM_GENRE,
  CONFUI_SWITCH_ENABLE_ITUNES,
  CONFUI_SWITCH_AUTO_ORGANIZE,
  CONFUI_SWITCH_MQ_SHOW_SONG_INFO,
  CONFUI_SWITCH_MQ_HIDE_ON_START,
  CONFUI_SWITCH_RC_ENABLE,
  CONFUI_SWITCH_FILTER_STATUS_PLAYABLE,
  CONFUI_SWITCH_MAX,
  CONFUI_WIDGET_AO_EXAMPLE_1,
  CONFUI_WIDGET_AO_EXAMPLE_2,
  CONFUI_WIDGET_AO_EXAMPLE_3,
  CONFUI_WIDGET_AO_EXAMPLE_4,
  CONFUI_WIDGET_FILTER_GENRE,
  CONFUI_WIDGET_FILTER_DANCELEVEL,
  CONFUI_WIDGET_FILTER_STATUS,
  CONFUI_WIDGET_FILTER_FAVORITE,
  /* the debug enums must be in numeric order */
  CONFUI_WIDGET_DEBUG_1,
  CONFUI_WIDGET_DEBUG_2,
  CONFUI_WIDGET_DEBUG_4,
  CONFUI_WIDGET_DEBUG_8,
  CONFUI_WIDGET_DEBUG_16,
  CONFUI_WIDGET_DEBUG_32,
  CONFUI_WIDGET_DEBUG_64,
  CONFUI_WIDGET_DEBUG_128,
  CONFUI_WIDGET_DEBUG_256,
  CONFUI_WIDGET_DEBUG_512,
  CONFUI_WIDGET_DEBUG_1024,
  CONFUI_WIDGET_DEBUG_2048,
  CONFUI_WIDGET_DEBUG_4096,
  CONFUI_WIDGET_DEBUG_8192,
  CONFUI_WIDGET_DEBUG_16384,
  CONFUI_WIDGET_DEBUG_32768,
  CONFUI_WIDGET_DEBUG_65536,
  CONFUI_WIDGET_DEBUG_131072,
  CONFUI_WIDGET_DEBUG_262144,
  CONFUI_WIDGET_DEBUG_524288,
  CONFUI_WIDGET_DEFAULT_VOL,
  CONFUI_WIDGET_FADE_IN_TIME,
  CONFUI_WIDGET_FADE_OUT_TIME,
  CONFUI_WIDGET_GAP,
  CONFUI_WIDGET_MMQ_PORT,
  CONFUI_WIDGET_MMQ_QR_CODE,
  CONFUI_WIDGET_MQ_ACCENT_COLOR,
  CONFUI_WIDGET_MQ_FONT,
  CONFUI_WIDGET_MQ_FONT_FS,
  CONFUI_WIDGET_MQ_QUEUE_LEN,
  CONFUI_WIDGET_PL_QUEUE_LEN,
  CONFUI_WIDGET_RC_PORT,
  CONFUI_WIDGET_RC_QR_CODE,
  CONFUI_WIDGET_UI_ACCENT_COLOR,
  CONFUI_WIDGET_UI_PROFILE_COLOR,
  CONFUI_WIDGET_UI_FONT,
  CONFUI_WIDGET_UI_LISTING_FONT,
  CONFUI_ITEM_MAX,
};

typedef struct {
  confuibasetype_t  basetype;
  confuiouttype_t   outtype;
  int               bdjoptIdx;
  union {
    uidropdown_t  *dropdown;
    uientry_t     *entry;
    uispinbox_t   *spinbox;
    uiswitch_t    *uiswitch;
  };
  int         listidx;        // for combobox, spinbox
  nlist_t     *displist;      // indexed by spinbox/combobox index
                              //    value: display
  nlist_t     *sbkeylist;     // indexed by spinbox index
                              //    value: key
  int         danceidx;       // for dance edit
  GtkWidget   *widget;
  UIWidget    uiwidget;
  UICallback  callback;
  char        *uri;
} confuiitem_t;

typedef enum {
  CONFUI_ID_DANCE,
  CONFUI_ID_GENRES,
  CONFUI_ID_RATINGS,
  CONFUI_ID_LEVELS,
  CONFUI_ID_STATUS,
  CONFUI_ID_DISP_SEL_LIST,
  CONFUI_ID_DISP_SEL_TABLE,
  CONFUI_ID_TABLE_MAX,
  CONFUI_ID_FILTER,
  CONFUI_ID_NONE,
  CONFUI_ID_MOBILE_MQ,
  CONFUI_ID_REM_CONTROL,
  CONFUI_ID_ORGANIZATION,
} confuiident_t;

enum {
  CONFUI_MOVE_PREV,
  CONFUI_MOVE_NEXT,
};

enum {
  CONFUI_TABLE_TEXT,
  CONFUI_TABLE_NUM,
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
  CONFUI_PLAYER_DESC,
  CONFUI_PLAYER_INTFC,
  CONFUI_PLAYER_OS,
  CONFUI_PLAYER_MAX,
};

enum {
  CONFUI_TABLE_CB_UP,
  CONFUI_TABLE_CB_DOWN,
  CONFUI_TABLE_CB_REMOVE,
  CONFUI_TABLE_CB_ADD,
  CONFUI_TABLE_CB_MAX,
};

typedef struct configui configui_t;
typedef struct confuigui confuigui_t;
typedef struct confuitable confuitable_t;
typedef void (*savefunc_t) (confuigui_t *);

typedef struct confuitable {
  GtkWidget *tree;
  GtkTreeSelection  *sel;
  UICallback  callback [CONFUI_TABLE_CB_MAX];
  int       radiorow;
  int       togglecol;
  int       flags;
  bool      changed;
  int       currcount;
  int       saveidx;
  ilist_t   *savelist;
  GtkTreeModelForeachFunc listcreatefunc;
  savefunc_t  savefunc;
} confuitable_t;

enum {
  CONFUI_DANCE_COL_DANCE,
  CONFUI_DANCE_COL_SB_PAD,
  CONFUI_DANCE_COL_DANCE_IDX,
  CONFUI_DANCE_COL_MAX,
};

enum {
  CONFUI_RATING_COL_R_EDITABLE,
  CONFUI_RATING_COL_W_EDITABLE,
  CONFUI_RATING_COL_RATING,
  CONFUI_RATING_COL_WEIGHT,
  CONFUI_RATING_COL_ADJUST,
  CONFUI_RATING_COL_DIGITS,
  CONFUI_RATING_COL_MAX,
};

enum {
  CONFUI_LEVEL_COL_EDITABLE,
  CONFUI_LEVEL_COL_LEVEL,
  CONFUI_LEVEL_COL_WEIGHT,
  CONFUI_LEVEL_COL_DEFAULT,
  CONFUI_LEVEL_COL_ADJUST,
  CONFUI_LEVEL_COL_DIGITS,
  CONFUI_LEVEL_COL_MAX,
};

enum {
  CONFUI_GENRE_COL_EDITABLE,
  CONFUI_GENRE_COL_GENRE,
  CONFUI_GENRE_COL_CLASSICAL,
  CONFUI_GENRE_COL_SB_PAD,
  CONFUI_GENRE_COL_MAX,
};

enum {
  CONFUI_STATUS_COL_EDITABLE,
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

typedef struct confuigui {
  confuiitem_t      uiitem [CONFUI_ITEM_MAX];
  char              *localip;
  /* main window */
  UIWidget          window;
  UICallback        closecb;
  /* main notebook */
  uiutilsnbtabid_t  *nbtabid;
  UIWidget          notebook;
  UICallback        nbcb;
  /* widgets */
  UIWidget          vbox;
  UIWidget          statusMsg;
  /* display select */
  dispsel_t         *dispsel;
  uiduallist_t      *dispselduallist;
  slist_t           *edittaglist;
  slist_t           *listingtaglist;
  /* tables */
  confuiident_t     tablecurr;
  confuitable_t     tables [CONFUI_ID_TABLE_MAX];
  /* dances */
  bool              indancechange : 1;
} confuigui_t;

/* confcommon.c */
nlist_t  * confuiGetThemeList (void);
slist_t  * confuiGetThemeNames (slist_t *themelist, slist_t *filelist);
void confuiLoadHTMLList (confuigui_t *gui);
void confuiLoadVolIntfcList (confuigui_t *gui);
void confuiLoadPlayerIntfcList (confuigui_t *gui);
void confuiLoadLocaleList (confuigui_t *gui);
void confuiLoadDanceTypeList (confuigui_t *gui);
void confuiLoadThemeList (confuigui_t *gui);
void confuiUpdateMobmqQrcode (confuigui_t *gui);
void confuiUpdateRemctrlQrcode (confuigui_t *gui);
void confuiUpdateOrgExamples (confuigui_t *gui, char *pathfmt);
void confuiSetStatusMsg (confuigui_t *gui, const char *msg);
void confuiSpinboxTextInitDataNum (confuigui_t *gui, char *tag, int widx, ...);
void confuiSelectFileDialog (confuigui_t *gui, int widx, char *startpath, char *mimefiltername, char *mimetype);
void confuiCreateTagListingDisp (confuigui_t *gui);
void confuiCreateTagSelectedDisp (confuigui_t *gui);

/* confdance.c */
void confuiBuildUIEditDances (confuigui_t *gui);
void confuiDanceSelect (GtkTreeView *tv, GtkTreePath *path, GtkTreeViewColumn *column, gpointer udata);
void confuiDanceSet (GtkListStore *store, GtkTreeIter *iter, char *dancedisp, ilistidx_t key);

/* confgeneral.c */
void confuiBuildUIGeneral (confuigui_t *gui);

/* confgenre.c */
void confuiBuildUIEditGenres (confuigui_t *gui);
void confuiCreateGenreTable (confuigui_t *gui);
void confuiGenreSet (GtkListStore *store, GtkTreeIter *iter, int editable, char *genredisp, int clflag);

/* confgui.c */
void confuiMakeNotebookTab (UIWidget *boxp, confuigui_t *gui, char *txt, int);
void confuiMakeItemEntry (confuigui_t *gui, UIWidget *boxp, UIWidget *sg, char *txt, int widx, int bdjoptIdx, char *disp);
void confuiMakeItemEntryChooser (confuigui_t *gui, UIWidget *boxp, UIWidget *sg, char *txt, int widx, int bdjoptIdx, char *disp, void *dialogFunc);
void confuiMakeItemEntryBasic (confuigui_t *gui, UIWidget *boxp, UIWidget *sg, char *txt, int widx, int bdjoptIdx, char *disp);
void confuiMakeItemCombobox (confuigui_t *gui, UIWidget *boxp, UIWidget *sg, char *txt, int widx, int bdjoptIdx, UILongCallbackFunc ddcb, char *value);
void confuiMakeItemLink (confuigui_t *gui, UIWidget *boxp, UIWidget *sg, char *txt, int widx, char *disp);
void confuiMakeItemFontButton (confuigui_t *gui, UIWidget *boxp, UIWidget *sg, char *txt, int widx, int bdjoptIdx, char *fontname);
void confuiMakeItemColorButton (confuigui_t *gui, UIWidget *boxp, UIWidget *sg, char *txt, int widx, int bdjoptIdx, char *color);
void confuiMakeItemSpinboxText (confuigui_t *gui, UIWidget *boxp, UIWidget *sg, UIWidget *sgB, char *txt, int widx, int bdjoptIdx, confuiouttype_t outtype, ssize_t value, void *cb);
void confuiMakeItemSpinboxTime (confuigui_t *gui, UIWidget *boxp, UIWidget *sg, UIWidget *sgB, char *txt, int widx, int bdjoptIdx, ssize_t value);
void confuiMakeItemSpinboxNum (confuigui_t *gui, UIWidget *boxp, UIWidget *sg, UIWidget *sgB, const char *txt, int widx, int bdjoptIdx, int min, int max, int value, void *cb);
void confuiMakeItemSpinboxDouble (confuigui_t *gui, UIWidget *boxp, UIWidget *sg, UIWidget *sgB, char *txt, int widx, int bdjoptIdx, double min, double max, double value);
void confuiMakeItemSwitch (confuigui_t *gui, UIWidget *boxp, UIWidget *sg, char *txt, int widx, int bdjoptIdx, int value, void *cb);
void confuiMakeItemLabelDisp (confuigui_t *gui, UIWidget *boxp, UIWidget *sg, char *txt, int widx, int bdjoptIdx);
void confuiMakeItemCheckButton (confuigui_t *gui, UIWidget *boxp, UIWidget *sg, const char *txt, int widx, int bdjoptIdx, int value);
void confuiMakeItemLabel (UIWidget *boxp, UIWidget *sg, const char *txt);

/* conflevel.c */
void confuiBuildUIEditLevels (confuigui_t *gui);
void confuiCreateLevelTable (confuigui_t *gui);
void confuiLevelSet (GtkListStore *store, GtkTreeIter *iter, int editable, char *leveldisp, long weight, int def);

/* confrating.c */
void confuiBuildUIEditRatings (confuigui_t *gui);
void confuiCreateRatingTable (confuigui_t *gui);
void confuiRatingSet (GtkListStore *store, GtkTreeIter *iter, int editable, char *ratingdisp, long weight);

/* confstatus.c */
void confuiBuildUIEditStatus (confuigui_t *gui);
void confuiCreateStatusTable (confuigui_t *gui);
void confuiStatusSet (GtkListStore *store, GtkTreeIter *iter, int editable, char *statusdisp, int playflag);

/* confguitable.c */
void confuiMakeItemTable (confuigui_t *gui, UIWidget *box, confuiident_t id, int flags);
void confuiTableSave (confuigui_t *gui, confuiident_t id);
void confuiTableEditText (GtkCellRendererText* r, const gchar* path, const gchar* ntext, gpointer udata);
void confuiTableToggle (GtkCellRendererToggle *renderer, gchar *path, gpointer udata);
void confuiTableEditSpinbox (GtkCellRendererText* r, const gchar* path, const gchar* ntext, gpointer udata);
void confuiTableRadioToggle (GtkCellRendererToggle *renderer, gchar *path, gpointer udata);
bool confuiSwitchTable (void *udata, long pagenum);

#endif /* INC_CONFIGUI_H */
