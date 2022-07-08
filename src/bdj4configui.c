#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <getopt.h>
#include <unistd.h>
#include <math.h>
#include <stdarg.h>

#include <gtk/gtk.h>

#include "bdj4.h"
#include "bdj4init.h"
#include "bdj4intl.h"
#include "bdjmsg.h"
#include "bdjopt.h"
#include "bdjstring.h"
#include "bdjvars.h"
#include "bdjvarsdf.h"
#include "bdjvarsdfload.h"
#include "conn.h"
#include "dance.h"
#include "datafile.h"
#include "dirop.h"
#include "dispsel.h"
#include "dnctypes.h"
#include "filedata.h"
#include "fileop.h"
#include "filemanip.h"
#include "genre.h"
#include "ilist.h"
#include "level.h"
#include "localeutil.h"
#include "lock.h"
#include "log.h"
#include "musicq.h"
#include "nlist.h"
#include "orgopt.h"
#include "orgutil.h"
#include "osuiutils.h"
#include "osutils.h"
#include "pathbld.h"
#include "pathutil.h"
#include "progstate.h"
#include "rating.h"
#include "slist.h"
#include "sock.h"
#include "sockh.h"
#include "songfilter.h"
#include "status.h"
#include "sysvars.h"
#include "tagdef.h"
#include "templateutil.h"
#include "tmutil.h"
#include "ui.h"
#include "uinbutil.h"
#include "uiduallist.h"
#include "validate.h"
#include "volume.h"
#include "webclient.h"

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

typedef struct {
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
  void      *savefunc;
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

typedef struct {
  progstate_t       *progstate;
  char              *locknm;
  conn_t            *conn;
  char              *localip;
  int               dbgflags;
  confuiitem_t      uiitem [CONFUI_ITEM_MAX];
  confuiident_t     tablecurr;
  uiutilsnbtabid_t  *nbtabid;
  slist_t           *listingtaglist;
  slist_t           *edittaglist;
  dispsel_t         *dispsel;
  int               stopwaitcount;
  datafile_t        *filterDisplayDf;
  nlist_t           *filterDisplaySel;
  nlist_t           *filterLookup;
  /* gtk stuff */
  UIWidget          window;
  UICallback        closecb;
  UIWidget          vbox;
  UIWidget          notebook;
  UICallback        nbcb;
  UIWidget          statusMsg;
  confuitable_t     tables [CONFUI_ID_TABLE_MAX];
  uiduallist_t      *dispselduallist;
  /* options */
  datafile_t        *optiondf;
  nlist_t           *options;
  /* flags */
  bool              indancechange : 1;
} configui_t;

typedef void (*savefunc_t) (configui_t *);

enum {
  CONFUI_POSITION_X,
  CONFUI_POSITION_Y,
  CONFUI_SIZE_X,
  CONFUI_SIZE_Y,
  CONFUI_KEY_MAX,
};

static datafilekey_t configuidfkeys [CONFUI_KEY_MAX] = {
  { "CONFUI_POS_X",     CONFUI_POSITION_X,    VALUE_NUM, NULL, -1 },
  { "CONFUI_POS_Y",     CONFUI_POSITION_Y,    VALUE_NUM, NULL, -1 },
  { "CONFUI_SIZE_X",    CONFUI_SIZE_X,        VALUE_NUM, NULL, -1 },
  { "CONFUI_SIZE_Y",    CONFUI_SIZE_Y,        VALUE_NUM, NULL, -1 },
};

static bool     confuiHandshakeCallback (void *udata, programstate_t programState);
static bool     confuiStoppingCallback (void *udata, programstate_t programState);
static bool     confuiStopWaitCallback (void *udata, programstate_t programState);
static bool     confuiClosingCallback (void *udata, programstate_t programState);
static void     confuiBuildUI (configui_t *confui);
static void     confuiBuildUIGeneral (configui_t *confui);
static void     confuiBuildUIPlayer (configui_t *confui);
static void     confuiBuildUIMarquee (configui_t *confui);
static void     confuiBuildUIUserInterface (configui_t *confui);
static void     confuiBuildUIDispSettings (configui_t *confui);
static void     confuiBuildUIFilterDisplay (configui_t *confui);
static void     confuiBuildUIOrganization (configui_t *confui);
static void     confuiBuildUIEditDances (configui_t *confui);
static void     confuiBuildUIEditRatings (configui_t *confui);
static void     confuiBuildUIEditStatus (configui_t *confui);
static void     confuiBuildUIEditLevels (configui_t *confui);
static void     confuiBuildUIEditGenres (configui_t *confui);
static void     confuiBuildUIMobileRemoteControl (configui_t *confui);
static void     confuiBuildUIMobileMarquee (configui_t *confui);
static void     confuiBuildUIDebugOptions (configui_t *confui);
static int      confuiMainLoop  (void *tconfui);
static int      confuiProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
                    bdjmsgmsg_t msg, char *args, void *udata);
static bool     confuiCloseWin (void *udata);
static void     confuiSigHandler (int sig);

static void confuiPopulateOptions (configui_t *confui);
static bool confuiSelectMusicDir (void *udata);
static bool confuiSelectStartup (void *udata);
static bool confuiSelectShutdown (void *udata);
static bool confuiSelectAnnouncement (void *udata);
static void confuiSelectFileDialog (configui_t *confui, int widx, char *startpath, char *mimefiltername, char *mimetype);

/* gui */
static void confuiMakeNotebookTab (UIWidget *boxp, configui_t *confui, UIWidget *notebook, char *txt, int);
static void confuiMakeItemEntry (configui_t *confui, UIWidget *boxp, UIWidget *sg, char *txt, int widx, int bdjoptIdx, char *disp);
static void confuiMakeItemEntryChooser (configui_t *confui, UIWidget *boxp, UIWidget *sg, char *txt, int widx, int bdjoptIdx, char *disp, void *dialogFunc);
static void confuiMakeItemEntryBasic (configui_t *confui, UIWidget *boxp, UIWidget *sg, char *txt, int widx, int bdjoptIdx, char *disp);
static void confuiMakeItemCombobox (configui_t *confui, UIWidget *boxp, UIWidget *sg, char *txt, int widx, int bdjoptIdx, UILongCallbackFunc ddcb, char *value);
static void confuiMakeItemLink (configui_t *confui, UIWidget *boxp, UIWidget *sg, char *txt, int widx, char *disp);
static void confuiMakeItemFontButton (configui_t *confui, UIWidget *boxp, UIWidget *sg, char *txt, int widx, int bdjoptIdx, char *fontname);
static void confuiMakeItemColorButton (configui_t *confui, UIWidget *boxp, UIWidget *sg, char *txt, int widx, int bdjoptIdx, char *color);
static void confuiMakeItemSpinboxText (configui_t *confui, UIWidget *boxp, UIWidget *sg, UIWidget *sgB, char *txt, int widx, int bdjoptIdx, confuiouttype_t outtype, ssize_t value, void *cb);
static void confuiMakeItemSpinboxTime (configui_t *confui, UIWidget *boxp, UIWidget *sg, UIWidget *sgB, char *txt, int widx, int bdjoptIdx, ssize_t value);
static void confuiMakeItemSpinboxNum (configui_t *confui, UIWidget *boxp, UIWidget *sg, UIWidget *sgB, const char *txt, int widx, int bdjoptIdx, int min, int max, int value, void *cb);
static void confuiMakeItemSpinboxDouble (configui_t *confui, UIWidget *boxp, UIWidget *sg, UIWidget *sgB, char *txt, int widx, int bdjoptIdx, double min, double max, double value);
static void confuiMakeItemSwitch (configui_t *confui, UIWidget *boxp, UIWidget *sg, char *txt, int widx, int bdjoptIdx, int value, void *cb);
static void confuiMakeItemLabelDisp (configui_t *confui, UIWidget *boxp, UIWidget *sg, char *txt, int widx, int bdjoptIdx);
static void confuiMakeItemCheckButton (configui_t *confui, UIWidget *boxp, UIWidget *sg, const char *txt, int widx, int bdjoptIdx, int value);
static void confuiMakeItemLabel (UIWidget *boxp, UIWidget *sg, const char *txt);
static void confuiMakeItemTable (configui_t *confui, UIWidget *box, confuiident_t id, int flags);

/* misc */
static nlist_t  * confuiGetThemeList (void);
static nlist_t  * confuiGetThemeNames (nlist_t *themelist, slist_t *filelist);
static void     confuiLoadHTMLList (configui_t *confui);
static void     confuiLoadVolIntfcList (configui_t *confui);
static void     confuiLoadPlayerIntfcList (configui_t *confui);
static void     confuiLoadLocaleList (configui_t *confui);
static void     confuiLoadDanceTypeList (configui_t *confui);
static void     confuiLoadTagList (configui_t *confui);
static void     confuiLoadThemeList (configui_t *confui);
static bool     confuiOrgPathSelect (void *udata, long idx);
static void     confuiUpdateMobmqQrcode (configui_t *confui);
static bool     confuiMobmqTypeChg (void *udata);
static bool     confuiMobmqPortChg (void *udata);
static int      confuiMobmqNameChg (uientry_t *entry, void *udata);
static int      confuiMobmqTitleChg (uientry_t *entry, void *udata);
static void     confuiUpdateRemctrlQrcode (configui_t *confui);
static bool     confuiRemctrlChg (void *udata, int value);
static bool     confuiRemctrlPortChg (void *udata);
static char     * confuiMakeQRCodeFile (configui_t *confui, char *title, char *uri);
static void     confuiUpdateOrgExamples (configui_t *confui, char *pathfmt);
static void     confuiUpdateOrgExample (configui_t *config, org_t *org, char *data, UIWidget *uiwidgetp);
static char     * confuiGetLocalIP (configui_t *confui);
static void     confuiSetStatusMsg (configui_t *confui, const char *msg);
static void     confuiSpinboxTextInitDataNum (configui_t *confui, char *tag, int widx, ...);
static bool     confuiLinkCallback (void *udata);

/* table editing */
static bool   confuiTableMoveUp (void *udata);
static bool   confuiTableMoveDown (void *udata);
static void   confuiTableMove (configui_t *confui, int dir);
static bool   confuiTableRemove (void *udata);
static bool   confuiTableAdd (void *udata);
static bool   confuiSwitchTable (void *udata, long pagenum);
static void   confuiTableSetDefaultSelection (configui_t *confui, GtkWidget *tree, GtkTreeSelection *sel);
static void   confuiCreateDanceTable (configui_t *confui);
static void   confuiDanceSet (GtkListStore *store, GtkTreeIter *iter, char *dancedisp, ilistidx_t key);
static void   confuiCreateRatingTable (configui_t *confui);
static void   confuiRatingSet (GtkListStore *store, GtkTreeIter *iter, int editable, char *ratingdisp, long weight);
static void   confuiCreateStatusTable (configui_t *confui);
static void   confuiStatusSet (GtkListStore *store, GtkTreeIter *iter, int editable, char *statusdisp, int playflag);
static void   confuiCreateLevelTable (configui_t *confui);
static void   confuiLevelSet (GtkListStore *store, GtkTreeIter *iter, int editable, char *leveldisp, long weight, int def);
static void   confuiCreateGenreTable (configui_t *confui);
static void   confuiGenreSet (GtkListStore *store, GtkTreeIter *iter, int editable, char *genredisp, int clflag);
static void   confuiTableToggle (GtkCellRendererToggle *renderer, gchar *path, gpointer udata);
static void   confuiTableRadioToggle (GtkCellRendererToggle *renderer, gchar *path, gpointer udata);
static void   confuiTableEditText (GtkCellRendererText* r, const gchar* path,
    const gchar* ntext, gpointer udata);
static void   confuiTableEditSpinbox (GtkCellRendererText* r, const gchar* path,
    const gchar* ntext, gpointer udata);
static void   confuiTableEdit (configui_t *confui, GtkCellRendererText* r,
    const gchar* path, const gchar* ntext, int type);
static void   confuiTableSave (configui_t *confui, confuiident_t id);
static int    confuiRatingListCreate (GtkTreeModel *model, GtkTreePath *path,
    GtkTreeIter *iter, gpointer udata);
static int    confuiLevelListCreate (GtkTreeModel *model, GtkTreePath *path,
    GtkTreeIter *iter, gpointer udata);
static int    confuiStatusListCreate (GtkTreeModel *model, GtkTreePath *path,
    GtkTreeIter *iter, gpointer udata);
static int    confuiGenreListCreate (GtkTreeModel *model, GtkTreePath *path,
    GtkTreeIter *iter, gpointer udata);
static void   confuiDanceSave (configui_t *confui);
static void   confuiRatingSave (configui_t *confui);
static void   confuiLevelSave (configui_t *confui);
static void   confuiStatusSave (configui_t *confui);
static void   confuiGenreSave (configui_t *confui);

/* dance table */
static void   confuiDanceSelect (GtkTreeView *tv, GtkTreePath *path,
    GtkTreeViewColumn *column, gpointer udata);
static int    confuiDanceEntryDanceChg (uientry_t *entry, void *udata);
static int    confuiDanceEntryTagsChg (uientry_t *entry, void *udata);
static int    confuiDanceEntryAnnouncementChg (uientry_t *entry, void *udata);
static int    confuiDanceEntryChg (uientry_t *e, void *udata, int widx);
static bool   confuiDanceSpinboxTypeChg (void *udata);
static bool   confuiDanceSpinboxSpeedChg (void *udata);
static bool   confuiDanceSpinboxLowBPMChg (void *udata);
static bool   confuiDanceSpinboxHighBPMChg (void *udata);
static bool   confuiDanceSpinboxTimeSigChg (void *udata);
static void   confuiDanceSpinboxChg (void *udata, int widx);
static int    confuiDanceValidateAnnouncement (uientry_t *entry, configui_t *confui);

/* display settings */
static bool   confuiDispSettingChg (void *udata);
static void   confuiDispSaveTable (configui_t *confui, int selidx);
static void   confuiCreateTagSelectedDisp (configui_t *confui);
static void   confuiCreateTagListingDisp (configui_t *confui);

/* misc */
static long confuiValMSCallback (void *udata, const char *txt);

static bool gKillReceived = false;

int
main (int argc, char *argv[])
{
  int             status = 0;
  uint16_t        listenPort;
  char            *uifont = NULL;
  nlist_t         *tlist = NULL;
  nlist_t         *llist = NULL;
  nlistidx_t      iteridx;
  int             count;
  char            *p = NULL;
  volsinklist_t   sinklist;
  configui_t      confui;
  volume_t        *volume = NULL;
  orgopt_t        *orgopt = NULL;
  int             flags;
  char            tbuff [MAXPATHLEN];
  char            tbuffb [MAXPATHLEN];
  char            tbuffc [MAXPATHLEN];


  confui.progstate = progstateInit ("configui");
  progstateSetCallback (confui.progstate, STATE_WAIT_HANDSHAKE,
      confuiHandshakeCallback, &confui);
  confui.locknm = NULL;
  confui.conn = NULL;
  confui.localip = NULL;
  confui.dbgflags = 0;
  confui.tablecurr = CONFUI_ID_NONE;
  confui.nbtabid = uiutilsNotebookIDInit ();
  confui.listingtaglist = NULL;
  confui.edittaglist = NULL;
  confui.dispsel = NULL;
  confui.stopwaitcount = 0;
  confui.filterDisplayDf = NULL;
  confui.filterDisplaySel = NULL;
  confui.filterLookup = NULL;
  confui.indancechange = false;

  uiutilsUIWidgetInit (&confui.window);
  uiutilsUICallbackInit (&confui.closecb, NULL, NULL);
  uiutilsUIWidgetInit (&confui.vbox);
  uiutilsUIWidgetInit (&confui.notebook);
  uiutilsUICallbackInit (&confui.nbcb, NULL, NULL);
  uiutilsUIWidgetInit (&confui.statusMsg);
  confui.dispselduallist = NULL;
  confui.optiondf = NULL;
  confui.options = NULL;

  for (int i = 0; i < CONFUI_ID_TABLE_MAX; ++i) {
    for (int j = 0; j < CONFUI_TABLE_CB_MAX; ++j) {
      uiutilsUICallbackInit (&confui.tables [i].callback [j], NULL, NULL);
    }
    confui.tables [i].tree = NULL;
    confui.tables [i].sel = NULL;
    confui.tables [i].radiorow = 0;
    confui.tables [i].togglecol = -1;
    confui.tables [i].flags = CONFUI_TABLE_NONE;
    confui.tables [i].changed = false;
    confui.tables [i].currcount = 0;
    confui.tables [i].saveidx = 0;
    confui.tables [i].savelist = NULL;
    confui.tables [i].listcreatefunc = NULL;
    confui.tables [i].savefunc = NULL;
  }

  for (int i = 0; i < CONFUI_ITEM_MAX; ++i) {
    confui.uiitem [i].widget = NULL;
    confui.uiitem [i].basetype = CONFUI_NONE;
    confui.uiitem [i].outtype = CONFUI_OUT_NONE;
    confui.uiitem [i].bdjoptIdx = -1;
    confui.uiitem [i].listidx = 0;
    confui.uiitem [i].displist = NULL;
    confui.uiitem [i].sbkeylist = NULL;
    confui.uiitem [i].danceidx = DANCE_DANCE;
    uiutilsUIWidgetInit (&confui.uiitem [i].uiwidget);
    uiutilsUICallbackInit (&confui.uiitem [i].callback, NULL, NULL);
    confui.uiitem [i].uri = NULL;

    if (i > CONFUI_BEGIN && i < CONFUI_COMBOBOX_MAX) {
      confui.uiitem [i].dropdown = uiDropDownInit ();
    }
    if (i > CONFUI_ENTRY_MAX && i < CONFUI_SPINBOX_MAX) {
      if (i == CONFUI_SPINBOX_MAX_PLAY_TIME) {
        confui.uiitem [i].spinbox = uiSpinboxTimeInit (SB_TIME_BASIC);
      } else {
        confui.uiitem [i].spinbox = uiSpinboxTextInit ();
      }
    }
    if (i > CONFUI_SPINBOX_MAX && i < CONFUI_SWITCH_MAX) {
      confui.uiitem [i].uiswitch = NULL;
    }
  }

  confui.uiitem [CONFUI_ENTRY_DANCE_TAGS].entry = uiEntryInit (30, 100);
  confui.uiitem [CONFUI_ENTRY_DANCE_ANNOUNCEMENT].entry = uiEntryInit (30, 300);
  confui.uiitem [CONFUI_ENTRY_DANCE_DANCE].entry = uiEntryInit (30, 50);
  confui.uiitem [CONFUI_ENTRY_MM_NAME].entry = uiEntryInit (10, 40);
  confui.uiitem [CONFUI_ENTRY_MM_TITLE].entry = uiEntryInit (20, 100);
  confui.uiitem [CONFUI_ENTRY_MUSIC_DIR].entry = uiEntryInit (50, 300);
  confui.uiitem [CONFUI_ENTRY_PROFILE_NAME].entry = uiEntryInit (20, 30);
  confui.uiitem [CONFUI_ENTRY_COMPLETE_MSG].entry = uiEntryInit (20, 30);
  confui.uiitem [CONFUI_ENTRY_QUEUE_NM_A].entry = uiEntryInit (20, 30);
  confui.uiitem [CONFUI_ENTRY_QUEUE_NM_B].entry = uiEntryInit (20, 30);
  confui.uiitem [CONFUI_ENTRY_RC_PASS].entry = uiEntryInit (10, 20);
  confui.uiitem [CONFUI_ENTRY_RC_USER_ID].entry = uiEntryInit (10, 30);
  confui.uiitem [CONFUI_ENTRY_STARTUP].entry = uiEntryInit (50, 300);
  confui.uiitem [CONFUI_ENTRY_SHUTDOWN].entry = uiEntryInit (50, 300);

  osSetStandardSignals (confuiSigHandler);

  flags = BDJ4_INIT_NO_DB_LOAD;
  confui.dbgflags = bdj4startup (argc, argv, NULL,
      "cu", ROUTE_CONFIGUI, flags);
  logProcBegin (LOG_PROC, "configui");

  confui.dispsel = dispselAlloc ();

  orgopt = orgoptAlloc ();
  assert (orgopt != NULL);
  tlist = orgoptGetList (orgopt);

  confui.uiitem [CONFUI_COMBOBOX_AO_PATHFMT].displist = tlist;
  slistStartIterator (tlist, &iteridx);
  confui.uiitem [CONFUI_COMBOBOX_AO_PATHFMT].listidx = 0;
  count = 0;
  while ((p = slistIterateValueData (tlist, &iteridx)) != NULL) {
    if (strcmp (p, bdjoptGetStr (OPT_G_AO_PATHFMT)) == 0) {
      confui.uiitem [CONFUI_COMBOBOX_AO_PATHFMT].listidx = count;
      break;
    }
    ++count;
  }

  volume = volumeInit ();
  volumeSinklistInit (&sinklist);
  assert (volume != NULL);
  volumeGetSinkList (volume, "", &sinklist);
  tlist = nlistAlloc ("cu-audio-out", LIST_ORDERED, free);
  llist = nlistAlloc ("cu-audio-out-l", LIST_ORDERED, free);
  /* CONTEXT: audio: The default audio sink (audio output) */
  nlistSetStr (tlist, 0, _("Default"));
  nlistSetStr (llist, 0, "default");
  confui.uiitem [CONFUI_SPINBOX_AUDIO_OUTPUT].listidx = 0;
  for (size_t i = 0; i < sinklist.count; ++i) {
    if (strcmp (sinklist.sinklist [i].name, bdjoptGetStr (OPT_M_AUDIOSINK)) == 0) {
      confui.uiitem [CONFUI_SPINBOX_AUDIO_OUTPUT].listidx = i + 1;
    }
    nlistSetStr (tlist, i + 1, sinklist.sinklist [i].description);
    nlistSetStr (llist, i + 1, sinklist.sinklist [i].name);
  }
  confui.uiitem [CONFUI_SPINBOX_AUDIO_OUTPUT].displist = tlist;
  confui.uiitem [CONFUI_SPINBOX_AUDIO_OUTPUT].sbkeylist = llist;
  volumeFreeSinkList (&sinklist);
  volumeFree (volume);

  confuiSpinboxTextInitDataNum (&confui, "cu-audio-file-tags",
      CONFUI_SPINBOX_WRITE_AUDIO_FILE_TAGS,
      /* CONTEXT: write audio file tags: do not write any tags to the audio file */
      WRITE_TAGS_NONE, _("Don't Write"),
      /* CONTEXT: write audio file tags: only write BDJ tags to the audio file */
      WRITE_TAGS_BDJ_ONLY, _("BDJ Tags Only"),
      /* CONTEXT: write audio file tags: write all tags (BDJ and standard) to the audio file */
      WRITE_TAGS_ALL, _("All Tags"),
      -1);

  confuiSpinboxTextInitDataNum (&confui, "cu-bpm",
      CONFUI_SPINBOX_BPM,
      /* CONTEXT: BPM: beats per minute (not bars per minute) */
      BPM_BPM, "BPM",
      /* CONTEXT: MPM: measures per minute (aka bars per minute) */
      BPM_MPM, "MPM",
      -1);

  confuiSpinboxTextInitDataNum (&confui, "cu-dance-speed",
      CONFUI_SPINBOX_DANCE_SPEED,
      /* CONTEXT: dance speed */
      DANCE_SPEED_SLOW, _("slow"),
      /* CONTEXT: dance speed */
      DANCE_SPEED_NORMAL, _("normal"),
      /* CONTEXT: dance speed */
      DANCE_SPEED_FAST, _("fast"),
      -1);

  confuiSpinboxTextInitDataNum (&confui, "cu-dance-time-sig",
      CONFUI_SPINBOX_DANCE_TIME_SIG,
      /* CONTEXT: dance time signature */
      DANCE_TIMESIG_24, _("2/4"),
      /* CONTEXT: dance time signature */
      DANCE_TIMESIG_34, _("3/4"),
      /* CONTEXT: dance time signature */
      DANCE_TIMESIG_44, _("4/4"),
      /* CONTEXT: dance time signature */
      DANCE_TIMESIG_48, _("4/8"),
      -1);

  /* CONTEXT: display settings for: song editor column N */
  snprintf (tbuff, sizeof (tbuff), _("Song Editor - Column %d"), 1);
  snprintf (tbuffb, sizeof (tbuffb), _("Song Editor - Column %d"), 2);
  snprintf (tbuffc, sizeof (tbuffc), _("Song Editor - Column %d"), 3);
  confuiSpinboxTextInitDataNum (&confui, "cu-disp-settings",
      CONFUI_SPINBOX_DISP_SEL,
      /* CONTEXT: display settings for: music queue */
      DISP_SEL_MUSICQ, _("Music Queue"),
      /* CONTEXT: display settings for: requests */
      DISP_SEL_REQUEST, _("Request"),
      /* CONTEXT: display settings for: song list */
      DISP_SEL_SONGLIST, _("Song List"),
      /* CONTEXT: display settings for: song selection */
      DISP_SEL_SONGSEL, _("Song Selection"),
      /* CONTEXT: display settings for: easy song list */
      DISP_SEL_EZSONGLIST, _("Easy Song List"),
      /* CONTEXT: display settings for: easy song selection */
      DISP_SEL_EZSONGSEL, _("Easy Song Selection"),
      /* CONTEXT: display settings for: music manager */
      DISP_SEL_MM, _("Music Manager"),
      DISP_SEL_SONGEDIT_A, tbuff,
      DISP_SEL_SONGEDIT_B, tbuffb,
      DISP_SEL_SONGEDIT_C, tbuffc,
      -1);
  confui.uiitem [CONFUI_SPINBOX_DISP_SEL].listidx = DISP_SEL_MUSICQ;

  confuiLoadHTMLList (&confui);
  confuiLoadVolIntfcList (&confui);
  confuiLoadPlayerIntfcList (&confui);
  confuiLoadLocaleList (&confui);
  confuiLoadDanceTypeList (&confui);
  confuiLoadTagList (&confui);
  confuiLoadThemeList (&confui);

  confuiSpinboxTextInitDataNum (&confui, "cu-mob-mq",
      CONFUI_SPINBOX_MOBILE_MQ,
      /* CONTEXT: mobile marquee: off */
      MOBILEMQ_OFF, _("Off"),
      /* CONTEXT: mobile marquee: use local router */
      MOBILEMQ_LOCAL, _("Local"),
      /* CONTEXT: mobile marquee: route via the internet */
      MOBILEMQ_INTERNET, _("Internet"),
      -1);

  pathbldMakePath (tbuff, sizeof (tbuff),
      "ds-songfilter", BDJ4_CONFIG_EXT, PATHBLD_MP_DATA | PATHBLD_MP_USEIDX);
  confui.filterDisplayDf = datafileAllocParse ("cu-filter",
      DFTYPE_KEY_VAL, tbuff, filterdisplaydfkeys, FILTER_DISP_MAX, DATAFILE_NO_LOOKUP);
  confui.filterDisplaySel = datafileGetList (confui.filterDisplayDf);
  llist = nlistAlloc ("cu-filter-out", LIST_ORDERED, free);
  nlistStartIterator (confui.filterDisplaySel, &iteridx);
  nlistSetNum (llist, CONFUI_WIDGET_FILTER_GENRE, FILTER_DISP_GENRE);
  nlistSetNum (llist, CONFUI_WIDGET_FILTER_DANCELEVEL, FILTER_DISP_DANCELEVEL);
  nlistSetNum (llist, CONFUI_WIDGET_FILTER_STATUS, FILTER_DISP_STATUS);
  nlistSetNum (llist, CONFUI_WIDGET_FILTER_FAVORITE, FILTER_DISP_FAVORITE);
  nlistSetNum (llist, CONFUI_SWITCH_FILTER_STATUS_PLAYABLE, FILTER_DISP_STATUSPLAYABLE);
  confui.filterLookup = llist;

  listenPort = bdjvarsGetNum (BDJVL_CONFIGUI_PORT);
  confui.conn = connInit (ROUTE_CONFIGUI);

  pathbldMakePath (tbuff, sizeof (tbuff),
      "configui", BDJ4_CONFIG_EXT, PATHBLD_MP_DATA | PATHBLD_MP_USEIDX);
  confui.optiondf = datafileAllocParse ("configui-opt", DFTYPE_KEY_VAL, tbuff,
      configuidfkeys, CONFUI_KEY_MAX, DATAFILE_NO_LOOKUP);
  confui.options = datafileGetList (confui.optiondf);
  if (confui.options == NULL) {
    confui.options = nlistAlloc ("configui-opt", LIST_ORDERED, free);

    nlistSetNum (confui.options, CONFUI_POSITION_X, -1);
    nlistSetNum (confui.options, CONFUI_POSITION_Y, -1);
    nlistSetNum (confui.options, CONFUI_SIZE_X, 1200);
    nlistSetNum (confui.options, CONFUI_SIZE_Y, 600);
  }

  /* register these after calling the sub-window initialization */
  /* then these will be run last, after the other closing callbacks */
  progstateSetCallback (confui.progstate, STATE_STOPPING,
      confuiStoppingCallback, &confui);
  progstateSetCallback (confui.progstate, STATE_STOP_WAIT,
      confuiStopWaitCallback, &confui);
  progstateSetCallback (confui.progstate, STATE_CLOSING,
      confuiClosingCallback, &confui);

  uiUIInitialize ();
  uifont = bdjoptGetStr (OPT_MP_UIFONT);
  uiSetUIFont (uifont);

  confuiBuildUI (&confui);
  sockhMainLoop (listenPort, confuiProcessMsg, confuiMainLoop, &confui);
  connFree (confui.conn);
  progstateFree (confui.progstate);
  orgoptFree (orgopt);

  logProcEnd (LOG_PROC, "configui", "");
  logEnd ();
  return status;
}

