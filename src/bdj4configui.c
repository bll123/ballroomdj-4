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
#include "nlist.h"
#include "orgopt.h"
#include "orgutil.h"
#include "osuiutils.h"
#include "pathbld.h"
#include "pathutil.h"
#include "procutil.h"
#include "progstate.h"
#include "rating.h"
#include "slist.h"
#include "sock.h"
#include "sockh.h"
#include "status.h"
#include "sysvars.h"
#include "tagdef.h"
#include "templateutil.h"
#include "tmutil.h"
#include "uiutils.h"
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
  CONFUI_SPINBOX_DANCE_HIGH_BPM,
  CONFUI_SPINBOX_DANCE_LOW_BPM,
  CONFUI_SPINBOX_DANCE_SPEED,
  CONFUI_SPINBOX_DANCE_TIME_SIG,
  CONFUI_SPINBOX_DANCE_TYPE,
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
  CONFUI_WIDGET_AO_EXAMPLE_1,
  CONFUI_WIDGET_AO_EXAMPLE_2,
  CONFUI_WIDGET_AO_EXAMPLE_3,
  CONFUI_WIDGET_AO_EXAMPLE_4,
  CONFUI_WIDGET_AO_CHG_SPACE,
  CONFUI_WIDGET_AUTO_ORGANIZE,
  CONFUI_WIDGET_DB_LOAD_FROM_GENRE,
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
  CONFUI_WIDGET_DEFAULT_VOL,
  CONFUI_WIDGET_ENABLE_ITUNES,
  CONFUI_WIDGET_FADE_IN_TIME,
  CONFUI_WIDGET_FADE_OUT_TIME,
  CONFUI_WIDGET_GAP,
  CONFUI_WIDGET_HIDE_MARQUEE_ON_START,
  CONFUI_WIDGET_INSERT_LOC,
  CONFUI_WIDGET_MMQ_PORT,
  CONFUI_WIDGET_MMQ_QR_CODE,
  CONFUI_WIDGET_MQ_ACCENT_COLOR,
  CONFUI_WIDGET_MQ_FONT,
  CONFUI_WIDGET_MQ_FONT_FS,
  CONFUI_WIDGET_MQ_QUEUE_LEN,
  CONFUI_WIDGET_MQ_SHOW_SONG_INFO,
  CONFUI_WIDGET_PL_QUEUE_LEN,
  CONFUI_WIDGET_RC_ENABLE,
  CONFUI_WIDGET_RC_PORT,
  CONFUI_WIDGET_RC_QR_CODE,
  CONFUI_WIDGET_UI_ACCENT_COLOR,
  CONFUI_WIDGET_UI_FONT,
  CONFUI_WIDGET_UI_LISTING_FONT,
  CONFUI_ITEM_MAX,
};

typedef struct {
  confuibasetype_t  basetype;
  confuiouttype_t   outtype;
  int               bdjoptIdx;
  union {
    uiutilsdropdown_t dropdown;
    uiutilsentry_t    entry;
    uiutilsspinbox_t  spinbox;
    GtkWidget         *widget;
  } u;
  int                 listidx;    // for combobox, spinbox
  nlist_t             *list;
  nlist_t             *sblookuplist;
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

typedef struct {
  GtkWidget *tree;
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
  procutil_t        *processes [ROUTE_MAX];
  conn_t            *conn;
  sockserver_t      *sockserver;
  char              *localip;
  int               dbgflags;
  confuiitem_t      uiitem [CONFUI_ITEM_MAX];
  int               tabcount;
  confuiident_t     tablecurr;
  confuiident_t     *tableidents;
  slist_t           *listingtaglist;
  dispsel_t         *dispsel;
  /* gtk stuff */
  GtkApplication    *app;
  GtkWidget         *window;
  GtkWidget         *vbox;
  GtkWidget         *notebook;
  confuitable_t     tables [CONFUI_ID_TABLE_MAX];
  GtkWidget         *fadetypeImage;
  /* options */
  datafile_t        *optiondf;
  nlist_t           *options;
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

#define CONFUI_EXIT_WAIT_COUNT      20

static bool     confuiHandshakeCallback (void *udata, programstate_t programState);
static bool     confuiStoppingCallback (void *udata, programstate_t programState);
static bool     confuiClosingCallback (void *udata, programstate_t programState);
static void     confuiActivate (GApplication *app, gpointer userdata);
gboolean        confuiMainLoop  (void *tconfui);
static int      confuiProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
                    bdjmsgmsg_t msg, char *args, void *udata);
static gboolean confuiCloseWin (GtkWidget *window, GdkEvent *event, gpointer userdata);
static void     confuiSigHandler (int sig);

static void confuiPopulateOptions (configui_t *confui);
static void confuiSelectMusicDir (GtkButton *b, gpointer udata);
static void confuiSelectStartup (GtkButton *b, gpointer udata);
static void confuiSelectShutdown (GtkButton *b, gpointer udata);
static void confuiSelectAnnouncement (GtkButton *b, gpointer udata);
static void confuiSelectFileDialog (configui_t *confui, int widx, char *startpath, char *mimefiltername, char *mimetype);

/* gui */
static GtkWidget * confuiMakeNotebookTab (configui_t *confui, GtkWidget *notebook, char *txt, int);
/* makeitementry returns the hbox */
static GtkWidget * confuiMakeItemEntry (configui_t *confui, GtkWidget *vbox, GtkSizeGroup *sg, char *txt, int widx, int bdjoptIdx, char *disp);
static void confuiMakeItemEntryChooser (configui_t *confui, GtkWidget *vbox, GtkSizeGroup *sg, char *txt, int widx, int bdjoptIdx, char *disp, void *dialogFunc);
/* makeitemcombobox returns the hbox */
static GtkWidget * confuiMakeItemCombobox (configui_t *confui, GtkWidget *vbox, GtkSizeGroup *sg, char *txt, int widx, int bdjoptIdx, void *ddcb, char *value);
static void confuiMakeItemLink (configui_t *confui, GtkWidget *vbox, GtkSizeGroup *sg, char *txt, int widx, char *disp);
static void confuiMakeItemFontButton (configui_t *confui, GtkWidget *vbox, GtkSizeGroup *sg, char *txt, int widx, int bdjoptIdx, char *fontname);
static void confuiMakeItemColorButton (configui_t *confui, GtkWidget *vbox, GtkSizeGroup *sg, char *txt, int widx, int bdjoptIdx, char *color);
/* makeitemspinboxtext returns the widget */
static GtkWidget *confuiMakeItemSpinboxText (configui_t *confui, GtkWidget *vbox, GtkSizeGroup *sg, char *txt, int widx, int bdjoptIdx, ssize_t value);
static void confuiMakeItemSpinboxTime (configui_t *confui, GtkWidget *vbox, GtkSizeGroup *sg, char *txt, int widx, int bdjoptIdx, ssize_t value);
/* makeitemspinboxint returns the widget */
static GtkWidget *confuiMakeItemSpinboxInt (configui_t *confui, GtkWidget *vbox, GtkSizeGroup *sg, char *txt, int widx, int bdjoptIdx, int min, int max, int value);
static void confuiMakeItemSpinboxDouble (configui_t *confui, GtkWidget *vbox, GtkSizeGroup *sg, char *txt, int widx, int bdjoptIdx, double min, double max, double value);
/* makeitemswitch returns the widget */
static GtkWidget *confuiMakeItemSwitch (configui_t *confui, GtkWidget *vbox, GtkSizeGroup *sg, char *txt, int widx, int bdjoptIdx, int value);
static GtkWidget *confuiMakeItemLabelDisp (configui_t *confui, GtkWidget *vbox, GtkSizeGroup *sg, char *txt, int widx, int bdjoptIdx);
static void confuiMakeItemCheckButton (configui_t *confui, GtkWidget *vbox, GtkSizeGroup *sg, const char *txt, int widx, int bdjoptIdx, int value);
static GtkWidget * confuiMakeItemLabel (GtkWidget *vbox, GtkSizeGroup *sg, char *txt);
static GtkWidget * confuiMakeItemTable (configui_t *confui, GtkWidget *vbox, confuiident_t id, int flags);

/* misc */
static nlist_t  * confuiGetThemeList (void);
static nlist_t  * confuiGetThemeNames (nlist_t *themelist, slist_t *filelist);
static void     confuiLoadHTMLList (configui_t *confui);
static void     confuiLoadVolIntfcList (configui_t *confui);
static void     confuiLoadPlayerIntfcList (configui_t *confui);
static void     confuiLoadLocaleList (configui_t *confui);
static void     confuiLoadDanceTypeList (configui_t *confui);
static void     confuiLoadTagList (configui_t *confui);
static gboolean confuiFadeTypeTooltip (GtkWidget *, gint, gint, gboolean, GtkTooltip *, void *);
static void     confuiOrgPathSelect (GtkTreeView *tv, GtkTreePath *path,
    GtkTreeViewColumn *column, gpointer udata);
static char     * confuiComboboxSelect (configui_t *confui, GtkTreePath *path, int widx);
static void     confuiUpdateMobmqQrcode (configui_t *confui);
static void     confuiMobmqTypeChg (GtkSpinButton *sb, gpointer udata);
static void     confuiMobmqPortChg (GtkSpinButton *sb, gpointer udata);
static bool     confuiMobmqNameChg (void *edata, void *udata);
static bool     confuiMobmqTitleChg (void *edata, void *udata);
static void     confuiUpdateRemctrlQrcode (configui_t *confui);
static gboolean confuiRemctrlChg (GtkSwitch *sw, gboolean value, gpointer udata);
static void     confuiRemctrlPortChg (GtkSpinButton *sb, gpointer udata);
static char     * confuiMakeQRCodeFile (configui_t *confui, char *title, char *uri);
static void     confuiUpdateOrgExamples (configui_t *confui, char *pathfmt);
static void     confuiUpdateOrgExample (configui_t *config, org_t *org, char *data, GtkWidget *widget);
static char     * confuiGetLocalIP (configui_t *confui);
static int      confuiGetSelectionIter (GtkWidget *tree, GtkTreeModel **model, GtkTreeIter *iter);

/* table editing */
static void   confuiTableMoveUp (GtkButton *b, gpointer udata);
static void   confuiTableMoveDown (GtkButton *b, gpointer udata);
static void   confuiTableMove (configui_t *confui, int dir);
static void   confuiTableRemove (GtkButton *b, gpointer udata);
static void   confuiTableAdd (GtkButton *b, gpointer udata);
static void   confuiSwitchTable (GtkNotebook *nb, GtkWidget *page, guint pagenum, gpointer udata);
static void   confuiTableSetDefaultSelection (configui_t *confui, GtkWidget *tree);
static void   confuiCreateDanceTable (configui_t *confui);
static void   confuiDanceSet (GtkListStore *store, GtkTreeIter *iter, char *dancedisp, ilistidx_t key);
static void   confuiCreateRatingTable (configui_t *confui);
static void   confuiRatingSet (GtkListStore *store, GtkTreeIter *iter, int editable, char *ratingdisp, ssize_t weight);
static void   confuiCreateStatusTable (configui_t *confui);
static void   confuiStatusSet (GtkListStore *store, GtkTreeIter *iter, int editable, char *statusdisp, int playflag);
static void   confuiCreateLevelTable (configui_t *confui);
static void   confuiLevelSet (GtkListStore *store, GtkTreeIter *iter, int editable, char *leveldisp, ssize_t weight, int def);
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
static void   confuiDanceEntryChg (GtkEditable *e, gpointer udata);
static void   confuiDanceSpinboxChg (GtkSpinButton *sb, gpointer udata);
static bool   confuiDanceValidateAnnouncement (void *edata, void *udata);

/* display settings */
static void   confuiCreateTagListingTable (configui_t *confui);
static void   confuiDispSettingChg (GtkSpinButton *sb, gpointer udata);
static void   confuiDispSaveTable (configui_t *confui, int selidx);
static void   confuiCreateTagTableDisp (configui_t *confui);
static void   confuiCreateTagListingDisp (configui_t *confui);
static void   confuiDispSelect (GtkButton *b, gpointer udata);
static void   confuiDispRemove (GtkButton *b, gpointer udata);

static int gKillReceived = 0;
static int gdone = 0;

int
main (int argc, char *argv[])
{
  int             status = 0;
  uint16_t        listenPort;
  configui_t      confui;
  char            *uifont;
  char            tbuff [MAXPATHLEN];
  nlist_t         *tlist;
  nlist_t         *llist;
  nlistidx_t      iteridx;
  int             count;
  char            *p;
  volsinklist_t   sinklist;
  volume_t        *volume;
  orgopt_t        *orgopt;


  confui.notebook = NULL;
  confui.progstate = progstateInit ("configui");
  progstateSetCallback (confui.progstate, STATE_WAIT_HANDSHAKE,
      confuiHandshakeCallback, &confui);
  confui.sockserver = NULL;
  confui.window = NULL;
  confui.tabcount = 0;
  confui.tablecurr = CONFUI_ID_NONE;
  confui.tableidents = NULL;
  confui.dispsel = NULL;
  confui.listingtaglist = NULL;
  confui.localip = NULL;
  for (int i = 0; i < CONFUI_ID_TABLE_MAX; ++i) {
    confui.tables [i].tree = NULL;
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

  for (bdjmsgroute_t i = ROUTE_NONE; i < ROUTE_MAX; ++i) {
    confui.processes [i] = NULL;
  }

  for (int i = 0; i < CONFUI_ITEM_MAX; ++i) {
    confui.uiitem [i].list = NULL;
    confui.uiitem [i].u.widget = NULL;
    confui.uiitem [i].sblookuplist = NULL;
    confui.uiitem [i].basetype = CONFUI_NONE;
    confui.uiitem [i].outtype = CONFUI_OUT_NONE;
    confui.uiitem [i].bdjoptIdx = -1;
    confui.uiitem [i].listidx = 0;
    if (i > CONFUI_BEGIN && i < CONFUI_COMBOBOX_MAX) {
      uiutilsDropDownInit (&confui.uiitem [i].u.dropdown);
    }
    if (i > CONFUI_ENTRY_MAX && i < CONFUI_SPINBOX_MAX) {
      uiutilsSpinboxTextInit (&confui.uiitem [i].u.spinbox);
    }
  }

  uiutilsEntryInit (&confui.uiitem [CONFUI_ENTRY_DANCE_TAGS].u.entry, 30, 100);
  uiutilsEntryInit (&confui.uiitem [CONFUI_ENTRY_DANCE_ANNOUNCEMENT].u.entry, 50, 200);
  uiutilsEntryInit (&confui.uiitem [CONFUI_ENTRY_DANCE_DANCE].u.entry, 30, 50);
  uiutilsEntryInit (&confui.uiitem [CONFUI_ENTRY_MUSIC_DIR].u.entry, 50, 300);
  uiutilsEntryInit (&confui.uiitem [CONFUI_ENTRY_PROFILE_NAME].u.entry, 20, 30);
  uiutilsEntryInit (&confui.uiitem [CONFUI_ENTRY_STARTUP].u.entry, 50, 300);
  uiutilsEntryInit (&confui.uiitem [CONFUI_ENTRY_SHUTDOWN].u.entry, 50, 300);
  uiutilsEntryInit (&confui.uiitem [CONFUI_ENTRY_COMPLETE_MSG].u.entry, 20, 30);
  uiutilsEntryInit (&confui.uiitem [CONFUI_ENTRY_QUEUE_NM_A].u.entry, 20, 30);
  uiutilsEntryInit (&confui.uiitem [CONFUI_ENTRY_QUEUE_NM_B].u.entry, 20, 30);
  uiutilsEntryInit (&confui.uiitem [CONFUI_ENTRY_RC_USER_ID].u.entry, 10, 30);
  uiutilsEntryInit (&confui.uiitem [CONFUI_ENTRY_RC_PASS].u.entry, 10, 20);
  uiutilsEntryInit (&confui.uiitem [CONFUI_ENTRY_MM_NAME].u.entry, 10, 40);
  uiutilsEntryInit (&confui.uiitem [CONFUI_ENTRY_MM_TITLE].u.entry, 20, 100);

#if _define_SIGHUP
  procutilCatchSignal (confuiSigHandler, SIGHUP);
#endif
  procutilCatchSignal (confuiSigHandler, SIGINT);
  procutilDefaultSignal (SIGTERM);
#if _define_SIGCHLD
  procutilDefaultSignal (SIGCHLD);
#endif

  confui.dbgflags = bdj4startup (argc, argv, "cu", ROUTE_CONFIGUI, BDJ4_INIT_NONE);
  logProcBegin (LOG_PROC, "configui");

  confui.dispsel = dispselAlloc ();

  orgopt = orgoptAlloc ();
  assert (orgopt != NULL);
  tlist = orgoptGetList (orgopt);

  confui.uiitem [CONFUI_COMBOBOX_AO_PATHFMT].list = tlist;
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
  tlist = nlistAlloc ("cu-audio-out", LIST_UNORDERED, free);
  llist = nlistAlloc ("cu-audio-out-l", LIST_ORDERED, free);
  /* CONTEXT: audio: The default audio sink (output) */
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
  confui.uiitem [CONFUI_SPINBOX_AUDIO_OUTPUT].list = tlist;
  confui.uiitem [CONFUI_SPINBOX_AUDIO_OUTPUT].sblookuplist = llist;
  volumeFreeSinkList (&sinklist);
  volumeFree (volume);

  tlist = nlistAlloc ("cu-writetags", LIST_UNORDERED, free);
  /* order these in the same order as defined in bdjopt.h */
  /* CONTEXT: write tags: do not write any tags to the audio file */
  nlistSetStr (tlist, WRITE_TAGS_NONE, _("Don't Write"));
  /* CONTEXT: write tags: only write BDJ tags to the audio file */
  nlistSetStr (tlist, WRITE_TAGS_BDJ_ONLY, _("BDJ Tags Only"));
  /* CONTEXT: write tags: write all tags (BDJ and standard) to the audio file */
  nlistSetStr (tlist, WRITE_TAGS_ALL, _("All Tags"));
  confui.uiitem [CONFUI_SPINBOX_WRITE_AUDIO_FILE_TAGS].list = tlist;

  tlist = nlistAlloc ("cu-bpm", LIST_UNORDERED, free);
  /* order these in the same order as defined in bdjopt.h */
  /* CONTEXT: BPM: beats per minute (not bars per minute) */
  nlistSetStr (tlist, BPM_BPM, _("BPM"));
  /* CONTEXT: MPM: measures per minute (aka bars per minute) */
  nlistSetStr (tlist, BPM_MPM, _("MPM"));
  confui.uiitem [CONFUI_SPINBOX_BPM].list = tlist;

  tlist = nlistAlloc ("cu-fadetype", LIST_UNORDERED, free);
  /* order these in the same order as defined in bdjopt.h */
  /* CONTEXT: fade-out type */
  nlistSetStr (tlist, FADETYPE_TRIANGLE, _("Triangle"));
  /* CONTEXT: fade-out type */
  nlistSetStr (tlist, FADETYPE_QUARTER_SINE, _("Quarter Sine Wave"));
  /* CONTEXT: fade-out type */
  nlistSetStr (tlist, FADETYPE_HALF_SINE, _("Half Sine Wave"));
  /* CONTEXT: fade-out type */
  nlistSetStr (tlist, FADETYPE_LOGARITHMIC, _("Logarithmic"));
  /* CONTEXT: fade-out type */
  nlistSetStr (tlist, FADETYPE_INVERTED_PARABOLA, _("Inverted Parabola"));
  confui.uiitem [CONFUI_SPINBOX_FADE_TYPE].list = tlist;

  tlist = nlistAlloc ("cu-dance-speed", LIST_UNORDERED, free);
  /* order these in the same order as defined in dance.h */
  /* CONTEXT: dance speed */
  nlistSetStr (tlist, DANCE_SPEED_SLOW, _("slow"));
  /* CONTEXT: dance speed */
  nlistSetStr (tlist, DANCE_SPEED_NORMAL, _("normal"));
  /* CONTEXT: dance speed */
  nlistSetStr (tlist, DANCE_SPEED_FAST, _("fast"));
  confui.uiitem [CONFUI_SPINBOX_DANCE_SPEED].list = tlist;

  tlist = nlistAlloc ("cu-dance-time-sig", LIST_UNORDERED, free);
  /* order these in the same order as defined in dance.h */
  /* CONTEXT: dance time signature */
  nlistSetStr (tlist, DANCE_TIMESIG_24, _("2/4"));
  /* CONTEXT: dance time signature */
  nlistSetStr (tlist, DANCE_TIMESIG_34, _("3/4"));
  /* CONTEXT: dance time signature */
  nlistSetStr (tlist, DANCE_TIMESIG_44, _("4/4"));
  /* CONTEXT: dance time signature */
  nlistSetStr (tlist, DANCE_TIMESIG_48, _("4/8"));
  confui.uiitem [CONFUI_SPINBOX_DANCE_TIME_SIG].list = tlist;

  tlist = nlistAlloc ("cu-display-settings", LIST_UNORDERED, free);
  /* CONTEXT: display settings for: music manager */
  nlistSetStr (tlist, DISP_SEL_MM, _("Music Manager"));
  /* CONTEXT: display settings for: music queue */
  nlistSetStr (tlist, DISP_SEL_MUSICQ, _("Music Queue"));
  /* CONTEXT: display settings for: requests */
  nlistSetStr (tlist, DISP_SEL_REQUEST, _("Request"));
  /* CONTEXT: display settings for: song editor column 1 */
  nlistSetStr (tlist, DISP_SEL_SONGEDIT_A, _("Song Editor - Column 1"));
  /* CONTEXT: display settings for: song editor column 2 */
  nlistSetStr (tlist, DISP_SEL_SONGEDIT_B, _("Song Editor - Column 2"));
  /* CONTEXT: display settings for: song list */
  nlistSetStr (tlist, DISP_SEL_SONGLIST, _("Song List"));
  /* CONTEXT: display settings for: song selection */
  nlistSetStr (tlist, DISP_SEL_SONGSEL, _("Song Selection"));
  confui.uiitem [CONFUI_SPINBOX_DISP_SEL].list = tlist;

  confuiLoadHTMLList (&confui);
  confuiLoadVolIntfcList (&confui);
  confuiLoadPlayerIntfcList (&confui);
  confuiLoadLocaleList (&confui);
  confuiLoadDanceTypeList (&confui);
  confuiLoadTagList (&confui);

  tlist = confuiGetThemeList ();
  nlistStartIterator (tlist, &iteridx);
  count = 0;
  while ((p = nlistIterateValueData (tlist, &iteridx)) != NULL) {
    if (strcmp (p, bdjoptGetStr (OPT_MP_MQ_THEME)) == 0) {
      confui.uiitem [CONFUI_SPINBOX_MQ_THEME].listidx = count;
    }
    if (strcmp (p, bdjoptGetStr (OPT_MP_UI_THEME)) == 0) {
      confui.uiitem [CONFUI_SPINBOX_UI_THEME].listidx = count;
    }
    ++count;
  }
  confui.uiitem [CONFUI_SPINBOX_UI_THEME].list = tlist;
  confui.uiitem [CONFUI_SPINBOX_UI_THEME].sblookuplist = tlist;
  confui.uiitem [CONFUI_SPINBOX_MQ_THEME].list = tlist;
  confui.uiitem [CONFUI_SPINBOX_MQ_THEME].sblookuplist = tlist;

  tlist = nlistAlloc ("cu-mob-mq", LIST_UNORDERED, free);
  /* order these in the same order as defined in bdjopt.h */
  /* CONTEXT: mobile marquee: off */
  nlistSetStr (tlist, MOBILEMQ_OFF, _("Off"));
  /* CONTEXT: mobile marquee: use local router */
  nlistSetStr (tlist, MOBILEMQ_LOCAL, _("Local"));
  /* CONTEXT: mobile marquee: route via the internet */
  nlistSetStr (tlist, MOBILEMQ_INTERNET, _("Internet"));
  confui.uiitem [CONFUI_SPINBOX_MOBILE_MQ].list = tlist;

  listenPort = bdjvarsGetNum (BDJVL_CONFIGUI_PORT);
  confui.conn = connInit (ROUTE_CONFIGUI);

  pathbldMakePath (tbuff, sizeof (tbuff),
      "configui", ".txt", PATHBLD_MP_USEIDX);
  confui.optiondf = datafileAllocParse ("configui-opt", DFTYPE_KEY_VAL, tbuff,
      configuidfkeys, CONFUI_KEY_MAX, DATAFILE_NO_LOOKUP);
  confui.options = datafileGetList (confui.optiondf);
  if (confui.options == NULL) {
    confui.options = nlistAlloc ("configui-opt", LIST_ORDERED, free);

    nlistSetNum (confui.options, CONFUI_POSITION_X, -1);
    nlistSetNum (confui.options, CONFUI_POSITION_Y, -1);
    nlistSetNum (confui.options, CONFUI_SIZE_X, 1200);
    nlistSetNum (confui.options, CONFUI_SIZE_Y, 800);
  }

  /* register these after calling the sub-window initialization */
  /* then these will be run last, after the other closing callbacks */
  progstateSetCallback (confui.progstate, STATE_STOPPING,
      confuiStoppingCallback, &confui);
  progstateSetCallback (confui.progstate, STATE_CLOSING,
      confuiClosingCallback, &confui);

  confui.sockserver = sockhStartServer (listenPort);

  uiutilsInitUILog ();
  gtk_init (&argc, NULL);
  uifont = bdjoptGetStr (OPT_MP_UIFONT);
  uiutilsSetUIFont (uifont);

  g_timeout_add (UI_MAIN_LOOP_TIMER, confuiMainLoop, &confui);

  status = uiutilsCreateApplication (0, NULL, "configui",
      &confui.app, confuiActivate, &confui);

  while (progstateShutdownProcess (confui.progstate) != STATE_CLOSED) {
    ;
  }
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
  gint          x, y;
  char          fn [MAXPATHLEN];

  logProcBegin (LOG_PROC, "confuiStoppingCallback");

  confuiPopulateOptions (confui);
  bdjoptSave ();
  for (confuiident_t i = 0; i < CONFUI_ID_TABLE_MAX; ++i) {
    confuiTableSave (confui, i);
  }

  gtk_window_get_size (GTK_WINDOW (confui->window), &x, &y);
  nlistSetNum (confui->options, CONFUI_SIZE_X, x);
  nlistSetNum (confui->options, CONFUI_SIZE_Y, y);
  gtk_window_get_position (GTK_WINDOW (confui->window), &x, &y);
  nlistSetNum (confui->options, CONFUI_POSITION_X, x);
  nlistSetNum (confui->options, CONFUI_POSITION_Y, y);

  pathbldMakePath (fn, sizeof (fn),
      "configui", ".txt", PATHBLD_MP_USEIDX);
  datafileSaveKeyVal ("configui", fn, configuidfkeys, CONFUI_KEY_MAX, confui->options);

  for (bdjmsgroute_t i = ROUTE_NONE; i < ROUTE_MAX; ++i) {
    if (confui->processes [i] != NULL) {
      procutilStopProcess (confui->processes [i], confui->conn, i, false);
    }
  }

  gdone = 1;
  connDisconnectAll (confui->conn);
  logProcEnd (LOG_PROC, "confuiStoppingCallback", "");
  return true;
}

static bool
confuiClosingCallback (void *udata, programstate_t programState)
{
  configui_t    *confui = udata;

  logProcBegin (LOG_PROC, "confuiClosingCallback");

  if (GTK_IS_WIDGET (confui->window)) {
    gtk_widget_destroy (confui->window);
  }

  for (int i = CONFUI_BEGIN + 1; i < CONFUI_COMBOBOX_MAX; ++i) {
    uiutilsDropDownFree (&confui->uiitem [i].u.dropdown);
  }

  for (int i = CONFUI_COMBOBOX_MAX + 1; i < CONFUI_ENTRY_MAX; ++i) {
    uiutilsEntryFree (&confui->uiitem [i].u.entry);
  }

  for (int i = CONFUI_ENTRY_MAX + 1; i < CONFUI_SPINBOX_MAX; ++i) {
    uiutilsSpinboxTextFree (&confui->uiitem [i].u.spinbox);
    /* the mq and ui-theme share both list and sblookuplist */
    /* ui-theme > mq-theme */
    if (i == CONFUI_SPINBOX_UI_THEME) {
      continue;
    }
    if (confui->uiitem [i].list != NULL) {
      nlistFree (confui->uiitem [i].list);
    }
    /* mq-theme and dance-type share the list and sblookuplist */
    if (i != CONFUI_SPINBOX_MQ_THEME &&
        i != CONFUI_SPINBOX_DANCE_TYPE &&
        confui->uiitem [i].sblookuplist != NULL) {
      nlistFree (confui->uiitem [i].sblookuplist);
    }
    confui->uiitem [i].list = NULL;
  }

  dispselFree (confui->dispsel);
  if (confui->localip != NULL) {
    free (confui->localip);
  }
  if (confui->options != datafileGetList (confui->optiondf)) {
    nlistFree (confui->options);
  }
  datafileFree (confui->optiondf);
  if (confui->tableidents != NULL) {
    free (confui->tableidents);
  }
  if (confui->listingtaglist != NULL) {
    slistFree (confui->listingtaglist);
  }

  sockhCloseServer (confui->sockserver);
  bdj4shutdown (ROUTE_CONFIGUI);

  /* give the other processes some time to shut down */
  mssleep (200);
  for (bdjmsgroute_t i = ROUTE_NONE; i < ROUTE_MAX; ++i) {
    if (confui->processes [i] != NULL) {
      procutilStopProcess (confui->processes [i], confui->conn, i, true);
      procutilFree (confui->processes [i]);
    }
  }

  connFree (confui->conn);
  uiutilsCleanup ();

  logProcEnd (LOG_PROC, "confuiClosingCallback", "");
  return true;
}

static void
confuiActivate (GApplication *app, gpointer userdata)
{
  configui_t    *confui = userdata;
  GError        *gerr = NULL;
  GtkWidget     *vbox;
  GtkWidget     *widget;
  GtkWidget     *tree;
  GtkWidget     *image;
  GtkSizeGroup  *sg;
  GtkWidget     *dvbox;
  GtkWidget     *hbox;
  char          imgbuff [MAXPATHLEN];
  char          tbuff [MAXPATHLEN];
  gint          x, y;
  ssize_t       val;

  logProcBegin (LOG_PROC, "confuiActivate");

  pathbldMakePath (imgbuff, sizeof (imgbuff),
      "bdj4_icon_config", ".svg", PATHBLD_MP_IMGDIR);

  confui->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  assert (confui->window != NULL);
  gtk_window_set_application (GTK_WINDOW (confui->window), GTK_APPLICATION (app));
  gtk_window_set_application (GTK_WINDOW (confui->window), confui->app);
  gtk_window_set_default_icon_from_file (imgbuff, &gerr);
  g_signal_connect (confui->window, "delete-event", G_CALLBACK (confuiCloseWin), confui);
  /* CONTEXT: configuration ui window title */
  snprintf (tbuff, sizeof (tbuff), _("%s Configuration"), BDJ4_NAME);
  gtk_window_set_title (GTK_WINDOW (confui->window), tbuff);

  confui->vbox = uiutilsCreateVertBox ();
  uiutilsBoxSetMargins (confui->vbox, 4);
  gtk_container_add (GTK_CONTAINER (confui->window), confui->vbox);

  confui->notebook = uiutilsCreateNotebook ();
  gtk_notebook_set_tab_pos (GTK_NOTEBOOK (confui->notebook), GTK_POS_LEFT);
  assert (confui->notebook != NULL);
  gtk_box_pack_start (GTK_BOX (confui->vbox), confui->notebook, TRUE, TRUE, 0);

  /* general options */
  vbox = confuiMakeNotebookTab (confui, confui->notebook,
      /* CONTEXT: config: general options that apply to everything */
      _("General Options"), CONFUI_ID_NONE);
  sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  strlcpy (tbuff, bdjoptGetStr (OPT_M_DIR_MUSIC), sizeof (tbuff));
  if (isWindows ()) {
    pathWinPath (tbuff, sizeof (tbuff));
  }

  /* CONTEXT: config: the music folder where the user store their music */
  confuiMakeItemEntryChooser (confui, vbox, sg, _("Music Folder"),
      CONFUI_ENTRY_MUSIC_DIR, OPT_M_DIR_MUSIC,
      tbuff, confuiSelectMusicDir);
  uiutilsEntrySetValidate (&confui->uiitem [CONFUI_ENTRY_MUSIC_DIR].u.entry,
      uiutilsEntryValidateDir, confui);

  /* CONTEXT: config: the name of this profile */
  confuiMakeItemEntry (confui, vbox, sg, _("Profile Name"),
      CONFUI_ENTRY_PROFILE_NAME, OPT_P_PROFILENAME,
      bdjoptGetStr (OPT_P_PROFILENAME));

  /* CONTEXT: config: Whether to display BPM as BPM or MPM */
  confuiMakeItemSpinboxText (confui, vbox, sg, _("BPM"),
      CONFUI_SPINBOX_BPM, OPT_G_BPM,
      bdjoptGetNum (OPT_G_BPM));

  /* database */
  /* CONTEXT: config: which audio tags will be written to the audio file */
  confuiMakeItemSpinboxText (confui, vbox, sg, _("Write Audio File Tags"),
      CONFUI_SPINBOX_WRITE_AUDIO_FILE_TAGS, OPT_G_WRITETAGS,
      bdjoptGetNum (OPT_G_WRITETAGS));
  confuiMakeItemSwitch (confui, vbox, sg,
      _("Database Loads Dance From Genre"),
      CONFUI_WIDGET_DB_LOAD_FROM_GENRE, OPT_G_LOADDANCEFROMGENRE,
      bdjoptGetNum (OPT_G_LOADDANCEFROMGENRE));
  confuiMakeItemSwitch (confui, vbox, sg,
      /* CONTEXT: config: enable itunes support */
      _("Enable iTunes Support"),
      CONFUI_WIDGET_ENABLE_ITUNES, OPT_G_ITUNESSUPPORT,
      bdjoptGetNum (OPT_G_ITUNESSUPPORT));

  /* bdj4 */
  /* CONTEXT: config: the locale to use (e.g. English or Nederlands */
  confuiMakeItemSpinboxText (confui, vbox, sg, _("Locale"),
      CONFUI_SPINBOX_LOCALE, -1,
      confui->uiitem [CONFUI_SPINBOX_LOCALE].listidx);
  /* CONTEXT: config: the startup script to run before starting the player.  Used on Linux. */
  confuiMakeItemEntryChooser (confui, vbox, sg, _("Startup Script"),
      CONFUI_ENTRY_STARTUP, OPT_M_STARTUPSCRIPT,
      bdjoptGetStr (OPT_M_STARTUPSCRIPT), confuiSelectStartup);
  uiutilsEntrySetValidate (&confui->uiitem [CONFUI_ENTRY_STARTUP].u.entry,
      uiutilsEntryValidateFile, confui);

  /* CONTEXT: config: the shutdown script to run before starting the player.  Used on Linux. */
  confuiMakeItemEntryChooser (confui, vbox, sg, _("Shutdown Script"),
      CONFUI_ENTRY_SHUTDOWN, OPT_M_SHUTDOWNSCRIPT,
      bdjoptGetStr (OPT_M_SHUTDOWNSCRIPT), confuiSelectShutdown);
  uiutilsEntrySetValidate (&confui->uiitem [CONFUI_ENTRY_SHUTDOWN].u.entry,
      uiutilsEntryValidateFile, confui);

  /* player options */
  vbox = confuiMakeNotebookTab (confui, confui->notebook,
      /* CONTEXT: config: options associated with the player */
      _("Player Options"), CONFUI_ID_NONE);
  sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  /* CONTEXT: config: which player interface to use */
  confuiMakeItemSpinboxText (confui, vbox, sg, _("Player"),
      CONFUI_SPINBOX_PLAYER, OPT_M_PLAYER_INTFC,
      confui->uiitem [CONFUI_SPINBOX_PLAYER].listidx);
  /* CONTEXT: config: which audio interface to use */
  confuiMakeItemSpinboxText (confui, vbox, sg, _("Audio"),
      CONFUI_SPINBOX_VOL_INTFC, OPT_M_VOLUME_INTFC,
      confui->uiitem [CONFUI_SPINBOX_VOL_INTFC].listidx);
  /* CONTEXT: config: which audio sink (output) to use */
  confuiMakeItemSpinboxText (confui, vbox, sg, _("Audio Output"),
      CONFUI_SPINBOX_AUDIO_OUTPUT, OPT_M_AUDIOSINK,
      confui->uiitem [CONFUI_SPINBOX_AUDIO_OUTPUT].listidx);
  /* CONTEXT: config: the volume used when starting the player */
  confuiMakeItemSpinboxInt (confui, vbox, sg, _("Default Volume"),
      CONFUI_WIDGET_DEFAULT_VOL, OPT_P_DEFAULTVOLUME,
      10, 100, bdjoptGetNum (OPT_P_DEFAULTVOLUME));
  /* CONTEXT: config: the amount of time to do a volume fade-in when playing a song */
  confuiMakeItemSpinboxDouble (confui, vbox, sg, _("Fade In Time"),
      CONFUI_WIDGET_FADE_IN_TIME, OPT_P_FADEINTIME,
      0.0, 2.0, (double) bdjoptGetNum (OPT_P_FADEINTIME) / 1000.0);
  /* CONTEXT: config: the amount of time to do a volume fade-out when playing a song */
  confuiMakeItemSpinboxDouble (confui, vbox, sg, _("Fade Out Time"),
      CONFUI_WIDGET_FADE_OUT_TIME, OPT_P_FADEOUTTIME,
      0.0, 10.0, (double) bdjoptGetNum (OPT_P_FADEOUTTIME) / 1000.0);

  /* CONTEXT: config: the type of fade */
  widget = confuiMakeItemSpinboxText (confui, vbox, sg, _("Fade Type"),
      CONFUI_SPINBOX_FADE_TYPE, OPT_P_FADETYPE,
      bdjoptGetNum (OPT_P_FADETYPE));
  pathbldMakePath (tbuff, sizeof (tbuff),  "fades", ".svg",
      PATHBLD_MP_IMGDIR);
  image = gtk_image_new_from_file (tbuff);
  gtk_widget_set_margin_top (image, 2);
  gtk_widget_set_margin_bottom (image, 2);
  gtk_widget_set_margin_start (image, 2);
  gtk_widget_set_margin_end (image, 2);
  gtk_widget_set_size_request (image, 275, -1);
  confui->fadetypeImage = image;
  gtk_widget_set_has_tooltip (widget, TRUE);
  g_signal_connect (widget, "query-tooltip", G_CALLBACK (confuiFadeTypeTooltip), confui);

  /* CONTEXT: config: the amount of time to wait inbetween songs */
  confuiMakeItemSpinboxDouble (confui, vbox, sg, _("Gap Between Songs"),
      CONFUI_WIDGET_GAP, OPT_P_GAP,
      0.0, 60.0, (double) bdjoptGetNum (OPT_P_GAP) / 1000.0);
  /* CONTEXT: config: the maximum amount of time to play a song */
  confuiMakeItemSpinboxTime (confui, vbox, sg, _("Maximum Play Time"),
      CONFUI_SPINBOX_MAX_PLAY_TIME, OPT_P_MAXPLAYTIME,
      bdjoptGetNum (OPT_P_MAXPLAYTIME));
  /* CONTEXT: config: the length of the music queue to display */
  confuiMakeItemSpinboxInt (confui, vbox, sg, _("Queue Length"),
      CONFUI_WIDGET_PL_QUEUE_LEN, OPT_G_PLAYERQLEN,
      20, 400, bdjoptGetNum (OPT_G_PLAYERQLEN));
  /* CONTEXT: config: where to insert a requested song into the music queue */
  confuiMakeItemSpinboxInt (confui, vbox, sg, _("Request Insert Location"),
      CONFUI_WIDGET_INSERT_LOC, OPT_P_INSERT_LOCATION,
      1, 10, bdjoptGetNum (OPT_P_INSERT_LOCATION));

  /* CONTEXT: config: The completion message displayed on the marquee when a playlist is finished */
  confuiMakeItemEntry (confui, vbox, sg, _("Completion Message"),
      CONFUI_ENTRY_COMPLETE_MSG, OPT_P_COMPLETE_MSG,
      bdjoptGetStr (OPT_P_COMPLETE_MSG));
  /* CONTEXT: config: The name of the first music queue (main) */
  confuiMakeItemEntry (confui, vbox, sg, _("Queue A Name"),
      CONFUI_ENTRY_QUEUE_NM_A, OPT_P_QUEUE_NAME_A,
      bdjoptGetStr (OPT_P_QUEUE_NAME_A));
  /* CONTEXT: config: The name of the second music queue */
  confuiMakeItemEntry (confui, vbox, sg, _("Queue B Name"),
      CONFUI_ENTRY_QUEUE_NM_B, OPT_P_QUEUE_NAME_B,
      bdjoptGetStr (OPT_P_QUEUE_NAME_B));

  /* marquee options */
  vbox = confuiMakeNotebookTab (confui, confui->notebook,
      /* CONTEXT: config: options associated with the marquee */
      _("Marquee Options"), CONFUI_ID_NONE);
  sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  /* CONTEXT: config: The theme to use for the marquee display */
  confuiMakeItemSpinboxText (confui, vbox, sg, _("Marquee Theme"),
      CONFUI_SPINBOX_MQ_THEME, OPT_MP_MQ_THEME,
      confui->uiitem [CONFUI_SPINBOX_MQ_THEME].listidx);
  /* CONTEXT: config: The font to use for the marquee display */
  confuiMakeItemFontButton (confui, vbox, sg, _("Marquee Font"),
      CONFUI_WIDGET_MQ_FONT, OPT_MP_MQFONT,
      bdjoptGetStr (OPT_MP_MQFONT));
  /* CONTEXT: config: the length of the queue displayed on the marquee */
  confuiMakeItemSpinboxInt (confui, vbox, sg, _("Queue Length"),
      CONFUI_WIDGET_MQ_QUEUE_LEN, OPT_P_MQQLEN,
      1, 20, bdjoptGetNum (OPT_P_MQQLEN));
  /* CONTEXT: config: marquee: show the song information (artist/title) on the marquee */
  confuiMakeItemSwitch (confui, vbox, sg, _("Show Song Information"),
      CONFUI_WIDGET_MQ_SHOW_SONG_INFO, OPT_P_MQ_SHOW_INFO,
      bdjoptGetNum (OPT_P_MQ_SHOW_INFO));
  /* CONTEXT: config: marquee: the accent color used for the marquee */
  confuiMakeItemColorButton (confui, vbox, sg, _("Accent Colour"),
      CONFUI_WIDGET_MQ_ACCENT_COLOR, OPT_P_MQ_ACCENT_COL,
      bdjoptGetStr (OPT_P_MQ_ACCENT_COL));
  /* CONTEXT: config: marquee: minimize the marquee when the player is started */
  confuiMakeItemSwitch (confui, vbox, sg, _("Hide Marquee on Start"),
      CONFUI_WIDGET_HIDE_MARQUEE_ON_START, OPT_P_HIDE_MARQUEE_ON_START,
      bdjoptGetNum (OPT_P_HIDE_MARQUEE_ON_START));

  /* user interface */
  vbox = confuiMakeNotebookTab (confui, confui->notebook,
      /* CONTEXT: config: options associated with the user interface */
      _("User Interface"), CONFUI_ID_NONE);
  sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  /* CONTEXT: config: the theme to use for the user interface */
  confuiMakeItemSpinboxText (confui, vbox, sg, _("Theme"),
      CONFUI_SPINBOX_UI_THEME, OPT_MP_UI_THEME,
      confui->uiitem [CONFUI_SPINBOX_UI_THEME].listidx);
  /* CONTEXT: config: the font to use for the user interface */
  confuiMakeItemFontButton (confui, vbox, sg, _("Font"),
      CONFUI_WIDGET_UI_FONT, OPT_MP_UIFONT,
      bdjoptGetStr (OPT_MP_UIFONT));
  /* CONTEXT: config: the font to use for the queues and song lists */
  confuiMakeItemFontButton (confui, vbox, sg, _("Listing Font"),
      CONFUI_WIDGET_UI_LISTING_FONT, OPT_MP_LISTING_FONT,
      bdjoptGetStr (OPT_MP_LISTING_FONT));
  /* CONTEXT: config: the accent color to use for the user interface */
  confuiMakeItemColorButton (confui, vbox, sg, _("Accent Colour"),
      CONFUI_WIDGET_UI_ACCENT_COLOR, OPT_P_UI_ACCENT_COL,
      bdjoptGetStr (OPT_P_UI_ACCENT_COL));

  /* display settings */
  vbox = confuiMakeNotebookTab (confui, confui->notebook,
      /* CONTEXT: config: change which fields are displayed in different contexts */
      _("Display Settings"), CONFUI_ID_DISP_SEL_LIST);
  gtk_widget_set_vexpand (vbox, TRUE);
  sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  /* CONTEXT: config: display settings: which set of display settings to update */
  widget = confuiMakeItemSpinboxText (confui, vbox, sg, _("Display"),
      CONFUI_SPINBOX_DISP_SEL, -1, 0);
  g_signal_connect (widget, "value-changed", G_CALLBACK (confuiDispSettingChg), confui);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_set_halign (hbox, GTK_ALIGN_START);
  gtk_widget_set_vexpand (hbox, FALSE);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  widget = uiutilsCreateScrolledWindow ();
  gtk_scrolled_window_set_min_content_height (GTK_SCROLLED_WINDOW (widget), 200);
  gtk_widget_set_vexpand (widget, TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

  tree = uiutilsCreateTreeView ();
  uiutilsSetCss (tree,
      "treeview { background-color: shade(@theme_base_color,0.8); } "
      "treeview:selected { background-color: @theme_selected_bg_color; } ");
  confui->tables [CONFUI_ID_DISP_SEL_LIST].tree = tree;
  confui->tables [CONFUI_ID_DISP_SEL_LIST].flags = CONFUI_TABLE_NONE;
  gtk_widget_set_margin_start (tree, 16);
  gtk_widget_set_margin_top (tree, 16);
  gtk_widget_set_vexpand (tree, TRUE);
  gtk_container_add (GTK_CONTAINER (widget), tree);

  dvbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_vexpand (dvbox, FALSE);
  gtk_widget_set_margin_start (dvbox, 8);
  gtk_widget_set_margin_end (dvbox, 8);
  gtk_widget_set_margin_top (dvbox, 128);
  gtk_widget_set_valign (dvbox, GTK_ALIGN_START);
  gtk_box_pack_start (GTK_BOX (hbox), dvbox, FALSE, FALSE, 0);

  /* CONTEXT: config: display settings: button: add the selected field */
  widget = uiutilsCreateButton (_("Select"), "button_right",
      confuiDispSelect, confui);
  gtk_box_pack_start (GTK_BOX (dvbox), widget, FALSE, FALSE, 0);

  /* CONTEXT: config: display settings: button: remove the selected field */
  widget = uiutilsCreateButton (_("Remove"), "button_left",
      confuiDispRemove, confui);
  gtk_box_pack_start (GTK_BOX (dvbox), widget, FALSE, FALSE, 0);

  widget = uiutilsCreateScrolledWindow ();
  gtk_scrolled_window_set_min_content_height (GTK_SCROLLED_WINDOW (widget), 200);
  gtk_widget_set_vexpand (widget, TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

  tree = uiutilsCreateTreeView ();
  uiutilsSetCss (tree,
      "treeview { background-color: shade(@theme_base_color,0.8); } "
      "treeview:selected { background-color: @theme_selected_bg_color; } ");
  confui->tables [CONFUI_ID_DISP_SEL_TABLE].tree = tree;
  confui->tables [CONFUI_ID_DISP_SEL_TABLE].flags = CONFUI_TABLE_NONE;
  gtk_widget_set_margin_start (tree, 16);
  gtk_widget_set_margin_top (tree, 16);
  gtk_widget_set_vexpand (tree, TRUE);
  gtk_container_add (GTK_CONTAINER (widget), tree);

  dvbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_vexpand (dvbox, FALSE);
  gtk_widget_set_margin_start (dvbox, 8);
  gtk_widget_set_margin_end (dvbox, 8);
  gtk_widget_set_margin_top (dvbox, 128);
  gtk_widget_set_valign (dvbox, GTK_ALIGN_START);
  gtk_box_pack_start (GTK_BOX (hbox), dvbox, FALSE, FALSE, 0);

  /* CONTEXT: config: display settings: button: move the selected field up */
  widget = uiutilsCreateButton (_("Move Up"), "button_up",
      confuiTableMoveUp, confui);
  gtk_box_pack_start (GTK_BOX (dvbox), widget, FALSE, FALSE, 0);

  /* CONTEXT: config: display settings: button: move the selected field down */
  widget = uiutilsCreateButton (_("Move Down"), "button_down",
      confuiTableMoveDown, confui);
  gtk_box_pack_start (GTK_BOX (dvbox), widget, FALSE, FALSE, 0);

  /* call this after both tree views have been instantiated */
  confuiCreateTagListingTable (confui);

  /* organization */
  vbox = confuiMakeNotebookTab (confui, confui->notebook,
      /* CONTEXT: config: options associated with how audio files are organized */
      _("Organisation"), CONFUI_ID_ORGANIZATION);
  sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  /* CONTEXT: config: the audio file organization path */
  confuiMakeItemCombobox (confui, vbox, sg, _("Organisation Path"),
      CONFUI_COMBOBOX_AO_PATHFMT, OPT_G_AO_PATHFMT,
      confuiOrgPathSelect, bdjoptGetStr (OPT_G_AO_PATHFMT));
  /* CONTEXT: config: examples displayed for the audio file organization path */
  confuiMakeItemLabelDisp (confui, vbox, sg, _("Examples"),
      CONFUI_WIDGET_AO_EXAMPLE_1, -1);
  confuiMakeItemLabelDisp (confui, vbox, sg, "",
      CONFUI_WIDGET_AO_EXAMPLE_2, -1);
  confuiMakeItemLabelDisp (confui, vbox, sg, "",
      CONFUI_WIDGET_AO_EXAMPLE_3, -1);
  confuiMakeItemLabelDisp (confui, vbox, sg, "",
      CONFUI_WIDGET_AO_EXAMPLE_4, -1);

  /* CONTEXT: config: when automatic organization is enabled */
  confuiMakeItemSwitch (confui, vbox, sg, _("Auto Organise"),
      CONFUI_WIDGET_AUTO_ORGANIZE, OPT_G_AUTOORGANIZE,
      bdjoptGetNum (OPT_G_AUTOORGANIZE));

  /* CONTEXT: config: remove spaces when renaming audio files */
  confuiMakeItemSwitch (confui, vbox, sg, _("Remove Spaces"),
      CONFUI_WIDGET_AO_CHG_SPACE, OPT_G_AO_CHANGESPACE,
      bdjoptGetNum (OPT_G_AO_CHANGESPACE));

  /* edit dances */
  vbox = confuiMakeNotebookTab (confui, confui->notebook,
      /* CONTEXT: config: edit the dance table */
      _("Edit Dances"), CONFUI_ID_DANCE);
  sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  g_signal_connect (confui->notebook, "switch-page",
      G_CALLBACK (confuiSwitchTable), confui);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_set_halign (hbox, GTK_ALIGN_START);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  confuiMakeItemTable (confui, hbox, CONFUI_ID_DANCE, CONFUI_TABLE_NO_UP_DOWN);
  confui->tables [CONFUI_ID_DANCE].savefunc = confuiDanceSave;
  confuiCreateDanceTable (confui);
  g_signal_connect (confui->tables [CONFUI_ID_DANCE].tree, "row-activated",
      G_CALLBACK (confuiDanceSelect), confui);

  dvbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_vexpand (dvbox, FALSE);
  gtk_widget_set_margin_start (dvbox, 16);
  gtk_box_pack_start (GTK_BOX (hbox), dvbox, FALSE, FALSE, 0);

  /* CONTEXT: config: dances: the name of the dance */
  confuiMakeItemEntry (confui, dvbox, sg, _("Dance"),
      CONFUI_ENTRY_DANCE_DANCE, -1, "");
  g_object_set_data (
      G_OBJECT (confui->uiitem [CONFUI_ENTRY_DANCE_DANCE].u.entry.entry),
      "confuiwidx", GUINT_TO_POINTER (CONFUI_ENTRY_DANCE_DANCE));
  g_object_set_data (
      G_OBJECT (confui->uiitem [CONFUI_ENTRY_DANCE_DANCE].u.entry.entry),
      "confuididx", GUINT_TO_POINTER (DANCE_DANCE));
  g_signal_connect (confui->uiitem [CONFUI_ENTRY_DANCE_DANCE].u.entry.entry,
      "changed", G_CALLBACK (confuiDanceEntryChg), confui);

  /* CONTEXT: config: dances: the type of the dance (club/latin/standard) */
  widget = confuiMakeItemSpinboxText (confui, dvbox, sg, _("Type"),
      CONFUI_SPINBOX_DANCE_TYPE, -1, 0);
  g_object_set_data (G_OBJECT (widget), "confuididx",
      GUINT_TO_POINTER (DANCE_TYPE));
  g_signal_connect (widget, "value-changed", G_CALLBACK (confuiDanceSpinboxChg), confui);

  /* CONTEXT: config: dances: the speed of the dance (fast/normal/slow) */
  widget = confuiMakeItemSpinboxText (confui, dvbox, sg, _("Speed"),
      CONFUI_SPINBOX_DANCE_SPEED, -1, 0);
  g_object_set_data (G_OBJECT (widget), "confuididx",
      GUINT_TO_POINTER (DANCE_SPEED));
  g_signal_connect (widget, "value-changed", G_CALLBACK (confuiDanceSpinboxChg), confui);

  /* CONTEXT: config: dances: tags associated with the dance */
  confuiMakeItemEntry (confui, dvbox, sg, _("Tags"),
      CONFUI_ENTRY_DANCE_TAGS, -1, "");
  g_object_set_data (
      G_OBJECT (confui->uiitem [CONFUI_ENTRY_DANCE_TAGS].u.entry.entry),
      "confuiwidx", GUINT_TO_POINTER (CONFUI_ENTRY_DANCE_TAGS));
  g_object_set_data (
      G_OBJECT (confui->uiitem [CONFUI_ENTRY_DANCE_TAGS].u.entry.entry),
      "confuididx", GUINT_TO_POINTER (DANCE_TAGS));
  g_signal_connect (confui->uiitem [CONFUI_ENTRY_DANCE_TAGS].u.entry.entry,
      "changed", G_CALLBACK (confuiDanceEntryChg), confui);

  /* CONTEXT: config: dances: play the selected announcement before the dance is played */
  confuiMakeItemEntryChooser (confui, dvbox, sg, _("Announcement"),
      CONFUI_ENTRY_DANCE_ANNOUNCEMENT, -1, "",
      confuiSelectAnnouncement);
  g_object_set_data (
      G_OBJECT (confui->uiitem [CONFUI_ENTRY_DANCE_ANNOUNCEMENT].u.entry.entry),
      "confuiwidx", GUINT_TO_POINTER (CONFUI_ENTRY_DANCE_ANNOUNCEMENT));
  g_object_set_data (
      G_OBJECT (confui->uiitem [CONFUI_ENTRY_DANCE_ANNOUNCEMENT].u.entry.entry),
      "confuididx", GUINT_TO_POINTER (DANCE_ANNOUNCE));
  uiutilsEntrySetValidate (
      &confui->uiitem [CONFUI_ENTRY_DANCE_ANNOUNCEMENT].u.entry,
      confuiDanceValidateAnnouncement, confui);
  g_signal_connect (confui->uiitem [CONFUI_ENTRY_DANCE_ANNOUNCEMENT].u.entry.entry,
      "changed", G_CALLBACK (confuiDanceEntryChg), confui);

  /* CONTEXT: config: dances: low BPM (or MPM) setting */
  widget = confuiMakeItemSpinboxInt (confui, dvbox, sg, _("Low BPM"),
      CONFUI_SPINBOX_DANCE_LOW_BPM, -1, 10, 500, 0);
  g_object_set_data (G_OBJECT (widget), "confuididx",
      GUINT_TO_POINTER (DANCE_LOW_BPM));
  g_signal_connect (widget, "value-changed", G_CALLBACK (confuiDanceSpinboxChg), confui);

  /* CONTEXT: config: dances: high BPM (or MPM) setting */
  widget = confuiMakeItemSpinboxInt (confui, dvbox, sg, _("High BPM"),
      CONFUI_SPINBOX_DANCE_HIGH_BPM, -1, 10, 500, 0);
  g_object_set_data (G_OBJECT (widget), "confuididx",
      GUINT_TO_POINTER (DANCE_HIGH_BPM));
  g_signal_connect (widget, "value-changed", G_CALLBACK (confuiDanceSpinboxChg), confui);

  /* CONTEXT: config: dances: time signature for the dance */
  widget = confuiMakeItemSpinboxText (confui, dvbox, sg, _("Time Signature"),
      CONFUI_SPINBOX_DANCE_TIME_SIG, -1, 0);
  g_object_set_data (G_OBJECT (widget), "confuididx",
      GUINT_TO_POINTER (DANCE_TIMESIG));
  g_signal_connect (widget, "value-changed", G_CALLBACK (confuiDanceSpinboxChg), confui);

  /* edit ratings */
  vbox = confuiMakeNotebookTab (confui, confui->notebook,
      /* CONTEXT: config: edit the dance ratings table */
      _("Edit Ratings"), CONFUI_ID_RATINGS);
  sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  g_signal_connect (confui->notebook, "switch-page",
      G_CALLBACK (confuiSwitchTable), confui);

  /* CONTEXT: config: dance ratings: information on how to order the ratings */
  widget = uiutilsCreateLabel (_("Order from the lowest rating to the highest rating."));
  gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);

  /* CONTEXT: config: dance ratings: information on how to edit a rating entry */
  widget = uiutilsCreateLabel (_("Double click on a field to edit."));
  gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);

  confuiMakeItemTable (confui, vbox, CONFUI_ID_RATINGS, CONFUI_TABLE_KEEP_FIRST);
  confui->tables [CONFUI_ID_RATINGS].listcreatefunc = confuiRatingListCreate;
  confui->tables [CONFUI_ID_RATINGS].savefunc = confuiRatingSave;
  confuiCreateRatingTable (confui);

  /* edit status */
  vbox = confuiMakeNotebookTab (confui, confui->notebook,
      /* CONTEXT: config: edit status table */
      _("Edit Status"), CONFUI_ID_STATUS);
  sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  g_signal_connect (confui->notebook, "switch-page",
      G_CALLBACK (confuiSwitchTable), confui);

  /* CONTEXT: config: status: information on how to edit a status entry */
  widget = uiutilsCreateLabel (_("Double click on a field to edit."));
  gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);

  confuiMakeItemTable (confui, vbox, CONFUI_ID_STATUS,
      CONFUI_TABLE_KEEP_FIRST | CONFUI_TABLE_KEEP_LAST);
  confui->tables [CONFUI_ID_STATUS].togglecol = CONFUI_STATUS_COL_PLAY_FLAG;
  confui->tables [CONFUI_ID_STATUS].listcreatefunc = confuiStatusListCreate;
  confui->tables [CONFUI_ID_STATUS].savefunc = confuiStatusSave;
  confuiCreateStatusTable (confui);

  /* edit levels */
  vbox = confuiMakeNotebookTab (confui, confui->notebook,
      /* CONTEXT: config: edit dance levels table */
      _("Edit Levels"), CONFUI_ID_LEVELS);
  sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  g_signal_connect (confui->notebook, "switch-page",
      G_CALLBACK (confuiSwitchTable), confui);

  widget = uiutilsCreateLabel (_("Order from easiest to most advanced."));
  gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);

  /* CONTEXT: config: dance levels: information on how to edit a level entry */
  widget = uiutilsCreateLabel (_("Double click on a field to edit."));
  gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);

  confuiMakeItemTable (confui, vbox, CONFUI_ID_LEVELS, CONFUI_TABLE_NONE);
  confui->tables [CONFUI_ID_LEVELS].togglecol = CONFUI_LEVEL_COL_DEFAULT;
  confui->tables [CONFUI_ID_LEVELS].listcreatefunc = confuiLevelListCreate;
  confui->tables [CONFUI_ID_LEVELS].savefunc = confuiLevelSave;
  confuiCreateLevelTable (confui);

  /* edit genres */
  vbox = confuiMakeNotebookTab (confui, confui->notebook,
      /* CONTEXT: config: edit genres table */
      _("Edit Genres"), CONFUI_ID_GENRES);
  sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  g_signal_connect (confui->notebook, "switch-page",
      G_CALLBACK (confuiSwitchTable), confui);

  /* CONTEXT: config: genres: information on how to edit a genre entry */
  widget = uiutilsCreateLabel (_("Double click on a field to edit."));
  gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);

  confuiMakeItemTable (confui, vbox, CONFUI_ID_GENRES, CONFUI_TABLE_NONE);
  confui->tables [CONFUI_ID_GENRES].togglecol = CONFUI_GENRE_COL_CLASSICAL;
  confui->tables [CONFUI_ID_GENRES].listcreatefunc = confuiGenreListCreate;
  confui->tables [CONFUI_ID_GENRES].savefunc = confuiGenreSave;
  confuiCreateGenreTable (confui);

  /* mobile remote control */
  vbox = confuiMakeNotebookTab (confui, confui->notebook,
      /* CONTEXT: config: options associated with mobile remote control */
      _("Mobile Remote Control"), CONFUI_ID_REM_CONTROL);
  sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  /* CONTEXT: config: remote control: enable/disable */
  widget = confuiMakeItemSwitch (confui, vbox, sg, _("Enable Remote Control"),
      CONFUI_WIDGET_RC_ENABLE, OPT_P_REMOTECONTROL,
      bdjoptGetNum (OPT_P_REMOTECONTROL));
  g_signal_connect (widget, "state-set", G_CALLBACK (confuiRemctrlChg), confui);

  /* CONTEXT: config: remote control: the HTML template to use */
  confuiMakeItemSpinboxText (confui, vbox, sg, _("HTML Template"),
      CONFUI_SPINBOX_RC_HTML_TEMPLATE, OPT_G_REMCONTROLHTML,
      confui->uiitem [CONFUI_SPINBOX_RC_HTML_TEMPLATE].listidx);

  /* CONTEXT: config: remote control: the user ID for sign-on to remote control */
  confuiMakeItemEntry (confui, vbox, sg, _("User ID"),
      CONFUI_ENTRY_RC_USER_ID,  OPT_P_REMCONTROLUSER,
      bdjoptGetStr (OPT_P_REMCONTROLUSER));

  /* CONTEXT: config: remote control: the password for sign-on to remote control */
  confuiMakeItemEntry (confui, vbox, sg, _("Password"),
      CONFUI_ENTRY_RC_PASS, OPT_P_REMCONTROLPASS,
      bdjoptGetStr (OPT_P_REMCONTROLPASS));

  /* CONTEXT: config: remote control: the port to use for remote control */
  widget = confuiMakeItemSpinboxInt (confui, vbox, sg, _("Port"),
      CONFUI_WIDGET_RC_PORT, OPT_P_REMCONTROLPORT,
      8000, 30000, bdjoptGetNum (OPT_P_REMCONTROLPORT));
  g_signal_connect (widget, "value-changed", G_CALLBACK (confuiRemctrlPortChg), confui);

  /* CONTEXT: config: remote control: the link to display the QR code for remote control */
  confuiMakeItemLink (confui, vbox, sg, _("QR Code"),
      CONFUI_WIDGET_RC_QR_CODE, "");

  /* mobile marquee */
  vbox = confuiMakeNotebookTab (confui, confui->notebook,
      /* CONTEXT: config: options associated with the mobile marquee */
      _("Mobile Marquee"), CONFUI_ID_MOBILE_MQ);
  sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  /* CONTEXT: config: set mobile marquee mode (off/local/internet) */
  widget = confuiMakeItemSpinboxText (confui, vbox, sg, _("Mobile Marquee"),
      CONFUI_SPINBOX_MOBILE_MQ, OPT_P_MOBILEMARQUEE,
      bdjoptGetNum (OPT_P_MOBILEMARQUEE));
  g_signal_connect (widget, "value-changed", G_CALLBACK (confuiMobmqTypeChg), confui);

  /* CONTEXT: config: the port to use for the mobile marquee */
  widget = confuiMakeItemSpinboxInt (confui, vbox, sg, _("Port"),
      CONFUI_WIDGET_MMQ_PORT, OPT_P_MOBILEMQPORT,
      8000, 30000, bdjoptGetNum (OPT_P_MOBILEMQPORT));
  g_signal_connect (widget, "value-changed", G_CALLBACK (confuiMobmqPortChg), confui);

  /* CONTEXT: config: the name to use for the mobile marquee internet mode */
  confuiMakeItemEntry (confui, vbox, sg, _("Name"),
      CONFUI_ENTRY_MM_NAME, OPT_P_MOBILEMQTAG,
      bdjoptGetStr (OPT_P_MOBILEMQTAG));
  uiutilsEntrySetValidate (&confui->uiitem [CONFUI_ENTRY_MM_NAME].u.entry,
      confuiMobmqNameChg, confui);

  /* CONTEXT: config: the title to display on the mobile marquee */
  confuiMakeItemEntry (confui, vbox, sg, _("Title"),
      CONFUI_ENTRY_MM_TITLE, OPT_P_MOBILEMQTITLE,
      bdjoptGetStr (OPT_P_MOBILEMQTITLE));
  uiutilsEntrySetValidate (&confui->uiitem [CONFUI_ENTRY_MM_TITLE].u.entry,
      confuiMobmqTitleChg, confui);

  /* CONTEXT: config: mobile marquee: the link to display the QR code for the mobile marquee */
  confuiMakeItemLink (confui, vbox, sg, _("QR Code"),
      CONFUI_WIDGET_MMQ_QR_CODE, "");

  /* debug options */
  vbox = confuiMakeNotebookTab (confui, confui->notebook,
      /* CONTEXT: config: debug options that can be turned on and off */
      _("Debug Options"), CONFUI_ID_NONE);
  sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  val = bdjoptGetNum (OPT_G_DEBUGLVL);
  confuiMakeItemCheckButton (confui, vbox, sg, "Important",
      CONFUI_WIDGET_DEBUG_1, -1,
      (val & 1));
  confui->uiitem [CONFUI_WIDGET_DEBUG_1].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (confui, vbox, sg, "Basic",
      CONFUI_WIDGET_DEBUG_2, -1,
      (val & 2));
  confui->uiitem [CONFUI_WIDGET_DEBUG_2].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (confui, vbox, sg, "Messages",
      CONFUI_WIDGET_DEBUG_4, -1,
      (val & 4));
  confui->uiitem [CONFUI_WIDGET_DEBUG_4].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (confui, vbox, sg, "Main",
      CONFUI_WIDGET_DEBUG_8, -1,
      (val & 8));
  confui->uiitem [CONFUI_WIDGET_DEBUG_8].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (confui, vbox, sg, "List",
      CONFUI_WIDGET_DEBUG_16, -1,
      (val & 16));
  confui->uiitem [CONFUI_WIDGET_DEBUG_16].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (confui, vbox, sg, "Song Selection",
      CONFUI_WIDGET_DEBUG_32, -1,
      (val & 32));
  confui->uiitem [CONFUI_WIDGET_DEBUG_32].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (confui, vbox, sg, "Dance Selection",
      CONFUI_WIDGET_DEBUG_64, -1,
      (val & 64));
  confui->uiitem [CONFUI_WIDGET_DEBUG_64].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (confui, vbox, sg, "Volume",
      CONFUI_WIDGET_DEBUG_128, -1,
      (val & 128));
  confui->uiitem [CONFUI_WIDGET_DEBUG_128].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (confui, vbox, sg, "Socket",
      CONFUI_WIDGET_DEBUG_256, -1,
      (val & 256));
  confui->uiitem [CONFUI_WIDGET_DEBUG_256].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (confui, vbox, sg, "Database",
      CONFUI_WIDGET_DEBUG_512, -1,
      (val & 512));
  confui->uiitem [CONFUI_WIDGET_DEBUG_512].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (confui, vbox, sg, "Random Access File",
      CONFUI_WIDGET_DEBUG_1024, -1,
      (val & 1024));
  confui->uiitem [CONFUI_WIDGET_DEBUG_1024].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (confui, vbox, sg, "Procedures",
      CONFUI_WIDGET_DEBUG_2048, -1,
      (val & 2048));
  confui->uiitem [CONFUI_WIDGET_DEBUG_2048].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (confui, vbox, sg, "Player",
      CONFUI_WIDGET_DEBUG_4096, -1,
      (val & 4096));
  confui->uiitem [CONFUI_WIDGET_DEBUG_4096].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (confui, vbox, sg, "Datafile",
      CONFUI_WIDGET_DEBUG_8192, -1,
      (val & 8192));
  confui->uiitem [CONFUI_WIDGET_DEBUG_8192].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (confui, vbox, sg, "Process",
      CONFUI_WIDGET_DEBUG_16384, -1,
      (val & 16384));
  confui->uiitem [CONFUI_WIDGET_DEBUG_16384].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (confui, vbox, sg, "Web Server",
      CONFUI_WIDGET_DEBUG_32768, -1,
      (val & 32768));
  confui->uiitem [CONFUI_WIDGET_DEBUG_32768].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (confui, vbox, sg, "Web Client",
      CONFUI_WIDGET_DEBUG_65536, -1,
      (val & 65536));
  confui->uiitem [CONFUI_WIDGET_DEBUG_65536].outtype = CONFUI_OUT_DEBUG;
  confuiMakeItemCheckButton (confui, vbox, sg, "Database Update",
      CONFUI_WIDGET_DEBUG_131072, -1,
      (val & 131072));
  confui->uiitem [CONFUI_WIDGET_DEBUG_131072].outtype = CONFUI_OUT_DEBUG;

  x = nlistGetNum (confui->options, CONFUI_SIZE_X);
  y = nlistGetNum (confui->options, CONFUI_SIZE_Y);
  gtk_window_set_default_size (GTK_WINDOW (confui->window), x, y);

  gtk_widget_show_all (confui->window);

  x = nlistGetNum (confui->options, CONFUI_POSITION_X);
  y = nlistGetNum (confui->options, CONFUI_POSITION_Y);
  if (x != -1 && y != -1) {
    gtk_window_move (GTK_WINDOW (confui->window), x, y);
  }

  pathbldMakePath (imgbuff, sizeof (imgbuff),
      "bdj4_icon", ".png", PATHBLD_MP_IMGDIR);
  osuiSetIcon (imgbuff);

  logProcEnd (LOG_PROC, "confuiActivate", "");
}