/* internal routines */

static bool
confuiStoppingCallback (void *udata, programstate_t programState)
{
  configui_t    * confui = udata;
  int           x, y;
  char          fn [MAXPATHLEN];

  logProcBegin (LOG_PROC, "confuiStoppingCallback");

  confuiPopulateOptions (confui);
  bdjoptSave ();
  for (confuiident_t i = 0; i < CONFUI_ID_TABLE_MAX; ++i) {
    confuiTableSave (confui, i);
  }

  uiWindowGetSize (&confui->window, &x, &y);
  nlistSetNum (confui->options, CONFUI_SIZE_X, x);
  nlistSetNum (confui->options, CONFUI_SIZE_Y, y);
  uiWindowGetPosition (&confui->window, &x, &y);
  nlistSetNum (confui->options, CONFUI_POSITION_X, x);
  nlistSetNum (confui->options, CONFUI_POSITION_Y, y);

  pathbldMakePath (fn, sizeof (fn),
      "configui", BDJ4_CONFIG_EXT, PATHBLD_MP_DATA | PATHBLD_MP_USEIDX);
  datafileSaveKeyVal ("configui", fn, configuidfkeys,
      CONFUI_KEY_MAX, confui->options);

  pathbldMakePath (fn, sizeof (fn),
      "ds-songfilter", BDJ4_CONFIG_EXT, PATHBLD_MP_DATA | PATHBLD_MP_USEIDX);
  datafileSaveKeyVal ("ds-songfilter", fn, filterdisplaydfkeys,
      FILTER_DISP_MAX, confui->filterDisplaySel);

  connDisconnect (confui->conn, ROUTE_STARTERUI);

  logProcEnd (LOG_PROC, "confuiStoppingCallback", "");
  return STATE_FINISHED;
}

static bool
confuiStopWaitCallback (void *udata, programstate_t programState)
{
  configui_t  * confui = udata;
  bool        rc;

  rc = connWaitClosed (confui->conn, &confui->stopwaitcount);
  return rc;
}

static bool
confuiClosingCallback (void *udata, programstate_t programState)
{
  configui_t    *confui = udata;

  logProcBegin (LOG_PROC, "confuiClosingCallback");

  uiCloseWindow (&confui->window);

  for (int i = CONFUI_BEGIN + 1; i < CONFUI_COMBOBOX_MAX; ++i) {
    uiDropDownFree (confui->uiitem [i].dropdown);
  }

  for (int i = CONFUI_COMBOBOX_MAX + 1; i < CONFUI_ENTRY_MAX; ++i) {
    uiEntryFree (confui->uiitem [i].entry);
  }

  for (int i = CONFUI_ENTRY_MAX + 1; i < CONFUI_SPINBOX_MAX; ++i) {
    uiSpinboxTextFree (confui->uiitem [i].spinbox);
    /* the mq and ui-theme share the list */
    if (i == CONFUI_SPINBOX_UI_THEME) {
      continue;
    }
    if (confui->uiitem [i].displist != NULL) {
      nlistFree (confui->uiitem [i].displist);
    }
    if (confui->uiitem [i].sbkeylist != NULL) {
      nlistFree (confui->uiitem [i].sbkeylist);
    }
  }

  for (int i = CONFUI_SPINBOX_MAX + 1; i < CONFUI_SWITCH_MAX; ++i) {
    uiSwitchFree (confui->uiitem [i].uiswitch);
  }

  for (int i = CONFUI_SWITCH_MAX + 1; i < CONFUI_ITEM_MAX; ++i) {
    if (confui->uiitem [i].uri != NULL) {
      free (confui->uiitem [i].uri);
    }
  }

  if (confui->dispselduallist != NULL) {
    uiduallistFree (confui->dispselduallist);
  }
  if (confui->filterDisplayDf != NULL) {
    datafileFree (confui->filterDisplayDf);
  }
  if (confui->filterLookup != NULL) {
    nlistFree (confui->filterLookup);
  }
  dispselFree (confui->dispsel);
  if (confui->localip != NULL) {
    free (confui->localip);
  }
  if (confui->optiondf != NULL) {
    datafileFree (confui->optiondf);
  } else if (confui->options != NULL) {
    nlistFree (confui->options);
  }
  if (confui->nbtabid != NULL) {
    uiutilsNotebookIDFree (confui->nbtabid);
  }
  if (confui->listingtaglist != NULL) {
    slistFree (confui->listingtaglist);
  }
  if (confui->edittaglist != NULL) {
    slistFree (confui->edittaglist);
  }

  bdj4shutdown (ROUTE_CONFIGUI, NULL);

  uiCleanup ();

  logProcEnd (LOG_PROC, "confuiClosingCallback", "");
  return STATE_FINISHED;
}

static void
confuiBuildUI (configui_t *confui)
{
  UIWidget      menubar;
  UIWidget      hbox;
  UIWidget      uiwidget;
  char          imgbuff [MAXPATHLEN];
  char          tbuff [MAXPATHLEN];
  int           x, y;

  logProcBegin (LOG_PROC, "confuiBuildUI");

  pathbldMakePath (imgbuff, sizeof (imgbuff),
      "bdj4_icon_config", ".svg", PATHBLD_MP_IMGDIR);
  /* CONTEXT: configuration user interface window title */
  snprintf (tbuff, sizeof (tbuff), _("%s Configuration"),
      bdjoptGetStr (OPT_P_PROFILENAME));
  uiutilsUICallbackInit (&confui->closecb, confuiCloseWin, confui);
  uiCreateMainWindow (&confui->window, &confui->closecb, tbuff, imgbuff);

  uiCreateVertBox (&confui->vbox);
  uiWidgetExpandHoriz (&confui->vbox);
  uiWidgetExpandVert (&confui->vbox);
  uiWidgetSetAllMargins (&confui->vbox, uiBaseMarginSz * 2);
  uiBoxPackInWindow (&confui->window, &confui->vbox);

  uiCreateHorizBox (&hbox);
  uiWidgetExpandHoriz (&hbox);
  uiBoxPackStart (&confui->vbox, &hbox);

  uiCreateLabel (&uiwidget, "");
  uiWidgetSetSizeRequest (&uiwidget, 30, -1);
  uiWidgetSetMarginStart (&uiwidget, uiBaseMarginSz * 3);
  uiLabelSetBackgroundColor (&uiwidget, bdjoptGetStr (OPT_P_UI_PROFILE_COL));
  uiBoxPackEnd (&hbox, &uiwidget);

  uiCreateLabel (&uiwidget, "");
  uiLabelSetColor (&uiwidget, bdjoptGetStr (OPT_P_UI_ERROR_COL));
  uiBoxPackEnd (&hbox, &uiwidget);
  uiutilsUIWidgetCopy (&confui->statusMsg, &uiwidget);

  uiCreateMenubar (&menubar);
  uiBoxPackStart (&hbox, &menubar);

  uiCreateNotebook (&confui->notebook);
  uiNotebookTabPositionLeft (&confui->notebook);
  uiBoxPackStartExpand (&confui->vbox, &confui->notebook);

  confuiBuildUIGeneral (confui);
  confuiBuildUIPlayer (confui);
  confuiBuildUIMarquee (confui);
  confuiBuildUIUserInterface (confui);
  confuiBuildUIDispSettings (confui);
  confuiBuildUIFilterDisplay (confui);
  confuiBuildUIOrganization (confui);
  confuiBuildUIEditDances (confui);
  confuiBuildUIEditRatings (confui);
  confuiBuildUIEditStatus (confui);
  confuiBuildUIEditLevels (confui);
  confuiBuildUIEditGenres (confui);
  confuiBuildUIMobileRemoteControl (confui);
  confuiBuildUIMobileMarquee (confui);
  confuiBuildUIDebugOptions (confui);

  uiutilsUICallbackLongInit (&confui->nbcb, confuiSwitchTable, confui);
  uiNotebookSetCallback (&confui->notebook, &confui->nbcb);

  x = nlistGetNum (confui->options, CONFUI_SIZE_X);
  y = nlistGetNum (confui->options, CONFUI_SIZE_Y);
  uiWindowSetDefaultSize (&confui->window, x, y);

  uiWidgetShowAll (&confui->window);

  x = nlistGetNum (confui->options, CONFUI_POSITION_X);
  y = nlistGetNum (confui->options, CONFUI_POSITION_Y);
  uiWindowMove (&confui->window, x, y);

  pathbldMakePath (imgbuff, sizeof (imgbuff),
      "bdj4_icon_config", ".png", PATHBLD_MP_IMGDIR);
  osuiSetIcon (imgbuff);

  logProcEnd (LOG_PROC, "confuiBuildUI", "");
}

static void
confuiBuildUIGeneral (configui_t *confui)
{
  UIWidget      vbox;
  UIWidget      sg;
  char          tbuff [MAXPATHLEN];

  logProcBegin (LOG_PROC, "confuiBuildUIGeneral");
  uiCreateVertBox (&vbox);

  /* general options */
  confuiMakeNotebookTab (&vbox, confui, &confui->notebook,
      /* CONTEXT: configuration: general options that apply to everything */
      _("General Options"), CONFUI_ID_NONE);
  uiCreateSizeGroupHoriz (&sg);

  strlcpy (tbuff, bdjoptGetStr (OPT_M_DIR_MUSIC), sizeof (tbuff));
  if (isWindows ()) {
    pathWinPath (tbuff, sizeof (tbuff));
  }

  /* CONTEXT: configuration: the music folder where the user store their music */
  confuiMakeItemEntryChooser (confui, &vbox, &sg, _("Music Folder"),
      CONFUI_ENTRY_MUSIC_DIR, OPT_M_DIR_MUSIC,
      tbuff, confuiSelectMusicDir);
  uiEntrySetValidate (confui->uiitem [CONFUI_ENTRY_MUSIC_DIR].entry,
      uiEntryValidateDir, confui, UIENTRY_DELAYED);

  /* CONTEXT: configuration: the name of this profile */
  confuiMakeItemEntry (confui, &vbox, &sg, _("Profile Name"),
      CONFUI_ENTRY_PROFILE_NAME, OPT_P_PROFILENAME,
      bdjoptGetStr (OPT_P_PROFILENAME));

  /* CONTEXT: configuration: the profile accent color (identifies profile) */
  confuiMakeItemColorButton (confui, &vbox, &sg, _("Profile Colour"),
      CONFUI_WIDGET_UI_PROFILE_COLOR, OPT_P_UI_PROFILE_COL,
      bdjoptGetStr (OPT_P_UI_PROFILE_COL));

  /* CONTEXT: configuration: Whether to display BPM as BPM or MPM */
  confuiMakeItemSpinboxText (confui, &vbox, &sg, NULL, _("BPM"),
      CONFUI_SPINBOX_BPM, OPT_G_BPM,
      CONFUI_OUT_NUM, bdjoptGetNum (OPT_G_BPM), NULL);

  /* database */
  /* CONTEXT: configuration: which audio tags will be written to the audio file */
  confuiMakeItemSpinboxText (confui, &vbox, &sg, NULL, _("Write Audio File Tags"),
      CONFUI_SPINBOX_WRITE_AUDIO_FILE_TAGS, OPT_G_WRITETAGS,
      CONFUI_OUT_NUM, bdjoptGetNum (OPT_G_WRITETAGS), NULL);
  confuiMakeItemSwitch (confui, &vbox, &sg,
      /* CONTEXT: checkbox: the database will load the dance from the audio file genre tag */
      _("Database Loads Dance From Genre"),
      CONFUI_SWITCH_DB_LOAD_FROM_GENRE, OPT_G_LOADDANCEFROMGENRE,
      bdjoptGetNum (OPT_G_LOADDANCEFROMGENRE), NULL);
  confuiMakeItemSwitch (confui, &vbox, &sg,
      /* CONTEXT: configuration: enable itunes support */
      _("Enable iTunes Support"),
      CONFUI_SWITCH_ENABLE_ITUNES, OPT_G_ITUNESSUPPORT,
      bdjoptGetNum (OPT_G_ITUNESSUPPORT), NULL);

  /* bdj4 */
  /* CONTEXT: configuration: the locale to use (e.g. English or Nederlands) */
  confuiMakeItemSpinboxText (confui, &vbox, &sg, NULL, _("Locale"),
      CONFUI_SPINBOX_LOCALE, -1,
      CONFUI_OUT_STR, confui->uiitem [CONFUI_SPINBOX_LOCALE].listidx, NULL);

  /* CONTEXT: configuration: the startup script to run before starting the player.  Used on Linux. */
  confuiMakeItemEntryChooser (confui, &vbox, &sg, _("Startup Script"),
      CONFUI_ENTRY_STARTUP, OPT_M_STARTUPSCRIPT,
      bdjoptGetStr (OPT_M_STARTUPSCRIPT), confuiSelectStartup);
  uiEntrySetValidate (confui->uiitem [CONFUI_ENTRY_STARTUP].entry,
      uiEntryValidateFile, confui, UIENTRY_DELAYED);

  /* CONTEXT: configuration: the shutdown script to run before starting the player.  Used on Linux. */
  confuiMakeItemEntryChooser (confui, &vbox, &sg, _("Shutdown Script"),
      CONFUI_ENTRY_SHUTDOWN, OPT_M_SHUTDOWNSCRIPT,
      bdjoptGetStr (OPT_M_SHUTDOWNSCRIPT), confuiSelectShutdown);
  uiEntrySetValidate (confui->uiitem [CONFUI_ENTRY_SHUTDOWN].entry,
      uiEntryValidateFile, confui, UIENTRY_DELAYED);
  logProcEnd (LOG_PROC, "confuiBuildUIGeneral", "");
}

static void
confuiBuildUIPlayer (configui_t *confui)
{
  UIWidget      vbox;
  UIWidget      sg;
  UIWidget      sgB;
  char          *mqnames [MUSICQ_PB_MAX];

  /* CONTEXT: configuration: The name of the music queue */
  mqnames [MUSICQ_PB_A] = _("Queue A Name");
  /* CONTEXT: configuration: The name of the music queue */
  mqnames [MUSICQ_PB_B] = _("Queue B Name");

  logProcBegin (LOG_PROC, "confuiBuildUIPlayer");
  uiCreateVertBox (&vbox);

  /* player options */
  confuiMakeNotebookTab (&vbox, confui, &confui->notebook,
      /* CONTEXT: configuration: options associated with the player */
      _("Player Options"), CONFUI_ID_NONE);
  uiCreateSizeGroupHoriz (&sg);
  uiCreateSizeGroupHoriz (&sgB);

  /* CONTEXT: configuration: which player interface to use */
  confuiMakeItemSpinboxText (confui, &vbox, &sg, NULL, _("Player"),
      CONFUI_SPINBOX_PLAYER, OPT_M_PLAYER_INTFC,
      CONFUI_OUT_STR, confui->uiitem [CONFUI_SPINBOX_PLAYER].listidx, NULL);

  /* CONTEXT: configuration: which audio interface to use */
  confuiMakeItemSpinboxText (confui, &vbox, &sg, NULL, _("Audio"),
      CONFUI_SPINBOX_VOL_INTFC, OPT_M_VOLUME_INTFC,
      CONFUI_OUT_STR, confui->uiitem [CONFUI_SPINBOX_VOL_INTFC].listidx, NULL);

  /* CONTEXT: configuration: which audio sink (output) to use */
  confuiMakeItemSpinboxText (confui, &vbox, &sg, NULL, _("Audio Output"),
      CONFUI_SPINBOX_AUDIO_OUTPUT, OPT_M_AUDIOSINK,
      CONFUI_OUT_STR, confui->uiitem [CONFUI_SPINBOX_AUDIO_OUTPUT].listidx, NULL);

  /* CONTEXT: configuration: the volume used when starting the player */
  confuiMakeItemSpinboxNum (confui, &vbox, &sg, &sgB, _("Default Volume"),
      CONFUI_WIDGET_DEFAULT_VOL, OPT_P_DEFAULTVOLUME,
      10, 100, bdjoptGetNum (OPT_P_DEFAULTVOLUME), NULL);

  /* CONTEXT: configuration: the amount of time to do a volume fade-in when playing a song */
  confuiMakeItemSpinboxDouble (confui, &vbox, &sg, &sgB, _("Fade In Time"),
      CONFUI_WIDGET_FADE_IN_TIME, OPT_P_FADEINTIME,
      0.0, 2.0, (double) bdjoptGetNum (OPT_P_FADEINTIME) / 1000.0);

  /* CONTEXT: configuration: the amount of time to do a volume fade-out when playing a song */
  confuiMakeItemSpinboxDouble (confui, &vbox, &sg, &sgB, _("Fade Out Time"),
      CONFUI_WIDGET_FADE_OUT_TIME, OPT_P_FADEOUTTIME,
      0.0, 10.0, (double) bdjoptGetNum (OPT_P_FADEOUTTIME) / 1000.0);

  /* CONTEXT: configuration: the amount of time to wait inbetween songs */
  confuiMakeItemSpinboxDouble (confui, &vbox, &sg, &sgB, _("Gap Between Songs"),
      CONFUI_WIDGET_GAP, OPT_P_GAP,
      0.0, 60.0, (double) bdjoptGetNum (OPT_P_GAP) / 1000.0);

  /* CONTEXT: configuration: the maximum amount of time to play a song */
  confuiMakeItemSpinboxTime (confui, &vbox, &sg, &sgB, _("Maximum Play Time"),
      CONFUI_SPINBOX_MAX_PLAY_TIME, OPT_P_MAXPLAYTIME,
      bdjoptGetNum (OPT_P_MAXPLAYTIME));

  /* CONTEXT: configuration: the &sgB, of the music queue to display */
  confuiMakeItemSpinboxNum (confui, &vbox, &sg, &sgB, _("Queue Length"),
      CONFUI_WIDGET_PL_QUEUE_LEN, OPT_G_PLAYERQLEN,
      20, 400, bdjoptGetNum (OPT_G_PLAYERQLEN), NULL);

  /* CONTEXT: configuration: The completion message displayed on the marquee when a playlist is finished */
  confuiMakeItemEntry (confui, &vbox, &sg, _("Completion Message"),
      CONFUI_ENTRY_COMPLETE_MSG, OPT_P_COMPLETE_MSG,
      bdjoptGetStr (OPT_P_COMPLETE_MSG));

  for (musicqidx_t i = 0; i < MUSICQ_PB_MAX; ++i) {
    confuiMakeItemEntry (confui, &vbox, &sg, mqnames [i],
        CONFUI_ENTRY_QUEUE_NM_A + i, OPT_P_QUEUE_NAME_A + i,
        bdjoptGetStr (OPT_P_QUEUE_NAME_A + i));
  }
  logProcEnd (LOG_PROC, "confuiBuildUIPlayer", "");
}

static void
confuiBuildUIMarquee (configui_t *confui)
{
  UIWidget      vbox;
  UIWidget      sg;

  logProcBegin (LOG_PROC, "confuiBuildUIMarquee");
  uiCreateVertBox (&vbox);

  /* marquee options */
  confuiMakeNotebookTab (&vbox, confui, &confui->notebook,
      /* CONTEXT: configuration: options associated with the marquee */
      _("Marquee Options"), CONFUI_ID_NONE);
  uiCreateSizeGroupHoriz (&sg);

  /* CONTEXT: configuration: The theme to use for the marquee display */
  confuiMakeItemSpinboxText (confui, &vbox, &sg, NULL, _("Marquee Theme"),
      CONFUI_SPINBOX_MQ_THEME, OPT_MP_MQ_THEME,
      CONFUI_OUT_STR, confui->uiitem [CONFUI_SPINBOX_MQ_THEME].listidx, NULL);

  /* CONTEXT: configuration: The font to use for the marquee display */
  confuiMakeItemFontButton (confui, &vbox, &sg, _("Marquee Font"),
      CONFUI_WIDGET_MQ_FONT, OPT_MP_MQFONT,
      bdjoptGetStr (OPT_MP_MQFONT));

  /* CONTEXT: configuration: the length of the queue displayed on the marquee */
  confuiMakeItemSpinboxNum (confui, &vbox, &sg, NULL, _("Queue Length"),
      CONFUI_WIDGET_MQ_QUEUE_LEN, OPT_P_MQQLEN,
      1, 20, bdjoptGetNum (OPT_P_MQQLEN), NULL);

  /* CONTEXT: configuration: marquee: show the song information (artist/title) on the marquee */
  confuiMakeItemSwitch (confui, &vbox, &sg, _("Show Song Information"),
      CONFUI_SWITCH_MQ_SHOW_SONG_INFO, OPT_P_MQ_SHOW_INFO,
      bdjoptGetNum (OPT_P_MQ_SHOW_INFO), NULL);

  /* CONTEXT: configuration: marquee: the accent color used for the marquee */
  confuiMakeItemColorButton (confui, &vbox, &sg, _("Accent Colour"),
      CONFUI_WIDGET_MQ_ACCENT_COLOR, OPT_P_MQ_ACCENT_COL,
      bdjoptGetStr (OPT_P_MQ_ACCENT_COL));

  /* CONTEXT: configuration: marquee: minimize the marquee when the player is started */
  confuiMakeItemSwitch (confui, &vbox, &sg, _("Hide Marquee on Start"),
      CONFUI_SWITCH_MQ_HIDE_ON_START, OPT_P_HIDE_MARQUEE_ON_START,
      bdjoptGetNum (OPT_P_HIDE_MARQUEE_ON_START), NULL);
  logProcEnd (LOG_PROC, "confuiBuildUIMarquee", "");
}