gboolean
confuiMainLoop (void *tconfui)
{
  configui_t   *confui = tconfui;
  int         tdone = 0;
  gboolean    cont = TRUE;

  tdone = sockhProcessMain (confui->sockserver, confuiProcessMsg, confui);
  if (tdone || gdone) {
    ++gdone;
  }
  if (gdone > CONFUI_EXIT_WAIT_COUNT) {
    g_application_quit (G_APPLICATION (confui->app));
    cont = FALSE;
  }

  if (! progstateIsRunning (confui->progstate)) {
    progstateProcess (confui->progstate);
    if (gKillReceived) {
      logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
      progstateShutdownProcess (confui->progstate);
    }
    return cont;
  }

  for (int i = CONFUI_COMBOBOX_MAX + 1; i < CONFUI_ENTRY_MAX; ++i) {
    uiutilsEntryValidate (&confui->uiitem [i].u.entry);
  }

  if (gKillReceived) {
    logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
    progstateShutdownProcess (confui->progstate);
  }
  return cont;
}

static bool
confuiHandshakeCallback (void *udata, programstate_t programState)
{
  configui_t   *confui = udata;

  progstateLogTime (confui->progstate, "time-to-start-gui");
  return true;
}

static int
confuiProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
    bdjmsgmsg_t msg, char *args, void *udata)
{
  configui_t       *confui = udata;

  logProcBegin (LOG_PROC, "confuiProcessMsg");

  logMsg (LOG_DBG, LOG_MSGS, "got: from:%ld/%s route:%ld/%s msg:%ld/%s args:%s",
      routefrom, msgRouteDebugText (routefrom),
      route, msgRouteDebugText (route), msg, msgDebugText (msg), args);

  switch (route) {
    case ROUTE_NONE:
    case ROUTE_CONFIGUI: {
      switch (msg) {
        case MSG_HANDSHAKE: {
          connProcessHandshake (confui->conn, routefrom);
          connConnectResponse (confui->conn, routefrom);
          break;
        }
        case MSG_EXIT_REQUEST: {
          logMsg (LOG_SESS, LOG_IMPORTANT, "got exit request");
          gKillReceived = 0;
          logMsg (LOG_DBG, LOG_MSGS, "got: req-exit");
          progstateShutdownProcess (confui->progstate);
          logProcEnd (LOG_PROC, "confuiProcessMsg", "req-exit");
          return 1;
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


static gboolean
confuiCloseWin (GtkWidget *window, GdkEvent *event, gpointer userdata)
{
  configui_t   *confui = userdata;

  logProcBegin (LOG_PROC, "confuiCloseWin");
  if (! gdone) {
    progstateShutdownProcess (confui->progstate);
    logMsg (LOG_DBG, LOG_MSGS, "got: close win request");
    logProcEnd (LOG_PROC, "confuiCloseWin", "not-done");
    return TRUE;
  }

  logProcEnd (LOG_PROC, "confuiCloseWin", "");
  return FALSE;
}

static void
confuiSigHandler (int sig)
{
  gKillReceived = 1;
}

static void
confuiPopulateOptions (configui_t *confui)
{
  const char  *sval;
  ssize_t     nval;
  double      dval;
  confuibasetype_t basetype;
  confuiouttype_t outtype;
  char        tbuff [MAXPATHLEN];
  GdkRGBA     gcolor;
  long        debug = 0;
  bool        accentcolorchanged = false;
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
        sval = uiutilsEntryGetValue (&confui->uiitem [i].u.entry);
        break;
      }
      case CONFUI_SPINBOX_TEXT: {
        nval = uiutilsSpinboxTextGetValue (&confui->uiitem [i].u.spinbox);
        if (confui->uiitem [i].sblookuplist != NULL) {
          sval = nlistGetStr (confui->uiitem [i].sblookuplist, nval);
          outtype = CONFUI_OUT_STR;
        }
        break;
      }
      case CONFUI_SPINBOX_NUM: {
        nval = (ssize_t) uiutilsSpinboxGetValue (confui->uiitem [i].u.widget);
        break;
      }
      case CONFUI_SPINBOX_DOUBLE: {
        dval = uiutilsSpinboxGetValue (confui->uiitem [i].u.widget);
        nval = (ssize_t) (dval * 1000.0);
        outtype = CONFUI_OUT_NUM;
        break;
      }
      case CONFUI_SPINBOX_TIME: {
        nval = (ssize_t) uiutilsSpinboxGetValue (confui->uiitem [i].u.widget);
        break;
      }
      case CONFUI_COLOR: {
        gtk_color_chooser_get_rgba (
            GTK_COLOR_CHOOSER (confui->uiitem [i].u.widget), &gcolor);
        snprintf (tbuff, sizeof (tbuff), "#%02x%02x%02x",
            (int) round (gcolor.red * 255.0),
            (int) round (gcolor.green * 255.0),
            (int) round (gcolor.blue * 255.0));
        sval = tbuff;
        break;
      }
      case CONFUI_FONT: {
        sval = gtk_font_chooser_get_font (
            GTK_FONT_CHOOSER (confui->uiitem [i].u.widget));
        break;
      }
      case CONFUI_SWITCH: {
        nval = gtk_switch_get_active (GTK_SWITCH (confui->uiitem [i].u.widget));
        break;
      }
      case CONFUI_CHECK_BUTTON: {
        nval = gtk_toggle_button_get_active (
            GTK_TOGGLE_BUTTON (confui->uiitem [i].u.widget));
        break;
      }
      case CONFUI_COMBOBOX: {
        sval = slistGetDataByIdx (confui->uiitem [i].list,
            confui->uiitem [i].listidx);
        outtype = CONFUI_OUT_STR;
        break;
      }
    }

    if (i == CONFUI_SPINBOX_AUDIO_OUTPUT) {
      uiutilsspinbox_t  *spinbox;

      spinbox = &confui->uiitem [i].u.spinbox;
      if (! uiutilsSpinboxIsChanged (spinbox)) {
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
          "locale", ".txt", PATHBLD_MP_NONE);
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

      templateImageLocaleCopy ();
    }

    if (i == CONFUI_ENTRY_MUSIC_DIR) {
      strlcpy (tbuff, bdjoptGetStr (confui->uiitem [i].bdjoptIdx), sizeof (tbuff));
      pathNormPath (tbuff, sizeof (tbuff));
      bdjoptSetStr (confui->uiitem [i].bdjoptIdx, tbuff);
    }

    if (i == CONFUI_SPINBOX_UI_THEME &&
        themechanged) {
      FILE    *fh;

      pathbldMakePath (tbuff, sizeof (tbuff),
          "theme", ".txt", PATHBLD_MP_NONE);
      fh = fopen (tbuff, "w");
      sval = bdjoptGetStr (confui->uiitem [i].bdjoptIdx);
      if (sval != NULL) {
        fprintf (fh, "%s\n", sval);
      }
      fclose (fh);
    }

    if (i == CONFUI_WIDGET_UI_ACCENT_COLOR &&
        accentcolorchanged) {
      templateImageCopy (sval);
    }

    if (i == CONFUI_SPINBOX_RC_HTML_TEMPLATE) {
      sval = bdjoptGetStr (confui->uiitem [i].bdjoptIdx);
      templateFileCopy (sval, "bdj.html");
    }
  } /* for each item */

  confuiDispSaveTable (confui, confui->uiitem [CONFUI_SPINBOX_DISP_SEL].listidx);

  bdjoptSetNum (OPT_G_DEBUGLVL, debug);
  logProcEnd (LOG_PROC, "confuiPopulateOptions", "");
}


static void
confuiSelectMusicDir (GtkButton *b, gpointer udata)
{
  configui_t            *confui = udata;
  char                  *fn = NULL;
  uiutilsselect_t       selectdata;

  logProcBegin (LOG_PROC, "confuiSelectMusicDir");
  /* CONTEXT: config: folder selection dialog: window title */
  selectdata.label = _("Select Music Folder Location");
  selectdata.window = confui->window;
  selectdata.startpath = bdjoptGetStr (OPT_M_DIR_MUSIC);
  selectdata.mimetype = NULL;
  fn = uiutilsSelectDirDialog (&selectdata);
  if (fn != NULL) {
    gtk_entry_buffer_set_text (confui->uiitem [CONFUI_ENTRY_MUSIC_DIR].u.entry.buffer,
        fn, -1);
    free (fn);
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "selected loc: %s", fn);
  }
  logProcEnd (LOG_PROC, "confuiSelectMusicDir", "");
}

static void
confuiSelectStartup (GtkButton *b, gpointer udata)
{
  configui_t            *confui = udata;

  logProcBegin (LOG_PROC, "confuiSelectStartup");
  confuiSelectFileDialog (confui, CONFUI_ENTRY_STARTUP,
      sysvarsGetStr (SV_BDJ4DATATOPDIR), NULL, NULL);
  logProcEnd (LOG_PROC, "confuiSelectStartup", "");
}

static void
confuiSelectShutdown (GtkButton *b, gpointer udata)
{
  configui_t            *confui = udata;

  logProcBegin (LOG_PROC, "confuiSelectShutdown");
  confuiSelectFileDialog (confui, CONFUI_ENTRY_SHUTDOWN,
      sysvarsGetStr (SV_BDJ4DATATOPDIR), NULL, NULL);
  logProcEnd (LOG_PROC, "confuiSelectShutdown", "");
}

static void
confuiSelectAnnouncement (GtkButton *b, gpointer udata)
{
  configui_t            *confui = udata;

  logProcBegin (LOG_PROC, "confuiSelectAnnouncement");
  confuiSelectFileDialog (confui, CONFUI_ENTRY_DANCE_ANNOUNCEMENT,
      /* CONTEXT: config: announcement selection dialog: audio file filter */
      bdjoptGetStr (OPT_M_DIR_MUSIC), _("Audio Files"), "audio/*");
  logProcEnd (LOG_PROC, "confuiSelectAnnouncement", "");
}

static void
confuiSelectFileDialog (configui_t *confui, int widx, char *startpath,
    char *mimefiltername, char *mimetype)
{
  char                  *fn = NULL;
  uiutilsselect_t       selectdata;

  logProcBegin (LOG_PROC, "confuiSelectFileDialog");
  /* CONTEXT: config: file selection dialog: window title */
  selectdata.label = _("Select File");
  selectdata.window = confui->window;
  selectdata.startpath = startpath;
  selectdata.mimefiltername = mimefiltername;
  selectdata.mimetype = mimetype;
  fn = uiutilsSelectFileDialog (&selectdata);
  if (fn != NULL) {
    gtk_entry_buffer_set_text (confui->uiitem [widx].u.entry.buffer,
        fn, -1);
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "selected loc: %s", fn);
    free (fn);
  }
  logProcEnd (LOG_PROC, "confuiSelectFileDialog", "");
}

static GtkWidget *
confuiMakeNotebookTab (configui_t *confui, GtkWidget *nb, char *txt, int id)
{
  GtkWidget   *tablabel;
  GtkWidget   *vbox;

  logProcBegin (LOG_PROC, "confuiMakeNotebookTab");
  tablabel = uiutilsCreateLabel (txt);
  gtk_widget_set_margin_top (tablabel, 0);
  gtk_widget_set_margin_start (tablabel, 0);
  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_margin_top (vbox, 4);
  gtk_widget_set_margin_bottom (vbox, 4);
  gtk_widget_set_margin_start (vbox, 4);
  gtk_widget_set_margin_end (vbox, 4);
  uiutilsNotebookAppendPage (nb, vbox, tablabel);

  confui->tableidents = realloc (confui->tableidents,
      sizeof (int) * (confui->tabcount + 1));
  confui->tableidents [confui->tabcount] = id;
  ++confui->tabcount;

  logProcEnd (LOG_PROC, "confuiMakeNotebookTab", "");
  return vbox;
}

static GtkWidget *
confuiMakeItemEntry (configui_t *confui, GtkWidget *vbox, GtkSizeGroup *sg,
    char *txt, int widx, int bdjoptIdx, char *disp)
{
  GtkWidget   *hbox;
  GtkWidget   *widget;

  logProcBegin (LOG_PROC, "confuiMakeItemEntry");
  confui->uiitem [widx].basetype = CONFUI_ENTRY;
  confui->uiitem [widx].outtype = CONFUI_OUT_STR;
  hbox = confuiMakeItemLabel (vbox, sg, txt);
  widget = uiutilsEntryCreate (&confui->uiitem [widx].u.entry);
  gtk_widget_set_margin_start (widget, 8);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  if (disp != NULL) {
    uiutilsEntrySetValue (&confui->uiitem [widx].u.entry, disp);
  } else {
    uiutilsEntrySetValue (&confui->uiitem [widx].u.entry, "");
  }
  confui->uiitem [widx].bdjoptIdx = bdjoptIdx;
  logProcEnd (LOG_PROC, "confuiMakeItemEntry", "");
  return hbox;
}