static void
confuiBuildUIUserInterface (configui_t *confui)
{
  UIWidget      vbox;
  UIWidget      sg;
  char          *tstr;

  logProcBegin (LOG_PROC, "confuiBuildUIUserInterface");
  uiCreateVertBox (&vbox);

  /* user interface */
  confuiMakeNotebookTab (&vbox, confui, &confui->notebook,
      /* CONTEXT: configuration: options associated with the user interface */
      _("User Interface"), CONFUI_ID_NONE);
  uiCreateSizeGroupHoriz (&sg);

  /* CONTEXT: configuration: the theme to use for the user interface */
  confuiMakeItemSpinboxText (confui, &vbox, &sg, NULL, _("Theme"),
      CONFUI_SPINBOX_UI_THEME, OPT_MP_UI_THEME, CONFUI_OUT_STR,
      confui->uiitem [CONFUI_SPINBOX_UI_THEME].listidx, NULL);

  tstr = bdjoptGetStr (OPT_MP_UIFONT);
  if (! *tstr) {
    tstr = sysvarsGetStr (SV_FONT_DEFAULT);
  }
  /* CONTEXT: configuration: the font to use for the user interface */
  confuiMakeItemFontButton (confui, &vbox, &sg, _("Font"),
      CONFUI_WIDGET_UI_FONT, OPT_MP_UIFONT, tstr);

  tstr = bdjoptGetStr (OPT_MP_LISTING_FONT);
  if (! *tstr) {
    tstr = sysvarsGetStr (SV_FONT_DEFAULT);
  }
  /* CONTEXT: configuration: the font to use for the queues and song lists */
  confuiMakeItemFontButton (confui, &vbox, &sg, _("Listing Font"),
      CONFUI_WIDGET_UI_LISTING_FONT, OPT_MP_LISTING_FONT, tstr);

  /* CONTEXT: configuration: the accent color to use for the user interface */
  confuiMakeItemColorButton (confui, &vbox, &sg, _("Accent Colour"),
      CONFUI_WIDGET_UI_ACCENT_COLOR, OPT_P_UI_ACCENT_COL,
      bdjoptGetStr (OPT_P_UI_ACCENT_COL));
  logProcEnd (LOG_PROC, "confuiBuildUIUserInterface", "");
}

static void
confuiBuildUIDispSettings (configui_t *confui)
{
  UIWidget      vbox;
  UIWidget      uiwidget;
  UIWidget      sg;

  logProcBegin (LOG_PROC, "confuiBuildUIDispSettings");
  uiutilsUIWidgetInit (&uiwidget);
  uiutilsUIWidgetInit (&sg);

  uiCreateVertBox (&vbox);

  /* display settings */
  confuiMakeNotebookTab (&vbox, confui, &confui->notebook,
      /* CONTEXT: configuration: change which fields are displayed in different contexts */
      _("Display Settings"), CONFUI_ID_DISP_SEL_LIST);
  uiCreateSizeGroupHoriz (&sg);

  /* CONTEXT: configuration: display settings: which set of display settings to update */
  confuiMakeItemSpinboxText (confui, &vbox, &sg, NULL, _("Display"),
      CONFUI_SPINBOX_DISP_SEL, -1, CONFUI_OUT_NUM,
      confui->uiitem [CONFUI_SPINBOX_DISP_SEL].listidx,
      confuiDispSettingChg);

  confui->tables [CONFUI_ID_DISP_SEL_LIST].flags = CONFUI_TABLE_NONE;
  confui->tables [CONFUI_ID_DISP_SEL_TABLE].flags = CONFUI_TABLE_NONE;

  confui->dispselduallist = uiCreateDualList (&vbox,
      DUALLIST_FLAGS_NONE, NULL, NULL);
  logProcEnd (LOG_PROC, "confuiBuildUIDispSettings", "");
}

static void
confuiBuildUIFilterDisplay (configui_t *confui)
{
  UIWidget      vbox;
  UIWidget      sg;
  nlistidx_t    val;

  logProcBegin (LOG_PROC, "confuiBuildUIFilterDisplay");
  uiCreateVertBox (&vbox);

  /* filter display */
  confuiMakeNotebookTab (&vbox, confui, &confui->notebook,
      /* CONTEXT: configuration: song filter display settings */
      _("Filter Display"), CONFUI_ID_FILTER);
  uiCreateSizeGroupHoriz (&sg);

  val = nlistGetNum (confui->filterDisplaySel, FILTER_DISP_GENRE);
  /* CONTEXT: configuration: filter display: checkbox: genre */
  confuiMakeItemCheckButton (confui, &vbox, &sg, _("Genre"),
      CONFUI_WIDGET_FILTER_GENRE, -1, val);
  confui->uiitem [CONFUI_WIDGET_FILTER_GENRE].outtype = CONFUI_OUT_CB;

  val = nlistGetNum (confui->filterDisplaySel, FILTER_DISP_DANCELEVEL);
  /* CONTEXT: configuration: filter display: checkbox: dance level */
  confuiMakeItemCheckButton (confui, &vbox, &sg, _("Dance Level"),
      CONFUI_WIDGET_FILTER_DANCELEVEL, -1, val);
  confui->uiitem [CONFUI_WIDGET_FILTER_DANCELEVEL].outtype = CONFUI_OUT_CB;

  val = nlistGetNum (confui->filterDisplaySel, FILTER_DISP_STATUS);
  /* CONTEXT: configuration: filter display: checkbox: status */
  confuiMakeItemCheckButton (confui, &vbox, &sg, _("Status"),
      CONFUI_WIDGET_FILTER_STATUS, -1, val);
  confui->uiitem [CONFUI_WIDGET_FILTER_STATUS].outtype = CONFUI_OUT_CB;

  val = nlistGetNum (confui->filterDisplaySel, FILTER_DISP_FAVORITE);
  /* CONTEXT: configuration: filter display: checkbox: favorite selection */
  confuiMakeItemCheckButton (confui, &vbox, &sg, _("Favorite"),
      CONFUI_WIDGET_FILTER_FAVORITE, -1, val);
  confui->uiitem [CONFUI_WIDGET_FILTER_FAVORITE].outtype = CONFUI_OUT_CB;

  val = nlistGetNum (confui->filterDisplaySel, FILTER_DISP_STATUSPLAYABLE);
  /* CONTEXT: configuration: filter display: checkbox: status is playable */
  confuiMakeItemCheckButton (confui, &vbox, &sg, _("Playable Status"),
      CONFUI_SWITCH_FILTER_STATUS_PLAYABLE, -1, val);
  confui->uiitem [CONFUI_SWITCH_FILTER_STATUS_PLAYABLE].outtype = CONFUI_OUT_CB;
  logProcEnd (LOG_PROC, "confuiBuildUIFilterDisplay", "");
}

static void
confuiBuildUIOrganization (configui_t *confui)
{
  UIWidget      vbox;
  UIWidget      sg;

  logProcBegin (LOG_PROC, "confuiBuildUIOrganization");
  uiCreateVertBox (&vbox);

  /* organization */
  confuiMakeNotebookTab (&vbox, confui, &confui->notebook,
      /* CONTEXT: configuration: options associated with how audio files are organized */
      _("Organisation"), CONFUI_ID_ORGANIZATION);
  uiCreateSizeGroupHoriz (&sg);

  /* CONTEXT: configuration: the audio file organization path */
  confuiMakeItemCombobox (confui, &vbox, &sg, _("Organisation Path"),
      CONFUI_COMBOBOX_AO_PATHFMT, OPT_G_AO_PATHFMT,
      confuiOrgPathSelect, bdjoptGetStr (OPT_G_AO_PATHFMT));
  /* CONTEXT: configuration: examples displayed for the audio file organization path */
  confuiMakeItemLabelDisp (confui, &vbox, &sg, _("Examples"),
      CONFUI_WIDGET_AO_EXAMPLE_1, -1);
  confuiMakeItemLabelDisp (confui, &vbox, &sg, "",
      CONFUI_WIDGET_AO_EXAMPLE_2, -1);
  confuiMakeItemLabelDisp (confui, &vbox, &sg, "",
      CONFUI_WIDGET_AO_EXAMPLE_3, -1);
  confuiMakeItemLabelDisp (confui, &vbox, &sg, "",
      CONFUI_WIDGET_AO_EXAMPLE_4, -1);

  /* CONTEXT: configuration: checkbox: is automatic organization enabled */
  confuiMakeItemSwitch (confui, &vbox, &sg, _("Auto Organise"),
      CONFUI_SWITCH_AUTO_ORGANIZE, OPT_G_AUTOORGANIZE,
      bdjoptGetNum (OPT_G_AUTOORGANIZE), NULL);
  logProcEnd (LOG_PROC, "confuiBuildUIOrganization", "");
}

static void
confuiBuildUIEditDances (configui_t *confui)
{
  UIWidget      vbox;
  UIWidget      hbox;
  UIWidget      dvbox;
  UIWidget      sg;
  UIWidget      sgB;
  UIWidget      sgC;
  char          *bpmstr;
  char          tbuff [MAXPATHLEN];

  logProcBegin (LOG_PROC, "confuiBuildUIEditDances");
  confui->indancechange = true;
  uiCreateVertBox (&vbox);

  /* edit dances */
  confuiMakeNotebookTab (&vbox, confui, &confui->notebook,
      /* CONTEXT: configuration: edit the dance table */
      _("Edit Dances"), CONFUI_ID_DANCE);
  uiCreateSizeGroupHoriz (&sg);
  uiCreateSizeGroupHoriz (&sgB);
  uiCreateSizeGroupHoriz (&sgC);

  uiCreateHorizBox (&hbox);
  uiBoxPackStartExpand (&vbox, &hbox);

  confuiMakeItemTable (confui, &hbox, CONFUI_ID_DANCE, CONFUI_TABLE_NO_UP_DOWN);
  confui->tables [CONFUI_ID_DANCE].savefunc = confuiDanceSave;
  confuiCreateDanceTable (confui);
  g_signal_connect (confui->tables [CONFUI_ID_DANCE].tree, "row-activated",
      G_CALLBACK (confuiDanceSelect), confui);

  uiCreateVertBox (&dvbox);
  uiWidgetSetMarginStart (&dvbox, uiBaseMarginSz * 8);
  uiBoxPackStart (&hbox, &dvbox);

  /* CONTEXT: configuration: dances: the name of the dance */
  confuiMakeItemEntry (confui, &dvbox, &sg, _("Dance"),
      CONFUI_ENTRY_DANCE_DANCE, -1, "");
  uiEntrySetValidate (confui->uiitem [CONFUI_ENTRY_DANCE_DANCE].entry,
      confuiDanceEntryDanceChg, confui, UIENTRY_IMMEDIATE);
  confui->uiitem [CONFUI_ENTRY_DANCE_DANCE].danceidx = DANCE_DANCE;

  /* CONTEXT: configuration: dances: the type of the dance (club/latin/standard) */
  confuiMakeItemSpinboxText (confui, &dvbox, &sg, &sgB, _("Type"),
      CONFUI_SPINBOX_DANCE_TYPE, -1, CONFUI_OUT_NUM, 0,
      confuiDanceSpinboxTypeChg);
  confui->uiitem [CONFUI_SPINBOX_DANCE_TYPE].danceidx = DANCE_TYPE;

  /* CONTEXT: configuration: dances: the speed of the dance (fast/normal/slow) */
  confuiMakeItemSpinboxText (confui, &dvbox, &sg, &sgB, _("Speed"),
      CONFUI_SPINBOX_DANCE_SPEED, -1, CONFUI_OUT_NUM, 0,
      confuiDanceSpinboxSpeedChg);
  confui->uiitem [CONFUI_SPINBOX_DANCE_SPEED].danceidx = DANCE_SPEED;

  /* CONTEXT: configuration: dances: tags associated with the dance */
  confuiMakeItemEntry (confui, &dvbox, &sg, _("Tags"),
      CONFUI_ENTRY_DANCE_TAGS, -1, "");
  uiEntrySetValidate (confui->uiitem [CONFUI_ENTRY_DANCE_TAGS].entry,
      confuiDanceEntryTagsChg, confui, UIENTRY_IMMEDIATE);
  confui->uiitem [CONFUI_ENTRY_DANCE_TAGS].danceidx = DANCE_TAGS;

  /* CONTEXT: configuration: dances: play the selected announcement before the dance is played */
  confuiMakeItemEntryChooser (confui, &dvbox, &sg, _("Announcement"),
      CONFUI_ENTRY_DANCE_ANNOUNCEMENT, -1, "",
      confuiSelectAnnouncement);
  uiEntrySetValidate (confui->uiitem [CONFUI_ENTRY_DANCE_ANNOUNCEMENT].entry,
      confuiDanceEntryAnnouncementChg, confui, UIENTRY_DELAYED);
  confui->uiitem [CONFUI_ENTRY_DANCE_ANNOUNCEMENT].danceidx = DANCE_ANNOUNCE;

  bpmstr = tagdefs [TAG_BPM].displayname;
  /* CONTEXT: configuration: dances: low BPM (or MPM) setting */
  snprintf (tbuff, sizeof (tbuff), _("Low %s"), bpmstr);
  confuiMakeItemSpinboxNum (confui, &dvbox, &sg, &sgC, tbuff,
      CONFUI_SPINBOX_DANCE_LOW_BPM, -1, 10, 500, 0,
      confuiDanceSpinboxLowBPMChg);
  confui->uiitem [CONFUI_SPINBOX_DANCE_LOW_BPM].danceidx = DANCE_LOW_BPM;

  /* CONTEXT: configuration: dances: high BPM (or MPM) setting */
  snprintf (tbuff, sizeof (tbuff), _("High %s"), bpmstr);
  confuiMakeItemSpinboxNum (confui, &dvbox, &sg, &sgC, tbuff,
      CONFUI_SPINBOX_DANCE_HIGH_BPM, -1, 10, 500, 0,
      confuiDanceSpinboxHighBPMChg);
  confui->uiitem [CONFUI_SPINBOX_DANCE_HIGH_BPM].danceidx = DANCE_HIGH_BPM;

  /* CONTEXT: configuration: dances: time signature for the dance */
  confuiMakeItemSpinboxText (confui, &dvbox, &sg, &sgC, _("Time Signature"),
      CONFUI_SPINBOX_DANCE_TIME_SIG, -1, CONFUI_OUT_NUM, 0,
      confuiDanceSpinboxTimeSigChg);
  confui->uiitem [CONFUI_SPINBOX_DANCE_TIME_SIG].danceidx = DANCE_TIMESIG;

  confui->indancechange = false;
  logProcEnd (LOG_PROC, "confuiBuildUIEditDances", "");
}

static void
confuiBuildUIEditRatings (configui_t *confui)
{
  UIWidget      vbox;
  UIWidget      hbox;
  UIWidget      uiwidget;
  UIWidget      sg;

  logProcBegin (LOG_PROC, "confuiBuildUIEditRatings");
  uiCreateVertBox (&vbox);

  /* edit ratings */
  confuiMakeNotebookTab (&vbox, confui, &confui->notebook,
      /* CONTEXT: configuration: edit the dance ratings table */
      _("Edit Ratings"), CONFUI_ID_RATINGS);
  uiCreateSizeGroupHoriz (&sg);

  /* CONTEXT: configuration: dance ratings: information on how to order the ratings */
  uiCreateLabel (&uiwidget, _("Order from the lowest rating to the highest rating."));
  uiBoxPackStart (&vbox, &uiwidget);

  /* CONTEXT: configuration: dance ratings: information on how to edit a rating entry */
  uiCreateLabel (&uiwidget, _("Double click on a field to edit."));
  uiBoxPackStart (&vbox, &uiwidget);

  uiCreateHorizBox (&hbox);
  uiBoxPackStartExpand (&vbox, &hbox);

  confuiMakeItemTable (confui, &hbox, CONFUI_ID_RATINGS, CONFUI_TABLE_KEEP_FIRST);
  confui->tables [CONFUI_ID_RATINGS].listcreatefunc = confuiRatingListCreate;
  confui->tables [CONFUI_ID_RATINGS].savefunc = confuiRatingSave;
  confuiCreateRatingTable (confui);
  logProcEnd (LOG_PROC, "confuiBuildUIEditRatings", "");
}

static void
confuiBuildUIEditStatus (configui_t *confui)
{
  UIWidget      vbox;
  UIWidget      hbox;
  UIWidget      uiwidget;
  UIWidget      sg;

  logProcBegin (LOG_PROC, "confuiBuildUIEditStatus");
  uiCreateVertBox (&vbox);

  /* edit status */
  confuiMakeNotebookTab (&vbox, confui, &confui->notebook,
      /* CONTEXT: configuration: edit status table */
      _("Edit Status"), CONFUI_ID_STATUS);
  uiCreateSizeGroupHoriz (&sg);

  /* CONTEXT: configuration: status: information on how to edit a status entry */
  uiCreateLabel (&uiwidget, _("Double click on a field to edit."));
  uiBoxPackStart (&vbox, &uiwidget);

  uiCreateHorizBox (&hbox);
  uiBoxPackStartExpand (&vbox, &hbox);

  confuiMakeItemTable (confui, &hbox, CONFUI_ID_STATUS,
      CONFUI_TABLE_KEEP_FIRST | CONFUI_TABLE_KEEP_LAST);
  confui->tables [CONFUI_ID_STATUS].togglecol = CONFUI_STATUS_COL_PLAY_FLAG;
  confui->tables [CONFUI_ID_STATUS].listcreatefunc = confuiStatusListCreate;
  confui->tables [CONFUI_ID_STATUS].savefunc = confuiStatusSave;
  confuiCreateStatusTable (confui);
  logProcEnd (LOG_PROC, "confuiBuildUIEditStatus", "");
}

static void
confuiBuildUIEditLevels (configui_t *confui)
{
  UIWidget      vbox;
  UIWidget      hbox;
  UIWidget      uiwidget;
  UIWidget      sg;

  logProcBegin (LOG_PROC, "confuiBuildUIEditLevels");
  uiCreateVertBox (&vbox);

  /* edit levels */
  confuiMakeNotebookTab (&vbox, confui, &confui->notebook,
      /* CONTEXT: configuration: edit dance levels table */
      _("Edit Levels"), CONFUI_ID_LEVELS);
  uiCreateSizeGroupHoriz (&sg);

  /* CONTEXT: configuration: dance levels: instructions */
  uiCreateLabel (&uiwidget, _("Order from easiest to most advanced."));
  uiBoxPackStart (&vbox, &uiwidget);

  /* CONTEXT: configuration: dance levels: information on how to edit a level entry */
  uiCreateLabel (&uiwidget, _("Double click on a field to edit."));
  uiBoxPackStart (&vbox, &uiwidget);

  uiCreateHorizBox (&hbox);
  uiBoxPackStartExpand (&vbox, &hbox);

  confuiMakeItemTable (confui, &hbox, CONFUI_ID_LEVELS, CONFUI_TABLE_NONE);
  confui->tables [CONFUI_ID_LEVELS].togglecol = CONFUI_LEVEL_COL_DEFAULT;
  confui->tables [CONFUI_ID_LEVELS].listcreatefunc = confuiLevelListCreate;
  confui->tables [CONFUI_ID_LEVELS].savefunc = confuiLevelSave;
  confuiCreateLevelTable (confui);
  logProcEnd (LOG_PROC, "confuiBuildUIEditLevels", "");
}

static void
confuiBuildUIEditGenres (configui_t *confui)
{
  UIWidget      vbox;
  UIWidget      hbox;
  UIWidget      uiwidget;
  UIWidget      sg;

  logProcBegin (LOG_PROC, "confuiBuildUIEditGenres");
  uiCreateVertBox (&vbox);

  /* edit genres */
  confuiMakeNotebookTab (&vbox, confui, &confui->notebook,
      /* CONTEXT: configuration: edit genres table */
      _("Edit Genres"), CONFUI_ID_GENRES);
  uiCreateSizeGroupHoriz (&sg);

  /* CONTEXT: configuration: genres: information on how to edit a genre entry */
  uiCreateLabel (&uiwidget, _("Double click on a field to edit."));
  uiBoxPackStart (&vbox, &uiwidget);

  uiCreateHorizBox (&hbox);
  uiBoxPackStartExpand (&vbox, &hbox);

  confuiMakeItemTable (confui, &hbox, CONFUI_ID_GENRES, CONFUI_TABLE_NONE);
  confui->tables [CONFUI_ID_GENRES].togglecol = CONFUI_GENRE_COL_CLASSICAL;
  confui->tables [CONFUI_ID_GENRES].listcreatefunc = confuiGenreListCreate;
  confui->tables [CONFUI_ID_GENRES].savefunc = confuiGenreSave;
  confuiCreateGenreTable (confui);
  logProcEnd (LOG_PROC, "confuiBuildUIEditGenres", "");
}

static void
confuiBuildUIMobileRemoteControl (configui_t *confui)
{
  UIWidget      vbox;
  UIWidget      sg;

  logProcBegin (LOG_PROC, "confuiBuildUIMobileRemoteControl");
  uiCreateVertBox (&vbox);

  /* mobile remote control */
  confuiMakeNotebookTab (&vbox, confui, &confui->notebook,
      /* CONTEXT: configuration: options associated with mobile remote control */
      _("Mobile Remote Control"), CONFUI_ID_REM_CONTROL);
  uiCreateSizeGroupHoriz (&sg);

  /* CONTEXT: configuration: remote control: checkbox: enable/disable */
  confuiMakeItemSwitch (confui, &vbox, &sg, _("Enable Remote Control"),
      CONFUI_SWITCH_RC_ENABLE, OPT_P_REMOTECONTROL,
      bdjoptGetNum (OPT_P_REMOTECONTROL),
      confuiRemctrlChg);

  /* CONTEXT: configuration: remote control: the HTML template to use */
  confuiMakeItemSpinboxText (confui, &vbox, &sg, NULL, _("HTML Template"),
      CONFUI_SPINBOX_RC_HTML_TEMPLATE, OPT_G_REMCONTROLHTML,
      CONFUI_OUT_STR, confui->uiitem [CONFUI_SPINBOX_RC_HTML_TEMPLATE].listidx, NULL);

  /* CONTEXT: configuration: remote control: the user ID for sign-on to remote control */
  confuiMakeItemEntry (confui, &vbox, &sg, _("User ID"),
      CONFUI_ENTRY_RC_USER_ID,  OPT_P_REMCONTROLUSER,
      bdjoptGetStr (OPT_P_REMCONTROLUSER));

  /* CONTEXT: configuration: remote control: the password for sign-on to remote control */
  confuiMakeItemEntry (confui, &vbox, &sg, _("Password"),
      CONFUI_ENTRY_RC_PASS, OPT_P_REMCONTROLPASS,
      bdjoptGetStr (OPT_P_REMCONTROLPASS));

  /* CONTEXT: configuration: remote control: the port to use for remote control */
  confuiMakeItemSpinboxNum (confui, &vbox, &sg, NULL, _("Port"),
      CONFUI_WIDGET_RC_PORT, OPT_P_REMCONTROLPORT,
      8000, 30000, bdjoptGetNum (OPT_P_REMCONTROLPORT),
      confuiRemctrlPortChg);

  /* CONTEXT: configuration: remote control: the link to display the QR code for remote control */
  confuiMakeItemLink (confui, &vbox, &sg, _("QR Code"),
      CONFUI_WIDGET_RC_QR_CODE, "");
  logProcEnd (LOG_PROC, "confuiBuildUIMobileRemoteControl", "");
}

static void
confuiBuildUIMobileMarquee (configui_t *confui)
{
  UIWidget      vbox;
  UIWidget      sg;

  logProcBegin (LOG_PROC, "confuiBuildUIMobileMarquee");
  uiCreateVertBox (&vbox);

  /* mobile marquee */
  confuiMakeNotebookTab (&vbox, confui, &confui->notebook,
      /* CONTEXT: configuration: options associated with the mobile marquee */
      _("Mobile Marquee"), CONFUI_ID_MOBILE_MQ);
  uiCreateSizeGroupHoriz (&sg);

  /* CONTEXT: configuration: set mobile marquee mode (off/local/internet) */
  confuiMakeItemSpinboxText (confui, &vbox, &sg, NULL, _("Mobile Marquee"),
      CONFUI_SPINBOX_MOBILE_MQ, OPT_P_MOBILEMARQUEE,
      CONFUI_OUT_NUM, bdjoptGetNum (OPT_P_MOBILEMARQUEE),
      confuiMobmqTypeChg);

  /* CONTEXT: configuration: the port to use for the mobile marquee */
  confuiMakeItemSpinboxNum (confui, &vbox, &sg, NULL, _("Port"),
      CONFUI_WIDGET_MMQ_PORT, OPT_P_MOBILEMQPORT,
      8000, 30000, bdjoptGetNum (OPT_P_MOBILEMQPORT),
      confuiMobmqPortChg);

  /* CONTEXT: configuration: the name to use for the mobile marquee internet mode */
  confuiMakeItemEntry (confui, &vbox, &sg, _("Name"),
      CONFUI_ENTRY_MM_NAME, OPT_P_MOBILEMQTAG,
      bdjoptGetStr (OPT_P_MOBILEMQTAG));
  uiEntrySetValidate (confui->uiitem [CONFUI_ENTRY_MM_NAME].entry,
      confuiMobmqNameChg, confui, UIENTRY_IMMEDIATE);

  /* CONTEXT: configuration: the title to display on the mobile marquee */
  confuiMakeItemEntry (confui, &vbox, &sg, _("Title"),
      CONFUI_ENTRY_MM_TITLE, OPT_P_MOBILEMQTITLE,
      bdjoptGetStr (OPT_P_MOBILEMQTITLE));
  uiEntrySetValidate (confui->uiitem [CONFUI_ENTRY_MM_TITLE].entry,
      confuiMobmqTitleChg, confui, UIENTRY_IMMEDIATE);

  /* CONTEXT: configuration: mobile marquee: the link to display the QR code for the mobile marquee */
  confuiMakeItemLink (confui, &vbox, &sg, _("QR Code"),
      CONFUI_WIDGET_MMQ_QR_CODE, "");
  logProcEnd (LOG_PROC, "confuiBuildUIMobileMarquee", "");
}