static void
confuiMakeItemEntryChooser (configui_t *confui, GtkWidget *vbox,
    GtkSizeGroup *sg, char *txt, int widx, int bdjoptIdx,
    char *disp, void *dialogFunc)
{
  GtkWidget   *hbox;
  GtkWidget   *widget;
  GtkWidget   *image;

  logProcBegin (LOG_PROC, "confuiMakeItemEntryChooser");
  hbox = confuiMakeItemEntry (confui, vbox, sg, txt, widx, bdjoptIdx, disp);

  widget = uiutilsCreateButton ("", NULL, dialogFunc, confui);
  image = gtk_image_new_from_icon_name ("folder", GTK_ICON_SIZE_BUTTON);
  gtk_button_set_image (GTK_BUTTON (widget), image);
  gtk_button_set_always_show_image (GTK_BUTTON (widget), TRUE);
  gtk_widget_set_margin_start (widget, 0);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
  confui->uiitem [widx].bdjoptIdx = bdjoptIdx;
  logProcEnd (LOG_PROC, "confuiMakeItemEntryChooser", "");
}

static GtkWidget *
confuiMakeItemCombobox (configui_t *confui, GtkWidget *vbox, GtkSizeGroup *sg,
    char *txt, int widx, int bdjoptIdx, void *ddcb, char *value)
{
  GtkWidget   *hbox;
  GtkWidget   *widget;

  logProcBegin (LOG_PROC, "confuiMakeItemCombobox");
  confui->uiitem [widx].basetype = CONFUI_COMBOBOX;
  confui->uiitem [widx].outtype = CONFUI_OUT_STR;
  hbox = confuiMakeItemLabel (vbox, sg, txt);
  widget = uiutilsComboboxCreate (confui->window, txt,
      ddcb, &confui->uiitem [widx].u.dropdown, confui);
  uiutilsDropDownSetList (&confui->uiitem [widx].u.dropdown,
      confui->uiitem [widx].list, NULL);
  uiutilsDropDownSelectionSetStr (&confui->uiitem [widx].u.dropdown, value);
  gtk_widget_set_margin_start (widget, 8);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  confui->uiitem [widx].bdjoptIdx = bdjoptIdx;
  logProcEnd (LOG_PROC, "confuiMakeItemCombobox", "");
  return hbox;
}

static void
confuiMakeItemLink (configui_t *confui, GtkWidget *vbox, GtkSizeGroup *sg,
    char *txt, int widx, char *disp)
{
  GtkWidget   *hbox;
  GtkWidget   *widget;

  logProcBegin (LOG_PROC, "confuiMakeItemLink");
  hbox = confuiMakeItemLabel (vbox, sg, txt);
  widget = gtk_link_button_new (disp);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  confui->uiitem [widx].u.widget = widget;
  logProcEnd (LOG_PROC, "confuiMakeItemLink", "");
}

static void
confuiMakeItemFontButton (configui_t *confui, GtkWidget *vbox, GtkSizeGroup *sg,
    char *txt, int widx, int bdjoptIdx, char *fontname)
{
  GtkWidget   *hbox;
  GtkWidget   *widget;

  logProcBegin (LOG_PROC, "confuiMakeItemFontButton");
  confui->uiitem [widx].basetype = CONFUI_FONT;
  confui->uiitem [widx].outtype = CONFUI_OUT_STR;
  hbox = confuiMakeItemLabel (vbox, sg, txt);
  if (fontname != NULL && *fontname) {
    widget = gtk_font_button_new_with_font (fontname);
  } else {
    widget = gtk_font_button_new ();
  }
  gtk_widget_set_margin_start (widget, 8);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  confui->uiitem [widx].u.widget = widget;
  confui->uiitem [widx].bdjoptIdx = bdjoptIdx;
  logProcEnd (LOG_PROC, "confuiMakeItemFontButton", "");
}

static void
confuiMakeItemColorButton (configui_t *confui, GtkWidget *vbox, GtkSizeGroup *sg,
    char *txt, int widx, int bdjoptIdx, char *color)
{
  GtkWidget   *hbox;
  GtkWidget   *widget;
  GdkRGBA     rgba;

  logProcBegin (LOG_PROC, "confuiMakeItemColorButton");

  confui->uiitem [widx].basetype = CONFUI_COLOR;
  confui->uiitem [widx].outtype = CONFUI_OUT_STR;
  hbox = confuiMakeItemLabel (vbox, sg, txt);
  if (color != NULL && *color) {
    gdk_rgba_parse (&rgba, color);
    widget = gtk_color_button_new_with_rgba (&rgba);
  } else {
    widget = gtk_color_button_new ();
  }
  gtk_widget_set_margin_start (widget, 8);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  confui->uiitem [widx].u.widget = widget;
  confui->uiitem [widx].bdjoptIdx = bdjoptIdx;
  logProcEnd (LOG_PROC, "confuiMakeItemColorButton", "");
}

static GtkWidget *
confuiMakeItemSpinboxText (configui_t *confui, GtkWidget *vbox, GtkSizeGroup *sg,
    char *txt, int widx, int bdjoptIdx, ssize_t value)
{
  GtkWidget   *hbox;
  GtkWidget   *widget;
  nlist_t     *list;
  size_t      maxWidth;

  logProcBegin (LOG_PROC, "confuiMakeItemSpinboxText");

  confui->uiitem [widx].basetype = CONFUI_SPINBOX_TEXT;
  confui->uiitem [widx].outtype = CONFUI_OUT_NUM;
  hbox = confuiMakeItemLabel (vbox, sg, txt);
  widget = uiutilsSpinboxTextCreate (&confui->uiitem [widx].u.spinbox, confui);
  list = confui->uiitem [widx].list;
  maxWidth = 0;
  if (list != NULL) {
    nlistidx_t    iteridx;
    char          *val;

    nlistStartIterator (list, &iteridx);
    while ((val = nlistIterateValueData (list, &iteridx)) != NULL) {
      size_t      len;

      len = strlen (val);
      maxWidth = len > maxWidth ? len : maxWidth;
    }
  }

  uiutilsSpinboxTextSet (&confui->uiitem [widx].u.spinbox, 0,
      nlistGetCount (list), maxWidth, list, NULL);
  uiutilsSpinboxTextSetValue (&confui->uiitem [widx].u.spinbox, value);
  gtk_widget_set_margin_start (widget, 8);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  confui->uiitem [widx].bdjoptIdx = bdjoptIdx;

  logProcEnd (LOG_PROC, "confuiMakeItemSpinboxText", "");
  return widget;
}