static void
confuiBuildUIDebugOptions (configui_t *confui)
{
  UIWidget      vbox;
  UIWidget      hbox;
  UIWidget      sg;
  nlistidx_t    val;

  logProcBegin (LOG_PROC, "confuiBuildUIDebugOptions");
  uiCreateVertBox (&vbox);

  /* debug options */
  confuiMakeNotebookTab (&vbox, confui, &confui->notebook,
      /* CONTEXT: configuration: debug options that can be turned on and off */
      _("Debug Options"), CONFUI_ID_NONE);
  uiCreateSizeGroupHoriz (&sg);

  uiCreateHorizBox (&hbox);
  uiBoxPackStart (&vbox, &hbox);

  uiCreateVertBox (&vbox);
  uiBoxPackStart (&hbox, &vbox);

  val = bdjoptGetNum (OPT_G_DEBUGLVL);
  confuiMakeItemCheckButton (confui, &vbox, &sg, "Important",
      CONFUI_WIDGET_DEBUG_1, -1,
      (val & 1));
  confui->uiitem [CONFUI_WIDGET_DEBUG_1].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (confui, &vbox, &sg, "Basic",
      CONFUI_WIDGET_DEBUG_2, -1,
      (val & 2));
  confui->uiitem [CONFUI_WIDGET_DEBUG_2].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (confui, &vbox, &sg, "Messages",
      CONFUI_WIDGET_DEBUG_4, -1,
      (val & 4));
  confui->uiitem [CONFUI_WIDGET_DEBUG_4].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (confui, &vbox, &sg, "Main",
      CONFUI_WIDGET_DEBUG_8, -1,
      (val & 8));
  confui->uiitem [CONFUI_WIDGET_DEBUG_8].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (confui, &vbox, &sg, "List",
      CONFUI_WIDGET_DEBUG_16, -1,
      (val & 16));
  confui->uiitem [CONFUI_WIDGET_DEBUG_16].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (confui, &vbox, &sg, "Song Selection",
      CONFUI_WIDGET_DEBUG_32, -1,
      (val & 32));
  confui->uiitem [CONFUI_WIDGET_DEBUG_32].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (confui, &vbox, &sg, "Dance Selection",
      CONFUI_WIDGET_DEBUG_64, -1,
      (val & 64));
  confui->uiitem [CONFUI_WIDGET_DEBUG_64].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (confui, &vbox, &sg, "Volume",
      CONFUI_WIDGET_DEBUG_128, -1,
      (val & 128));
  confui->uiitem [CONFUI_WIDGET_DEBUG_128].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (confui, &vbox, &sg, "Socket",
      CONFUI_WIDGET_DEBUG_256, -1,
      (val & 256));
  confui->uiitem [CONFUI_WIDGET_DEBUG_256].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (confui, &vbox, &sg, "Database",
      CONFUI_WIDGET_DEBUG_512, -1,
      (val & 512));

  uiCreateVertBox (&vbox);
  uiBoxPackStart (&hbox, &vbox);

  uiCreateSizeGroupHoriz (&sg);

  confui->uiitem [CONFUI_WIDGET_DEBUG_512].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (confui, &vbox, &sg, "Random Access File",
      CONFUI_WIDGET_DEBUG_1024, -1,
      (val & 1024));
  confui->uiitem [CONFUI_WIDGET_DEBUG_1024].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (confui, &vbox, &sg, "Procedures",
      CONFUI_WIDGET_DEBUG_2048, -1,
      (val & 2048));
  confui->uiitem [CONFUI_WIDGET_DEBUG_2048].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (confui, &vbox, &sg, "Player",
      CONFUI_WIDGET_DEBUG_4096, -1,
      (val & 4096));
  confui->uiitem [CONFUI_WIDGET_DEBUG_4096].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (confui, &vbox, &sg, "Datafile",
      CONFUI_WIDGET_DEBUG_8192, -1,
      (val & 8192));
  confui->uiitem [CONFUI_WIDGET_DEBUG_8192].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (confui, &vbox, &sg, "Process",
      CONFUI_WIDGET_DEBUG_16384, -1,
      (val & 16384));
  confui->uiitem [CONFUI_WIDGET_DEBUG_16384].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (confui, &vbox, &sg, "Web Server",
      CONFUI_WIDGET_DEBUG_32768, -1,
      (val & 32768));
  confui->uiitem [CONFUI_WIDGET_DEBUG_32768].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (confui, &vbox, &sg, "Web Client",
      CONFUI_WIDGET_DEBUG_65536, -1,
      (val & 65536));
  confui->uiitem [CONFUI_WIDGET_DEBUG_65536].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (confui, &vbox, &sg, "Database Update",
      CONFUI_WIDGET_DEBUG_131072, -1,
      (val & 131072));
  confui->uiitem [CONFUI_WIDGET_DEBUG_131072].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (confui, &vbox, &sg, "Program State",
      CONFUI_WIDGET_DEBUG_262144, -1,
      (val & 262144));
  confui->uiitem [CONFUI_WIDGET_DEBUG_262144].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (confui, &vbox, &sg, "Actions",
      CONFUI_WIDGET_DEBUG_524288, -1,
      (val & 524288));
  confui->uiitem [CONFUI_WIDGET_DEBUG_524288].outtype = CONFUI_OUT_DEBUG;
  logProcEnd (LOG_PROC, "confuiBuildUIDebugOptions", "");
}


static int
confuiMainLoop (void *tconfui)
{
  configui_t    *confui = tconfui;
  int           stop = SOCKH_CONTINUE;

  if (! stop) {
    uiUIProcessEvents ();
  }

  if (! progstateIsRunning (confui->progstate)) {
    progstateProcess (confui->progstate);
    if (progstateCurrState (confui->progstate) == STATE_CLOSED) {
      stop = SOCKH_STOP;
    }
    if (gKillReceived) {
      logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
      progstateShutdownProcess (confui->progstate);
      gKillReceived = false;
    }
    return stop;
  }

  connProcessUnconnected (confui->conn);

  for (int i = CONFUI_COMBOBOX_MAX + 1; i < CONFUI_ENTRY_MAX; ++i) {
    uiEntryValidate (confui->uiitem [i].entry, false);
  }

  if (gKillReceived) {
    logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
    progstateShutdownProcess (confui->progstate);
    gKillReceived = false;
  }
  return stop;
}

static bool
confuiHandshakeCallback (void *udata, programstate_t programState)
{
  configui_t   *confui = udata;

  logProcBegin (LOG_PROC, "confuiHandshakeCallback");
  connProcessUnconnected (confui->conn);

  progstateLogTime (confui->progstate, "time-to-start-gui");
  logProcEnd (LOG_PROC, "confuiHandshakeCallback", "");
  return STATE_FINISHED;
}

static int
confuiProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
    bdjmsgmsg_t msg, char *args, void *udata)
{
  configui_t       *confui = udata;

  logProcBegin (LOG_PROC, "confuiProcessMsg");

  logMsg (LOG_DBG, LOG_MSGS, "got: from:%d/%s route:%d/%s msg:%d/%s args:%s",
      routefrom, msgRouteDebugText (routefrom),
      route, msgRouteDebugText (route), msg, msgDebugText (msg), args);

  switch (route) {
    case ROUTE_NONE:
    case ROUTE_CONFIGUI: {
      switch (msg) {
        case MSG_HANDSHAKE: {
          connProcessHandshake (confui->conn, routefrom);
          break;
        }
        case MSG_SOCKET_CLOSE: {
          connDisconnect (confui->conn, routefrom);
          break;
        }
        case MSG_EXIT_REQUEST: {
          logMsg (LOG_SESS, LOG_IMPORTANT, "got exit request");
          gKillReceived = false;
          progstateShutdownProcess (confui->progstate);
          break;
        }
        default: {
          break;
        }
      }
      break;
    }
    default: {
      break;
    }
  }

  if (gKillReceived) {
    logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
  }
  logProcEnd (LOG_PROC, "confuiProcessMsg", "");
  return gKillReceived;
}


static bool
confuiCloseWin (void *udata)
{
  configui_t   *confui = udata;

  logProcBegin (LOG_PROC, "confuiCloseWin");
  if (progstateCurrState (confui->progstate) <= STATE_RUNNING) {
    progstateShutdownProcess (confui->progstate);
    logMsg (LOG_DBG, LOG_MSGS, "got: close win request");
    logProcEnd (LOG_PROC, "confuiCloseWin", "not-done");
    return UICB_STOP;
  }

  logProcEnd (LOG_PROC, "confuiCloseWin", "");
  return UICB_STOP;
}

static void
confuiSigHandler (int sig)
{
  gKillReceived = true;
}

static void
confuiPopulateOptions (configui_t *confui)
{
  const char  *sval;
  ssize_t     nval;
  nlistidx_t  selidx;
  double      dval;
  confuibasetype_t basetype;
  confuiouttype_t outtype;
  char        tbuff [MAXPATHLEN];
  GdkRGBA     gcolor;
  long        debug = 0;
  bool        accentcolorchanged = false;
  bool        profilecolorchanged = false;
  bool        localechanged = false;
  bool        themechanged = false;

  logProcBegin (LOG_PROC, "confuiPopulateOptions");
  for (int i = 0; i < CONFUI_ITEM_MAX; ++i) {
    sval = "fail";
    nval = -1;
    dval = -1.0;

    basetype = confui->uiitem [i].basetype;
    outtype = confui->uiitem [i].outtype;

    switch (basetype) {
      case CONFUI_NONE: {
        break;
      }
      case CONFUI_ENTRY: {
        sval = uiEntryGetValue (confui->uiitem [i].entry);
        break;
      }
      case CONFUI_SPINBOX_TEXT: {
        nval = uiSpinboxTextGetValue (confui->uiitem [i].spinbox);
        if (outtype == CONFUI_OUT_STR) {
          if (confui->uiitem [i].sbkeylist != NULL) {
            sval = nlistGetStr (confui->uiitem [i].sbkeylist, nval);
          } else {
            sval = nlistGetStr (confui->uiitem [i].displist, nval);
          }
        }
        break;
      }
      case CONFUI_SPINBOX_NUM: {
        nval = (ssize_t) uiSpinboxGetValue (&confui->uiitem [i].uiwidget);
        break;
      }
      case CONFUI_SPINBOX_DOUBLE: {
        dval = uiSpinboxGetValue (&confui->uiitem [i].uiwidget);
        nval = (ssize_t) (dval * 1000.0);
        outtype = CONFUI_OUT_NUM;
        break;
      }
      case CONFUI_SPINBOX_TIME: {
        nval = (ssize_t) uiSpinboxTimeGetValue (confui->uiitem [i].spinbox);
        break;
      }
      case CONFUI_COLOR: {
        gtk_color_chooser_get_rgba (
            GTK_COLOR_CHOOSER (confui->uiitem [i].widget), &gcolor);
        snprintf (tbuff, sizeof (tbuff), "#%02x%02x%02x",
            (int) round (gcolor.red * 255.0),
            (int) round (gcolor.green * 255.0),
            (int) round (gcolor.blue * 255.0));
        sval = tbuff;
        break;
      }
      case CONFUI_FONT: {
        sval = gtk_font_chooser_get_font (
            GTK_FONT_CHOOSER (confui->uiitem [i].widget));
        break;
      }
      case CONFUI_SWITCH: {
        nval = uiSwitchGetValue (confui->uiitem [i].uiswitch);
        break;
      }
      case CONFUI_CHECK_BUTTON: {
        nval = uiToggleButtonIsActive (&confui->uiitem [i].uiwidget);
        break;
      }
      case CONFUI_COMBOBOX: {
        sval = slistGetDataByIdx (confui->uiitem [i].displist,
            confui->uiitem [i].listidx);
        outtype = CONFUI_OUT_STR;
        break;
      }
    }

    if (i == CONFUI_SPINBOX_AUDIO_OUTPUT) {
      uispinbox_t  *spinbox;

      spinbox = confui->uiitem [i].spinbox;
      if (! uiSpinboxIsChanged (spinbox)) {
        continue;
      }
    }

    switch (outtype) {
      case CONFUI_OUT_NONE: {
        break;
      }
      case CONFUI_OUT_STR: {
        if (i == CONFUI_SPINBOX_LOCALE) {
          if (strcmp (sysvarsGetStr (SV_LOCALE), sval) != 0) {
            localechanged = true;
          }
        }
        if (i == CONFUI_WIDGET_UI_ACCENT_COLOR) {
          if (strcmp (bdjoptGetStr (confui->uiitem [i].bdjoptIdx), sval) != 0) {
            accentcolorchanged = true;
          }
        }
        if (i == CONFUI_WIDGET_UI_PROFILE_COLOR) {
          if (strcmp (bdjoptGetStr (confui->uiitem [i].bdjoptIdx), sval) != 0) {
            profilecolorchanged = true;
          }
        }
        if (i == CONFUI_SPINBOX_UI_THEME) {
          if (strcmp (bdjoptGetStr (confui->uiitem [i].bdjoptIdx), sval) != 0) {
            themechanged = true;
          }
        }
        bdjoptSetStr (confui->uiitem [i].bdjoptIdx, sval);
        break;
      }
      case CONFUI_OUT_NUM: {
        bdjoptSetNum (confui->uiitem [i].bdjoptIdx, nval);
        break;
      }
      case CONFUI_OUT_DOUBLE: {
        bdjoptSetNum (confui->uiitem [i].bdjoptIdx, dval);
        break;
      }
      case CONFUI_OUT_BOOL: {
        bdjoptSetNum (confui->uiitem [i].bdjoptIdx, nval);
        break;
      }
      case CONFUI_OUT_CB: {
        nlistSetNum (confui->filterDisplaySel,
            nlistGetNum (confui->filterLookup, i), nval);
        break;
      }
      case CONFUI_OUT_DEBUG: {
        if (nval) {
          long  idx;
          long  bitval;

          idx = i - CONFUI_WIDGET_DEBUG_1;
          bitval = 1 << idx;
          debug |= bitval;
        }
        break;
      }
    } /* out type */

    if (i == CONFUI_SPINBOX_LOCALE &&
        localechanged) {
      sysvarsSetStr (SV_LOCALE, sval);
      snprintf (tbuff, sizeof (tbuff), "%.2s", sval);
      sysvarsSetStr (SV_LOCALE_SHORT, tbuff);
      pathbldMakePath (tbuff, sizeof (tbuff),
          "locale", BDJ4_CONFIG_EXT, PATHBLD_MP_DATA);
      fileopDelete (tbuff);

      /* if the set locale does not match the system or default locale */
      /* save it in the locale file */
      if (strcmp (sval, sysvarsGetStr (SV_LOCALE_SYSTEM)) != 0 &&
          strcmp (sval, "en_GB") != 0) {
        FILE    *fh;

        fh = fopen (tbuff, "w");
        fprintf (fh, "%s\n", sval);
        fclose (fh);
      }
    }

    if (i == CONFUI_ENTRY_MUSIC_DIR) {
      strlcpy (tbuff, bdjoptGetStr (confui->uiitem [i].bdjoptIdx), sizeof (tbuff));
      pathNormPath (tbuff, sizeof (tbuff));
      bdjoptSetStr (confui->uiitem [i].bdjoptIdx, tbuff);
    }

    if (i == CONFUI_SPINBOX_UI_THEME && themechanged) {
      FILE    *fh;

      sval = bdjoptGetStr (confui->uiitem [i].bdjoptIdx);
      pathbldMakePath (tbuff, sizeof (tbuff),
          "theme", BDJ4_CONFIG_EXT, PATHBLD_MP_DATA);
      /* if the theme name is the same as the current default theme */
      /* don't write out the theme file.  want to use the default */
      if (strcmp (sval, sysvarsGetStr (SV_THEME_DEFAULT)) != 0) {
        fh = fopen (tbuff, "w");
        if (sval != NULL) {
          fprintf (fh, "%s\n", sval);
        }
        fclose (fh);
      } else {
        fileopDelete (tbuff);
      }
    }

    if (i == CONFUI_WIDGET_UI_ACCENT_COLOR &&
        accentcolorchanged) {
      templateImageCopy (sval);
    }

    if (i == CONFUI_SPINBOX_RC_HTML_TEMPLATE) {
      sval = bdjoptGetStr (confui->uiitem [i].bdjoptIdx);
      templateHttpCopy (sval, "bdj4remote.html");
    }
  } /* for each item */

  selidx = uiSpinboxTextGetValue (
      confui->uiitem [CONFUI_SPINBOX_DISP_SEL].spinbox);
  confuiDispSaveTable (confui, selidx);

  bdjoptSetNum (OPT_G_DEBUGLVL, debug);
  logProcEnd (LOG_PROC, "confuiPopulateOptions", "");
}


static bool
confuiSelectMusicDir (void *udata)
{
  configui_t  *confui = udata;
  char        *fn = NULL;
  uiselect_t  *selectdata;

  logProcBegin (LOG_PROC, "confuiSelectMusicDir");
  selectdata = uiDialogCreateSelect (&confui->window,
      /* CONTEXT: configuration: folder selection dialog: window title */
      _("Select Music Folder Location"),
      bdjoptGetStr (OPT_M_DIR_MUSIC), NULL, NULL, NULL);
  fn = uiSelectDirDialog (selectdata);
  if (fn != NULL) {
    uiEntrySetValue (confui->uiitem [CONFUI_ENTRY_MUSIC_DIR].entry, fn);
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "selected loc: %s", fn);
    free (fn);
  }
  free (selectdata);
  logProcEnd (LOG_PROC, "confuiSelectMusicDir", "");
  return UICB_CONT;
}

static bool
confuiSelectStartup (void *udata)
{
  configui_t  *confui = udata;

  logProcBegin (LOG_PROC, "confuiSelectStartup");
  confuiSelectFileDialog (confui, CONFUI_ENTRY_STARTUP,
      sysvarsGetStr (SV_BDJ4DATATOPDIR), NULL, NULL);
  logProcEnd (LOG_PROC, "confuiSelectStartup", "");
  return UICB_CONT;
}

static bool
confuiSelectShutdown (void *udata)
{
  configui_t *confui = udata;

  logProcBegin (LOG_PROC, "confuiSelectShutdown");
  confuiSelectFileDialog (confui, CONFUI_ENTRY_SHUTDOWN,
      sysvarsGetStr (SV_BDJ4DATATOPDIR), NULL, NULL);
  logProcEnd (LOG_PROC, "confuiSelectShutdown", "");
  return UICB_CONT;
}

static bool
confuiSelectAnnouncement (void *udata)
{
  configui_t *confui = udata;

  logProcBegin (LOG_PROC, "confuiSelectAnnouncement");
  confuiSelectFileDialog (confui, CONFUI_ENTRY_DANCE_ANNOUNCEMENT,
      /* CONTEXT: configuration: announcement selection dialog: audio file filter */
      bdjoptGetStr (OPT_M_DIR_MUSIC), _("Audio Files"), "audio/*");
  logProcEnd (LOG_PROC, "confuiSelectAnnouncement", "");
  return UICB_CONT;
}

static void
confuiSelectFileDialog (configui_t *confui, int widx, char *startpath,
    char *mimefiltername, char *mimetype)
{
  char        *fn = NULL;
  uiselect_t  *selectdata;

  logProcBegin (LOG_PROC, "confuiSelectFileDialog");
  selectdata = uiDialogCreateSelect (&confui->window,
      /* CONTEXT: configuration: file selection dialog: window title */
      _("Select File"), startpath, NULL, mimefiltername, mimetype);
  fn = uiSelectFileDialog (selectdata);
  if (fn != NULL) {
    uiEntrySetValue (confui->uiitem [widx].entry, fn);
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "selected loc: %s", fn);
    free (fn);
  }
  free (selectdata);
  logProcEnd (LOG_PROC, "confuiSelectFileDialog", "");
}

static void
confuiMakeNotebookTab (UIWidget *boxp, configui_t *confui, UIWidget *nb, char *txt, int id)
{
  UIWidget    uiwidget;

  logProcBegin (LOG_PROC, "confuiMakeNotebookTab");
  uiCreateLabel (&uiwidget, txt);
  uiWidgetSetAllMargins (&uiwidget, 0);
  uiWidgetExpandHoriz (boxp);
  uiWidgetExpandVert (boxp);
  uiWidgetSetAllMargins (boxp, uiBaseMarginSz * 2);
  uiNotebookAppendPage (nb, boxp, &uiwidget);
  uiutilsNotebookIDAdd (confui->nbtabid, id);

  logProcEnd (LOG_PROC, "confuiMakeNotebookTab", "");
}

static void
confuiMakeItemEntry (configui_t *confui, UIWidget *boxp, UIWidget *sg,
    char *txt, int widx, int bdjoptIdx, char *disp)
{
  UIWidget    hbox;

  logProcBegin (LOG_PROC, "confuiMakeItemEntry");
  uiCreateHorizBox (&hbox);
  confuiMakeItemEntryBasic (confui, &hbox, sg, txt, widx, bdjoptIdx, disp);
  uiBoxPackStart (boxp, &hbox);
  logProcEnd (LOG_PROC, "confuiMakeItemEntry", "");
}

static void
confuiMakeItemEntryChooser (configui_t *confui, UIWidget *boxp,
    UIWidget *sg, char *txt, int widx, int bdjoptIdx,
    char *disp, void *dialogFunc)
{
  UIWidget    hbox;
  UIWidget    uiwidget;

  logProcBegin (LOG_PROC, "confuiMakeItemEntryChooser");
  uiCreateHorizBox (&hbox);
  confuiMakeItemEntryBasic (confui, &hbox, sg, txt, widx, bdjoptIdx, disp);
  uiutilsUICallbackInit (&confui->uiitem [widx].callback,
      dialogFunc, confui);
  uiCreateButton (&uiwidget, &confui->uiitem [widx].callback,
      "", NULL);
  uiButtonSetImageIcon (&uiwidget, "folder");
  uiWidgetSetMarginStart (&uiwidget, 0);
  uiBoxPackStart (&hbox, &uiwidget);
  uiBoxPackStart (boxp, &hbox);
  logProcEnd (LOG_PROC, "confuiMakeItemEntryChooser", "");
}

static void
confuiMakeItemEntryBasic (configui_t *confui, UIWidget *boxp, UIWidget *sg,
    char *txt, int widx, int bdjoptIdx, char *disp)
{
  UIWidget  *uiwidgetp;

  confui->uiitem [widx].basetype = CONFUI_ENTRY;
  confui->uiitem [widx].outtype = CONFUI_OUT_STR;
  confuiMakeItemLabel (boxp, sg, txt);
  uiEntryCreate (confui->uiitem [widx].entry);
  uiwidgetp = uiEntryGetUIWidget (confui->uiitem [widx].entry);
  uiutilsUIWidgetCopy (&confui->uiitem [widx].uiwidget, uiwidgetp);
  uiWidgetSetMarginStart (uiwidgetp, uiBaseMarginSz * 4);
  uiBoxPackStart (boxp, uiwidgetp);
  if (disp != NULL) {
    uiEntrySetValue (confui->uiitem [widx].entry, disp);
  } else {
    uiEntrySetValue (confui->uiitem [widx].entry, "");
  }
  confui->uiitem [widx].bdjoptIdx = bdjoptIdx;
}

static void
confuiMakeItemCombobox (configui_t *confui, UIWidget *boxp, UIWidget *sg,
    char *txt, int widx, int bdjoptIdx, UILongCallbackFunc ddcb, char *value)
{
  UIWidget    hbox;
  UIWidget    uiwidget;
  UIWidget    *uiwidgetp;

  logProcBegin (LOG_PROC, "confuiMakeItemCombobox");
  confui->uiitem [widx].basetype = CONFUI_COMBOBOX;
  confui->uiitem [widx].outtype = CONFUI_OUT_STR;

  uiCreateHorizBox (&hbox);
  confuiMakeItemLabel (&hbox, sg, txt);

  uiutilsUICallbackLongInit (&confui->uiitem [widx].callback, ddcb, confui);
  uiwidgetp = uiComboboxCreate (&confui->window, txt,
      &confui->uiitem [widx].callback,
      confui->uiitem [widx].dropdown, confui);

  uiDropDownSetList (confui->uiitem [widx].dropdown,
      confui->uiitem [widx].displist, NULL);
  uiDropDownSelectionSetStr (confui->uiitem [widx].dropdown, value);
  uiWidgetSetMarginStart (uiwidgetp, uiBaseMarginSz * 4);
  uiBoxPackStart (&hbox, uiwidgetp);
  uiBoxPackStart (boxp, &hbox);

  uiutilsUIWidgetCopy (&confui->uiitem [widx].uiwidget, &uiwidget);
  confui->uiitem [widx].bdjoptIdx = bdjoptIdx;
  logProcEnd (LOG_PROC, "confuiMakeItemCombobox", "");
}

static void
confuiMakeItemLink (configui_t *confui, UIWidget *boxp, UIWidget *sg,
    char *txt, int widx, char *disp)
{
  UIWidget    hbox;
  UIWidget    uiwidget;

  logProcBegin (LOG_PROC, "confuiMakeItemLink");
  uiCreateHorizBox (&hbox);
  confuiMakeItemLabel (&hbox, sg, txt);
  uiutilsUIWidgetInit (&uiwidget);
  uiCreateLink (&uiwidget, disp, NULL);
  if (isMacOS ()) {
    uiutilsUICallbackInit (&confui->uiitem [widx].callback,
        confuiLinkCallback, confui);
    confui->uiitem [widx].uri = NULL;
    uiLinkSetActivateCallback (&uiwidget, &confui->uiitem [widx].callback);
  }
  uiBoxPackStart (&hbox, &uiwidget);
  uiBoxPackStart (boxp, &hbox);
  uiutilsUIWidgetCopy (&confui->uiitem [widx].uiwidget, &uiwidget);
  logProcEnd (LOG_PROC, "confuiMakeItemLink", "");
}

static void
confuiMakeItemFontButton (configui_t *confui, UIWidget *boxp, UIWidget *sg,
    char *txt, int widx, int bdjoptIdx, char *fontname)
{
  UIWidget    hbox;
  GtkWidget   *widget;

  logProcBegin (LOG_PROC, "confuiMakeItemFontButton");
  confui->uiitem [widx].basetype = CONFUI_FONT;
  confui->uiitem [widx].outtype = CONFUI_OUT_STR;
  uiCreateHorizBox (&hbox);
  confuiMakeItemLabel (&hbox, sg, txt);
  if (fontname != NULL && *fontname) {
    widget = gtk_font_button_new_with_font (fontname);
  } else {
    widget = gtk_font_button_new ();
  }
  uiWidgetSetMarginStartW (widget, uiBaseMarginSz * 4);
  uiBoxPackStartUW (&hbox, widget);
  uiBoxPackStart (boxp, &hbox);
  confui->uiitem [widx].widget = widget;
  confui->uiitem [widx].bdjoptIdx = bdjoptIdx;
  logProcEnd (LOG_PROC, "confuiMakeItemFontButton", "");
}