static void
confuiMakeItemSpinboxTime (configui_t *confui, GtkWidget *vbox,
    GtkSizeGroup *sg, char *txt, int widx, int bdjoptIdx, ssize_t value)
{
  GtkWidget   *hbox;
  GtkWidget   *widget;

  logProcBegin (LOG_PROC, "confuiMakeItemSpinboxTime");

  confui->uiitem [widx].basetype = CONFUI_SPINBOX_TIME;
  confui->uiitem [widx].outtype = CONFUI_OUT_NUM;
  hbox = confuiMakeItemLabel (vbox, sg, txt);
  widget = uiutilsSpinboxTimeCreate (&confui->uiitem [widx].u.spinbox, confui);
  uiutilsSpinboxTimeSetValue (&confui->uiitem [widx].u.spinbox, value);
  gtk_widget_set_margin_start (widget, 8);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  confui->uiitem [widx].bdjoptIdx = bdjoptIdx;
  logProcEnd (LOG_PROC, "confuiMakeItemSpinboxTime", "");
}

static GtkWidget *
confuiMakeItemSpinboxInt (configui_t *confui, GtkWidget *vbox, GtkSizeGroup *sg,
    char *txt, int widx, int bdjoptIdx, int min, int max, int value)
{
  GtkWidget   *hbox;
  GtkWidget   *widget;

  logProcBegin (LOG_PROC, "confuiMakeItemSpinboxInt");

  confui->uiitem [widx].basetype = CONFUI_SPINBOX_NUM;
  confui->uiitem [widx].outtype = CONFUI_OUT_NUM;
  hbox = confuiMakeItemLabel (vbox, sg, txt);
  widget = uiutilsSpinboxIntCreate ();
  uiutilsSpinboxSet (widget, (double) min, (double) max);
  uiutilsSpinboxSetValue (widget, (double) value);
  gtk_widget_set_margin_start (widget, 8);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  confui->uiitem [widx].u.widget = widget;
  confui->uiitem [widx].bdjoptIdx = bdjoptIdx;
  logProcEnd (LOG_PROC, "confuiMakeItemSpinboxInt", "");
  return widget;
}

static void
confuiMakeItemSpinboxDouble (configui_t *confui, GtkWidget *vbox, GtkSizeGroup *sg,
    char *txt, int widx, int bdjoptIdx, double min, double max, double value)
{
  GtkWidget   *hbox;
  GtkWidget   *widget;

  logProcBegin (LOG_PROC, "confuiMakeItemSpinboxDouble");

  confui->uiitem [widx].basetype = CONFUI_SPINBOX_DOUBLE;
  confui->uiitem [widx].outtype = CONFUI_OUT_DOUBLE;
  hbox = confuiMakeItemLabel (vbox, sg, txt);
  widget = uiutilsSpinboxDoubleCreate ();
  uiutilsSpinboxSet (widget, min, max);
  uiutilsSpinboxSetValue (widget, value);
  gtk_widget_set_margin_start (widget, 8);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  confui->uiitem [widx].u.widget = widget;
  confui->uiitem [widx].bdjoptIdx = bdjoptIdx;
  logProcEnd (LOG_PROC, "confuiMakeItemSpinboxDouble", "");
}

static GtkWidget *
confuiMakeItemSwitch (configui_t *confui, GtkWidget *vbox, GtkSizeGroup *sg,
    char *txt, int widx, int bdjoptIdx, int value)
{
  GtkWidget   *hbox;
  GtkWidget   *widget;

  logProcBegin (LOG_PROC, "confuiMakeItemSwitch");

  confui->uiitem [widx].basetype = CONFUI_SWITCH;
  confui->uiitem [widx].outtype = CONFUI_OUT_BOOL;
  hbox = confuiMakeItemLabel (vbox, sg, txt);
  widget = uiutilsCreateSwitch (value);
  gtk_widget_set_margin_start (widget, 8);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  confui->uiitem [widx].u.widget = widget;
  confui->uiitem [widx].bdjoptIdx = bdjoptIdx;
  logProcEnd (LOG_PROC, "confuiMakeItemSwitch", "");
  return widget;
}

static GtkWidget *
confuiMakeItemLabelDisp (configui_t *confui, GtkWidget *vbox, GtkSizeGroup *sg,
    char *txt, int widx, int bdjoptIdx)
{
  GtkWidget   *hbox;
  GtkWidget   *widget;

  logProcBegin (LOG_PROC, "confuiMakeItemLabelDisp");

  confui->uiitem [widx].basetype = CONFUI_NONE;
  confui->uiitem [widx].outtype = CONFUI_OUT_NONE;
  hbox = confuiMakeItemLabel (vbox, sg, txt);
  widget = uiutilsCreateLabel ("");
  gtk_widget_set_margin_start (widget, 8);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  confui->uiitem [widx].u.widget = widget;
  confui->uiitem [widx].bdjoptIdx = bdjoptIdx;
  logProcEnd (LOG_PROC, "confuiMakeItemLabelDisp", "");
  return widget;
}

static void
confuiMakeItemCheckButton (configui_t *confui, GtkWidget *vbox, GtkSizeGroup *sg,
    const char *txt, int widx, int bdjoptIdx, int value)
{
  GtkWidget   *widget;

  logProcBegin (LOG_PROC, "confuiMakeItemCheckButton");

  confui->uiitem [widx].basetype = CONFUI_CHECK_BUTTON;
  confui->uiitem [widx].outtype = CONFUI_OUT_BOOL;
  widget = uiutilsCreateCheckButton (txt, value);
  gtk_widget_set_margin_start (widget, 8);
  gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);
  confui->uiitem [widx].u.widget = widget;
  confui->uiitem [widx].bdjoptIdx = bdjoptIdx;
  logProcEnd (LOG_PROC, "confuiMakeItemCheckButton", "");
}

static GtkWidget *
confuiMakeItemLabel (GtkWidget *vbox, GtkSizeGroup *sg, char *txt)
{
  GtkWidget   *hbox;
  GtkWidget   *widget;

  logProcBegin (LOG_PROC, "confuiMakeItemLabel");
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  if (*txt == '\0') {
    widget = uiutilsCreateLabel (txt);
  } else {
    widget = uiutilsCreateColonLabel (txt);
  }
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
  gtk_size_group_add_widget (sg, widget);
  logProcEnd (LOG_PROC, "confuiMakeItemLabel", "");
  return hbox;
}

static GtkWidget *
confuiMakeItemTable (configui_t *confui, GtkWidget *vbox, confuiident_t id,
    int flags)
{
  GtkWidget   *mhbox;
  GtkWidget   *bvbox;
  GtkWidget   *widget;
  GtkWidget   *tree;

  logProcBegin (LOG_PROC, "confuiMakeItemTable");
  mhbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  assert (mhbox != NULL);
  gtk_widget_set_halign (mhbox, GTK_ALIGN_START);
  gtk_widget_set_margin_top (mhbox, 4);
  gtk_widget_set_hexpand (mhbox, TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), mhbox, FALSE, FALSE, 0);

  widget = uiutilsCreateScrolledWindow ();
  gtk_scrolled_window_set_min_content_height (GTK_SCROLLED_WINDOW (widget), 200);
  gtk_box_pack_start (GTK_BOX (mhbox), widget, FALSE, FALSE, 0);

  tree = uiutilsCreateTreeView ();
  confui->tables [id].tree = tree;
  confui->tables [id].flags = flags;
  gtk_widget_set_margin_start (tree, 16);
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tree), TRUE);
  gtk_container_add (GTK_CONTAINER (widget), tree);

  bvbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  assert (bvbox != NULL);
  gtk_widget_set_margin_top (bvbox, 2);
  gtk_widget_set_margin_start (bvbox, 8);
  gtk_widget_set_valign (bvbox, GTK_ALIGN_CENTER);
  gtk_box_pack_start (GTK_BOX (mhbox), bvbox, FALSE, FALSE, 0);

  if ((flags & CONFUI_TABLE_NO_UP_DOWN) != CONFUI_TABLE_NO_UP_DOWN) {
    /* CONTEXT: config: table edit: button: move selection up */
    widget = uiutilsCreateButton (_("Move Up"), "button_up",
        confuiTableMoveUp, confui);
    gtk_box_pack_start (GTK_BOX (bvbox), widget, FALSE, FALSE, 0);

    /* CONTEXT: config: table edit: button: move selection down */
    widget = uiutilsCreateButton (_("Move Down"), "button_down",
        confuiTableMoveDown, confui);
    gtk_box_pack_start (GTK_BOX (bvbox), widget, FALSE, FALSE, 0);
  }

  /* CONTEXT: config: table edit: button: delete selection */
  widget = uiutilsCreateButton (_("Delete"), "button_remove",
      confuiTableRemove, confui);
  gtk_box_pack_start (GTK_BOX (bvbox), widget, FALSE, FALSE, 0);

  /* CONTEXT: config: table edit: button: add new selection */
  widget = uiutilsCreateButton (_("Add New"), "button_add",
      confuiTableAdd, confui);
  gtk_box_pack_start (GTK_BOX (bvbox), widget, FALSE, FALSE, 0);

  logProcEnd (LOG_PROC, "confuiMakeItemTable", "");
  return vbox;
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
    filelist = filemanipRecursiveDirList (tbuff, FILEMANIP_DIRS);
    confuiGetThemeNames (themelist, filelist);
    slistFree (filelist);
    count = nlistGetCount (themelist);
    nlistSetStr (themelist, count, "Adwaita");
  } else {
    filelist = filemanipRecursiveDirList ("/usr/share/themes", FILEMANIP_DIRS);
    confuiGetThemeNames (themelist, filelist);
    slistFree (filelist);

    snprintf (tbuff, sizeof (tbuff), "%s/.themes", sysvarsGetStr (SV_HOME));
    filelist = filemanipRecursiveDirList (tbuff, FILEMANIP_DIRS);
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
  nlist_t       *llist;
  int           count;

  logProcBegin (LOG_PROC, "confuiLoadHTMLList");

  tlist = nlistAlloc ("cu-html-list", LIST_ORDERED, free);
  llist = nlistAlloc ("cu-html-list-l", LIST_ORDERED, free);

  pathbldMakePath (tbuff, sizeof (tbuff),
      "html-list", ".txt", PATHBLD_MP_TEMPLATEDIR);
  df = datafileAllocParse ("conf-html-list", DFTYPE_KEY_VAL, tbuff,
      NULL, 0, DATAFILE_NO_LOOKUP);
  list = datafileGetList (df);

  slistStartIterator (list, &iteridx);
  count = 0;
  while ((key = slistIterateKey (list, &iteridx)) != NULL) {
    data = slistGetStr (list, key);
    if (strcmp (data, bdjoptGetStr (OPT_G_REMCONTROLHTML)) == 0) {
      confui->uiitem [CONFUI_SPINBOX_RC_HTML_TEMPLATE].listidx = count;
    }
    nlistSetStr (tlist, count, key);
    nlistSetStr (llist, count, data);
    ++count;
  }
  datafileFree (df);

  confui->uiitem [CONFUI_SPINBOX_RC_HTML_TEMPLATE].list = tlist;
  confui->uiitem [CONFUI_SPINBOX_RC_HTML_TEMPLATE].sblookuplist = llist;
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
      "volintfc", ".txt", PATHBLD_MP_TEMPLATEDIR);
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

  confui->uiitem [CONFUI_SPINBOX_VOL_INTFC].list = tlist;
  confui->uiitem [CONFUI_SPINBOX_VOL_INTFC].sblookuplist = llist;
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
      "playerintfc", ".txt", PATHBLD_MP_TEMPLATEDIR);
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

  confui->uiitem [CONFUI_SPINBOX_PLAYER].list = tlist;
  confui->uiitem [CONFUI_SPINBOX_PLAYER].sblookuplist = llist;
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
  int           engbidx;
  int           shortidx;

  logProcBegin (LOG_PROC, "confuiLoadLocaleList");

  tlist = nlistAlloc ("cu-locale-list", LIST_ORDERED, free);
  llist = nlistAlloc ("cu-locale-list-l", LIST_ORDERED, free);

  pathbldMakePath (tbuff, sizeof (tbuff),
      "locales", ".txt", PATHBLD_MP_LOCALEDIR);
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

  confui->uiitem [CONFUI_SPINBOX_LOCALE].list = tlist;
  confui->uiitem [CONFUI_SPINBOX_LOCALE].sblookuplist = llist;
  logProcEnd (LOG_PROC, "confuiLoadLocaleList", "");
}


static void
confuiLoadDanceTypeList (configui_t *confui)
{
  nlist_t       *tlist = NULL;
  dnctype_t     *dnctypes;
  slistidx_t    iteridx;
  char          *key;
  int           count;

  logProcBegin (LOG_PROC, "confuiLoadDanceTypeList");

  tlist = nlistAlloc ("cu-dance-type", LIST_UNORDERED, free);

  dnctypes = bdjvarsdfGet (BDJVDF_DANCE_TYPES);
  dnctypesStartIterator (dnctypes, &iteridx);
  count = 0;
  while ((key = dnctypesIterate (dnctypes, &iteridx)) != NULL) {
    nlistSetStr (tlist, count, key);
    ++count;
  }

  confui->uiitem [CONFUI_SPINBOX_DANCE_TYPE].list = tlist;
  confui->uiitem [CONFUI_SPINBOX_DANCE_TYPE].sblookuplist = tlist;
  logProcEnd (LOG_PROC, "confuiLoadDanceTypeList", "");
}

static void
confuiLoadTagList (configui_t *confui)
{
  slist_t       *llist = NULL;

  logProcBegin (LOG_PROC, "confuiLoadTagList");

  llist = slistAlloc ("cu-tag-list", LIST_ORDERED, NULL);

  for (tagdefkey_t i = 0; i < TAG_KEY_MAX; ++i) {
    if (tagdefs [i].listingDisplay) {
      slistSetNum (llist, tagdefs [i].displayname, i);
    }
  }

  confui->listingtaglist = llist;
  logProcEnd (LOG_PROC, "confuiLoadTagList", "");
}


static gboolean
confuiFadeTypeTooltip (GtkWidget *w, gint x, gint y, gboolean kbmode,
    GtkTooltip *tt, void *udata)
{
  configui_t  *confui = udata;
  GdkPixbuf   *pixbuf;

  logProcBegin (LOG_PROC, "confuiFadeTypeTooltip");
  pixbuf = gtk_image_get_pixbuf (GTK_IMAGE (confui->fadetypeImage));
  gtk_tooltip_set_icon (tt, pixbuf);
  logProcEnd (LOG_PROC, "confuiFadeTypeTooltip", "");
  return TRUE;
}

static void
confuiOrgPathSelect (GtkTreeView *tv, GtkTreePath *path,
    GtkTreeViewColumn *column, gpointer udata)
{
  configui_t        *confui = udata;
  char              *sval;

  logProcBegin (LOG_PROC, "confuiOrgPathSelect");
  sval = confuiComboboxSelect (confui, path, CONFUI_COMBOBOX_AO_PATHFMT);
  confuiUpdateOrgExamples (confui, sval);
  logProcEnd (LOG_PROC, "confuiOrgPathSelect", "");
}

static char *
confuiComboboxSelect (configui_t *confui, GtkTreePath *path, int widx)
{
  uiutilsdropdown_t *dd = NULL;
  ssize_t           idx;
  char              *sval;

  logProcBegin (LOG_PROC, "confuiComboboxSelect");
  dd = &confui->uiitem [widx].u.dropdown;
  idx = uiutilsDropDownSelectionGet (dd, path);
  sval = slistGetDataByIdx (confui->uiitem [widx].list, idx);
  confui->uiitem [widx].listidx = idx;
  logProcEnd (LOG_PROC, "confuiComboboxSelect", "");
  return sval;
}

static void
confuiUpdateMobmqQrcode (configui_t *confui)
{
  char          uri [MAXPATHLEN];
  char          *qruri = "";
  char          tbuff [MAXPATHLEN];
  char          *tag;
  bdjmobilemq_t type;
  GtkWidget     *widget;

  logProcBegin (LOG_PROC, "confuiUpdateMobmqQrcode");

  type = (bdjmobilemq_t) bdjoptGetNum (OPT_P_MOBILEMARQUEE);

  if (type == MOBILEMQ_OFF) {
    *tbuff = '\0';
    *uri = '\0';
    qruri = "";
  }
  if (type == MOBILEMQ_INTERNET) {
    tag = bdjoptGetStr (OPT_P_MOBILEMQTAG);
    snprintf (uri, sizeof (uri), "%s%s?v=1&tag=%s",
        sysvarsGetStr (SV_MOBMQ_HOST), sysvarsGetStr (SV_MOBMQ_URL),
        tag);
  }
  if (type == MOBILEMQ_LOCAL) {
    char *ip;

    ip = confuiGetLocalIP (confui);
    snprintf (uri, sizeof (uri), "http://%s:%zd",
        ip, bdjoptGetNum (OPT_P_MOBILEMQPORT));
  }

  if (type != MOBILEMQ_OFF) {
    /* CONTEXT: config: qr code: title display for mobile marquee */
    qruri = confuiMakeQRCodeFile (confui, _("Mobile Marquee"), uri);
  }

  widget = confui->uiitem [CONFUI_WIDGET_MMQ_QR_CODE].u.widget;
  gtk_link_button_set_uri (GTK_LINK_BUTTON (widget), qruri);
  gtk_button_set_label (GTK_BUTTON (widget), uri);
  if (*qruri) {
    free (qruri);
  }
  logProcEnd (LOG_PROC, "confuiUpdateMobmqQrcode", "");
}

static void
confuiMobmqTypeChg (GtkSpinButton *sb, gpointer udata)
{
  configui_t    *confui = udata;
  GtkAdjustment *adjustment;
  double        value;
  ssize_t       nval;

  logProcBegin (LOG_PROC, "confuiMobmqTypeChg");
  adjustment = gtk_spin_button_get_adjustment (sb);
  value = gtk_adjustment_get_value (adjustment);
  nval = (ssize_t) value;
  bdjoptSetNum (OPT_P_MOBILEMARQUEE, nval);
  confuiUpdateMobmqQrcode (confui);
  logProcEnd (LOG_PROC, "confuiMobmqTypeChg", "");
}