static void
confuiMakeItemColorButton (configui_t *confui, UIWidget *boxp, UIWidget *sg,
    char *txt, int widx, int bdjoptIdx, char *color)
{
  UIWidget    hbox;
  GtkWidget   *widget;
  GdkRGBA     rgba;

  logProcBegin (LOG_PROC, "confuiMakeItemColorButton");

  confui->uiitem [widx].basetype = CONFUI_COLOR;
  confui->uiitem [widx].outtype = CONFUI_OUT_STR;
  uiCreateHorizBox (&hbox);
  confuiMakeItemLabel (&hbox, sg, txt);
  if (color != NULL && *color) {
    gdk_rgba_parse (&rgba, color);
    widget = gtk_color_button_new_with_rgba (&rgba);
  } else {
    widget = gtk_color_button_new ();
  }
  uiWidgetSetMarginStartW (widget, uiBaseMarginSz * 4);
  uiBoxPackStartUW (&hbox, widget);
  uiBoxPackStart (boxp, &hbox);
  confui->uiitem [widx].widget = widget;
  confui->uiitem [widx].bdjoptIdx = bdjoptIdx;
  logProcEnd (LOG_PROC, "confuiMakeItemColorButton", "");
}

static void
confuiMakeItemSpinboxText (configui_t *confui, UIWidget *boxp, UIWidget *sg,
    UIWidget *sgB, char *txt, int widx, int bdjoptIdx,
    confuiouttype_t outtype, ssize_t value, void *cb)
{
  UIWidget    hbox;
  UIWidget    *uiwidgetp;
  nlist_t     *list;
  nlist_t     *keylist;
  size_t      maxWidth;

  logProcBegin (LOG_PROC, "confuiMakeItemSpinboxText");

  confui->uiitem [widx].basetype = CONFUI_SPINBOX_TEXT;
  confui->uiitem [widx].outtype = outtype;
  uiCreateHorizBox (&hbox);
  confuiMakeItemLabel (&hbox, sg, txt);
  uiSpinboxTextCreate (confui->uiitem [widx].spinbox, confui);
  list = confui->uiitem [widx].displist;
  keylist = confui->uiitem [widx].sbkeylist;
  if (outtype == CONFUI_OUT_STR) {
    keylist = NULL;
  }
  maxWidth = 0;
  if (list != NULL) {
    nlistidx_t    iteridx;
    nlistidx_t    key;
    char          *val;

    nlistStartIterator (list, &iteridx);
    while ((key = nlistIterateKey (list, &iteridx)) >= 0) {
      size_t      len;

      val = nlistGetStr (list, key);
      len = strlen (val);
      maxWidth = len > maxWidth ? len : maxWidth;
    }
  }

  uiSpinboxTextSet (confui->uiitem [widx].spinbox, 0,
      nlistGetCount (list), maxWidth, list, keylist, NULL);
  uiSpinboxTextSetValue (confui->uiitem [widx].spinbox, value);
  uiwidgetp = uiSpinboxGetUIWidget (confui->uiitem [widx].spinbox);
  uiWidgetSetMarginStart (uiwidgetp, uiBaseMarginSz * 4);
  if (sgB != NULL) {
    uiSizeGroupAdd (sgB, uiwidgetp);
  }
  uiBoxPackStart (&hbox, uiwidgetp);
  uiBoxPackStart (boxp, &hbox);

  uiutilsUIWidgetCopy (&confui->uiitem [widx].uiwidget, uiwidgetp);
  confui->uiitem [widx].bdjoptIdx = bdjoptIdx;

  if (cb != NULL) {
    uiutilsUICallbackInit (&confui->uiitem [widx].callback, cb, confui);
    uiSpinboxTextSetValueChangedCallback (confui->uiitem [widx].spinbox,
        &confui->uiitem [widx].callback);
  }
  logProcEnd (LOG_PROC, "confuiMakeItemSpinboxText", "");
}

static void
confuiMakeItemSpinboxTime (configui_t *confui, UIWidget *boxp,
    UIWidget *sg, UIWidget *sgB, char *txt, int widx,
    int bdjoptIdx, ssize_t value)
{
  UIWidget    hbox;
  UIWidget    *uiwidgetp;

  logProcBegin (LOG_PROC, "confuiMakeItemSpinboxTime");

  confui->uiitem [widx].basetype = CONFUI_SPINBOX_TIME;
  confui->uiitem [widx].outtype = CONFUI_OUT_NUM;
  uiCreateHorizBox (&hbox);
  confuiMakeItemLabel (&hbox, sg, txt);

  uiutilsUICallbackStrInit (&confui->uiitem [widx].callback,
      confuiValMSCallback, confui);
  uiSpinboxTimeCreate (confui->uiitem [widx].spinbox, confui,
      &confui->uiitem [widx].callback);
  uiSpinboxTimeSetValue (confui->uiitem [widx].spinbox, value);
  uiwidgetp = uiSpinboxGetUIWidget (confui->uiitem [widx].spinbox);
  uiutilsUIWidgetCopy (&confui->uiitem [widx].uiwidget, uiwidgetp);
  uiWidgetSetMarginStart (uiwidgetp, uiBaseMarginSz * 4);
  if (sgB != NULL) {
    uiSizeGroupAdd (sgB, uiwidgetp);
  }
  uiBoxPackStart (&hbox, uiwidgetp);
  uiBoxPackStart (boxp, &hbox);
  confui->uiitem [widx].bdjoptIdx = bdjoptIdx;
  logProcEnd (LOG_PROC, "confuiMakeItemSpinboxTime", "");
}

static void
confuiMakeItemSpinboxNum (configui_t *confui, UIWidget *boxp, UIWidget *sg,
    UIWidget *sgB, const char *txt, int widx, int bdjoptIdx,
    int min, int max, int value, void *cb)
{
  UIWidget    hbox;
  UIWidget    uiwidget;

  logProcBegin (LOG_PROC, "confuiMakeItemSpinboxNum");

  confui->uiitem [widx].basetype = CONFUI_SPINBOX_NUM;
  confui->uiitem [widx].outtype = CONFUI_OUT_NUM;
  uiCreateHorizBox (&hbox);
  confuiMakeItemLabel (&hbox, sg, txt);
  uiSpinboxIntCreate (&uiwidget);
  uiSpinboxSet (&uiwidget, (double) min, (double) max);
  uiSpinboxSetValue (&uiwidget, (double) value);
  uiWidgetSetMarginStart (&uiwidget, uiBaseMarginSz * 4);
  if (sgB != NULL) {
    uiSizeGroupAdd (sgB, &uiwidget);
  }
  uiBoxPackStart (&hbox, &uiwidget);
  uiBoxPackStart (boxp, &hbox);
  confui->uiitem [widx].bdjoptIdx = bdjoptIdx;
  if (cb != NULL) {
    uiutilsUICallbackInit (&confui->uiitem [widx].callback, cb, confui);
    uiSpinboxSetValueChangedCallback (&uiwidget, &confui->uiitem [widx].callback);
  }
  uiutilsUIWidgetCopy (&confui->uiitem [widx].uiwidget, &uiwidget);
  logProcEnd (LOG_PROC, "confuiMakeItemSpinboxNum", "");
}

static void
confuiMakeItemSpinboxDouble (configui_t *confui, UIWidget *boxp, UIWidget *sg,
    UIWidget *sgB, char *txt, int widx, int bdjoptIdx,
    double min, double max, double value)
{
  UIWidget    hbox;
  UIWidget    uiwidget;

  logProcBegin (LOG_PROC, "confuiMakeItemSpinboxDouble");

  confui->uiitem [widx].basetype = CONFUI_SPINBOX_DOUBLE;
  confui->uiitem [widx].outtype = CONFUI_OUT_DOUBLE;
  uiCreateHorizBox (&hbox);
  confuiMakeItemLabel (&hbox, sg, txt);
  uiSpinboxDoubleCreate (&uiwidget);
  uiSpinboxSet (&uiwidget, min, max);
  uiSpinboxSetValue (&uiwidget, value);
  uiWidgetSetMarginStart (&uiwidget, uiBaseMarginSz * 4);
  if (sgB != NULL) {
    uiSizeGroupAdd (sgB, &uiwidget);
  }
  uiBoxPackStart (&hbox, &uiwidget);
  uiBoxPackStart (boxp, &hbox);
  confui->uiitem [widx].bdjoptIdx = bdjoptIdx;
  uiutilsUIWidgetCopy (&confui->uiitem [widx].uiwidget, &uiwidget);
  logProcEnd (LOG_PROC, "confuiMakeItemSpinboxDouble", "");
}

static void
confuiMakeItemSwitch (configui_t *confui, UIWidget *boxp, UIWidget *sg,
    char *txt, int widx, int bdjoptIdx, int value, void *cb)
{
  UIWidget    hbox;
  UIWidget    *uiwidgetp;

  logProcBegin (LOG_PROC, "confuiMakeItemSwitch");

  confui->uiitem [widx].basetype = CONFUI_SWITCH;
  confui->uiitem [widx].outtype = CONFUI_OUT_BOOL;
  uiCreateHorizBox (&hbox);
  confuiMakeItemLabel (&hbox, sg, txt);

  confui->uiitem [widx].uiswitch = uiCreateSwitch (value);
  uiwidgetp = uiSwitchGetUIWidget (confui->uiitem [widx].uiswitch);
  uiWidgetSetMarginStart (uiwidgetp, uiBaseMarginSz * 4);
  uiBoxPackStart (&hbox, uiwidgetp);
  uiBoxPackStart (boxp, &hbox);
  confui->uiitem [widx].bdjoptIdx = bdjoptIdx;

  if (cb != NULL) {
    uiutilsUICallbackInit (&confui->uiitem [widx].callback, cb, confui);
    uiSwitchSetCallback (confui->uiitem [widx].uiswitch,
        &confui->uiitem [widx].callback);
  }

  logProcEnd (LOG_PROC, "confuiMakeItemSwitch", "");
}

static void
confuiMakeItemLabelDisp (configui_t *confui, UIWidget *boxp, UIWidget *sg,
    char *txt, int widx, int bdjoptIdx)
{
  UIWidget    hbox;
  UIWidget    uiwidget;

  logProcBegin (LOG_PROC, "confuiMakeItemLabelDisp");

  confui->uiitem [widx].basetype = CONFUI_NONE;
  confui->uiitem [widx].outtype = CONFUI_OUT_NONE;
  uiCreateHorizBox (&hbox);
  confuiMakeItemLabel (&hbox, sg, txt);
  uiCreateLabel (&uiwidget, "");
  uiWidgetSetMarginStart (&uiwidget, uiBaseMarginSz * 4);
  uiBoxPackStart (&hbox, &uiwidget);
  uiBoxPackStart (boxp, &hbox);
  uiutilsUIWidgetCopy (&confui->uiitem [widx].uiwidget, &uiwidget);
  confui->uiitem [widx].bdjoptIdx = bdjoptIdx;
  logProcEnd (LOG_PROC, "confuiMakeItemLabelDisp", "");
}

static void
confuiMakeItemCheckButton (configui_t *confui, UIWidget *boxp, UIWidget *sg,
    const char *txt, int widx, int bdjoptIdx, int value)
{
  UIWidget    uiwidget;

  logProcBegin (LOG_PROC, "confuiMakeItemCheckButton");

  confui->uiitem [widx].basetype = CONFUI_CHECK_BUTTON;
  confui->uiitem [widx].outtype = CONFUI_OUT_BOOL;
  uiCreateCheckButton (&uiwidget, txt, value);
  uiWidgetSetMarginStart (&uiwidget, uiBaseMarginSz * 4);
  uiBoxPackStart (boxp, &uiwidget);
  uiutilsUIWidgetCopy (&confui->uiitem [widx].uiwidget, &uiwidget);
  confui->uiitem [widx].bdjoptIdx = bdjoptIdx;
  logProcEnd (LOG_PROC, "confuiMakeItemCheckButton", "");
}

static void
confuiMakeItemLabel (UIWidget *boxp, UIWidget *sg, const char *txt)
{
  UIWidget    uiwidget;

  logProcBegin (LOG_PROC, "confuiMakeItemLabel");
  if (*txt == '\0') {
    uiCreateLabel (&uiwidget, txt);
  } else {
    uiCreateColonLabel (&uiwidget, txt);
  }
  uiBoxPackStart (boxp, &uiwidget);
  uiSizeGroupAdd (sg, &uiwidget);
  logProcEnd (LOG_PROC, "confuiMakeItemLabel", "");
}

static void
confuiMakeItemTable (configui_t *confui, UIWidget *boxp, confuiident_t id,
    int flags)
{
  UIWidget    mhbox;
  UIWidget    bvbox;
  UIWidget    uiwidget;
  GtkWidget   *tree;

  logProcBegin (LOG_PROC, "confuiMakeItemTable");
  uiCreateHorizBox (&mhbox);
  uiWidgetSetMarginTop (&mhbox, uiBaseMarginSz * 2);
  uiBoxPackStart (boxp, &mhbox);

  uiCreateScrolledWindow (&uiwidget, 300);
  uiWidgetExpandVert (&uiwidget);
  uiBoxPackStartExpand (&mhbox, &uiwidget);

  tree = uiCreateTreeView ();
  confui->tables [id].tree = tree;
  confui->tables [id].sel =
      gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
  confui->tables [id].flags = flags;
  uiWidgetSetMarginStartW (tree, uiBaseMarginSz * 8);
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tree), TRUE);
  uiBoxPackInWindowUW (&uiwidget, tree);

  uiCreateVertBox (&bvbox);
  uiWidgetSetAllMargins (&bvbox, uiBaseMarginSz * 4);
  uiWidgetSetMarginTop (&bvbox, uiBaseMarginSz * 32);
  uiWidgetAlignVertStart (&bvbox);
  uiBoxPackStart (&mhbox, &bvbox);

  if ((flags & CONFUI_TABLE_NO_UP_DOWN) != CONFUI_TABLE_NO_UP_DOWN) {
    uiutilsUICallbackInit (
        &confui->tables [id].callback [CONFUI_TABLE_CB_UP],
        confuiTableMoveUp, confui);
    uiCreateButton (&uiwidget,
        &confui->tables [id].callback [CONFUI_TABLE_CB_UP],
        /* CONTEXT: configuration: table edit: button: move selection up */
        _("Move Up"), "button_up");
    uiBoxPackStart (&bvbox, &uiwidget);

    uiutilsUICallbackInit (
        &confui->tables [id].callback [CONFUI_TABLE_CB_DOWN],
        confuiTableMoveDown, confui);
    uiCreateButton (&uiwidget,
        &confui->tables [id].callback [CONFUI_TABLE_CB_DOWN],
        /* CONTEXT: configuration: table edit: button: move selection down */
        _("Move Down"), "button_down");
    uiBoxPackStart (&bvbox, &uiwidget);
  }

  uiutilsUICallbackInit (
      &confui->tables [id].callback [CONFUI_TABLE_CB_REMOVE],
      confuiTableRemove, confui);
  uiCreateButton (&uiwidget,
      &confui->tables [id].callback [CONFUI_TABLE_CB_REMOVE],
      /* CONTEXT: configuration: table edit: button: delete selection */
      _("Delete"), "button_remove");
  uiBoxPackStart (&bvbox, &uiwidget);

  uiutilsUICallbackInit (
      &confui->tables [id].callback [CONFUI_TABLE_CB_ADD],
      confuiTableAdd, confui);
  uiCreateButton (&uiwidget,
      &confui->tables [id].callback [CONFUI_TABLE_CB_ADD],
      /* CONTEXT: configuration: table edit: button: add new selection */
      _("Add New"), "button_add");
  uiBoxPackStart (&bvbox, &uiwidget);

  logProcEnd (LOG_PROC, "confuiMakeItemTable", "");
}

static nlist_t *
confuiGetThemeList (void)
{
  slist_t     *filelist = NULL;
  nlist_t     *themelist = NULL;
  char        tbuff [MAXPATHLEN];

  logProcBegin (LOG_PROC, "confuiGetThemeList");
  themelist = nlistAlloc ("cu-themes", LIST_ORDERED, free);

  if (isWindows ()) {
    nlistidx_t    count;

    snprintf (tbuff, sizeof (tbuff), "%s/plocal/share/themes",
        sysvarsGetStr (SV_BDJ4MAINDIR));
    filelist = diropRecursiveDirList (tbuff, FILEMANIP_DIRS);
    confuiGetThemeNames (themelist, filelist);
    slistFree (filelist);
    count = nlistGetCount (themelist);
    nlistSetStr (themelist, count, "Adwaita");
  } else {
    /* for macos */
    filelist = diropRecursiveDirList ("/opt/local/share/themes", FILEMANIP_DIRS);
    confuiGetThemeNames (themelist, filelist);
    slistFree (filelist);

    filelist = diropRecursiveDirList ("/usr/share/themes", FILEMANIP_DIRS);
    confuiGetThemeNames (themelist, filelist);
    slistFree (filelist);

    snprintf (tbuff, sizeof (tbuff), "%s/.themes", sysvarsGetStr (SV_HOME));
    filelist = diropRecursiveDirList (tbuff, FILEMANIP_DIRS);
    confuiGetThemeNames (themelist, filelist);
    slistFree (filelist);
  }

  logProcEnd (LOG_PROC, "confuiGetThemeList", "");
  return themelist;
}

static nlist_t *
confuiGetThemeNames (nlist_t *themelist, slist_t *filelist)
{
  slistidx_t    iteridx;
  char          *fn;
  pathinfo_t    *pi;
  static char   *srchdir = "gtk-3.0";
  char          tbuff [MAXPATHLEN];
  char          tmp [MAXPATHLEN];
  int           count;

  logProcBegin (LOG_PROC, "confuiGetThemeNames");
  if (filelist == NULL) {
    logProcEnd (LOG_PROC, "confuiGetThemeNames", "no-filelist");
    return NULL;
  }

  count = nlistGetCount (themelist);
  slistStartIterator (filelist, &iteridx);

  /* the key value used here is meaningless */
  while ((fn = slistIterateKey (filelist, &iteridx)) != NULL) {
    if (fileopIsDirectory (fn)) {
      pi = pathInfo (fn);
      if (pi->flen == strlen (srchdir) &&
          strncmp (pi->filename, srchdir, strlen (srchdir)) == 0) {
        strlcpy (tbuff, pi->dirname, pi->dlen + 1);
        pathInfoFree (pi);
        pi = pathInfo (tbuff);
        strlcpy (tmp, pi->filename, pi->flen + 1);
        nlistSetStr (themelist, count++, tmp);
      }
      pathInfoFree (pi);
    } /* is directory */
  } /* for each file */

  logProcEnd (LOG_PROC, "confuiGetThemeNames", "");
  return themelist;
}

static void
confuiLoadHTMLList (configui_t *confui)
{
  char          tbuff [MAXPATHLEN];
  nlist_t       *tlist = NULL;
  datafile_t    *df = NULL;
  slist_t       *list = NULL;
  slistidx_t    iteridx;
  char          *key;
  char          *data;
  char          *tstr;
  nlist_t       *llist;
  int           count;

  logProcBegin (LOG_PROC, "confuiLoadHTMLList");

  tlist = nlistAlloc ("cu-html-list", LIST_ORDERED, free);
  llist = nlistAlloc ("cu-html-list-l", LIST_ORDERED, free);

  pathbldMakePath (tbuff, sizeof (tbuff),
      "html-list", BDJ4_CONFIG_EXT, PATHBLD_MP_TEMPLATEDIR);
  df = datafileAllocParse ("conf-html-list", DFTYPE_KEY_VAL, tbuff,
      NULL, 0, DATAFILE_NO_LOOKUP);
  list = datafileGetList (df);

  tstr = bdjoptGetStr (OPT_G_REMCONTROLHTML);
  slistStartIterator (list, &iteridx);
  count = 0;
  while ((key = slistIterateKey (list, &iteridx)) != NULL) {
    data = slistGetStr (list, key);
    if (tstr != NULL && strcmp (data, bdjoptGetStr (OPT_G_REMCONTROLHTML)) == 0) {
      confui->uiitem [CONFUI_SPINBOX_RC_HTML_TEMPLATE].listidx = count;
    }
    nlistSetStr (tlist, count, key);
    nlistSetStr (llist, count, data);
    ++count;
  }
  datafileFree (df);

  confui->uiitem [CONFUI_SPINBOX_RC_HTML_TEMPLATE].displist = tlist;
  confui->uiitem [CONFUI_SPINBOX_RC_HTML_TEMPLATE].sbkeylist = llist;
  logProcEnd (LOG_PROC, "confuiLoadHTMLList", "");
}

static void
confuiLoadVolIntfcList (configui_t *confui)
{
  char          tbuff [MAXPATHLEN];
  nlist_t       *tlist = NULL;
  datafile_t    *df = NULL;
  ilist_t       *list = NULL;
  ilistidx_t    iteridx;
  ilistidx_t    key;
  char          *os;
  char          *intfc;
  char          *desc;
  nlist_t       *llist;
  int           count;

  static datafilekey_t dfkeys [CONFUI_VOL_MAX] = {
    { "DESC",   CONFUI_VOL_DESC,  VALUE_STR, NULL, -1 },
    { "INTFC",  CONFUI_VOL_INTFC, VALUE_STR, NULL, -1 },
    { "OS",     CONFUI_VOL_OS,    VALUE_STR, NULL, -1 },
  };

  logProcBegin (LOG_PROC, "confuiLoadVolIntfcList");

  tlist = nlistAlloc ("cu-volintfc-list", LIST_ORDERED, free);
  llist = nlistAlloc ("cu-volintfc-list-l", LIST_ORDERED, free);

  pathbldMakePath (tbuff, sizeof (tbuff),
      "volintfc", BDJ4_CONFIG_EXT, PATHBLD_MP_TEMPLATEDIR);
  df = datafileAllocParse ("conf-volintfc-list", DFTYPE_INDIRECT, tbuff,
      dfkeys, CONFUI_VOL_MAX, DATAFILE_NO_LOOKUP);
  list = datafileGetList (df);

  ilistStartIterator (list, &iteridx);
  count = 0;
  while ((key = ilistIterateKey (list, &iteridx)) >= 0) {
    intfc = ilistGetStr (list, key, CONFUI_VOL_INTFC);
    desc = ilistGetStr (list, key, CONFUI_VOL_DESC);
    os = ilistGetStr (list, key, CONFUI_VOL_OS);
    if (strcmp (os, sysvarsGetStr (SV_OSNAME)) == 0 ||
        strcmp (os, "all") == 0) {
      if (strcmp (intfc, bdjoptGetStr (OPT_M_VOLUME_INTFC)) == 0) {
        confui->uiitem [CONFUI_SPINBOX_VOL_INTFC].listidx = count;
      }
      nlistSetStr (tlist, count, desc);
      nlistSetStr (llist, count, intfc);
      ++count;
    }
  }
  datafileFree (df);

  confui->uiitem [CONFUI_SPINBOX_VOL_INTFC].displist = tlist;
  confui->uiitem [CONFUI_SPINBOX_VOL_INTFC].sbkeylist = llist;
  logProcEnd (LOG_PROC, "confuiLoadVolIntfcList", "");
}

static void
confuiLoadPlayerIntfcList (configui_t *confui)
{
  char          tbuff [MAXPATHLEN];
  nlist_t       *tlist = NULL;
  datafile_t    *df = NULL;
  ilist_t       *list = NULL;
  ilistidx_t    iteridx;
  ilistidx_t    key;
  char          *os;
  char          *intfc;
  char          *desc;
  nlist_t       *llist;
  int           count;

  static datafilekey_t dfkeys [CONFUI_PLAYER_MAX] = {
    { "DESC",   CONFUI_PLAYER_DESC,  VALUE_STR, NULL, -1 },
    { "INTFC",  CONFUI_PLAYER_INTFC, VALUE_STR, NULL, -1 },
    { "OS",     CONFUI_PLAYER_OS,    VALUE_STR, NULL, -1 },
  };

  logProcBegin (LOG_PROC, "confuiLoadPlayerIntfcList");

  tlist = nlistAlloc ("cu-playerintfc-list", LIST_ORDERED, free);
  llist = nlistAlloc ("cu-playerintfc-list-l", LIST_ORDERED, free);

  pathbldMakePath (tbuff, sizeof (tbuff),
      "playerintfc", BDJ4_CONFIG_EXT, PATHBLD_MP_TEMPLATEDIR);
  df = datafileAllocParse ("conf-playerintfc-list", DFTYPE_INDIRECT, tbuff,
      dfkeys, CONFUI_PLAYER_MAX, DATAFILE_NO_LOOKUP);
  list = datafileGetList (df);

  ilistStartIterator (list, &iteridx);
  count = 0;
  while ((key = ilistIterateKey (list, &iteridx)) >= 0) {
    intfc = ilistGetStr (list, key, CONFUI_PLAYER_INTFC);
    desc = ilistGetStr (list, key, CONFUI_PLAYER_DESC);
    os = ilistGetStr (list, key, CONFUI_PLAYER_OS);
    if (strcmp (os, sysvarsGetStr (SV_OSNAME)) == 0 ||
        strcmp (os, "all") == 0) {
      if (strcmp (intfc, bdjoptGetStr (OPT_M_PLAYER_INTFC)) == 0) {
        confui->uiitem [CONFUI_SPINBOX_PLAYER].listidx = count;
      }
      nlistSetStr (tlist, count, desc);
      nlistSetStr (llist, count, intfc);
      ++count;
    }
  }
  datafileFree (df);

  confui->uiitem [CONFUI_SPINBOX_PLAYER].displist = tlist;
  confui->uiitem [CONFUI_SPINBOX_PLAYER].sbkeylist = llist;
  logProcEnd (LOG_PROC, "confuiLoadPlayerIntfcList", "");
}

static void
confuiLoadLocaleList (configui_t *confui)
{
  char          tbuff [MAXPATHLEN];
  nlist_t       *tlist = NULL;
  datafile_t    *df = NULL;
  slist_t       *list = NULL;
  slistidx_t    iteridx;
  char          *key;
  char          *data;
  nlist_t       *llist;
  int           count;
  bool          found;
  int           engbidx = 0;
  int           shortidx = 0;

  logProcBegin (LOG_PROC, "confuiLoadLocaleList");

  tlist = nlistAlloc ("cu-locale-list", LIST_ORDERED, free);
  llist = nlistAlloc ("cu-locale-list-l", LIST_ORDERED, free);

  pathbldMakePath (tbuff, sizeof (tbuff),
      "locales", BDJ4_CONFIG_EXT, PATHBLD_MP_LOCALEDIR);
  df = datafileAllocParse ("conf-locale-list", DFTYPE_KEY_VAL, tbuff,
      NULL, 0, DATAFILE_NO_LOOKUP);
  list = datafileGetList (df);

  slistStartIterator (list, &iteridx);
  count = 0;
  found = false;
  shortidx = -1;
  while ((key = slistIterateKey (list, &iteridx)) != NULL) {
    data = slistGetStr (list, key);
    if (strcmp (data, "en_GB") == 0) {
      engbidx = count;
    }
    if (strcmp (data, sysvarsGetStr (SV_LOCALE)) == 0) {
      confui->uiitem [CONFUI_SPINBOX_LOCALE].listidx = count;
      found = true;
    }
    if (strncmp (data, sysvarsGetStr (SV_LOCALE_SHORT), 2) == 0) {
      shortidx = count;
    }
    nlistSetStr (tlist, count, key);
    nlistSetStr (llist, count, data);
    ++count;
  }
  if (! found && shortidx >= 0) {
    confui->uiitem [CONFUI_SPINBOX_LOCALE].listidx = shortidx;
  } else if (! found) {
    confui->uiitem [CONFUI_SPINBOX_LOCALE].listidx = engbidx;
  }
  datafileFree (df);

  confui->uiitem [CONFUI_SPINBOX_LOCALE].displist = tlist;
  confui->uiitem [CONFUI_SPINBOX_LOCALE].sbkeylist = llist;
  logProcEnd (LOG_PROC, "confuiLoadLocaleList", "");
}


static void
confuiLoadDanceTypeList (configui_t *confui)
{
  nlist_t       *tlist = NULL;
  nlist_t       *llist = NULL;
  dnctype_t     *dnctypes;
  slistidx_t    iteridx;
  char          *key;
  int           count;

  logProcBegin (LOG_PROC, "confuiLoadDanceTypeList");

  tlist = nlistAlloc ("cu-dance-type", LIST_ORDERED, free);
  llist = nlistAlloc ("cu-dance-type-l", LIST_ORDERED, free);

  dnctypes = bdjvarsdfGet (BDJVDF_DANCE_TYPES);
  dnctypesStartIterator (dnctypes, &iteridx);
  count = 0;
  while ((key = dnctypesIterate (dnctypes, &iteridx)) != NULL) {
    nlistSetStr (tlist, count, key);
    nlistSetNum (llist, count, count);
    ++count;
  }

  confui->uiitem [CONFUI_SPINBOX_DANCE_TYPE].displist = tlist;
  confui->uiitem [CONFUI_SPINBOX_DANCE_TYPE].sbkeylist = llist;
  logProcEnd (LOG_PROC, "confuiLoadDanceTypeList", "");
}

static void
confuiLoadTagList (configui_t *confui)
{
  slist_t       *llist = NULL;
  slist_t       *elist = NULL;

  logProcBegin (LOG_PROC, "confuiLoadTagList");

  llist = slistAlloc ("cu-list-tag-list", LIST_ORDERED, NULL);
  elist = slistAlloc ("cu-edit-tag-list", LIST_ORDERED, NULL);

  for (tagdefkey_t i = 0; i < TAG_KEY_MAX; ++i) {
    if (tagdefs [i].listingDisplay) {
      slistSetNum (llist, tagdefs [i].displayname, i);
    }
    if (tagdefs [i].isEditable ||
        (tagdefs [i].listingDisplay && tagdefs [i].editType == ET_LABEL)) {
      slistSetNum (elist, tagdefs [i].displayname, i);
    }
  }

  confui->listingtaglist = llist;
  confui->edittaglist = elist;
  logProcEnd (LOG_PROC, "confuiLoadTagList", "");
}

static void
confuiLoadThemeList (configui_t *confui)
{
  nlist_t     *tlist;
  nlistidx_t  iteridx;
  int         count;
  bool        usesys = false;
  char        *p;

  p = bdjoptGetStr (OPT_MP_UI_THEME);
  /* use the system default if the ui theme is empty */
  if (! *p) {
    usesys = true;
  }

  tlist = confuiGetThemeList ();
  nlistStartIterator (tlist, &iteridx);
  count = 0;
  while ((p = nlistIterateValueData (tlist, &iteridx)) != NULL) {
    if (strcmp (p, bdjoptGetStr (OPT_MP_MQ_THEME)) == 0) {
      confui->uiitem [CONFUI_SPINBOX_MQ_THEME].listidx = count;
    }
    if (! usesys &&
        strcmp (p, bdjoptGetStr (OPT_MP_UI_THEME)) == 0) {
      confui->uiitem [CONFUI_SPINBOX_UI_THEME].listidx = count;
    }
    if (usesys &&
        strcmp (p, sysvarsGetStr (SV_THEME_DEFAULT)) == 0) {
      confui->uiitem [CONFUI_SPINBOX_UI_THEME].listidx = count;
    }
    ++count;
  }
  /* the theme list is ordered */
  confui->uiitem [CONFUI_SPINBOX_UI_THEME].displist = tlist;
  confui->uiitem [CONFUI_SPINBOX_MQ_THEME].displist = tlist;
}


static bool
confuiOrgPathSelect (void *udata, long idx)
{
  configui_t  *confui = udata;
  char        *sval = NULL;
  int         widx;

  logProcBegin (LOG_PROC, "confuiOrgPathSelect");
  widx = CONFUI_COMBOBOX_AO_PATHFMT;
  sval = slistGetDataByIdx (confui->uiitem [widx].displist, idx);
  confui->uiitem [widx].listidx = idx;
  if (sval != NULL && *sval) {
    bdjoptSetStr (OPT_G_AO_PATHFMT, sval);
  }
  confuiUpdateOrgExamples (confui, sval);
  logProcEnd (LOG_PROC, "confuiOrgPathSelect", "");
  return UICB_CONT;
}

static void
confuiUpdateMobmqQrcode (configui_t *confui)
{
  char          uridisp [MAXPATHLEN];
  char          *qruri = "";
  char          tbuff [MAXPATHLEN];
  char          *tag;
  const char    *valstr;
  bdjmobilemq_t type;
  UIWidget      *uiwidgetp;

  logProcBegin (LOG_PROC, "confuiUpdateMobmqQrcode");

  type = (bdjmobilemq_t) bdjoptGetNum (OPT_P_MOBILEMARQUEE);

  confuiSetStatusMsg (confui, "");
  if (type == MOBILEMQ_OFF) {
    *tbuff = '\0';
    *uridisp = '\0';
    qruri = "";
  }
  if (type == MOBILEMQ_INTERNET) {
    tag = bdjoptGetStr (OPT_P_MOBILEMQTAG);
    valstr = validate (tag, VAL_NOT_EMPTY | VAL_NO_SPACES);
    if (valstr != NULL) {
      /* CONTEXT: mobile marquee: the name to use for internet routing */
      snprintf (tbuff, sizeof (tbuff), valstr, _("Name"));
      confuiSetStatusMsg (confui, tbuff);
    }
    snprintf (uridisp, sizeof (uridisp), "%s%s?v=1&tag=%s",
        sysvarsGetStr (SV_HOST_MOBMQ), sysvarsGetStr (SV_URI_MOBMQ),
        tag);
  }
  if (type == MOBILEMQ_LOCAL) {
    char *ip;

    ip = confuiGetLocalIP (confui);
    snprintf (uridisp, sizeof (uridisp), "http://%s:%zd",
        ip, bdjoptGetNum (OPT_P_MOBILEMQPORT));
  }

  if (type != MOBILEMQ_OFF) {
    /* CONTEXT: configuration: qr code: title display for mobile marquee */
    qruri = confuiMakeQRCodeFile (confui, _("Mobile Marquee"), uridisp);
  }

  uiwidgetp = &confui->uiitem [CONFUI_WIDGET_MMQ_QR_CODE].uiwidget;
  uiLinkSet (uiwidgetp, uridisp, qruri);
  if (confui->uiitem [CONFUI_WIDGET_MMQ_QR_CODE].uri != NULL) {
    free (confui->uiitem [CONFUI_WIDGET_MMQ_QR_CODE].uri);
  }
  confui->uiitem [CONFUI_WIDGET_MMQ_QR_CODE].uri = strdup (qruri);
  if (*qruri) {
    free (qruri);
  }
  logProcEnd (LOG_PROC, "confuiUpdateMobmqQrcode", "");
}

static bool
confuiMobmqTypeChg (void *udata)
{
  configui_t    *confui = udata;
  double        value;
  long          nval;

  logProcBegin (LOG_PROC, "confuiMobmqTypeChg");
  value = uiSpinboxGetValue (&confui->uiitem [CONFUI_SPINBOX_MOBILE_MQ].uiwidget);
  nval = (long) value;
  bdjoptSetNum (OPT_P_MOBILEMARQUEE, nval);
  confuiUpdateMobmqQrcode (confui);
  logProcEnd (LOG_PROC, "confuiMobmqTypeChg", "");
  return UICB_CONT;
}

static bool
confuiMobmqPortChg (void *udata)
{
  configui_t    *confui = udata;
  double        value;
  long          nval;

  logProcBegin (LOG_PROC, "confuiMobmqPortChg");
  value = uiSpinboxGetValue (&confui->uiitem [CONFUI_WIDGET_MMQ_PORT].uiwidget);
  nval = (long) value;
  bdjoptSetNum (OPT_P_MOBILEMQPORT, nval);
  confuiUpdateMobmqQrcode (confui);
  logProcEnd (LOG_PROC, "confuiMobmqPortChg", "");
  return UICB_CONT;
}

static int
confuiMobmqNameChg (uientry_t *entry, void *udata)
{
  configui_t    *confui = udata;
  const char      *sval;

  logProcBegin (LOG_PROC, "confuiMobmqNameChg");
  sval = uiEntryGetValue (entry);
  bdjoptSetStr (OPT_P_MOBILEMQTAG, sval);
  confuiUpdateMobmqQrcode (confui);
  logProcEnd (LOG_PROC, "confuiMobmqNameChg", "");
  return UIENTRY_OK;
}

static int
confuiMobmqTitleChg (uientry_t *entry, void *udata)
{
  configui_t      *confui = udata;
  const char      *sval;

  logProcBegin (LOG_PROC, "confuiMobmqTitleChg");
  sval = uiEntryGetValue (entry);
  bdjoptSetStr (OPT_P_MOBILEMQTITLE, sval);
  confuiUpdateMobmqQrcode (confui);
  logProcEnd (LOG_PROC, "confuiMobmqTitleChg", "");
  return UIENTRY_OK;
}

static void
confuiUpdateRemctrlQrcode (configui_t *confui)
{
  char          uridisp [MAXPATHLEN];
  char          *qruri = "";
  char          tbuff [MAXPATHLEN];
  long          onoff;
  UIWidget      *uiwidgetp;

  logProcBegin (LOG_PROC, "confuiUpdateRemctrlQrcode");

  onoff = (bdjmobilemq_t) bdjoptGetNum (OPT_P_REMOTECONTROL);

  if (onoff == 0) {
    *tbuff = '\0';
    *uridisp = '\0';
    qruri = "";
  }
  if (onoff == 1) {
    char *ip;

    ip = confuiGetLocalIP (confui);
    snprintf (uridisp, sizeof (uridisp), "http://%s:%zd",
        ip, bdjoptGetNum (OPT_P_REMCONTROLPORT));
  }

  if (onoff == 1) {
    /* CONTEXT: configuration: qr code: title display for mobile remote control */
    qruri = confuiMakeQRCodeFile (confui, _("Mobile Remote Control"), uridisp);
  }

  uiwidgetp = &confui->uiitem [CONFUI_WIDGET_RC_QR_CODE].uiwidget;
  uiLinkSet (uiwidgetp, uridisp, qruri);
  if (confui->uiitem [CONFUI_WIDGET_RC_QR_CODE].uri != NULL) {
    free (confui->uiitem [CONFUI_WIDGET_RC_QR_CODE].uri);
  }
  confui->uiitem [CONFUI_WIDGET_RC_QR_CODE].uri = strdup (qruri);
  if (*qruri) {
    free (qruri);
  }
  logProcEnd (LOG_PROC, "confuiUpdateRemctrlQrcode", "");
}

static bool
confuiRemctrlChg (void *udata, int value)
{
  configui_t  *confui = udata;

  logProcBegin (LOG_PROC, "confuiRemctrlChg");
  bdjoptSetNum (OPT_P_REMOTECONTROL, value);
  confuiUpdateRemctrlQrcode (confui);
  logProcEnd (LOG_PROC, "confuiRemctrlChg", "");
  return UICB_CONT;
}

static bool
confuiRemctrlPortChg (void *udata)
{
  configui_t    *confui = udata;
  double        value;
  long          nval;

  logProcBegin (LOG_PROC, "confuiRemctrlPortChg");
  value = uiSpinboxGetValue (&confui->uiitem [CONFUI_WIDGET_RC_PORT].uiwidget);
  nval = (long) value;
  bdjoptSetNum (OPT_P_REMCONTROLPORT, nval);
  confuiUpdateRemctrlQrcode (confui);
  logProcEnd (LOG_PROC, "confuiRemctrlPortChg", "");
  return UICB_CONT;
}


static char *
confuiMakeQRCodeFile (configui_t *confui, char *title, char *uri)
{
  char          *data;
  char          *ndata;
  char          *qruri;
  char          baseuri [MAXPATHLEN];
  char          tbuff [MAXPATHLEN];
  FILE          *fh;
  size_t        dlen;

  logProcBegin (LOG_PROC, "confuiMakeQRCodeFile");
  qruri = malloc (MAXPATHLEN);

  pathbldMakePath (baseuri, sizeof (baseuri),
      "", "", PATHBLD_MP_TEMPLATEDIR);
  pathbldMakePath (tbuff, sizeof (tbuff),
      "qrcode", ".html", PATHBLD_MP_TEMPLATEDIR);

  /* this is gross */
  data = filedataReadAll (tbuff, &dlen);
  ndata = filedataReplace (data, &dlen, "#TITLE#", title);
  free (data);
  data = ndata;
  ndata = filedataReplace (data, &dlen, "#BASEURL#", baseuri);
  free (data);
  data = ndata;
  ndata = filedataReplace (data, &dlen, "#QRCODEURL#", uri);
  free (data);

  pathbldMakePath (tbuff, sizeof (tbuff),
      "qrcode", ".html", PATHBLD_MP_TMPDIR);
  fh = fopen (tbuff, "w");
  fwrite (ndata, dlen, 1, fh);
  fclose (fh);
  /* windows requires an extra slash in front, and it doesn't hurt on linux */
  snprintf (qruri, MAXPATHLEN, "file:///%s/%s",
      sysvarsGetStr (SV_BDJ4DATATOPDIR), tbuff);

  free (ndata);
  logProcEnd (LOG_PROC, "confuiMakeQRCodeFile", "");
  return qruri;
}

static void
confuiUpdateOrgExamples (configui_t *confui, char *pathfmt)
{
  char      *data;
  org_t     *org;
  UIWidget  *uiwidgetp;

  if (pathfmt == NULL) {
    return;
  }

  logProcBegin (LOG_PROC, "confuiUpdateOrgExamples");
  org = orgAlloc (pathfmt);
  assert (org != NULL);

  data = "FILE\n..none\nDISC\n..1\nTRACKNUMBER\n..1\nALBUM\n..Smooth\nALBUMARTIST\n..Santana\nARTIST\n..Santana\nDANCE\n..Cha Cha\nGENRE\n..Ballroom Dance\nTITLE\n..Smooth\n";
  uiwidgetp = &confui->uiitem [CONFUI_WIDGET_AO_EXAMPLE_1].uiwidget;
  confuiUpdateOrgExample (confui, org, data, uiwidgetp);

  data = "FILE\n..none\nDISC\n..1\nTRACKNUMBER\n..2\nALBUM\n..The Ultimate Latin Album 4: Latin Eyes\nALBUMARTIST\n..WRD\nARTIST\n..Gizelle D'Cole\nDANCE\n..Rumba\nGENRE\n..Ballroom Dance\nTITLE\n..Asi\n";
  uiwidgetp = &confui->uiitem [CONFUI_WIDGET_AO_EXAMPLE_2].uiwidget;
  confuiUpdateOrgExample (confui, org, data, uiwidgetp);

  data = "FILE\n..none\nDISC\n..1\nTRACKNUMBER\n..3\nALBUM\n..Shaman\nALBUMARTIST\n..Santana\nARTIST\n..Santana\nDANCE\n..Waltz\nTITLE\n..The Game of Love\nGENRE\n..Latin";
  uiwidgetp = &confui->uiitem [CONFUI_WIDGET_AO_EXAMPLE_3].uiwidget;
  confuiUpdateOrgExample (confui, org, data, uiwidgetp);

  data = "FILE\n..none\nDISC\n..2\nTRACKNUMBER\n..2\nALBUM\n..The Ultimate Latin Album 9: Footloose\nALBUMARTIST\n..\nARTIST\n..Raphael\nDANCE\n..Rumba\nTITLE\n..Ni tú ni yo\nGENRE\n..Latin";
  uiwidgetp = &confui->uiitem [CONFUI_WIDGET_AO_EXAMPLE_4].uiwidget;
  confuiUpdateOrgExample (confui, org, data, uiwidgetp);

  orgFree (org);
  logProcEnd (LOG_PROC, "confuiUpdateOrgExamples", "");
}

static void
confuiUpdateOrgExample (configui_t *config, org_t *org, char *data, UIWidget *uiwidgetp)
{
  song_t    *song;
  char      *tdata;
  char      *disp;

  if (data == NULL || org == NULL) {
    return;
  }

  logProcBegin (LOG_PROC, "confuiUpdateOrgExample");
  tdata = strdup (data);
  song = songAlloc ();
  assert (song != NULL);
  songParse (song, tdata, 0);
  disp = orgMakeSongPath (org, song);
  if (isWindows ()) {
    pathWinPath (disp, strlen (disp));
  }
  uiLabelSetText (uiwidgetp, disp);
  songFree (song);
  free (disp);
  free (tdata);
  logProcEnd (LOG_PROC, "confuiUpdateOrgExample", "");
}

static char *
confuiGetLocalIP (configui_t *confui)
{
  char    *ip;

  if (confui->localip == NULL) {
    ip = webclientGetLocalIP ();
    confui->localip = strdup (ip);
    free (ip);
  }

  return confui->localip;
}

static void
confuiSetStatusMsg (configui_t *confui, const char *msg)
{
  uiLabelSetText (&confui->statusMsg, msg);
}

static void
confuiSpinboxTextInitDataNum (configui_t *confui, char *tag, int widx, ...)
{
  va_list     valist;
  nlistidx_t  key;
  char        *disp;
  int         sbidx;
  nlist_t     *tlist;
  nlist_t     *llist;

  va_start (valist, widx);

  tlist = nlistAlloc (tag, LIST_ORDERED, free);
  llist = nlistAlloc (tag, LIST_ORDERED, free);
  sbidx = 0;
  while ((key = va_arg (valist, nlistidx_t)) != -1) {
    disp = va_arg (valist, char *);

    nlistSetStr (tlist, sbidx, disp);
    nlistSetNum (llist, sbidx, key);
    ++sbidx;
  }
  confui->uiitem [widx].displist = tlist;
  confui->uiitem [widx].sbkeylist = llist;

  va_end (valist);
}

static bool
confuiLinkCallback (void *udata)
{
  configui_t  *confui = udata;
  char        *uri;
  char        tmp [200];
  int         widx = -1;

  if (confui->tablecurr == CONFUI_ID_MOBILE_MQ) {
    widx = CONFUI_WIDGET_MMQ_QR_CODE;
  }
  if (confui->tablecurr == CONFUI_ID_REM_CONTROL) {
    widx = CONFUI_WIDGET_RC_QR_CODE;
  }
  if (widx < 0) {
    return UICB_CONT;
  }

  uri = confui->uiitem [widx].uri;
  if (uri != NULL) {
    snprintf (tmp, sizeof (tmp), "open %s", uri);
    system (tmp);
    return UICB_STOP;
  }
  return UICB_CONT;
}

/* table editing */

static bool
confuiTableMoveUp (void *udata)
{
  logProcBegin (LOG_PROC, "confuiTableMoveUp");
  confuiTableMove (udata, CONFUI_MOVE_PREV);
  logProcEnd (LOG_PROC, "confuiTableMoveUp", "");
  return UICB_CONT;
}

static bool
confuiTableMoveDown (void *udata)
{
  logProcBegin (LOG_PROC, "confuiTableMoveDown");
  confuiTableMove (udata, CONFUI_MOVE_NEXT);
  logProcEnd (LOG_PROC, "confuiTableMoveDown", "");
  return UICB_CONT;
}

static void
confuiTableMove (configui_t *confui, int dir)
{
  GtkWidget         *tree;
  GtkTreeModel      *model;
  GtkTreeIter       iter;
  GtkTreeIter       citer;
  GtkTreePath       *path = NULL;
  char              *pathstr = NULL;
  int               count;
  gboolean          valid;
  int               idx;
  int               flags;

  logProcBegin (LOG_PROC, "confuiTableMove");
  tree = confui->tables [confui->tablecurr].tree;
  flags = confui->tables [confui->tablecurr].flags;

  if (tree == NULL) {
    return;
  }

  count = gtk_tree_selection_count_selected_rows (
      confui->tables [confui->tablecurr].sel);
  if (count != 1) {
    logProcEnd (LOG_PROC, "confuiTableMove", "no-selection");
    return;
  }

  gtk_tree_selection_get_selected (
      confui->tables [confui->tablecurr].sel, &model, &iter);

  path = gtk_tree_model_get_path (model, &iter);
  if (path == NULL) {
    return;
  }

  pathstr = gtk_tree_path_to_string (path);
  sscanf (pathstr, "%d", &idx);
  free (pathstr);
  gtk_tree_path_free (path);

  if (idx == 1 &&
      dir == CONFUI_MOVE_PREV &&
      (flags & CONFUI_TABLE_KEEP_FIRST) == CONFUI_TABLE_KEEP_FIRST) {
    logProcEnd (LOG_PROC, "confuiTableMove", "move-prev-keep-first");
    return;
  }
  if (idx == confui->tables [confui->tablecurr].currcount - 1 &&
      dir == CONFUI_MOVE_PREV &&
      (flags & CONFUI_TABLE_KEEP_LAST) == CONFUI_TABLE_KEEP_LAST) {
    logProcEnd (LOG_PROC, "confuiTableMove", "move-prev-keep-last");
    return;
  }
  if (idx == confui->tables [confui->tablecurr].currcount - 2 &&
      dir == CONFUI_MOVE_NEXT &&
      (flags & CONFUI_TABLE_KEEP_LAST) == CONFUI_TABLE_KEEP_LAST) {
    logProcEnd (LOG_PROC, "confuiTableMove", "move-next-keep-last");
    return;
  }
  if (idx == 0 &&
      dir == CONFUI_MOVE_NEXT &&
      (flags & CONFUI_TABLE_KEEP_FIRST) == CONFUI_TABLE_KEEP_FIRST) {
    logProcEnd (LOG_PROC, "confuiTableMove", "move-next-keep-first");
    return;
  }

  memcpy (&citer, &iter, sizeof (iter));
  if (dir == CONFUI_MOVE_PREV) {
    valid = gtk_tree_model_iter_previous (model, &iter);
    if (valid) {
      gtk_list_store_move_before (GTK_LIST_STORE (model), &citer, &iter);
    }
  } else {
    valid = gtk_tree_model_iter_next (model, &iter);
    if (valid) {
      gtk_list_store_move_after (GTK_LIST_STORE (model), &citer, &iter);
    }
  }
  confui->tables [confui->tablecurr].changed = true;
  logProcEnd (LOG_PROC, "confuiTableMove", "");
}

static bool
confuiTableRemove (void *udata)
{
  configui_t        *confui = udata;
  GtkWidget         *tree;
  GtkTreeModel      *model;
  GtkTreeIter       iter;
  GtkTreePath       *path;
  char              *pathstr;
  int               idx;
  int               count;
  int               flags;

  logProcBegin (LOG_PROC, "confuiTableRemove");
  tree = confui->tables [confui->tablecurr].tree;

  if (tree == NULL) {
    return UICB_STOP;
  }

  flags = confui->tables [confui->tablecurr].flags;
  count = uiTreeViewGetSelection (tree, &model, &iter);
  if (count != 1) {
    logProcEnd (LOG_PROC, "confuiTableRemove", "no-selection");
    return UICB_STOP;
  }

  path = gtk_tree_model_get_path (model, &iter);
  pathstr = gtk_tree_path_to_string (path);
  sscanf (pathstr, "%d", &idx);
  free (pathstr);
  gtk_tree_path_free (path);
  if (idx == 0 &&
      (flags & CONFUI_TABLE_KEEP_FIRST) == CONFUI_TABLE_KEEP_FIRST) {
    logProcEnd (LOG_PROC, "confuiTableRemove", "keep-first");
    return UICB_CONT;
  }
  if (idx == confui->tables [confui->tablecurr].currcount - 1 &&
      (flags & CONFUI_TABLE_KEEP_LAST) == CONFUI_TABLE_KEEP_LAST) {
    logProcEnd (LOG_PROC, "confuiTableRemove", "keep-last");
    return UICB_CONT;
  }

  if (confui->tablecurr == CONFUI_ID_DANCE) {
    long          idx;
    dance_t       *dances;
    GtkWidget     *tree;
    GtkTreeModel  *model;
    GtkTreeIter   iter;

    tree = confui->tables [confui->tablecurr].tree;
    uiTreeViewGetSelection (tree, &model, &iter);
    gtk_tree_model_get (model, &iter, CONFUI_DANCE_COL_DANCE_IDX, &idx, -1);
    dances = bdjvarsdfGet (BDJVDF_DANCES);
    danceDelete (dances, idx);
  }

  gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
  confui->tables [confui->tablecurr].changed = true;
  confui->tables [confui->tablecurr].currcount -= 1;
  logProcEnd (LOG_PROC, "confuiTableRemove", "");

  if (confui->tablecurr == CONFUI_ID_DANCE) {
    GtkWidget   *tree;
    GtkTreePath *path;
    GtkTreeIter iter;

    tree = confui->tables [confui->tablecurr].tree;
    uiTreeViewGetSelection (tree, &model, &iter);
    path = gtk_tree_model_get_path (model, &iter);
    confuiDanceSelect (GTK_TREE_VIEW (tree), path, NULL, confui);
  }

  return UICB_CONT;
}