static void
confuiMobmqPortChg (GtkSpinButton *sb, gpointer udata)
{
  configui_t    *confui = udata;
  GtkAdjustment *adjustment;
  double        value;
  ssize_t       nval;

  logProcBegin (LOG_PROC, "confuiMobmqPortChg");
  adjustment = gtk_spin_button_get_adjustment (sb);
  value = gtk_adjustment_get_value (adjustment);
  nval = (ssize_t) value;
  bdjoptSetNum (OPT_P_MOBILEMQPORT, nval);
  confuiUpdateMobmqQrcode (confui);
  logProcEnd (LOG_PROC, "confuiMobmqPortChg", "");
}

static bool
confuiMobmqNameChg (void *edata, void *udata)
{
  uiutilsentry_t  *entry = edata;
  configui_t    *confui = udata;
  const char      *sval;

  logProcBegin (LOG_PROC, "confuiMobmqNameChg");
  sval = uiutilsEntryGetValue (entry);
  bdjoptSetStr (OPT_P_MOBILEMQTAG, sval);
  confuiUpdateMobmqQrcode (confui);
  logProcEnd (LOG_PROC, "confuiMobmqNameChg", "");
  return true;
}

static bool
confuiMobmqTitleChg (void *edata, void *udata)
{
  uiutilsentry_t  *entry = edata;
  configui_t      *confui = udata;
  const char      *sval;

  logProcBegin (LOG_PROC, "confuiMobmqTitleChg");
  sval = uiutilsEntryGetValue (entry);
  bdjoptSetStr (OPT_P_MOBILEMQTITLE, sval);
  confuiUpdateMobmqQrcode (confui);
  logProcEnd (LOG_PROC, "confuiMobmqTitleChg", "");
  return true;
}

static void
confuiUpdateRemctrlQrcode (configui_t *confui)
{
  char          uri [MAXPATHLEN];
  char          *qruri = "";
  char          tbuff [MAXPATHLEN];
  ssize_t       onoff;
  GtkWidget     *widget;

  logProcBegin (LOG_PROC, "confuiUpdateRemctrlQrcode");

  onoff = (bdjmobilemq_t) bdjoptGetNum (OPT_P_REMOTECONTROL);

  if (onoff == 0) {
    *tbuff = '\0';
    *uri = '\0';
    qruri = "";
  }
  if (onoff == 1) {
    char *ip;

    ip = confuiGetLocalIP (confui);
    snprintf (uri, sizeof (uri), "http://%s:%zd",
        ip, bdjoptGetNum (OPT_P_REMCONTROLPORT));
  }

  if (onoff == 1) {
    /* CONTEXT: config: qr code: title display for mobile remote control */
    qruri = confuiMakeQRCodeFile (confui, _("Mobile Remote Control"), uri);
  }

  widget = confui->uiitem [CONFUI_WIDGET_RC_QR_CODE].u.widget;
  gtk_link_button_set_uri (GTK_LINK_BUTTON (widget), qruri);
  gtk_button_set_label (GTK_BUTTON (widget), uri);
  if (*qruri) {
    free (qruri);
  }
  logProcEnd (LOG_PROC, "confuiUpdateRemctrlQrcode", "");
}

static gboolean
confuiRemctrlChg (GtkSwitch *sw, gboolean value, gpointer udata)
{
  configui_t  *confui = udata;

  logProcBegin (LOG_PROC, "confuiRemctrlChg");
  bdjoptSetNum (OPT_P_REMOTECONTROL, value);
  confuiUpdateRemctrlQrcode (confui);
  logProcEnd (LOG_PROC, "confuiRemctrlChg", "");
  return FALSE;
}

static void
confuiRemctrlPortChg (GtkSpinButton *sb, gpointer udata)
{
  configui_t    *confui = udata;
  GtkAdjustment *adjustment;
  double        value;
  ssize_t       nval;

  logProcBegin (LOG_PROC, "confuiRemctrlPortChg");
  adjustment = gtk_spin_button_get_adjustment (sb);
  value = gtk_adjustment_get_value (adjustment);
  nval = (ssize_t) value;
  bdjoptSetNum (OPT_P_REMCONTROLPORT, nval);
  confuiUpdateRemctrlQrcode (confui);
  logProcEnd (LOG_PROC, "confuiRemctrlPortChg", "");
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

  pathbldMakePath (tbuff, sizeof (tbuff),
      "qrcode", ".html", PATHBLD_MP_TMPDIR);
  fh = fopen (tbuff, "w");
  fwrite (ndata, dlen, 1, fh);
  fclose (fh);
  snprintf (qruri, MAXPATHLEN, "file://%s/%s",
      sysvarsGetStr (SV_BDJ4DATATOPDIR), tbuff);

  free (data);
  free (ndata);
  logProcEnd (LOG_PROC, "confuiMakeQRCodeFile", "");
  return qruri;
}

static void
confuiUpdateOrgExamples (configui_t *confui, char *pathfmt)
{
  char      *data;
  org_t     *org;
  GtkWidget *widget;

  logProcBegin (LOG_PROC, "confuiUpdateOrgExamples");
  org = orgAlloc (pathfmt);

  data = "FILE\n..none\nDISC\n..1\nTRACKNUMBER\n..1\nALBUM\n..Smooth\nALBUMARTIST\n..Santana\nARTIST\n..Santana\nDANCE\n..Cha Cha\nGENRE\n..Ballroom Dance\nTITLE\n..Smooth\n";
  widget = confui->uiitem [CONFUI_WIDGET_AO_EXAMPLE_1].u.widget;
  confuiUpdateOrgExample (confui, org, data, widget);

  data = "FILE\n..none\nDISC\n..1\nTRACKNUMBER\n..2\nALBUM\n..The Ultimate Latin Album 4: Latin Eyes\nALBUMARTIST\n..WRD\nARTIST\n..Gizelle D'Cole\nDANCE\n..Rumba\nGENRE\n..Ballroom Dance\nTITLE\n..Asi\n";
  widget = confui->uiitem [CONFUI_WIDGET_AO_EXAMPLE_2].u.widget;
  confuiUpdateOrgExample (confui, org, data, widget);

  data = "FILE\n..none\nDISC\n..1\nTRACKNUMBER\n..3\nALBUM\n..Shaman\nALBUMARTIST\n..Santana\nARTIST\n..Santana\nDANCE\n..Waltz\nTITLE\n..The Game of Love\nGENRE\n..Latin";
  widget = confui->uiitem [CONFUI_WIDGET_AO_EXAMPLE_3].u.widget;
  confuiUpdateOrgExample (confui, org, data, widget);

  data = "FILE\n..none\nDISC\n..2\nTRACKNUMBER\n..2\nALBUM\n..The Ultimate Latin Album 9: Footloose\nALBUMARTIST\n..\nARTIST\n..Raphael\nDANCE\n..Rumba\nTITLE\n..Ni tú ni yo\nGENRE\n..Latin";
  widget = confui->uiitem [CONFUI_WIDGET_AO_EXAMPLE_4].u.widget;
  confuiUpdateOrgExample (confui, org, data, widget);

  orgFree (org);
  logProcEnd (LOG_PROC, "confuiUpdateOrgExamples", "");
}

static void
confuiUpdateOrgExample (configui_t *config, org_t *org, char *data, GtkWidget *widget)
{
  song_t    *song;
  char      *tdata;
  char      *disp;

  logProcBegin (LOG_PROC, "confuiUpdateOrgExample");
  tdata = strdup (data);
  song = songAlloc ();
  songParse (song, tdata, 0);
  disp = orgMakeSongPath (org, song);
  gtk_label_set_text (GTK_LABEL (widget), disp);
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

static int
confuiGetSelectionIter (GtkWidget *tree, GtkTreeModel **model, GtkTreeIter *iter)
{
  GtkTreeSelection  *sel;
  int               count;

  sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
  count = gtk_tree_selection_count_selected_rows (sel);
  if (count == 1) {
    gtk_tree_selection_get_selected (sel, model, iter);
  }
  return count;
}

/* table editing */
static void
confuiTableMoveUp (GtkButton *b, gpointer udata)
{
  logProcBegin (LOG_PROC, "confuiTableMoveUp");
  confuiTableMove (udata, CONFUI_MOVE_PREV);
  logProcEnd (LOG_PROC, "confuiTableMoveUp", "");
}

static void
confuiTableMoveDown (GtkButton *b, gpointer udata)
{
  logProcBegin (LOG_PROC, "confuiTableMoveDown");
  confuiTableMove (udata, CONFUI_MOVE_NEXT);
  logProcEnd (LOG_PROC, "confuiTableMoveDown", "");
}

static void
confuiTableMove (configui_t *confui, int dir)
{
  GtkTreeSelection  *sel;
  GtkWidget         *tree;
  GtkTreeModel      *model;
  GtkTreeIter       iter;
  GtkTreeIter       citer;
  GtkTreePath       *path;
  char              *pathstr;
  int               count;
  gboolean          valid;
  int               idx;
  int               flags;

  logProcBegin (LOG_PROC, "confuiTableMove");
  tree = confui->tables [confui->tablecurr].tree;
  if (confui->tablecurr == CONFUI_ID_DISP_SEL_LIST) {
    tree = confui->tables [CONFUI_ID_DISP_SEL_TABLE].tree;
  }
  flags = confui->tables [confui->tablecurr].flags;

  sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
  count = gtk_tree_selection_count_selected_rows (sel);
  if (count != 1) {
    logProcEnd (LOG_PROC, "confuiTableMove", "no-selection");
    return;
  }
  gtk_tree_selection_get_selected (sel, &model, &iter);

  path = gtk_tree_model_get_path (model, &iter);
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

static void
confuiTableRemove (GtkButton *b, gpointer udata)
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
  flags = confui->tables [confui->tablecurr].flags;
  count = confuiGetSelectionIter (tree, &model, &iter);
  if (count != 1) {
    logProcEnd (LOG_PROC, "confuiTableRemove", "no-selection");
    return;
  }

  path = gtk_tree_model_get_path (model, &iter);
  pathstr = gtk_tree_path_to_string (path);
  sscanf (pathstr, "%d", &idx);
  free (pathstr);
  gtk_tree_path_free (path);
  if (idx == 0 &&
      (flags & CONFUI_TABLE_KEEP_FIRST) == CONFUI_TABLE_KEEP_FIRST) {
    logProcEnd (LOG_PROC, "confuiTableRemove", "keep-first");
    return;
  }
  if (idx == confui->tables [confui->tablecurr].currcount - 1 &&
      (flags & CONFUI_TABLE_KEEP_LAST) == CONFUI_TABLE_KEEP_LAST) {
    logProcEnd (LOG_PROC, "confuiTableRemove", "keep-last");
    return;
  }

  if (confui->tablecurr == CONFUI_ID_DANCE) {
    gulong        idx;
    dance_t       *dances;
    GtkWidget     *tree;
    GtkTreeModel  *model;
    GtkTreeIter   iter;

    tree = confui->tables [confui->tablecurr].tree;
    confuiGetSelectionIter (tree, &model, &iter);
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
    confuiGetSelectionIter (tree, &model, &iter);
    path = gtk_tree_model_get_path (model, &iter);
    confuiDanceSelect (GTK_TREE_VIEW (tree), path, NULL, confui);
  }
}

static void
confuiTableAdd (GtkButton *b, gpointer udata)
{
  configui_t        *confui = udata;
  GtkTreeSelection  *sel = NULL;
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
    return;
  }

  tree = confui->tables [confui->tablecurr].tree;
  flags = confui->tables [confui->tablecurr].flags;
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree));
  sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
  if (sel != NULL) {
    count = gtk_tree_selection_count_selected_rows (sel);
    if (count != 1) {
      titer = NULL;
    } else {
      gtk_tree_selection_get_selected (sel, &model, &iter);
      titer = &iter;
    }
  } else {
    titer = NULL;
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
      /* CONTEXT: config: dance name that is set when adding a new dance */
      dkey = danceAdd (dances, _("New Dance"));
      /* CONTEXT: config: dance name that is set when adding a new dance */
      confuiDanceSet (GTK_LIST_STORE (model), &niter, _("New Dance"), dkey);
      break;
    }

    case CONFUI_ID_GENRES: {
      /* CONTEXT: config: genre name that is set when adding a new genre */
      confuiGenreSet (GTK_LIST_STORE (model), &niter, TRUE, _("New Genre"), 0);
      break;
    }

    case CONFUI_ID_RATINGS: {
      /* CONTEXT: config: rating name that is set when adding a new rating */
      confuiRatingSet (GTK_LIST_STORE (model), &niter, TRUE, _("New Rating"), 0);
      break;
    }

    case CONFUI_ID_LEVELS: {
      /* CONTEXT: config: level name that is set when adding a new level */
      confuiLevelSet (GTK_LIST_STORE (model), &niter, TRUE, _("New Level"), 0, 0);
      break;
    }

    case CONFUI_ID_STATUS: {
      /* CONTEXT: config: status name that is set when adding a new status */
      confuiStatusSet (GTK_LIST_STORE (model), &niter, TRUE, _("New Status"), 0);
      break;
    }

    default: {
      break;
    }
  }

  path = gtk_tree_model_get_path (model, &niter);
  gtk_tree_view_set_cursor (GTK_TREE_VIEW (tree), path, NULL, FALSE);
  if (confui->tablecurr == CONFUI_ID_DANCE) {
    confuiDanceSelect (GTK_TREE_VIEW (tree), path, NULL, confui);
  }
  gtk_tree_path_free (path);

  confui->tables [confui->tablecurr].changed = true;
  confui->tables [confui->tablecurr].currcount += 1;
  logProcEnd (LOG_PROC, "confuiTableAdd", "");
}

static void
confuiSwitchTable (GtkNotebook *nb, GtkWidget *page, guint pagenum, gpointer udata)
{
  configui_t        *confui = udata;
  GtkWidget         *tree;

  logProcBegin (LOG_PROC, "confuiSwitchTable");
  if (pagenum >= (unsigned int) confui->tabcount) {
    logProcEnd (LOG_PROC, "confuiSwitchTable", "bad-pagenum");
    return;
  }

  if (confui->tablecurr == confui->tableidents [pagenum]) {
    logProcEnd (LOG_PROC, "confuiSwitchTable", "same-page");
    return;
  }

  confui->tablecurr = confui->tableidents [pagenum];

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
    confuiCreateTagTableDisp (confui);
    confuiCreateTagListingDisp (confui);
  }

  if (confui->tablecurr >= CONFUI_ID_TABLE_MAX) {
    logProcEnd (LOG_PROC, "confuiSwitchTable", "non-table");
    return;
  }

  tree = confui->tables [confui->tablecurr].tree;

  confuiTableSetDefaultSelection (confui, tree);

  logProcEnd (LOG_PROC, "confuiSwitchTable", "");
}

static void
confuiTableSetDefaultSelection (configui_t *confui, GtkWidget *tree)
{
  int               count;
  GtkTreeIter       iter;
  GtkTreeModel      *model;


  count = confuiGetSelectionIter (tree, &model, &iter);
  if (count != 1) {
    GtkTreePath   *path;

    path = gtk_tree_path_new_from_string ("0");
    if (path != NULL) {
      gtk_tree_view_set_cursor (GTK_TREE_VIEW (tree), path, NULL, FALSE);
    }
    if (confui->tablecurr == CONFUI_ID_DANCE) {
      confuiDanceSelect (GTK_TREE_VIEW (tree), path, NULL, confui);
    }
    gtk_tree_path_free (path);
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
      G_TYPE_STRING, G_TYPE_STRING, G_TYPE_ULONG);
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
      CONFUI_DANCE_COL_DANCE_IDX, key,
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
      G_TYPE_ULONG, G_TYPE_ULONG, G_TYPE_STRING,
      G_TYPE_ULONG, G_TYPE_OBJECT, G_TYPE_ULONG);
  assert (store != NULL);

  ratingStartIterator (ratings, &iteridx);

  editable = FALSE;
  while ((key = ratingIterate (ratings, &iteridx)) >= 0) {
    char    *ratingdisp;
    ssize_t weight;

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
  /* CONTEXT: config: rating: title of the rating name column */
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
  /* CONTEXT: config: rating: title of the weight column */
  gtk_tree_view_column_set_title (column, _("Weight"));
  g_signal_connect (renderer, "edited", G_CALLBACK (confuiTableEditSpinbox), confui);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

  gtk_tree_view_set_model (GTK_TREE_VIEW (tree), GTK_TREE_MODEL (store));
  g_object_unref (store);
  logProcEnd (LOG_PROC, "confuiCreateRatingTable", "");
}

static void
confuiRatingSet (GtkListStore *store, GtkTreeIter *iter,
    int editable, char *ratingdisp, ssize_t weight)
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
      G_TYPE_ULONG, G_TYPE_STRING, G_TYPE_BOOLEAN);
  assert (store != NULL);

  statusStartIterator (status, &iteridx);

  editable = FALSE;
  while ((key = statusIterate (status, &iteridx)) >= 0) {
    char    *statusdisp;
    ssize_t playflag;

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
  /* CONTEXT: config: status: title of the status name column */
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
  /* CONTEXT: config: status: title of the "playable" column */
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
      G_TYPE_ULONG, G_TYPE_STRING, G_TYPE_ULONG, G_TYPE_BOOLEAN,
      G_TYPE_OBJECT, G_TYPE_ULONG);
  assert (store != NULL);

  levelStartIterator (levels, &iteridx);

  while ((key = levelIterate (levels, &iteridx)) >= 0) {
    char    *leveldisp;
    ssize_t weight;
    ssize_t def;

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
  /* CONTEXT: config: level: title of the level name column */
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
  /* CONTEXT: config: level: title of the weight column */
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
  /* CONTEXT: config: level: title of the default selection column */
  gtk_tree_view_column_set_title (column, _("Default"));
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

  gtk_tree_view_set_model (GTK_TREE_VIEW (tree), GTK_TREE_MODEL (store));
  g_object_unref (store);
  logProcEnd (LOG_PROC, "confuiCreateLevelTable", "");
}