static bool
confuiTableAdd (void *udata)
{
  configui_t        *confui = udata;
  GtkWidget         *tree = NULL;
  GtkTreeModel      *model = NULL;
  GtkTreeIter       iter;
  GtkTreeIter       niter;
  GtkTreeIter       *titer;
  GtkTreePath       *path;
  int               count;
  int               flags;

  logProcBegin (LOG_PROC, "confuiTableAdd");

  if (confui->tablecurr >= CONFUI_ID_TABLE_MAX) {
    logProcEnd (LOG_PROC, "confuiTableAdd", "non-table");
    return UICB_STOP;
  }

  tree = confui->tables [confui->tablecurr].tree;
  if (tree == NULL) {
    logProcEnd (LOG_PROC, "confuiTableAdd", "no-tree");
    return UICB_STOP;
  }

  flags = confui->tables [confui->tablecurr].flags;
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree));
  count = gtk_tree_selection_count_selected_rows (
      confui->tables [confui->tablecurr].sel);
  if (count != 1) {
    titer = NULL;
  } else {
    gtk_tree_selection_get_selected (
        confui->tables [confui->tablecurr].sel, &model, &iter);
    titer = &iter;
  }

  if (titer != NULL) {
    GtkTreePath       *path;
    char              *pathstr;
    int               idx;

    path = gtk_tree_model_get_path (model, titer);
    pathstr = gtk_tree_path_to_string (path);
    sscanf (pathstr, "%d", &idx);
    free (pathstr);
    gtk_tree_path_free (path);
    if (idx == 0 &&
        (flags & CONFUI_TABLE_KEEP_FIRST) == CONFUI_TABLE_KEEP_FIRST) {
      gtk_tree_model_iter_next (model, &iter);
    }
  }

  if (titer == NULL) {
    gtk_list_store_append (GTK_LIST_STORE (model), &niter);
  } else {
    gtk_list_store_insert_before (GTK_LIST_STORE (model), &niter, &iter);
  }

  switch (confui->tablecurr) {
    case CONFUI_ID_DANCE: {
      dance_t     *dances;
      ilistidx_t  dkey;

      dances = bdjvarsdfGet (BDJVDF_DANCES);
      /* CONTEXT: configuration: dance name that is set when adding a new dance */
      dkey = danceAdd (dances, _("New Dance"));
      /* CONTEXT: configuration: dance name that is set when adding a new dance */
      confuiDanceSet (GTK_LIST_STORE (model), &niter, _("New Dance"), dkey);
      break;
    }

    case CONFUI_ID_GENRES: {
      /* CONTEXT: configuration: genre name that is set when adding a new genre */
      confuiGenreSet (GTK_LIST_STORE (model), &niter, TRUE, _("New Genre"), 0);
      break;
    }

    case CONFUI_ID_RATINGS: {
      /* CONTEXT: configuration: rating name that is set when adding a new rating */
      confuiRatingSet (GTK_LIST_STORE (model), &niter, TRUE, _("New Rating"), 0);
      break;
    }

    case CONFUI_ID_LEVELS: {
      /* CONTEXT: configuration: level name that is set when adding a new level */
      confuiLevelSet (GTK_LIST_STORE (model), &niter, TRUE, _("New Level"), 0, 0);
      break;
    }

    case CONFUI_ID_STATUS: {
      /* CONTEXT: configuration: status name that is set when adding a new status */
      confuiStatusSet (GTK_LIST_STORE (model), &niter, TRUE, _("New Status"), 0);
      break;
    }

    default: {
      break;
    }
  }

  path = gtk_tree_model_get_path (model, &niter);
  gtk_tree_selection_select_path (
      confui->tables [confui->tablecurr].sel, path);
  if (confui->tablecurr == CONFUI_ID_DANCE) {
    confuiDanceSelect (GTK_TREE_VIEW (tree), path, NULL, confui);
  }
  gtk_tree_path_free (path);

  confui->tables [confui->tablecurr].changed = true;
  confui->tables [confui->tablecurr].currcount += 1;
  logProcEnd (LOG_PROC, "confuiTableAdd", "");
  return UICB_CONT;
}

static bool
confuiSwitchTable (void *udata, long pagenum)
{
  configui_t        *confui = udata;
  GtkWidget         *tree;
  confuiident_t     newid;

  logProcBegin (LOG_PROC, "confuiSwitchTable");
  if ((newid = (confuiident_t) uiutilsNotebookIDGet (confui->nbtabid, pagenum)) < 0) {
    logProcEnd (LOG_PROC, "confuiSwitchTable", "bad-pagenum");
    return UICB_STOP;
  }

  if (confui->tablecurr == newid) {
    logProcEnd (LOG_PROC, "confuiSwitchTable", "same-id");
    return UICB_CONT;
  }

  confuiSetStatusMsg (confui, "");

  confui->tablecurr = (confuiident_t) uiutilsNotebookIDGet (
      confui->nbtabid, pagenum);

  if (confui->tablecurr == CONFUI_ID_MOBILE_MQ) {
    confuiUpdateMobmqQrcode (confui);
  }
  if (confui->tablecurr == CONFUI_ID_REM_CONTROL) {
    confuiUpdateRemctrlQrcode (confui);
  }
  if (confui->tablecurr == CONFUI_ID_ORGANIZATION) {
    confuiUpdateOrgExamples (confui, bdjoptGetStr (OPT_G_AO_PATHFMT));
  }
  if (confui->tablecurr == CONFUI_ID_DISP_SEL_LIST) {
    /* be sure to create the listing first */
    confuiCreateTagListingDisp (confui);
    confuiCreateTagSelectedDisp (confui);
  }

  if (confui->tablecurr >= CONFUI_ID_TABLE_MAX) {
    logProcEnd (LOG_PROC, "confuiSwitchTable", "non-table");
    return UICB_CONT;
  }

  tree = confui->tables [confui->tablecurr].tree;
  if (tree == NULL) {
    logProcEnd (LOG_PROC, "confuiSwitchTable", "no-tree");
    return UICB_CONT;
  }

  confuiTableSetDefaultSelection (confui, tree,
      confui->tables [confui->tablecurr].sel);

  logProcEnd (LOG_PROC, "confuiSwitchTable", "");
  return UICB_CONT;
}

static void
confuiTableSetDefaultSelection (configui_t *confui, GtkWidget *tree,
    GtkTreeSelection *sel)
{
  int               count;
  GtkTreeIter       iter;
  GtkTreeModel      *model;


  count = uiTreeViewGetSelection (tree, &model, &iter);
  if (count != 1) {
    GtkTreePath   *path;

    path = gtk_tree_path_new_from_string ("0");
    if (path != NULL) {
      gtk_tree_selection_select_path (sel, path);
    }
    if (confui->tablecurr == CONFUI_ID_DANCE) {
      confuiDanceSelect (GTK_TREE_VIEW (tree), path, NULL, confui);
    }
    if (path != NULL) {
      gtk_tree_path_free (path);
    }
  }
}

static void
confuiCreateDanceTable (configui_t *confui)
{
  GtkTreeIter       iter;
  GtkListStore      *store = NULL;
  GtkCellRenderer   *renderer = NULL;
  GtkTreeViewColumn *column = NULL;
  slistidx_t        iteridx;
  ilistidx_t        key;
  dance_t           *dances;
  GtkWidget         *tree;
  slist_t           *dancelist;


  logProcBegin (LOG_PROC, "confuiCreateDanceTable");

  dances = bdjvarsdfGet (BDJVDF_DANCES);

  store = gtk_list_store_new (CONFUI_DANCE_COL_MAX,
      G_TYPE_STRING, G_TYPE_STRING, G_TYPE_LONG);
  assert (store != NULL);

  dancelist = danceGetDanceList (dances);
  slistStartIterator (dancelist, &iteridx);
  while ((key = slistIterateValueNum (dancelist, &iteridx)) >= 0) {
    char        *dancedisp;

    dancedisp = danceGetStr (dances, key, DANCE_DANCE);

    gtk_list_store_append (store, &iter);
    confuiDanceSet (store, &iter, dancedisp, key);
    confui->tables [CONFUI_ID_DANCE].currcount += 1;
  }

  tree = confui->tables [CONFUI_ID_DANCE].tree;
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tree), FALSE);

  renderer = gtk_cell_renderer_text_new ();
  g_object_set_data (G_OBJECT (renderer), "confuicolumn",
      GUINT_TO_POINTER (CONFUI_DANCE_COL_DANCE));
  column = gtk_tree_view_column_new_with_attributes ("", renderer,
      "text", CONFUI_DANCE_COL_DANCE,
      NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("", renderer,
      "text", CONFUI_DANCE_COL_SB_PAD,
      NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

  gtk_tree_view_set_model (GTK_TREE_VIEW (tree), GTK_TREE_MODEL (store));
  g_object_unref (store);
  logProcEnd (LOG_PROC, "confuiCreateDanceTable", "");
}

static void
confuiDanceSet (GtkListStore *store, GtkTreeIter *iter,
    char *dancedisp, ilistidx_t key)
{
  logProcBegin (LOG_PROC, "confuiDanceSet");
  gtk_list_store_set (store, iter,
      CONFUI_DANCE_COL_DANCE, dancedisp,
      CONFUI_DANCE_COL_SB_PAD, "    ",
      CONFUI_DANCE_COL_DANCE_IDX, (glong) key,
      -1);
  logProcEnd (LOG_PROC, "confuiDanceSet", "");
}

static void
confuiCreateRatingTable (configui_t *confui)
{
  GtkTreeIter       iter;
  GtkListStore      *store = NULL;
  GtkCellRenderer   *renderer = NULL;
  GtkTreeViewColumn *column = NULL;
  ilistidx_t        iteridx;
  ilistidx_t        key;
  rating_t          *ratings;
  GtkWidget         *tree;
  int               editable;

  logProcBegin (LOG_PROC, "confuiCreateRatingTable");

  ratings = bdjvarsdfGet (BDJVDF_RATINGS);

  store = gtk_list_store_new (CONFUI_RATING_COL_MAX,
      G_TYPE_LONG, G_TYPE_LONG, G_TYPE_STRING,
      G_TYPE_LONG, G_TYPE_OBJECT, G_TYPE_LONG);
  assert (store != NULL);

  ratingStartIterator (ratings, &iteridx);

  editable = FALSE;
  while ((key = ratingIterate (ratings, &iteridx)) >= 0) {
    char    *ratingdisp;
    long weight;

    ratingdisp = ratingGetRating (ratings, key);
    weight = ratingGetWeight (ratings, key);

    gtk_list_store_append (store, &iter);
    confuiRatingSet (store, &iter, editable, ratingdisp, weight);
    /* all cells other than the very first (Unrated) are editable */
    editable = TRUE;
    confui->tables [CONFUI_ID_RATINGS].currcount += 1;
  }

  tree = confui->tables [CONFUI_ID_RATINGS].tree;

  renderer = gtk_cell_renderer_text_new ();
  g_object_set_data (G_OBJECT (renderer), "confuicolumn",
      GUINT_TO_POINTER (CONFUI_RATING_COL_RATING));
  column = gtk_tree_view_column_new_with_attributes ("", renderer,
      "text", CONFUI_RATING_COL_RATING,
      "editable", CONFUI_RATING_COL_R_EDITABLE,
      NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  /* CONTEXT: configuration: rating: title of the rating name column */
  gtk_tree_view_column_set_title (column, _("Rating"));
  g_signal_connect (renderer, "edited", G_CALLBACK (confuiTableEditText), confui);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

  renderer = gtk_cell_renderer_spin_new ();
  gtk_cell_renderer_set_alignment (renderer, 1.0, 0.5);
  g_object_set_data (G_OBJECT (renderer), "confuicolumn",
      GUINT_TO_POINTER (CONFUI_RATING_COL_WEIGHT));
  column = gtk_tree_view_column_new_with_attributes ("", renderer,
      "text", CONFUI_RATING_COL_WEIGHT,
      "editable", CONFUI_RATING_COL_W_EDITABLE,
      "adjustment", CONFUI_RATING_COL_ADJUST,
      "digits", CONFUI_RATING_COL_DIGITS,
      NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  /* CONTEXT: configuration: rating: title of the weight column */
  gtk_tree_view_column_set_title (column, _("Weight"));
  g_signal_connect (renderer, "edited", G_CALLBACK (confuiTableEditSpinbox), confui);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

  gtk_tree_view_set_model (GTK_TREE_VIEW (tree), GTK_TREE_MODEL (store));
  g_object_unref (store);
  logProcEnd (LOG_PROC, "confuiCreateRatingTable", "");
}

static void
confuiRatingSet (GtkListStore *store, GtkTreeIter *iter,
    int editable, char *ratingdisp, long weight)
{
  GtkAdjustment     *adjustment;

  logProcBegin (LOG_PROC, "confuiRatingSet");
  adjustment = gtk_adjustment_new (weight, 0.0, 100.0, 1.0, 5.0, 0.0);
  gtk_list_store_set (store, iter,
      CONFUI_RATING_COL_R_EDITABLE, editable,
      CONFUI_RATING_COL_W_EDITABLE, TRUE,
      CONFUI_RATING_COL_RATING, ratingdisp,
      CONFUI_RATING_COL_WEIGHT, weight,
      CONFUI_RATING_COL_ADJUST, adjustment,
      CONFUI_RATING_COL_DIGITS, 0,
      -1);
  logProcEnd (LOG_PROC, "confuiRatingSet", "");
}

static void
confuiCreateStatusTable (configui_t *confui)
{
  GtkTreeIter       iter;
  GtkListStore      *store = NULL;
  GtkCellRenderer   *renderer = NULL;
  GtkTreeViewColumn *column = NULL;
  ilistidx_t        iteridx;
  ilistidx_t        key;
  status_t          *status;
  GtkWidget         *tree;
  int               editable;

  logProcBegin (LOG_PROC, "confuiCreateStatusTable");

  status = bdjvarsdfGet (BDJVDF_STATUS);

  store = gtk_list_store_new (CONFUI_STATUS_COL_MAX,
      G_TYPE_LONG, G_TYPE_STRING, G_TYPE_BOOLEAN);
  assert (store != NULL);

  statusStartIterator (status, &iteridx);

  editable = FALSE;
  while ((key = statusIterate (status, &iteridx)) >= 0) {
    char    *statusdisp;
    long playflag;

    statusdisp = statusGetStatus (status, key);
    playflag = statusGetPlayFlag (status, key);

    if (key == statusGetCount (status) - 1) {
      editable = FALSE;
    }

    gtk_list_store_append (store, &iter);
    confuiStatusSet (store, &iter, editable, statusdisp, playflag);
    /* all cells other than the very first (Unrated) are editable */
    editable = TRUE;
    confui->tables [CONFUI_ID_STATUS].currcount += 1;
  }

  tree = confui->tables [CONFUI_ID_STATUS].tree;

  renderer = gtk_cell_renderer_text_new ();
  g_object_set_data (G_OBJECT (renderer), "confuicolumn",
      GUINT_TO_POINTER (CONFUI_STATUS_COL_STATUS));
  column = gtk_tree_view_column_new_with_attributes ("", renderer,
      "text", CONFUI_STATUS_COL_STATUS,
      "editable", CONFUI_STATUS_COL_EDITABLE,
      NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  /* CONTEXT: configuration: status: title of the status name column */
  gtk_tree_view_column_set_title (column, _("Status"));
  g_signal_connect (renderer, "edited", G_CALLBACK (confuiTableEditText), confui);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

  renderer = gtk_cell_renderer_toggle_new ();
  g_signal_connect (G_OBJECT(renderer), "toggled",
      G_CALLBACK (confuiTableToggle), confui);
  column = gtk_tree_view_column_new_with_attributes ("", renderer,
      "active", CONFUI_STATUS_COL_PLAY_FLAG,
      NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  /* CONTEXT: configuration: status: title of the "playable" column */
  gtk_tree_view_column_set_title (column, _("Play?"));
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

  gtk_tree_view_set_model (GTK_TREE_VIEW (tree), GTK_TREE_MODEL (store));
  g_object_unref (store);
  logProcEnd (LOG_PROC, "confuiCreateStatusTable", "");
}

static void
confuiStatusSet (GtkListStore *store, GtkTreeIter *iter,
    int editable, char *statusdisp, int playflag)
{
  logProcBegin (LOG_PROC, "confuiStatusSet");
  gtk_list_store_set (store, iter,
      CONFUI_STATUS_COL_EDITABLE, editable,
      CONFUI_STATUS_COL_STATUS, statusdisp,
      CONFUI_STATUS_COL_PLAY_FLAG, playflag,
      -1);
  logProcEnd (LOG_PROC, "confuiStatusSet", "");
}

static void
confuiCreateLevelTable (configui_t *confui)
{
  GtkTreeIter       iter;
  GtkListStore      *store = NULL;
  GtkCellRenderer   *renderer = NULL;
  GtkTreeViewColumn *column = NULL;
  ilistidx_t        iteridx;
  ilistidx_t        key;
  level_t           *levels;
  GtkWidget         *tree;

  logProcBegin (LOG_PROC, "confuiCreateLevelTable");

  levels = bdjvarsdfGet (BDJVDF_LEVELS);

  store = gtk_list_store_new (CONFUI_LEVEL_COL_MAX,
      G_TYPE_LONG, G_TYPE_STRING, G_TYPE_LONG, G_TYPE_BOOLEAN,
      G_TYPE_OBJECT, G_TYPE_LONG);
  assert (store != NULL);

  levelStartIterator (levels, &iteridx);

  while ((key = levelIterate (levels, &iteridx)) >= 0) {
    char    *leveldisp;
    long weight;
    long def;

    leveldisp = levelGetLevel (levels, key);
    weight = levelGetWeight (levels, key);
    def = levelGetDefault (levels, key);
    if (def) {
      confui->tables [CONFUI_ID_LEVELS].radiorow = key;
    }

    gtk_list_store_append (store, &iter);
    confuiLevelSet (store, &iter, TRUE, leveldisp, weight, def);
    /* all cells other than the very first (Unrated) are editable */
    confui->tables [CONFUI_ID_LEVELS].currcount += 1;
  }

  tree = confui->tables [CONFUI_ID_LEVELS].tree;

  renderer = gtk_cell_renderer_text_new ();
  g_object_set_data (G_OBJECT (renderer), "confuicolumn",
      GUINT_TO_POINTER (CONFUI_LEVEL_COL_LEVEL));
  column = gtk_tree_view_column_new_with_attributes ("", renderer,
      "text", CONFUI_LEVEL_COL_LEVEL,
      "editable", CONFUI_LEVEL_COL_EDITABLE,
      NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  /* CONTEXT: configuration: level: title of the level name column */
  gtk_tree_view_column_set_title (column, _("Level"));
  g_signal_connect (renderer, "edited", G_CALLBACK (confuiTableEditText), confui);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

  renderer = gtk_cell_renderer_spin_new ();
  gtk_cell_renderer_set_alignment (renderer, 1.0, 0.5);
  g_object_set_data (G_OBJECT (renderer), "confuicolumn",
      GUINT_TO_POINTER (CONFUI_LEVEL_COL_WEIGHT));
  column = gtk_tree_view_column_new_with_attributes ("", renderer,
      "text", CONFUI_LEVEL_COL_WEIGHT,
      "editable", CONFUI_LEVEL_COL_EDITABLE,
      "adjustment", CONFUI_LEVEL_COL_ADJUST,
      "digits", CONFUI_LEVEL_COL_DIGITS,
      NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  /* CONTEXT: configuration: level: title of the weight column */
  gtk_tree_view_column_set_title (column, _("Weight"));
  g_signal_connect (renderer, "edited", G_CALLBACK (confuiTableEditSpinbox), confui);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

  renderer = gtk_cell_renderer_toggle_new ();
  g_signal_connect (G_OBJECT(renderer), "toggled",
      G_CALLBACK (confuiTableRadioToggle), confui);
  gtk_cell_renderer_toggle_set_radio (GTK_CELL_RENDERER_TOGGLE (renderer), TRUE);
  column = gtk_tree_view_column_new_with_attributes ("", renderer,
      "active", CONFUI_LEVEL_COL_DEFAULT,
      NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  /* CONTEXT: configuration: level: title of the default selection column */
  gtk_tree_view_column_set_title (column, _("Default"));
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

  gtk_tree_view_set_model (GTK_TREE_VIEW (tree), GTK_TREE_MODEL (store));
  g_object_unref (store);
  logProcEnd (LOG_PROC, "confuiCreateLevelTable", "");
}

static void
confuiLevelSet (GtkListStore *store, GtkTreeIter *iter,
    int editable, char *leveldisp, long weight, int def)
{
  GtkAdjustment     *adjustment;

  logProcBegin (LOG_PROC, "confuiLevelSet");
  adjustment = gtk_adjustment_new (weight, 0.0, 100.0, 1.0, 5.0, 0.0);
  gtk_list_store_set (store, iter,
      CONFUI_LEVEL_COL_EDITABLE, editable,
      CONFUI_LEVEL_COL_LEVEL, leveldisp,
      CONFUI_LEVEL_COL_WEIGHT, weight,
      CONFUI_LEVEL_COL_ADJUST, adjustment,
      CONFUI_LEVEL_COL_DIGITS, 0,
      CONFUI_LEVEL_COL_DEFAULT, def,
      -1);
  logProcEnd (LOG_PROC, "confuiLevelSet", "");
}


static void
confuiCreateGenreTable (configui_t *confui)
{
  GtkTreeIter       iter;
  GtkListStore      *store = NULL;
  GtkCellRenderer   *renderer = NULL;
  GtkTreeViewColumn *column = NULL;
  ilistidx_t        iteridx;
  ilistidx_t        key;
  genre_t           *genres;
  GtkWidget         *tree;

  logProcBegin (LOG_PROC, "confuiCreateGenreTable");

  genres = bdjvarsdfGet (BDJVDF_GENRES);

  store = gtk_list_store_new (CONFUI_GENRE_COL_MAX,
      G_TYPE_LONG, G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_STRING);
  assert (store != NULL);

  genreStartIterator (genres, &iteridx);

  while ((key = genreIterate (genres, &iteridx)) >= 0) {
    char    *genredisp;
    long    clflag;

    genredisp = genreGetGenre (genres, key);
    clflag = genreGetClassicalFlag (genres, key);

    gtk_list_store_append (store, &iter);
    confuiGenreSet (store, &iter, TRUE, genredisp, clflag);
    confui->tables [CONFUI_ID_GENRES].currcount += 1;
  }

  tree = confui->tables [CONFUI_ID_GENRES].tree;

  renderer = gtk_cell_renderer_text_new ();
  g_object_set_data (G_OBJECT (renderer), "confuicolumn",
      GUINT_TO_POINTER (CONFUI_GENRE_COL_GENRE));
  column = gtk_tree_view_column_new_with_attributes ("", renderer,
      "text", CONFUI_GENRE_COL_GENRE,
      "editable", CONFUI_GENRE_COL_EDITABLE,
      NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  /* CONTEXT: configuration: genre: title of the genre name column */
  gtk_tree_view_column_set_title (column, _("Genre"));
  g_signal_connect (renderer, "edited", G_CALLBACK (confuiTableEditText), confui);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

  renderer = gtk_cell_renderer_toggle_new ();
  g_signal_connect (G_OBJECT(renderer), "toggled",
      G_CALLBACK (confuiTableToggle), confui);
  column = gtk_tree_view_column_new_with_attributes ("", renderer,
      "active", CONFUI_GENRE_COL_CLASSICAL,
      NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  /* CONTEXT: configuration: genre: title of the classical setting column */
  gtk_tree_view_column_set_title (column, _("Classical?"));
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("", renderer,
      "text", CONFUI_GENRE_COL_SB_PAD,
      NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

  gtk_tree_view_set_model (GTK_TREE_VIEW (tree), GTK_TREE_MODEL (store));
  g_object_unref (store);
  logProcEnd (LOG_PROC, "confuiCreateGenreTable", "");
}


static void
confuiGenreSet (GtkListStore *store, GtkTreeIter *iter,
    int editable, char *genredisp, int clflag)
{
  logProcBegin (LOG_PROC, "confuiGenreSet");
  gtk_list_store_set (store, iter,
      CONFUI_GENRE_COL_EDITABLE, editable,
      CONFUI_GENRE_COL_GENRE, genredisp,
      CONFUI_GENRE_COL_CLASSICAL, clflag,
      -1);
  logProcEnd (LOG_PROC, "confuiGenreSet", "");
}

static void
confuiTableToggle (GtkCellRendererToggle *renderer, gchar *spath, gpointer udata)
{
  configui_t    *confui = udata;
  gboolean      val;
  GtkTreeIter   iter;
  GtkTreePath   *path;
  GtkTreeModel  *model;
  int           col;

  logProcBegin (LOG_PROC, "confuiTableToggle");
  model = gtk_tree_view_get_model (
      GTK_TREE_VIEW (confui->tables [confui->tablecurr].tree));
  path = gtk_tree_path_new_from_string (spath);
  if (path != NULL) {
    if (gtk_tree_model_get_iter (model, &iter, path) == FALSE) {
      logProcEnd (LOG_PROC, "confuiTableToggle", "no model/iter");
      return;
    }
    col = confui->tables [confui->tablecurr].togglecol;
    gtk_tree_model_get (model, &iter, col, &val, -1);
    gtk_list_store_set (GTK_LIST_STORE (model), &iter, col, !val, -1);
    gtk_tree_path_free (path);
  }
  confui->tables [confui->tablecurr].changed = true;
  logProcEnd (LOG_PROC, "confuiTableToggle", "");
}

static void
confuiTableRadioToggle (GtkCellRendererToggle *renderer, gchar *path, gpointer udata)
{
  configui_t    *confui = udata;
  GtkTreeIter   iter;
  GtkTreeModel  *model;
  GtkListStore  *store;
  char          tmp [40];
  int           col;
  int           row;

  logProcBegin (LOG_PROC, "confuiTableRadioToggle");
  model = gtk_tree_view_get_model (
      GTK_TREE_VIEW (confui->tables [confui->tablecurr].tree));

  store = GTK_LIST_STORE (model);
  col = confui->tables [confui->tablecurr].togglecol;
  row = confui->tables [confui->tablecurr].radiorow;
  snprintf (tmp, sizeof (tmp), "%d", row);

  if (gtk_tree_model_get_iter_from_string (model, &iter, path)) {
    gtk_list_store_set (store, &iter, col, 1, -1);
  }

  if (gtk_tree_model_get_iter_from_string (model, &iter, tmp)) {
    gtk_list_store_set (store, &iter, col, 0, -1);
  }

  sscanf (path, "%d", &row);
  confui->tables [confui->tablecurr].radiorow = row;
  confui->tables [confui->tablecurr].changed = true;
  logProcEnd (LOG_PROC, "confuiTableRadioToggle", "");
}

static void
confuiTableEditText (GtkCellRendererText* r, const gchar* path,
    const gchar* ntext, gpointer udata)
{
  configui_t    *confui = udata;

  logProcBegin (LOG_PROC, "confuiTableEditText");
  confuiTableEdit (confui, r, path, ntext, CONFUI_TABLE_TEXT);
  logProcEnd (LOG_PROC, "confuiTableEditText", "");
}

static void
confuiTableEditSpinbox (GtkCellRendererText* r, const gchar* path,
    const gchar* ntext, gpointer udata)
{
  configui_t    *confui = udata;

  logProcBegin (LOG_PROC, "confuiTableEditSpinbox");
  confuiTableEdit (confui, r, path, ntext, CONFUI_TABLE_NUM);
  logProcEnd (LOG_PROC, "confuiTableEditSpinbox", "");
}

static void
confuiTableEdit (configui_t *confui, GtkCellRendererText* r,
    const gchar* path, const gchar* ntext, int type)
{
  GtkWidget     *tree;
  GtkTreeModel  *model;
  GtkTreeIter   iter;
  int           col;

  logProcBegin (LOG_PROC, "confuiTableEdit");
  tree = confui->tables [confui->tablecurr].tree;
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree));
  gtk_tree_model_get_iter_from_string (model, &iter, path);
  col = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (r), "confuicolumn"));
  if (type == CONFUI_TABLE_TEXT) {
    gtk_list_store_set (GTK_LIST_STORE (model), &iter, col, ntext, -1);
  }
  if (type == CONFUI_TABLE_NUM) {
    long val = atol (ntext);
    gtk_list_store_set (GTK_LIST_STORE (model), &iter, col, val, -1);
  }
  confui->tables [confui->tablecurr].changed = true;
  logProcEnd (LOG_PROC, "confuiTableEdit", "");
}

static void
confuiTableSave (configui_t *confui, confuiident_t id)
{
  GtkWidget     *tree;
  GtkTreeModel  *model;
  savefunc_t    savefunc;
  char          tbuff [40];

  logProcBegin (LOG_PROC, "confuiTableSave");
  if (confui->tables [id].changed == false) {
    logProcEnd (LOG_PROC, "confuiTableSave", "not-changed");
    return;
  }
  if (confui->tables [id].savefunc == NULL) {
    logProcEnd (LOG_PROC, "confuiTableSave", "no-savefunc");
    return;
  }

  tree = confui->tables [id].tree;
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree));
  if (confui->tables [id].listcreatefunc != NULL) {
    snprintf (tbuff, sizeof (tbuff), "cu-table-save-%d", id);
    confui->tables [id].savelist = ilistAlloc (tbuff, LIST_ORDERED);
    confui->tables [id].saveidx = 0;
    gtk_tree_model_foreach (model, confui->tables [id].listcreatefunc, confui);
  }
  savefunc = confui->tables [id].savefunc;
  savefunc (confui);
  logProcEnd (LOG_PROC, "confuiTableSave", "");
}

static gboolean
confuiRatingListCreate (GtkTreeModel *model, GtkTreePath *path,
    GtkTreeIter *iter, gpointer udata)
{
  configui_t  *confui = udata;
  char        *ratingdisp;
  long        weight;

  logProcBegin (LOG_PROC, "confuiRatingListCreate");
  gtk_tree_model_get (model, iter,
      CONFUI_RATING_COL_RATING, &ratingdisp,
      CONFUI_RATING_COL_WEIGHT, &weight,
      -1);
  ilistSetStr (confui->tables [CONFUI_ID_RATINGS].savelist,
      confui->tables [CONFUI_ID_RATINGS].saveidx, RATING_RATING, ratingdisp);
  ilistSetNum (confui->tables [CONFUI_ID_RATINGS].savelist,
      confui->tables [CONFUI_ID_RATINGS].saveidx, RATING_WEIGHT, weight);
  free (ratingdisp);
  confui->tables [CONFUI_ID_RATINGS].saveidx += 1;
  logProcEnd (LOG_PROC, "confuiRatingListCreate", "");
  return FALSE;
}

static gboolean
confuiLevelListCreate (GtkTreeModel *model, GtkTreePath *path,
    GtkTreeIter *iter, gpointer udata)
{
  configui_t  *confui = udata;
  char        *leveldisp;
  long        weight;
  gboolean    def;

  logProcBegin (LOG_PROC, "confuiLevelListCreate");
  gtk_tree_model_get (model, iter,
      CONFUI_LEVEL_COL_LEVEL, &leveldisp,
      CONFUI_LEVEL_COL_WEIGHT, &weight,
      CONFUI_LEVEL_COL_DEFAULT, &def,
      -1);
  ilistSetStr (confui->tables [CONFUI_ID_LEVELS].savelist,
      confui->tables [CONFUI_ID_LEVELS].saveidx, LEVEL_LEVEL, leveldisp);
  ilistSetNum (confui->tables [CONFUI_ID_LEVELS].savelist,
      confui->tables [CONFUI_ID_LEVELS].saveidx, LEVEL_WEIGHT, weight);
  ilistSetNum (confui->tables [CONFUI_ID_LEVELS].savelist,
      confui->tables [CONFUI_ID_LEVELS].saveidx, LEVEL_DEFAULT_FLAG, def);
  free (leveldisp);
  confui->tables [CONFUI_ID_LEVELS].saveidx += 1;
  logProcEnd (LOG_PROC, "confuiLevelListCreate", "");
  return FALSE;
}

static gboolean
confuiStatusListCreate (GtkTreeModel *model, GtkTreePath *path,
    GtkTreeIter *iter, gpointer udata)
{
  configui_t  *confui = udata;
  char        *statusdisp;
  gboolean    playflag;

  logProcBegin (LOG_PROC, "confuiStatusListCreate");
  gtk_tree_model_get (model, iter,
      CONFUI_STATUS_COL_STATUS, &statusdisp,
      CONFUI_STATUS_COL_PLAY_FLAG, &playflag,
      -1);
  ilistSetStr (confui->tables [CONFUI_ID_STATUS].savelist,
      confui->tables [CONFUI_ID_STATUS].saveidx, STATUS_STATUS, statusdisp);
  ilistSetNum (confui->tables [CONFUI_ID_STATUS].savelist,
      confui->tables [CONFUI_ID_STATUS].saveidx, STATUS_PLAY_FLAG, playflag);
  free (statusdisp);
  confui->tables [CONFUI_ID_STATUS].saveidx += 1;
  logProcEnd (LOG_PROC, "confuiStatusListCreate", "");
  return FALSE;
}

static gboolean
confuiGenreListCreate (GtkTreeModel *model, GtkTreePath *path,
    GtkTreeIter *iter, gpointer udata)
{
  configui_t  *confui = udata;
  char        *genredisp;
  gboolean    clflag;

  logProcBegin (LOG_PROC, "confuiGenreListCreate");
  gtk_tree_model_get (model, iter,
      CONFUI_GENRE_COL_GENRE, &genredisp,
      CONFUI_GENRE_COL_CLASSICAL, &clflag,
      -1);
  ilistSetStr (confui->tables [CONFUI_ID_GENRES].savelist,
      confui->tables [CONFUI_ID_GENRES].saveidx, GENRE_GENRE, genredisp);
  ilistSetNum (confui->tables [CONFUI_ID_GENRES].savelist,
      confui->tables [CONFUI_ID_GENRES].saveidx, GENRE_CLASSICAL_FLAG, clflag);
  confui->tables [CONFUI_ID_GENRES].saveidx += 1;
  logProcEnd (LOG_PROC, "confuiGenreListCreate", "");
  return FALSE;
}

static void
confuiDanceSave (configui_t *confui)
{
  dance_t   *dances;

  logProcBegin (LOG_PROC, "confuiDanceSave");

  if (confui->tables [CONFUI_ID_DANCE].changed == false) {
    logProcEnd (LOG_PROC, "confuiTableSave", "not-changed");
    return;
  }

  dances = bdjvarsdfGet (BDJVDF_DANCES);
  /* the data is already saved in the dance list; just re-use it */
  danceSave (dances, NULL);
  logProcEnd (LOG_PROC, "confuiDanceSave", "");
}

static void
confuiRatingSave (configui_t *confui)
{
  rating_t    *ratings;

  logProcBegin (LOG_PROC, "confuiRatingSave");
  ratings = bdjvarsdfGet (BDJVDF_RATINGS);
  ratingSave (ratings, confui->tables [CONFUI_ID_RATINGS].savelist);
  ilistFree (confui->tables [CONFUI_ID_RATINGS].savelist);
  logProcEnd (LOG_PROC, "confuiRatingSave", "");
}

static void
confuiLevelSave (configui_t *confui)
{
  level_t    *levels;

  logProcBegin (LOG_PROC, "confuiLevelSave");
  levels = bdjvarsdfGet (BDJVDF_LEVELS);
  levelSave (levels, confui->tables [CONFUI_ID_LEVELS].savelist);
  ilistFree (confui->tables [CONFUI_ID_LEVELS].savelist);
  logProcEnd (LOG_PROC, "confuiLevelSave", "");
}

static void
confuiStatusSave (configui_t *confui)
{
  status_t    *status;

  logProcBegin (LOG_PROC, "confuiStatusSave");
  status = bdjvarsdfGet (BDJVDF_STATUS);
  statusSave (status, confui->tables [CONFUI_ID_STATUS].savelist);
  ilistFree (confui->tables [CONFUI_ID_STATUS].savelist);
  logProcEnd (LOG_PROC, "confuiStatusSave", "");
}

static void
confuiGenreSave (configui_t *confui)
{
  genre_t    *genres;

  logProcBegin (LOG_PROC, "confuiGenreSave");
  genres = bdjvarsdfGet (BDJVDF_GENRES);
  genreSave (genres, confui->tables [CONFUI_ID_GENRES].savelist);
  ilistFree (confui->tables [CONFUI_ID_GENRES].savelist);
  logProcEnd (LOG_PROC, "confuiGenreSave", "");
}

static void
confuiDanceSelect (GtkTreeView *tv, GtkTreePath *path,
    GtkTreeViewColumn *column, gpointer udata)
{
  configui_t    *confui = udata;
  GtkTreeIter   iter;
  GtkTreeModel  *model = NULL;
  unsigned long idx = 0;
  ilistidx_t    key;
  int           widx;
  char          *sval;
  nlistidx_t    num;
  slist_t       *slist;
  datafileconv_t conv;
  dance_t       *dances;

  logProcBegin (LOG_PROC, "confuiDanceSelect");
  confui->indancechange = true;

  if (path == NULL) {
    confui->indancechange = false;
    return;
  }

  model = gtk_tree_view_get_model (tv);

  if (! gtk_tree_model_get_iter (model, &iter, path)) {
    logProcEnd (LOG_PROC, "confuiDanceSelect", "no model/iter");
    confui->indancechange = false;
    return;
  }
  gtk_tree_model_get (model, &iter, CONFUI_DANCE_COL_DANCE_IDX, &idx, -1);
  key = (ilistidx_t) idx;

  dances = bdjvarsdfGet (BDJVDF_DANCES);

  sval = danceGetStr (dances, key, DANCE_DANCE);
  widx = CONFUI_ENTRY_DANCE_DANCE;
  uiEntrySetValue (confui->uiitem [widx].entry, sval);

  slist = danceGetList (dances, key, DANCE_TAGS);
  conv.allocated = false;
  conv.list = slist;
  conv.valuetype = VALUE_LIST;
  convTextList (&conv);
  sval = conv.str;
  widx = CONFUI_ENTRY_DANCE_TAGS;
  uiEntrySetValue (confui->uiitem [widx].entry, sval);
  if (conv.allocated) {
    free (conv.str);
    sval = NULL;
  }

  sval = danceGetStr (dances, key, DANCE_ANNOUNCE);
  widx = CONFUI_ENTRY_DANCE_ANNOUNCEMENT;
  uiEntrySetValue (confui->uiitem [widx].entry, sval);

  num = danceGetNum (dances, key, DANCE_HIGH_BPM);
  widx = CONFUI_SPINBOX_DANCE_HIGH_BPM;
  uiSpinboxSetValue (&confui->uiitem [widx].uiwidget, num);

  num = danceGetNum (dances, key, DANCE_LOW_BPM);
  widx = CONFUI_SPINBOX_DANCE_LOW_BPM;
  uiSpinboxSetValue (&confui->uiitem [widx].uiwidget, num);

  num = danceGetNum (dances, key, DANCE_SPEED);
  widx = CONFUI_SPINBOX_DANCE_SPEED;
  uiSpinboxTextSetValue (confui->uiitem [widx].spinbox, num);

  num = danceGetNum (dances, key, DANCE_TIMESIG);
  widx = CONFUI_SPINBOX_DANCE_TIME_SIG;
  uiSpinboxTextSetValue (confui->uiitem [widx].spinbox, num);

  num = danceGetNum (dances, key, DANCE_TYPE);
  widx = CONFUI_SPINBOX_DANCE_TYPE;
  uiSpinboxTextSetValue (confui->uiitem [widx].spinbox, num);

  confui->indancechange = false;
  logProcEnd (LOG_PROC, "confuiDanceSelect", "");
}

static int
confuiDanceEntryDanceChg (uientry_t *entry, void *udata)
{
  return confuiDanceEntryChg (entry, udata, CONFUI_ENTRY_DANCE_DANCE);
}

static int
confuiDanceEntryTagsChg (uientry_t *entry, void *udata)
{
  return confuiDanceEntryChg (entry, udata, CONFUI_ENTRY_DANCE_TAGS);
}

static int
confuiDanceEntryAnnouncementChg (uientry_t *entry, void *udata)
{
  return confuiDanceEntryChg (entry, udata, CONFUI_ENTRY_DANCE_ANNOUNCEMENT);
}

static int
confuiDanceEntryChg (uientry_t *entry, void *udata, int widx)
{
  configui_t      *confui = udata;
  const char      *str;
  GtkWidget       *tree;
  GtkTreeModel    *model;
  GtkTreeIter     iter;
  int             count;
  long            key;
  dance_t         *dances;
  int             didx;
  datafileconv_t  conv;
  int             entryrc = UIENTRY_ERROR;

  logProcBegin (LOG_PROC, "confuiDanceEntryChg");
  if (confui->indancechange) {
    logProcEnd (LOG_PROC, "confuiDanceEntryChg", "in-dance-select");
    return UIENTRY_OK;
  }

  str = uiEntryGetValue (entry);
  if (str == NULL) {
    logProcEnd (LOG_PROC, "confuiDanceEntryChg", "null-string");
    return UIENTRY_OK;
  }

  didx = confui->uiitem [widx].danceidx;

  tree = confui->tables [CONFUI_ID_DANCE].tree;
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree));
  count = uiTreeViewGetSelection (tree, &model, &iter);
  if (count != 1) {
    logProcEnd (LOG_PROC, "confuiDanceEntryChg", "no-selection");
    return UIENTRY_OK;
  }

  dances = bdjvarsdfGet (BDJVDF_DANCES);
  gtk_tree_model_get (model, &iter, CONFUI_DANCE_COL_DANCE_IDX, &key, -1);

  if (widx == CONFUI_ENTRY_DANCE_DANCE) {
    gtk_list_store_set (GTK_LIST_STORE (model), &iter,
        CONFUI_DANCE_COL_DANCE, str,
        -1);
    danceSetStr (dances, key, didx, str);
    entryrc = UIENTRY_OK;
  }
  if (widx == CONFUI_ENTRY_DANCE_ANNOUNCEMENT) {
    entryrc = confuiDanceValidateAnnouncement (entry, confui);
    if (entryrc == UIENTRY_OK) {
      danceSetStr (dances, key, didx, str);
    }
  }
  if (widx == CONFUI_ENTRY_DANCE_TAGS) {
    slist_t *slist;

    conv.allocated = true;
    conv.str = strdup (str);
    conv.valuetype = VALUE_STR;
    convTextList (&conv);
    slist = conv.list;
    danceSetList (dances, key, didx, slist);
    entryrc = UIENTRY_OK;
  }
  if (entryrc == UIENTRY_OK) {
    confui->tables [confui->tablecurr].changed = true;
  }
  logProcEnd (LOG_PROC, "confuiDanceEntryChg", "");
  return entryrc;
}

static bool
confuiDanceSpinboxTypeChg (void *udata)
{
  confuiDanceSpinboxChg (udata, CONFUI_SPINBOX_DANCE_TYPE);
  return UICB_CONT;
}

static bool
confuiDanceSpinboxSpeedChg (void *udata)
{
  confuiDanceSpinboxChg (udata, CONFUI_SPINBOX_DANCE_SPEED);
  return UICB_CONT;
}

static bool
confuiDanceSpinboxLowBPMChg (void *udata)
{
  confuiDanceSpinboxChg (udata, CONFUI_SPINBOX_DANCE_LOW_BPM);
  return UICB_CONT;
}

static bool
confuiDanceSpinboxHighBPMChg (void *udata)
{
  confuiDanceSpinboxChg (udata, CONFUI_SPINBOX_DANCE_HIGH_BPM);
  return UICB_CONT;
}

static bool
confuiDanceSpinboxTimeSigChg (void *udata)
{
  confuiDanceSpinboxChg (udata, CONFUI_SPINBOX_DANCE_TIME_SIG);
  return UICB_CONT;
}

static void
confuiDanceSpinboxChg (void *udata, int widx)
{
  configui_t      *confui = udata;
  GtkWidget       *tree;
  GtkTreeModel    *model;
  GtkTreeIter     iter;
  int             count;
  long            idx;
  double          value;
  long            nval = 0;
  ilistidx_t      key;
  dance_t         *dances;
  int             didx;

  logProcBegin (LOG_PROC, "confuiDanceSpinboxChg");
  if (confui->indancechange) {
    logProcEnd (LOG_PROC, "confuiDanceSpinboxChg", "in-dance-select");
    return;
  }

  didx = confui->uiitem [widx].danceidx;

  if (confui->uiitem [widx].basetype == CONFUI_SPINBOX_TEXT) {
    /* text spinbox */
    nval = uiSpinboxTextGetValue (confui->uiitem [widx].spinbox);
  }
  if (confui->uiitem [widx].basetype == CONFUI_SPINBOX_NUM) {
    value = uiSpinboxGetValue (&confui->uiitem [widx].uiwidget);
    nval = (long) value;
  }

  tree = confui->tables [CONFUI_ID_DANCE].tree;
  count = uiTreeViewGetSelection (tree, &model, &iter);
  if (count != 1) {
    logProcEnd (LOG_PROC, "confuiDanceSpinboxChg", "no-selection");
    return;
  }

  dances = bdjvarsdfGet (BDJVDF_DANCES);
  gtk_tree_model_get (model, &iter, CONFUI_DANCE_COL_DANCE_IDX, &idx, -1);
  key = (ilistidx_t) idx;
  danceSetNum (dances, key, didx, nval);
  confui->tables [confui->tablecurr].changed = true;
  logProcEnd (LOG_PROC, "confuiDanceSpinboxChg", "");
}

static int
confuiDanceValidateAnnouncement (uientry_t *entry, configui_t *confui)
{
  int               rc;
  const char        *fn;
  char              tbuff [MAXPATHLEN];
  char              nfn [MAXPATHLEN];
  char              *musicdir;
  size_t            mlen;

  logProcBegin (LOG_PROC, "confuiDanceValidateAnnouncement");

  musicdir = bdjoptGetStr (OPT_M_DIR_MUSIC);
  mlen = strlen (musicdir);

  rc = UIENTRY_ERROR;
  fn = uiEntryGetValue (entry);
  if (fn == NULL) {
    logProcEnd (LOG_PROC, "confuiDanceValidateAnnouncement", "bad-fn");
    return UIENTRY_ERROR;
  }

  strlcpy (nfn, fn, sizeof (nfn));
  pathNormPath (nfn, sizeof (nfn));
  if (strncmp (musicdir, fn, mlen) == 0) {
    strlcpy (nfn, fn + mlen + 1, sizeof (nfn));
    uiEntrySetValue (entry, nfn);
  }

  if (*nfn == '\0') {
    rc = UIENTRY_OK;
  } else {
    *tbuff = '\0';
    if (*nfn != '/' && *(nfn + 1) != ':') {
      strlcpy (tbuff, musicdir, sizeof (tbuff));
      strlcat (tbuff, "/", sizeof (tbuff));
    }
    strlcat (tbuff, nfn, sizeof (tbuff));
    if (fileopFileExists (tbuff)) {
      rc = UIENTRY_OK;
    }
  }

  /* sanitizeaddress creates a buffer underflow error */
  /* if tablecurr is set to CONFUI_ID_NONE */
  /* also this validation routine gets called at most any time, but */
  /* the changed flag should only be set for the edit dance tab */
  if (confui->tablecurr == CONFUI_ID_DANCE) {
    confui->tables [confui->tablecurr].changed = true;
  }
  logProcEnd (LOG_PROC, "confuiDanceValidateAnnouncement", "");
  return rc;
}

/* display settings */

static bool
confuiDispSettingChg (void *udata)
{
  configui_t  *confui = udata;
  int         oselidx;
  int         nselidx;

  logProcBegin (LOG_PROC, "confuiDispSettingChg");


  oselidx = confui->uiitem [CONFUI_SPINBOX_DISP_SEL].listidx;
  nselidx = uiSpinboxTextGetValue (
      confui->uiitem [CONFUI_SPINBOX_DISP_SEL].spinbox);
  confui->uiitem [CONFUI_SPINBOX_DISP_SEL].listidx = nselidx;

  confuiDispSaveTable (confui, oselidx);
  /* be sure to create the listing first */
  confuiCreateTagListingDisp (confui);
  confuiCreateTagSelectedDisp (confui);
  logProcEnd (LOG_PROC, "confuiDispSettingChg", "");
  return UICB_CONT;
}

static void
confuiDispSaveTable (configui_t *confui, int selidx)
{
  slist_t       *tlist;
  slist_t       *nlist;
  slistidx_t    val;
  slistidx_t    iteridx;
  char          *tstr;

  logProcBegin (LOG_PROC, "confuiDispSaveTable");

  if (! uiduallistIsChanged (confui->dispselduallist)) {
    logProcEnd (LOG_PROC, "confuiDispSaveTable", "not-changed");
    return;
  }

  nlist = slistAlloc ("dispsel-save-tmp", LIST_UNORDERED, NULL);
  tlist = uiduallistGetList (confui->dispselduallist);
  slistStartIterator (tlist, &iteridx);
  while ((val = slistIterateValueNum (tlist, &iteridx)) >= 0) {
    tstr = tagdefs [val].tag;
    slistSetNum (nlist, tstr, 0);
  }

  dispselSave (confui->dispsel, selidx, nlist);

  slistFree (tlist);
  slistFree (nlist);
  logProcEnd (LOG_PROC, "confuiDispSaveTable", "");
}

static void
confuiCreateTagSelectedDisp (configui_t *confui)
{
  dispselsel_t  selidx;
  slist_t       *sellist;
  dispsel_t     *dispsel;

  logProcBegin (LOG_PROC, "confuiCreateTagSelectedDisp");


  selidx = uiSpinboxTextGetValue (
      confui->uiitem [CONFUI_SPINBOX_DISP_SEL].spinbox);

  dispsel = confui->dispsel;
  sellist = dispselGetList (dispsel, selidx);

  uiduallistSet (confui->dispselduallist, sellist, DUALLIST_TREE_TARGET);
  logProcEnd (LOG_PROC, "confuiCreateTagSelectedDisp", "");
}


static void
confuiCreateTagListingDisp (configui_t *confui)
{
  dispselsel_t  selidx;

  logProcBegin (LOG_PROC, "confuiCreateTagListingDisp");

  selidx = uiSpinboxTextGetValue (confui->uiitem [CONFUI_SPINBOX_DISP_SEL].spinbox);

  if (selidx == DISP_SEL_SONGEDIT_A ||
      selidx == DISP_SEL_SONGEDIT_B ||
      selidx == DISP_SEL_SONGEDIT_C) {
    uiduallistSet (confui->dispselduallist, confui->edittaglist,
        DUALLIST_TREE_SOURCE);
  } else {
    uiduallistSet (confui->dispselduallist, confui->listingtaglist,
        DUALLIST_TREE_SOURCE);
  }
  logProcEnd (LOG_PROC, "confuiCreateTagListingDisp", "");
}

static long
confuiValMSCallback (void *udata, const char *txt)
{
  configui_t  *confui = udata;
  const char  *valstr;
  char        tbuff [200];
  long        val;

  logProcBegin (LOG_PROC, "confuiValMSCallback");

  uiLabelSetText (&confui->statusMsg, "");
  valstr = validate (txt, VAL_MIN_SEC);
  if (valstr != NULL) {
    snprintf (tbuff, sizeof (tbuff), valstr, txt);
    uiLabelSetText (&confui->statusMsg, tbuff);
    return -1;
  }

  val = tmutilStrToMS (txt);
  logProcEnd (LOG_PROC, "confuiValMSCallback", "");
  return val;
}