static void
confuiLevelSet (GtkListStore *store, GtkTreeIter *iter,
    int editable, char *leveldisp, ssize_t weight, int def)
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
      G_TYPE_ULONG, G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_STRING);
  assert (store != NULL);

  genreStartIterator (genres, &iteridx);

  while ((key = genreIterate (genres, &iteridx)) >= 0) {
    char    *genredisp;
    ssize_t clflag;

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
  /* CONTEXT: config: genre: title of the genre name column */
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
  /* CONTEXT: config: genre: title of the classical setting column */
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
  if (gtk_tree_model_get_iter (model, &iter, path) == FALSE) {
    logProcEnd (LOG_PROC, "confuiTableToggle", "no model/iter");
    return;
  }
  gtk_tree_path_free (path);
  col = confui->tables [confui->tablecurr].togglecol;
  gtk_tree_model_get (model, &iter, col, &val, -1);
  gtk_list_store_set (GTK_LIST_STORE (model), &iter, col, !val, -1);
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
    gulong val = atol (ntext);
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
  gulong      weight;

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
  gulong      weight;
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
  dances = bdjvarsdfGet (BDJVDF_DANCES);
  /* the data is already saved in the dance list; just re-use it */
  danceSave (dances, dances->dances);
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
  ssize_t       num;
  slist_t       *slist;
  datafileconv_t conv;
  dance_t       *dances;

  logProcBegin (LOG_PROC, "confuiDanceSelect");
  model = gtk_tree_view_get_model (tv);
  if (! gtk_tree_model_get_iter (model, &iter, path)) {
    logProcEnd (LOG_PROC, "confuiDanceSelect", "no model/iter");
    return;
  }
  gtk_tree_model_get (model, &iter, CONFUI_DANCE_COL_DANCE_IDX, &idx, -1);
  key = (ilistidx_t) idx;

  dances = bdjvarsdfGet (BDJVDF_DANCES);

  sval = danceGetStr (dances, key, DANCE_DANCE);
  widx = CONFUI_ENTRY_DANCE_DANCE;
  uiutilsEntrySetValue (&confui->uiitem [widx].u.entry, sval);

  slist = danceGetList (dances, key, DANCE_TAGS);
  conv.allocated = false;
  conv.u.list = slist;
  conv.valuetype = VALUE_LIST;
  convTextList (&conv);
  sval = conv.u.str;
  widx = CONFUI_ENTRY_DANCE_TAGS;
  uiutilsEntrySetValue (&confui->uiitem [widx].u.entry, sval);
  free (conv.u.str);

  sval = danceGetStr (dances, key, DANCE_ANNOUNCE);
  widx = CONFUI_ENTRY_DANCE_ANNOUNCEMENT;
  uiutilsEntrySetValue (&confui->uiitem [widx].u.entry, sval);

  num = danceGetNum (dances, key, DANCE_HIGH_BPM);
  widx = CONFUI_SPINBOX_DANCE_HIGH_BPM;
  uiutilsSpinboxTextSetValue (&confui->uiitem [widx].u.spinbox, (double) num);

  num = danceGetNum (dances, key, DANCE_LOW_BPM);
  widx = CONFUI_SPINBOX_DANCE_LOW_BPM;
  uiutilsSpinboxTextSetValue (&confui->uiitem [widx].u.spinbox, (double) num);

  num = danceGetNum (dances, key, DANCE_SPEED);
  widx = CONFUI_SPINBOX_DANCE_SPEED;
  uiutilsSpinboxTextSetValue (&confui->uiitem [widx].u.spinbox, (double) num);

  num = danceGetNum (dances, key, DANCE_TIMESIG);
  widx = CONFUI_SPINBOX_DANCE_TIME_SIG;
  uiutilsSpinboxTextSetValue (&confui->uiitem [widx].u.spinbox, (double) num);

  num = danceGetNum (dances, key, DANCE_TYPE);
  widx = CONFUI_SPINBOX_DANCE_TYPE;
  uiutilsSpinboxTextSetValue (&confui->uiitem [widx].u.spinbox, (double) num);
  logProcEnd (LOG_PROC, "confuiDanceSelect", "");
}

static void
confuiDanceEntryChg (GtkEditable *e, gpointer udata)
{
  configui_t      *confui = udata;
  uiutilsentry_t  *entry;
  const char      *str;
  GtkWidget       *tree;
  GtkTreeModel    *model;
  GtkTreeIter     iter;
  int             count;
  gulong          idx;
  ssize_t         key;
  dance_t         *dances;
  int             didx;
  datafileconv_t  conv;
  int             widx;

  logProcBegin (LOG_PROC, "confuiDanceEntryChg");
  widx = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (e), "confuiwidx"));

  entry = &confui->uiitem [widx].u.entry;
  if (entry->buffer == NULL) {
    logProcEnd (LOG_PROC, "confuiDanceEntryChg", "no-buffer");
    return;
  }
  str = gtk_entry_buffer_get_text (entry->buffer);
  if (str == NULL) {
    logProcEnd (LOG_PROC, "confuiDanceEntryChg", "null-string");
    return;
  }

  didx = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (e), "confuididx"));

  tree = confui->tables [CONFUI_ID_DANCE].tree;
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree));
  count = confuiGetSelectionIter (tree, &model, &iter);
  if (count != 1) {
    logProcEnd (LOG_PROC, "confuiDanceEntryChg", "no-selection");
    return;
  }

  if (didx == DANCE_DANCE) {
    gtk_list_store_set (GTK_LIST_STORE (model), &iter,
        CONFUI_DANCE_COL_DANCE, str,
        -1);
  }

  dances = bdjvarsdfGet (BDJVDF_DANCES);
  gtk_tree_model_get (model, &iter, CONFUI_DANCE_COL_DANCE_IDX, &idx, -1);
  key = (ssize_t) idx;

  if (didx == DANCE_TAGS) {
    slist_t *slist;

    conv.allocated = true;
    conv.u.str = strdup (str);
    conv.valuetype = VALUE_STR;
    convTextList (&conv);
    slist = conv.u.list;
    danceSetList (dances, key, didx, slist);
  } else {
    danceSetStr (dances, key, didx, str);
  }
  confui->tables [confui->tablecurr].changed = true;
  logProcEnd (LOG_PROC, "confuiDanceEntryChg", "");
}

static void
confuiDanceSpinboxChg (GtkSpinButton *sb, gpointer udata)
{
  configui_t      *confui = udata;
  GtkAdjustment   *adjustment;
  GtkWidget       *tree;
  GtkTreeModel    *model;
  GtkTreeIter     iter;
  int             count;
  gulong          idx;
  double          value;
  ssize_t         nval;
  ssize_t         key;
  dance_t         *dances;
  int             didx;

  logProcBegin (LOG_PROC, "confuiDanceSpinboxChg");
  adjustment = gtk_spin_button_get_adjustment (sb);
  value = gtk_adjustment_get_value (adjustment);
  nval = (ssize_t) value;

  tree = confui->tables [CONFUI_ID_DANCE].tree;
  count = confuiGetSelectionIter (tree, &model, &iter);
  if (count != 1) {
    logProcEnd (LOG_PROC, "confuiDanceSpinboxChg", "no-selection");
    return;
  }

  didx = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (sb), "confuididx"));

  dances = bdjvarsdfGet (BDJVDF_DANCES);
  gtk_tree_model_get (model, &iter, CONFUI_DANCE_COL_DANCE_IDX, &idx, -1);
  key = (ssize_t) idx;
  danceSetNum (dances, key, didx, nval);
  confui->tables [confui->tablecurr].changed = true;
  logProcEnd (LOG_PROC, "confuiDanceSpinboxChg", "");
}

static bool
confuiDanceValidateAnnouncement (void *edata, void *udata)
{
  configui_t        *confui = udata;
  uiutilsentry_t    *entry = edata;
  bool              rc;
  const char        *fn;
  char              tbuff [MAXPATHLEN];
  char              nfn [MAXPATHLEN];
  char              *musicdir;
  size_t            mlen;

  logProcBegin (LOG_PROC, "confuiDanceValidateAnnouncement");
  musicdir = bdjoptGetStr (OPT_M_DIR_MUSIC);
  mlen = strlen (musicdir);

  rc = false;
  if (entry->buffer != NULL) {
    fn = gtk_entry_buffer_get_text (entry->buffer);
    if (fn == NULL) {
      logProcEnd (LOG_PROC, "confuiDanceValidateAnnouncement", "bad-fn");
      return rc;
    }

    strlcpy (nfn, fn, sizeof (nfn));
    pathNormPath (nfn, sizeof (nfn));
    if (strncmp (musicdir, fn, mlen) == 0) {
      strlcpy (nfn, fn + mlen + 1, sizeof (nfn));
      gtk_entry_buffer_set_text (entry->buffer, nfn, -1);
    }

    if (*nfn == '\0') {
      rc = true;
    } else {
      *tbuff = '\0';
      if (*nfn != '/' && *(nfn + 1) != ':') {
        strlcpy (tbuff, musicdir, sizeof (tbuff));
        strlcat (tbuff, "/", sizeof (tbuff));
      }
      strlcat (tbuff, nfn, sizeof (tbuff));
      if (fileopFileExists (tbuff)) {
        rc = true;
      }
    }
  }

  confui->tables [confui->tablecurr].changed = true;
  logProcEnd (LOG_PROC, "confuiDanceValidateAnnouncement", "");
  return rc;
}

/* display settings */

static void
confuiCreateTagListingTable (configui_t *confui)
{
  GtkWidget     *tree;
  GtkCellRenderer   *renderer = NULL;
  GtkTreeViewColumn *column = NULL;

  tree = confui->tables [CONFUI_ID_DISP_SEL_LIST].tree;
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tree), FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("", renderer,
      "text", CONFUI_TAG_COL_TAG,
      NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("", renderer,
      "text", CONFUI_TAG_COL_SB_PAD,
      NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

  tree = confui->tables [CONFUI_ID_DISP_SEL_TABLE].tree;
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tree), FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("", renderer,
      "text", CONFUI_TAG_COL_TAG,
      NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("", renderer,
      "text", CONFUI_TAG_COL_SB_PAD,
      NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
}

static void
confuiDispSettingChg (GtkSpinButton *sb, gpointer udata)
{
  configui_t  *confui = udata;
  int         nval;
  int         oval;


  oval = confui->uiitem [CONFUI_SPINBOX_DISP_SEL].listidx;
  nval = uiutilsSpinboxTextGetValue (&confui->uiitem [CONFUI_SPINBOX_DISP_SEL].u.spinbox);
  confui->uiitem [CONFUI_SPINBOX_DISP_SEL].listidx = nval;

  confuiDispSaveTable (confui, oval);
  confuiCreateTagTableDisp (confui);
  confuiCreateTagListingDisp (confui);
}

static void
confuiDispSaveTable (configui_t *confui, int selidx)
{
  slist_t       *tlist;
  gboolean      valid;
  GtkWidget     *tree;
  GtkTreeModel  *model;
  GtkTreeIter   iter;


  if (! confui->tables [CONFUI_ID_DISP_SEL_TABLE].changed) {
    return;
  }

  tree = confui->tables [CONFUI_ID_DISP_SEL_TABLE].tree;
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree));

  tlist = slistAlloc ("dispsel-save", LIST_UNORDERED, NULL);
  valid = gtk_tree_model_get_iter_first (model, &iter);
  while (valid) {
    gulong  tval;
    char    *tag;

    gtk_tree_model_get (model, &iter, CONFUI_TAG_COL_TAG_IDX, &tval, -1);
    tag = tagdefs [tval].tag;
    slistSetNum (tlist, tag, 0);
    valid = gtk_tree_model_iter_next (model, &iter);
  }

  dispselSave (confui->dispsel, selidx, tlist);
  slistFree (tlist);
}

static void
confuiCreateTagTableDisp (configui_t *confui)
{
  GtkWidget     *tree;
  GtkListStore  *store;
  GtkTreeIter   iter;
  char          *keystr;
  slistidx_t    seliteridx;
  dispselsel_t  selidx;
  slist_t       *sellist;
  dispsel_t     *dispsel;


  selidx = confui->uiitem [CONFUI_SPINBOX_DISP_SEL].listidx;
  dispsel = confui->dispsel;
  sellist = dispselGetList (dispsel, selidx);

  tree = confui->tables [CONFUI_ID_DISP_SEL_TABLE].tree;

  store = gtk_list_store_new (CONFUI_TAG_COL_MAX,
      G_TYPE_STRING, G_TYPE_STRING, G_TYPE_ULONG);
  assert (store != NULL);

  slistStartIterator (sellist, &seliteridx);
  while ((keystr = slistIterateKey (sellist, &seliteridx)) != NULL) {
    int val;

    val = slistGetNum (sellist, keystr);
    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter,
        CONFUI_TAG_COL_TAG, keystr,
        CONFUI_TAG_COL_SB_PAD, "    ",
        CONFUI_TAG_COL_TAG_IDX, val,
        -1);
  }

  gtk_tree_view_set_model (GTK_TREE_VIEW (tree), GTK_TREE_MODEL (store));
  g_object_unref (store);

  confuiTableSetDefaultSelection (confui, tree);
}


static void
confuiCreateTagListingDisp (configui_t *confui)
{
  GtkWidget     *tree;
  GtkListStore  *store;
  GtkTreeModel  *model;
  GtkTreeIter   iter;
  GtkWidget     *tabletree;
  GtkTreeModel  *tablemodel;
  GtkTreeIter   tableiter;
  GtkTreePath   *path;
  char          *keystr;
  slistidx_t    iteridx;
  dispselsel_t  selidx;
  gboolean      valid;
  int           count;


  selidx = confui->uiitem [CONFUI_SPINBOX_DISP_SEL].listidx;

  tree = confui->tables [CONFUI_ID_DISP_SEL_LIST].tree;

  tabletree = confui->tables [CONFUI_ID_DISP_SEL_TABLE].tree;
  tablemodel = gtk_tree_view_get_model (GTK_TREE_VIEW (tabletree));

  /* get the current selection for the listing display */
  count = confuiGetSelectionIter (tree, &model, &iter);
  if (count == 1) {
    path = gtk_tree_model_get_path (model, &iter);
  }

  store = gtk_list_store_new (CONFUI_TAG_COL_MAX,
      G_TYPE_STRING, G_TYPE_STRING, G_TYPE_ULONG);
  assert (store != NULL);

  slistStartIterator (confui->listingtaglist, &iteridx);
  while ((keystr = slistIterateKey (confui->listingtaglist, &iteridx)) != NULL) {
    int   val;
    bool  match = false;

    val = slistGetNum (confui->listingtaglist, keystr);

    if (selidx != DISP_SEL_SONGEDIT_A && selidx != DISP_SEL_SONGEDIT_B) {
      if (tagdefs [val].songEditOnly) {
        continue;
      }
    }

    match = false;
    /* just do a brute force search; the lists are not long */
    /* horribly inefficient */
    valid = gtk_tree_model_get_iter_first (tablemodel, &tableiter);
    while (valid) {
      gulong  tval;

      gtk_tree_model_get (tablemodel, &tableiter, CONFUI_TAG_COL_TAG_IDX, &tval, -1);

      if ((int) tval == val) {
        match = true;
        break;
      }
      valid = gtk_tree_model_iter_next (tablemodel, &tableiter);
      if (! valid) {
        break;
      }
    }
    if (match) {
      continue;
    }

    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter,
        CONFUI_TAG_COL_TAG, keystr,
        CONFUI_TAG_COL_SB_PAD, "    ",
        CONFUI_TAG_COL_TAG_IDX, val,
        -1);
  }

  gtk_tree_view_set_model (GTK_TREE_VIEW (tree), GTK_TREE_MODEL (store));
  g_object_unref (store);

  if (count == 1) {
    gtk_tree_view_set_cursor (GTK_TREE_VIEW (tree), path, NULL, FALSE);
  }
  confuiTableSetDefaultSelection (confui, tree);
}

static void
confuiDispSelect (GtkButton *b, gpointer udata)
{
  configui_t    *confui = udata;
  GtkWidget     *tree;
  GtkTreeModel  *model;
  GtkTreeIter   iter;
  GtkTreePath   *path;
  int           count;

  tree = confui->tables [CONFUI_ID_DISP_SEL_LIST].tree;
  count = confuiGetSelectionIter (tree, &model, &iter);

  if (count == 1) {
    char          *str;
    gulong        tval;
    GtkWidget     *tabletree;
    GtkTreeModel  *tablemodel;

    tabletree = confui->tables [CONFUI_ID_DISP_SEL_TABLE].tree;
    tablemodel = gtk_tree_view_get_model (GTK_TREE_VIEW (tabletree));

    gtk_tree_model_get (model, &iter, CONFUI_TAG_COL_TAG, &str, -1);
    gtk_tree_model_get (model, &iter, CONFUI_TAG_COL_TAG_IDX, &tval, -1);

    gtk_list_store_append (GTK_LIST_STORE (tablemodel), &iter);
    gtk_list_store_set (GTK_LIST_STORE (tablemodel), &iter,
        CONFUI_TAG_COL_TAG, str,
        CONFUI_TAG_COL_SB_PAD, "    ",
        CONFUI_TAG_COL_TAG_IDX, tval,
        -1);

    path = gtk_tree_model_get_path (tablemodel, &iter);
    gtk_tree_view_set_cursor (GTK_TREE_VIEW (tabletree), path, NULL, FALSE);

    confui->tables [CONFUI_ID_DISP_SEL_TABLE].changed = true;
    confuiCreateTagListingDisp (confui);
  }
}

static void
confuiDispRemove (GtkButton *b, gpointer udata)
{
  configui_t    * confui = udata;

  GtkWidget     * tree;
  GtkTreeModel  * model;
  GtkTreeIter   iter;
  int           count;

  tree = confui->tables [CONFUI_ID_DISP_SEL_TABLE].tree;
  count = confuiGetSelectionIter (tree, &model, &iter);

  if (count == 1) {
    gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
    confui->tables [CONFUI_ID_DISP_SEL_TABLE].changed = true;
    confuiCreateTagListingDisp (confui);
  }
}
