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
  CONFUI_ENTRY_QUEUE_NM_A,
  CONFUI_ENTRY_QUEUE_NM_B,
  CONFUI_ENTRY_RC_PASS,
  CONFUI_ENTRY_RC_USER_ID,
  CONFUI_ENTRY_SHUTDOWN,
  CONFUI_ENTRY_STARTUP,
  CONFUI_ENTRY_MAX,
  CONFUI_SPINBOX_AUDIO,
  CONFUI_SPINBOX_AUDIO_OUTPUT,
  CONFUI_SPINBOX_BPM,
  CONFUI_SPINBOX_DANCE_HIGH_BPM,
  CONFUI_SPINBOX_DANCE_LOW_BPM,
  CONFUI_SPINBOX_DANCE_SPEED,
  CONFUI_SPINBOX_DANCE_TIME_SIG,
  CONFUI_SPINBOX_DANCE_TYPE,
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
  CONFUI_ID_MAX,
  CONFUI_ID_NONE,
} confuiident_t;

enum {
  CONFUI_TABLE_MOVE_PREV,
  CONFUI_TABLE_MOVE_NEXT,
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

typedef struct {
  progstate_t       *progstate;
  char              *locknm;
  procutil_t        *processes [ROUTE_MAX];
  conn_t            *conn;
  sockserver_t      *sockserver;
  int               dbgflags;
  confuiitem_t      uiitem [CONFUI_ITEM_MAX];
  int               uithemeidx;
  int               mqthemeidx;
  int               rchtmlidx;
  int               localeidx;
  int               audiosinkidx;
  int               tabcount;
  confuiident_t     tablecurr;
  int               *tableidents;
  volume_t          *volume;
  /* gtk stuff */
  GtkApplication    *app;
  GtkWidget         *window;
  GtkWidget         *vbox;
  GtkWidget         *notebook;
  confuitable_t     tables [CONFUI_ID_MAX];
  GtkWidget         *fadetypeImage;
  GtkWidget         *dsnotebook;
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
static int      confuiCreateGui (configui_t *confui, int argc, char *argv []);
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

static nlist_t  * confuiGetThemeList (void);
static nlist_t  * confuiGetThemeNames (nlist_t *themelist, slist_t *filelist);
static void     confuiLoadHTMLList (configui_t *confui);
static void     confuiLoadLocaleList (configui_t *confui);
static void     confuiLoadDanceTypeList (configui_t *confui);
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
static char *   confuiMakeQRCodeFile (configui_t *confui, char *title, char *uri);
static void     confuiUpdateOrgExamples (configui_t *confui, char *pathfmt);
static void     confuiUpdateOrgExample (configui_t *config, org_t *org, char *data, GtkWidget *widget);

/* table editing */
static void   confuiTableMoveUp (GtkButton *b, gpointer udata);
static void   confuiTableMoveDown (GtkButton *b, gpointer udata);
static void   confuiTableMove (configui_t *confui, int dir);
static void   confuiTableRemove (GtkButton *b, gpointer udata);
static void   confuiTableAdd (GtkButton *b, gpointer udata);
static void   confuiSwitchTable (GtkNotebook *nb, GtkWidget *page, guint pagenum, gpointer udata);
static void   confuiCreateDanceTable (configui_t *confui);
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
static void   confuiRatingSave (configui_t *confui);
static void   confuiLevelSave (configui_t *confui);
static void   confuiStatusSave (configui_t *confui);
static void   confuiGenreSave (configui_t *confui);

static void   confuiDanceSelect (GtkTreeView *tv, GtkTreePath *path,
    GtkTreeViewColumn *column, gpointer udata);


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
  orgopt_t        *orgopt;


  confui.notebook = NULL;
  confui.dsnotebook = NULL;
  confui.progstate = progstateInit ("configui");
  progstateSetCallback (confui.progstate, STATE_WAIT_HANDSHAKE,
      confuiHandshakeCallback, &confui);
  confui.sockserver = NULL;
  confui.window = NULL;
  confui.uithemeidx = 0;
  confui.mqthemeidx = 0;
  confui.rchtmlidx = 0;
  confui.localeidx = 0;
  confui.tabcount = 0;
  confui.tablecurr = CONFUI_ID_NONE;
  confui.tableidents = NULL;
  for (int i = 0; i < CONFUI_ID_MAX; ++i) {
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

  pathbldMakePath (tbuff, sizeof (tbuff), "",
      "orgopt", ".txt", PATHBLD_MP_NONE);
  orgopt = orgoptAlloc (tbuff);
  tlist = orgoptGetList (orgopt);
  confui.uiitem [CONFUI_COMBOBOX_AO_PATHFMT].list = tlist;
  slistStartIterator (tlist, &iteridx);
  count = 0;
  while ((p = slistIterateValueData (tlist, &iteridx)) != NULL) {
    if (strcmp (p, bdjoptGetStr (OPT_G_AO_PATHFMT)) == 0) {
      confui.uiitem [CONFUI_COMBOBOX_AO_PATHFMT].listidx = count;
      break;
    }
    ++count;
  }

  confui.volume = volumeInit ();
  assert (confui.volume != NULL);
  volumeGetSinkList (confui.volume, "", &sinklist);
  tlist = nlistAlloc ("cu-audio-out", LIST_UNORDERED, free);
  llist = nlistAlloc ("cu-audio-out-l", LIST_ORDERED, free);
  confui.audiosinkidx = 0;
  for (size_t i = 0; i < sinklist.count; ++i) {
    if (strcmp (sinklist.sinklist [i].name, bdjoptGetStr (OPT_M_AUDIOSINK)) == 0) {
      confui.audiosinkidx = i;
    }
    nlistSetStr (tlist, i, sinklist.sinklist [i].description);
    nlistSetStr (llist, i, sinklist.sinklist [i].name);
  }
  confui.uiitem [CONFUI_SPINBOX_AUDIO_OUTPUT].list = tlist;
  confui.uiitem [CONFUI_SPINBOX_AUDIO_OUTPUT].sblookuplist = llist;
  volumeFreeSinkList (&sinklist);

  tlist = nlistAlloc ("cu-writetags", LIST_UNORDERED, free);
  /* order these in the same order as defined in bdjopt.h */
  nlistSetStr (tlist, WRITE_TAGS_NONE, _("Don't Write"));
  nlistSetStr (tlist, WRITE_TAGS_BDJ_ONLY, _("BDJ Tags Only"));
  nlistSetStr (tlist, WRITE_TAGS_ALL, _("All Tags"));
  confui.uiitem [CONFUI_SPINBOX_WRITE_AUDIO_FILE_TAGS].list = tlist;

  tlist = nlistAlloc ("cu-bpm", LIST_UNORDERED, free);
  /* order these in the same order as defined in bdjopt.h */
  nlistSetStr (tlist, BPM_BPM, _("BPM"));
  nlistSetStr (tlist, BPM_MPM, _("MPM"));
  confui.uiitem [CONFUI_SPINBOX_BPM].list = tlist;

  tlist = nlistAlloc ("cu-fadetype", LIST_UNORDERED, free);
  /* order these in the same order as defined in bdjopt.h */
  nlistSetStr (tlist, FADETYPE_TRIANGLE, _("Triangle"));
  nlistSetStr (tlist, FADETYPE_QUARTER_SINE, _("Quarter Sine Wave"));
  nlistSetStr (tlist, FADETYPE_HALF_SINE, _("Half Sine Wave"));
  nlistSetStr (tlist, FADETYPE_LOGARITHMIC, _("Logarithmic"));
  nlistSetStr (tlist, FADETYPE_INVERTED_PARABOLA, _("Inverted Parabola"));
  confui.uiitem [CONFUI_SPINBOX_FADE_TYPE].list = tlist;

  tlist = nlistAlloc ("cu-player", LIST_UNORDERED, free);
  llist = nlistAlloc ("cu-player", LIST_ORDERED, free);
  nlistSetStr (tlist, 0, _("Integrated VLC"));
  nlistSetStr (llist, 0, "libplivlc");
  nlistSetStr (tlist, 1, _("Null Player"));
  nlistSetStr (llist, 1, "libnull");
  confui.uiitem [CONFUI_SPINBOX_PLAYER].list = tlist;
  confui.uiitem [CONFUI_SPINBOX_PLAYER].sblookuplist = llist;

  tlist = nlistAlloc ("cu-audio", LIST_UNORDERED, free);
  llist = nlistAlloc ("cu-audio-l", LIST_ORDERED, free);
  count = 0;
  if (isWindows ()) {
    nlistSetStr (tlist, count, "Windows");
    nlistSetStr (llist, count, "libvolwin");
    ++count;
  }
  if (isMacOS ()) {
    nlistSetStr (tlist, count, "MacOS");
    nlistSetStr (llist, count, "libvolmac");
    ++count;
  }
  if (isLinux ()) {
    nlistSetStr (tlist, count, "Pulse Audio");
    nlistSetStr (llist, count, "libvolpa");
    ++count;
    nlistSetStr (tlist, count, "ALSA");
    nlistSetStr (llist, count, "libvolalsa");
    ++count;
  }
  nlistSetStr (tlist, count++, _("Null Audio"));
  nlistSetStr (llist, count, "libvolnull");
  ++count;
  confui.uiitem [CONFUI_SPINBOX_AUDIO].list = tlist;
  confui.uiitem [CONFUI_SPINBOX_AUDIO].sblookuplist = llist;

  tlist = nlistAlloc ("cu-dance-speed", LIST_UNORDERED, free);
  /* order these in the same order as defined in dance.h */
  nlistSetStr (tlist, DANCE_SPEED_SLOW, _("slow"));
  nlistSetStr (tlist, DANCE_SPEED_NORMAL, _("normal"));
  nlistSetStr (tlist, DANCE_SPEED_FAST, _("fast"));
  confui.uiitem [CONFUI_SPINBOX_DANCE_SPEED].list = tlist;

  tlist = nlistAlloc ("cu-dance-time-sig", LIST_UNORDERED, free);
  /* order these in the same order as defined in dance.h */
  nlistSetStr (tlist, DANCE_TIMESIG_24, _("2/4"));
  nlistSetStr (tlist, DANCE_TIMESIG_34, _("3/4"));
  nlistSetStr (tlist, DANCE_TIMESIG_44, _("4/4"));
  nlistSetStr (tlist, DANCE_TIMESIG_48, _("4/8"));
  confui.uiitem [CONFUI_SPINBOX_DANCE_TIME_SIG].list = tlist;

  confuiLoadHTMLList (&confui);
  confuiLoadLocaleList (&confui);
  confuiLoadDanceTypeList (&confui);

  tlist = confuiGetThemeList ();
  nlistStartIterator (tlist, &iteridx);
  count = 0;
  while ((p = nlistIterateValueData (tlist, &iteridx)) != NULL) {
    if (strcmp (p, bdjoptGetStr (OPT_MP_MQ_THEME)) == 0) {
      confui.mqthemeidx = count;
    }
    if (strcmp (p, bdjoptGetStr (OPT_MP_UI_THEME)) == 0) {
      confui.uithemeidx = count;
    }
    ++count;
  }
  confui.uiitem [CONFUI_SPINBOX_UI_THEME].list = tlist;
  confui.uiitem [CONFUI_SPINBOX_UI_THEME].sblookuplist = tlist;
  confui.uiitem [CONFUI_SPINBOX_MQ_THEME].list = tlist;
  confui.uiitem [CONFUI_SPINBOX_MQ_THEME].sblookuplist = tlist;

  tlist = nlistAlloc ("cu-mob-mq", LIST_UNORDERED, free);
  /* order these in the same order as defined in bdjopt.h */
  nlistSetStr (tlist, MOBILEMQ_OFF, _("Off"));
  nlistSetStr (tlist, MOBILEMQ_LOCAL, _("Local"));
  nlistSetStr (tlist, MOBILEMQ_INTERNET, _("Internet"));
  confui.uiitem [CONFUI_SPINBOX_MOBILE_MQ].list = tlist;

  listenPort = bdjvarsGetNum (BDJVL_CONFIGUI_PORT);
  confui.conn = connInit (ROUTE_CONFIGUI);

  pathbldMakePath (tbuff, sizeof (tbuff), "",
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

  uiutilsInitGtkLog ();
  gtk_init (&argc, NULL);
  uifont = bdjoptGetStr (OPT_MP_UIFONT);
  uiutilsSetUIFont (uifont);

  g_timeout_add (UI_MAIN_LOOP_TIMER, confuiMainLoop, &confui);

  status = confuiCreateGui (&confui, 0, NULL);

  while (progstateShutdownProcess (confui.progstate) != STATE_CLOSED) {
    ;
  }
  progstateFree (confui.progstate);

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

  logProcBegin (LOG_PROC, "confuiStoppingCallback");

  confuiPopulateOptions (confui);
  bdjoptSave ();
  for (confuiident_t i = 0; i < CONFUI_ID_MAX; ++i) {
    confuiTableSave (confui, i);
  }

  gtk_window_get_size (GTK_WINDOW (confui->window), &x, &y);
  nlistSetNum (confui->options, CONFUI_SIZE_X, x);
  nlistSetNum (confui->options, CONFUI_SIZE_Y, y);
  gtk_window_get_position (GTK_WINDOW (confui->window), &x, &y);
  nlistSetNum (confui->options, CONFUI_POSITION_X, x);
  nlistSetNum (confui->options, CONFUI_POSITION_Y, y);

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
  char          fn [MAXPATHLEN];

  logProcBegin (LOG_PROC, "confuiClosingCallback");

  pathbldMakePath (fn, sizeof (fn), "",
      "configui", ".txt", PATHBLD_MP_USEIDX);
  datafileSaveKeyVal ("configui", fn, configuidfkeys, CONFUI_KEY_MAX, confui->options);

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

  if (confui->options != datafileGetList (confui->optiondf)) {
    nlistFree (confui->options);
  }
  datafileFree (confui->optiondf);
  if (confui->tableidents != NULL) {
    free (confui->tableidents);
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

static int
confuiCreateGui (configui_t *confui, int argc, char *argv [])
{
  int           status;

  logProcBegin (LOG_PROC, "confuiCreateGui");

  confui->app = gtk_application_new (
      "org.bdj4.BDJ4.configui",
      G_APPLICATION_NON_UNIQUE
  );

  g_signal_connect (confui->app, "activate", G_CALLBACK (confuiActivate), confui);

  /* gtk messes up the locale setting somehow; a re-bind is necessary */
  localeInit ();

  status = g_application_run (G_APPLICATION (confui->app), argc, argv);
  if (GTK_IS_WIDGET (confui->window)) {
    gtk_widget_destroy (confui->window);
  }
  g_object_unref (confui->app);

  logProcEnd (LOG_PROC, "confuiCreateGui", "");
  return status;
}

static void
confuiActivate (GApplication *app, gpointer userdata)
{
  configui_t    *confui = userdata;
  GError        *gerr = NULL;
  GtkWidget     *vbox;
  GtkWidget     *widget;
  GtkWidget     *image;
  GtkSizeGroup  *sg;
  GtkWidget     *dvbox;
  GtkWidget     *hbox;
  char          imgbuff [MAXPATHLEN];
  char          tbuff [MAXPATHLEN];
  gint          x, y;
  ssize_t       val;

  logProcBegin (LOG_PROC, "confuiActivate");

  pathbldMakePath (imgbuff, sizeof (imgbuff), "",
      "bdj4_icon_config", ".svg", PATHBLD_MP_IMGDIR);

  confui->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  assert (confui->window != NULL);
  gtk_window_set_application (GTK_WINDOW (confui->window), GTK_APPLICATION (app));
  gtk_window_set_application (GTK_WINDOW (confui->window), confui->app);
  gtk_window_set_default_icon_from_file (imgbuff, &gerr);
  g_signal_connect (confui->window, "delete-event", G_CALLBACK (confuiCloseWin), confui);
  gtk_window_set_title (GTK_WINDOW (confui->window), bdjoptGetStr (OPT_P_PROFILENAME));

  confui->vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (confui->window), confui->vbox);
  gtk_widget_set_margin_top (confui->vbox, 4);
  gtk_widget_set_margin_bottom (confui->vbox, 4);
  gtk_widget_set_margin_start (confui->vbox, 4);
  gtk_widget_set_margin_end (confui->vbox, 4);

  confui->notebook = uiutilsCreateNotebook ();
  assert (confui->notebook != NULL);
  gtk_box_pack_start (GTK_BOX (confui->vbox), confui->notebook,
      TRUE, TRUE, 0);

  /* general options */
  vbox = confuiMakeNotebookTab (confui, confui->notebook,
      _("General Options"), CONFUI_ID_NONE);
  sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  strlcpy (tbuff, bdjoptGetStr (OPT_M_DIR_MUSIC), sizeof (tbuff));
  if (isWindows ()) {
    pathWinPath (tbuff, sizeof (tbuff));
  }
  confuiMakeItemEntryChooser (confui, vbox, sg, _("Music Folder"),
      CONFUI_ENTRY_MUSIC_DIR, OPT_M_DIR_MUSIC,
      tbuff, confuiSelectMusicDir);
  uiutilsEntrySetValidate (&confui->uiitem [CONFUI_ENTRY_MUSIC_DIR].u.entry,
      uiutilsEntryValidateDir, confui);

  confuiMakeItemEntry (confui, vbox, sg, _("Profile Name"),
      CONFUI_ENTRY_PROFILE_NAME, OPT_P_PROFILENAME,
      bdjoptGetStr (OPT_P_PROFILENAME));

  confuiMakeItemSpinboxText (confui, vbox, sg, _("BPM"),
      CONFUI_SPINBOX_BPM, OPT_G_BPM,
      bdjoptGetNum (OPT_G_BPM));

  /* database */
  confuiMakeItemSpinboxText (confui, vbox, sg, _("Write Audio File Tags"),
      CONFUI_SPINBOX_WRITE_AUDIO_FILE_TAGS, OPT_G_WRITETAGS,
      bdjoptGetNum (OPT_G_WRITETAGS));
  confuiMakeItemSwitch (confui, vbox, sg,
      _("Database Loads Dance From Genre"),
      CONFUI_WIDGET_DB_LOAD_FROM_GENRE, OPT_G_LOADDANCEFROMGENRE,
      bdjoptGetNum (OPT_G_LOADDANCEFROMGENRE));
  confuiMakeItemSwitch (confui, vbox, sg,
      _("Enable iTunes Support"),
      CONFUI_WIDGET_ENABLE_ITUNES, OPT_G_ITUNESSUPPORT,
      bdjoptGetNum (OPT_G_ITUNESSUPPORT));

  /* bdj4 */
  confuiMakeItemSpinboxText (confui, vbox, sg, _("Locale"),
      CONFUI_SPINBOX_LOCALE, -1, confui->localeidx);
  confuiMakeItemEntryChooser (confui, vbox, sg, _("Startup Script"),
      CONFUI_ENTRY_STARTUP, OPT_M_STARTUPSCRIPT,
      bdjoptGetStr (OPT_M_STARTUPSCRIPT), confuiSelectStartup);
  uiutilsEntrySetValidate (&confui->uiitem [CONFUI_ENTRY_STARTUP].u.entry,
      uiutilsEntryValidateFile, confui);

  confuiMakeItemEntryChooser (confui, vbox, sg, _("Shutdown Script"),
      CONFUI_ENTRY_SHUTDOWN, OPT_M_SHUTDOWNSCRIPT,
      bdjoptGetStr (OPT_M_SHUTDOWNSCRIPT), confuiSelectShutdown);
  uiutilsEntrySetValidate (&confui->uiitem [CONFUI_ENTRY_SHUTDOWN].u.entry,
      uiutilsEntryValidateFile, confui);

  /* player options */
  vbox = confuiMakeNotebookTab (confui, confui->notebook,
      _("Player Options"), CONFUI_ID_NONE);
  sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  confuiMakeItemSpinboxText (confui, vbox, sg, _("Player"),
      CONFUI_SPINBOX_PLAYER, OPT_M_PLAYER_INTFC, 0);
  confuiMakeItemSpinboxText (confui, vbox, sg, _("Audio"),
      CONFUI_SPINBOX_AUDIO, OPT_M_VOLUME_INTFC, 0);
  confuiMakeItemSpinboxText (confui, vbox, sg, _("Audio Output"),
      CONFUI_SPINBOX_AUDIO_OUTPUT, OPT_M_AUDIOSINK,
      confui->audiosinkidx);
  confuiMakeItemSpinboxInt (confui, vbox, sg, _("Default Volume"),
      CONFUI_WIDGET_DEFAULT_VOL, OPT_P_DEFAULTVOLUME,
      10, 100, bdjoptGetNum (OPT_P_DEFAULTVOLUME));
  confuiMakeItemSpinboxDouble (confui, vbox, sg, _("Fade In Time"),
      CONFUI_WIDGET_FADE_IN_TIME, OPT_P_FADEINTIME,
      0.0, 2.0, (double) bdjoptGetNum (OPT_P_FADEINTIME) / 1000.0);
  confuiMakeItemSpinboxDouble (confui, vbox, sg, _("Fade Out Time"),
      CONFUI_WIDGET_FADE_OUT_TIME, OPT_P_FADEOUTTIME,
      0.0, 10.0, (double) bdjoptGetNum (OPT_P_FADEOUTTIME) / 1000.0);

  widget = confuiMakeItemSpinboxText (confui, vbox, sg, _("Fade Type"),
      CONFUI_SPINBOX_FADE_TYPE, OPT_P_FADETYPE,
      bdjoptGetNum (OPT_P_FADETYPE));
  pathbldMakePath (tbuff, sizeof (tbuff), "", "fades", ".svg",
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

  confuiMakeItemSpinboxDouble (confui, vbox, sg, _("Gap Between Songs"),
      CONFUI_WIDGET_GAP, OPT_P_GAP,
      0.0, 60.0, (double) bdjoptGetNum (OPT_P_GAP) / 1000.0);
  confuiMakeItemSpinboxTime (confui, vbox, sg, _("Maximum Play Time"),
      CONFUI_SPINBOX_MAX_PLAY_TIME, OPT_P_MAXPLAYTIME,
      bdjoptGetNum (OPT_P_MAXPLAYTIME));
  confuiMakeItemSpinboxInt (confui, vbox, sg, _("Queue Length"),
      CONFUI_WIDGET_PL_QUEUE_LEN, OPT_G_PLAYERQLEN,
      20, 400, bdjoptGetNum (OPT_G_PLAYERQLEN));
  confuiMakeItemSpinboxInt (confui, vbox, sg, _("Request Insert Location"),
      CONFUI_WIDGET_INSERT_LOC, OPT_P_INSERT_LOCATION,
      1, 10, bdjoptGetNum (OPT_P_INSERT_LOCATION));

  confuiMakeItemEntry (confui, vbox, sg, _("Queue A Name"),
      CONFUI_ENTRY_QUEUE_NM_A, OPT_P_QUEUE_NAME_A,
      bdjoptGetStr (OPT_P_QUEUE_NAME_A));
  confuiMakeItemEntry (confui, vbox, sg, _("Queue B Name"),
      CONFUI_ENTRY_QUEUE_NM_B, OPT_P_QUEUE_NAME_B,
      bdjoptGetStr (OPT_P_QUEUE_NAME_B));

  /* marquee options */
  vbox = confuiMakeNotebookTab (confui, confui->notebook,
      _("Marquee Options"), CONFUI_ID_NONE);
  sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  confuiMakeItemSpinboxText (confui, vbox, sg, _("Marquee Theme"),
      CONFUI_SPINBOX_MQ_THEME, OPT_MP_MQ_THEME,
      confui->mqthemeidx);
  confuiMakeItemFontButton (confui, vbox, sg, _("Marquee Font"),
      CONFUI_WIDGET_MQ_FONT, OPT_MP_MQFONT,
      bdjoptGetStr (OPT_MP_MQFONT));
  confuiMakeItemSpinboxInt (confui, vbox, sg, _("Queue Length"),
      CONFUI_WIDGET_MQ_QUEUE_LEN, OPT_P_MQQLEN,
      1, 20, bdjoptGetNum (OPT_P_MQQLEN));
  confuiMakeItemSwitch (confui, vbox, sg, _("Show Song Information"),
      CONFUI_WIDGET_MQ_SHOW_SONG_INFO, OPT_P_MQ_SHOW_INFO,
      bdjoptGetNum (OPT_P_MQ_SHOW_INFO));
  confuiMakeItemColorButton (confui, vbox, sg, _("Accent Colour"),
      CONFUI_WIDGET_MQ_ACCENT_COLOR, OPT_P_MQ_ACCENT_COL,
      bdjoptGetStr (OPT_P_MQ_ACCENT_COL));
  confuiMakeItemSwitch (confui, vbox, sg, _("Hide Marquee on Start"),
      CONFUI_WIDGET_HIDE_MARQUEE_ON_START, OPT_P_HIDE_MARQUEE_ON_START,
      bdjoptGetNum (OPT_P_HIDE_MARQUEE_ON_START));

  /* user infterface */
  vbox = confuiMakeNotebookTab (confui, confui->notebook,
      _("User Interface"), CONFUI_ID_NONE);
  sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  confuiMakeItemSpinboxText (confui, vbox, sg, _("Theme"),
      CONFUI_SPINBOX_UI_THEME, OPT_MP_UI_THEME,
      confui->uithemeidx);
  confuiMakeItemFontButton (confui, vbox, sg, _("Font"),
      CONFUI_WIDGET_UI_FONT, OPT_MP_UIFONT,
      bdjoptGetStr (OPT_MP_UIFONT));
  confuiMakeItemFontButton (confui, vbox, sg, _("Listing Font"),
      CONFUI_WIDGET_UI_LISTING_FONT, OPT_MP_LISTING_FONT,
      bdjoptGetStr (OPT_MP_LISTING_FONT));
  confuiMakeItemColorButton (confui, vbox, sg, _("Accent Colour"),
      CONFUI_WIDGET_UI_ACCENT_COLOR, OPT_P_UI_ACCENT_COL,
      bdjoptGetStr (OPT_P_UI_ACCENT_COL));

  /* display settings */
  vbox = confuiMakeNotebookTab (confui, confui->notebook,
      _("Display Settings"), CONFUI_ID_NONE);
  sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  confui->dsnotebook = uiutilsCreateNotebook ();
  gtk_box_pack_start (GTK_BOX (vbox), confui->dsnotebook, TRUE, TRUE, 0);

//      _("Music Queue")
//      _("Request")
//      _("Song List")
//      _("Song Selection")
//      _("Music Manager")
//      _("Song Editor")

  /* organization */
  vbox = confuiMakeNotebookTab (confui, confui->notebook,
      _("Organisation"), CONFUI_ID_NONE);
  sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  confuiMakeItemCombobox (confui, vbox, sg, _("Organisation Path"),
      CONFUI_COMBOBOX_AO_PATHFMT, OPT_G_AO_PATHFMT,
      confuiOrgPathSelect, bdjoptGetStr (OPT_G_AO_PATHFMT));
  confuiMakeItemLabelDisp (confui, vbox, sg, _("Examples"),
      CONFUI_WIDGET_AO_EXAMPLE_1, -1);
  confuiMakeItemLabelDisp (confui, vbox, sg, "",
      CONFUI_WIDGET_AO_EXAMPLE_2, -1);
  confuiMakeItemLabelDisp (confui, vbox, sg, "",
      CONFUI_WIDGET_AO_EXAMPLE_3, -1);
  confuiMakeItemLabelDisp (confui, vbox, sg, "",
      CONFUI_WIDGET_AO_EXAMPLE_4, -1);

  confuiMakeItemSwitch (confui, vbox, sg, _("Auto Organise"),
      CONFUI_WIDGET_AUTO_ORGANIZE, OPT_G_AUTOORGANIZE,
      bdjoptGetNum (OPT_G_AUTOORGANIZE));

  confuiMakeItemSwitch (confui, vbox, sg, _("Remove Spaces"),
      CONFUI_WIDGET_AO_CHG_SPACE, OPT_G_AO_CHANGESPACE,
      bdjoptGetNum (OPT_G_AO_CHANGESPACE));

  /* edit dances */
  vbox = confuiMakeNotebookTab (confui, confui->notebook,
      _("Edit Dances"), CONFUI_ID_DANCE);
  sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  g_signal_connect (confui->notebook, "switch-page",
      G_CALLBACK (confuiSwitchTable), confui);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_set_halign (hbox, GTK_ALIGN_START);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  confuiMakeItemTable (confui, hbox, CONFUI_ID_DANCE, CONFUI_TABLE_NO_UP_DOWN);
  confuiCreateDanceTable (confui);
  g_signal_connect (confui->tables [CONFUI_ID_DANCE].tree, "row-activated",
      G_CALLBACK (confuiDanceSelect), confui);

  dvbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_vexpand (dvbox, FALSE);
  gtk_widget_set_margin_start (dvbox, 16);
  gtk_box_pack_start (GTK_BOX (hbox), dvbox, FALSE, FALSE, 0);

  confuiMakeItemEntry (confui, dvbox, sg, _("Tags"),
      CONFUI_ENTRY_DANCE_DANCE, -1, "");

  confuiMakeItemSpinboxText (confui, dvbox, sg, _("Type"),
      CONFUI_SPINBOX_DANCE_TYPE, -1, 0);

  confuiMakeItemSpinboxText (confui, dvbox, sg, _("Speed"),
      CONFUI_SPINBOX_DANCE_SPEED, -1, 0);

  confuiMakeItemEntry (confui, dvbox, sg, _("Tags"),
      CONFUI_ENTRY_DANCE_TAGS, -1, "");

  confuiMakeItemEntryChooser (confui, dvbox, sg, _("Announcement"),
      CONFUI_ENTRY_DANCE_ANNOUNCEMENT, -1, "",
      confuiSelectAnnouncement);
  uiutilsEntrySetValidate (
      &confui->uiitem [CONFUI_ENTRY_DANCE_ANNOUNCEMENT].u.entry,
      uiutilsEntryValidateFile, confui);

  confuiMakeItemSpinboxInt (confui, dvbox, sg, _("Low BPM"),
      CONFUI_SPINBOX_DANCE_LOW_BPM, -1, 10, 500, 0);

  confuiMakeItemSpinboxInt (confui, dvbox, sg, _("High BPM"),
      CONFUI_SPINBOX_DANCE_HIGH_BPM, -1, 10, 500, 0);

  confuiMakeItemSpinboxText (confui, dvbox, sg, _("Time Signature"),
      CONFUI_SPINBOX_DANCE_TIME_SIG, -1, 0);

  /* edit ratings */
  vbox = confuiMakeNotebookTab (confui, confui->notebook,
      _("Edit Ratings"), CONFUI_ID_RATINGS);
  sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  g_signal_connect (confui->notebook, "switch-page",
      G_CALLBACK (confuiSwitchTable), confui);

  widget = uiutilsCreateLabel (_("Order from the lowest rating to the highest rating."));
  gtk_label_set_xalign (GTK_LABEL (widget), 0.0);
  gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);

  widget = uiutilsCreateLabel (_("Double click on a field to edit."));
  gtk_label_set_xalign (GTK_LABEL (widget), 0.0);
  gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);

  confuiMakeItemTable (confui, vbox, CONFUI_ID_RATINGS, CONFUI_TABLE_KEEP_FIRST);
  confui->tables [CONFUI_ID_RATINGS].listcreatefunc = confuiRatingListCreate;
  confui->tables [CONFUI_ID_RATINGS].savefunc = confuiRatingSave;
  confuiCreateRatingTable (confui);

  /* edit status */
  vbox = confuiMakeNotebookTab (confui, confui->notebook,
      _("Edit Status"), CONFUI_ID_STATUS);
  sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  g_signal_connect (confui->notebook, "switch-page",
      G_CALLBACK (confuiSwitchTable), confui);

  widget = uiutilsCreateLabel (_("Double click on a field to edit."));
  gtk_label_set_xalign (GTK_LABEL (widget), 0.0);
  gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);

  confuiMakeItemTable (confui, vbox, CONFUI_ID_STATUS,
      CONFUI_TABLE_KEEP_FIRST | CONFUI_TABLE_KEEP_LAST);
  confui->tables [CONFUI_ID_STATUS].togglecol = CONFUI_STATUS_COL_PLAY_FLAG;
  confui->tables [CONFUI_ID_STATUS].listcreatefunc = confuiStatusListCreate;
  confui->tables [CONFUI_ID_STATUS].savefunc = confuiStatusSave;
  confuiCreateStatusTable (confui);

  /* edit levels */
  vbox = confuiMakeNotebookTab (confui, confui->notebook,
      _("Edit Levels"), CONFUI_ID_LEVELS);
  sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  g_signal_connect (confui->notebook, "switch-page",
      G_CALLBACK (confuiSwitchTable), confui);

  widget = uiutilsCreateLabel (_("Order from easiest to most advanced."));
  gtk_label_set_xalign (GTK_LABEL (widget), 0.0);
  gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);

  widget = uiutilsCreateLabel (_("Double click on a field to edit."));
  gtk_label_set_xalign (GTK_LABEL (widget), 0.0);
  gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);

  confuiMakeItemTable (confui, vbox, CONFUI_ID_LEVELS, CONFUI_TABLE_NONE);
  confui->tables [CONFUI_ID_LEVELS].togglecol = CONFUI_LEVEL_COL_DEFAULT;
  confui->tables [CONFUI_ID_LEVELS].listcreatefunc = confuiLevelListCreate;
  confui->tables [CONFUI_ID_LEVELS].savefunc = confuiLevelSave;
  confuiCreateLevelTable (confui);

  /* edit genres */
  vbox = confuiMakeNotebookTab (confui, confui->notebook,
      _("Edit Genres"), CONFUI_ID_GENRES);
  sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  g_signal_connect (confui->notebook, "switch-page",
      G_CALLBACK (confuiSwitchTable), confui);

  widget = uiutilsCreateLabel (_("Double click on a field to edit."));
  gtk_label_set_xalign (GTK_LABEL (widget), 0.0);
  gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);

  confuiMakeItemTable (confui, vbox, CONFUI_ID_GENRES, CONFUI_TABLE_NONE);
  confui->tables [CONFUI_ID_GENRES].togglecol = CONFUI_GENRE_COL_CLASSICAL;
  confui->tables [CONFUI_ID_GENRES].listcreatefunc = confuiGenreListCreate;
  confui->tables [CONFUI_ID_GENRES].savefunc = confuiGenreSave;
  confuiCreateGenreTable (confui);

  /* mobile remote control */
  vbox = confuiMakeNotebookTab (confui, confui->notebook,
      _("Mobile Remote Control"), CONFUI_ID_NONE);
  sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  widget = confuiMakeItemSwitch (confui, vbox, sg, _("Enable Remote Control"),
      CONFUI_WIDGET_RC_ENABLE, OPT_P_REMOTECONTROL,
      bdjoptGetNum (OPT_P_REMOTECONTROL));
  g_signal_connect (widget, "state-set", G_CALLBACK (confuiRemctrlChg), confui);

  confuiMakeItemSpinboxText (confui, vbox, sg, _("HTML Template"),
      CONFUI_SPINBOX_RC_HTML_TEMPLATE, OPT_G_REMCONTROLHTML,
      confui->rchtmlidx);

  confuiMakeItemEntry (confui, vbox, sg, _("User ID"),
      CONFUI_ENTRY_RC_USER_ID,  OPT_P_REMCONTROLUSER,
      bdjoptGetStr (OPT_P_REMCONTROLUSER));

  confuiMakeItemEntry (confui, vbox, sg, _("Password"),
      CONFUI_ENTRY_RC_PASS, OPT_P_REMCONTROLPASS,
      bdjoptGetStr (OPT_P_REMCONTROLPASS));

  widget = confuiMakeItemSpinboxInt (confui, vbox, sg, _("Port"),
      CONFUI_WIDGET_RC_PORT, OPT_P_REMCONTROLPORT,
      8000, 30000, bdjoptGetNum (OPT_P_REMCONTROLPORT));
  g_signal_connect (widget, "value-changed", G_CALLBACK (confuiRemctrlPortChg), confui);

  confuiMakeItemLink (confui, vbox, sg, _("QR Code"),
      CONFUI_WIDGET_RC_QR_CODE, "");

  /* mobile marquee */
  vbox = confuiMakeNotebookTab (confui, confui->notebook,
      _("Mobile Marquee"), CONFUI_ID_NONE);
  sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  widget = confuiMakeItemSpinboxText (confui, vbox, sg, _("Mobile Marquee"),
      CONFUI_SPINBOX_MOBILE_MQ, OPT_P_MOBILEMARQUEE,
      bdjoptGetNum (OPT_P_MOBILEMARQUEE));
  g_signal_connect (widget, "value-changed", G_CALLBACK (confuiMobmqTypeChg), confui);

  widget = confuiMakeItemSpinboxInt (confui, vbox, sg, _("Port"),
      CONFUI_WIDGET_MMQ_PORT, OPT_P_MOBILEMQPORT,
      8000, 30000, bdjoptGetNum (OPT_P_MOBILEMQPORT));
  g_signal_connect (widget, "value-changed", G_CALLBACK (confuiMobmqPortChg), confui);

  confuiMakeItemEntry (confui, vbox, sg, _("Name"),
      CONFUI_ENTRY_MM_NAME, OPT_P_MOBILEMQTAG,
      bdjoptGetStr (OPT_P_MOBILEMQTAG));
  uiutilsEntrySetValidate (&confui->uiitem [CONFUI_ENTRY_MM_NAME].u.entry,
      confuiMobmqNameChg, confui);

  confuiMakeItemEntry (confui, vbox, sg, _("Title"),
      CONFUI_ENTRY_MM_TITLE, OPT_P_MOBILEMQTITLE,
      bdjoptGetStr (OPT_P_MOBILEMQTITLE));
  uiutilsEntrySetValidate (&confui->uiitem [CONFUI_ENTRY_MM_TITLE].u.entry,
      confuiMobmqTitleChg, confui);

  confuiMakeItemLink (confui, vbox, sg, _("QR Code"),
      CONFUI_WIDGET_MMQ_QR_CODE, "");

  vbox = confuiMakeNotebookTab (confui, confui->notebook,
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

  pathbldMakePath (imgbuff, sizeof (imgbuff), "",
      "bdj4_icon", ".png", PATHBLD_MP_IMGDIR);
  osuiSetIcon (imgbuff);

  confuiUpdateMobmqQrcode (confui);
  confuiUpdateRemctrlQrcode (confui);
  confuiUpdateOrgExamples (confui, bdjoptGetStr (OPT_G_AO_PATHFMT));

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
fprintf (stderr, "locale changed\n");
      sysvarsSetStr (SV_LOCALE, sval);
      snprintf (tbuff, sizeof (tbuff), "%.2s", sval);
      sysvarsSetStr (SV_LOCALE_SHORT, tbuff);
      pathbldMakePath (tbuff, sizeof (tbuff), "",
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

fprintf (stderr, "theme changed\n");
      pathbldMakePath (tbuff, sizeof (tbuff), "",
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
fprintf (stderr, "accent color changed\n");
      templateImageCopy (sval);
    }
  } /* for each item */

  bdjoptSetNum (OPT_G_DEBUGLVL, debug);
}


static void
confuiSelectMusicDir (GtkButton *b, gpointer udata)
{
  configui_t            *confui = udata;
  char                  *fn = NULL;
  uiutilsselect_t       selectdata;

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
}

static void
confuiSelectStartup (GtkButton *b, gpointer udata)
{
  configui_t            *confui = udata;

  confuiSelectFileDialog (confui, CONFUI_ENTRY_STARTUP,
      sysvarsGetStr (SV_BDJ4DATATOPDIR), NULL, NULL);
}

static void
confuiSelectShutdown (GtkButton *b, gpointer udata)
{
  configui_t            *confui = udata;

  confuiSelectFileDialog (confui, CONFUI_ENTRY_SHUTDOWN,
      sysvarsGetStr (SV_BDJ4DATATOPDIR), NULL, NULL);
}

static void
confuiSelectAnnouncement (GtkButton *b, gpointer udata)
{
  configui_t            *confui = udata;

  confuiSelectFileDialog (confui, CONFUI_ENTRY_DANCE_ANNOUNCEMENT,
      bdjoptGetStr (OPT_M_DIR_MUSIC), _("Audio Files"), "audio/*");
}

static void
confuiSelectFileDialog (configui_t *confui, int widx, char *startpath,
    char *mimefiltername, char *mimetype)
{
  char                  *fn = NULL;
  uiutilsselect_t       selectdata;

  selectdata.label = _("Select File");
  selectdata.window = confui->window;
  selectdata.startpath = startpath;
  selectdata.mimefiltername = mimefiltername;
  selectdata.mimetype = mimetype;
  fn = uiutilsSelectFileDialog (&selectdata);
  if (fn != NULL) {
    gtk_entry_buffer_set_text (confui->uiitem [widx].u.entry.buffer,
        fn, -1);
    free (fn);
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "selected loc: %s", fn);
  }
}

static GtkWidget *
confuiMakeNotebookTab (configui_t *confui, GtkWidget *nb, char *txt, int id)
{
  GtkWidget   *tablabel;
  GtkWidget   *vbox;

  tablabel = uiutilsCreateLabel (txt);
  gtk_label_set_xalign (GTK_LABEL (tablabel), 0.0);
  gtk_widget_set_margin_top (tablabel, 0);
  gtk_widget_set_margin_start (tablabel, 0);
  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_margin_top (vbox, 4);
  gtk_widget_set_margin_bottom (vbox, 4);
  gtk_widget_set_margin_start (vbox, 4);
  gtk_widget_set_margin_end (vbox, 4);
  gtk_notebook_append_page (GTK_NOTEBOOK (nb), vbox, tablabel);

  confui->tableidents = realloc (confui->tableidents,
      sizeof (int) * (confui->tabcount + 1));
  confui->tableidents [confui->tabcount] = id;
  ++confui->tabcount;

  return vbox;
}

static GtkWidget *
confuiMakeItemEntry (configui_t *confui, GtkWidget *vbox, GtkSizeGroup *sg,
    char *txt, int widx, int bdjoptIdx, char *disp)
{
  GtkWidget   *hbox;
  GtkWidget   *widget;

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

  hbox = confuiMakeItemEntry (confui, vbox, sg, txt, widx, bdjoptIdx, disp);

  widget = uiutilsCreateButton ("", NULL, dialogFunc, confui);
  image = gtk_image_new_from_icon_name ("folder", GTK_ICON_SIZE_BUTTON);
  gtk_button_set_image (GTK_BUTTON (widget), image);
  gtk_button_set_always_show_image (GTK_BUTTON (widget), TRUE);
  gtk_widget_set_margin_start (widget, 0);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
  confui->uiitem [widx].bdjoptIdx = bdjoptIdx;
}

static GtkWidget *
confuiMakeItemCombobox (configui_t *confui, GtkWidget *vbox, GtkSizeGroup *sg,
    char *txt, int widx, int bdjoptIdx, void *ddcb, char *value)
{
  GtkWidget   *hbox;
  GtkWidget   *widget;

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
  return hbox;
}

static void
confuiMakeItemLink (configui_t *confui, GtkWidget *vbox, GtkSizeGroup *sg,
    char *txt, int widx, char *disp)
{
  GtkWidget   *hbox;
  GtkWidget   *widget;

  hbox = confuiMakeItemLabel (vbox, sg, txt);
  widget = gtk_link_button_new (disp);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  confui->uiitem [widx].u.widget = widget;
}

static void
confuiMakeItemFontButton (configui_t *confui, GtkWidget *vbox, GtkSizeGroup *sg,
    char *txt, int widx, int bdjoptIdx, char *fontname)
{
  GtkWidget   *hbox;
  GtkWidget   *widget;

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
}

static void
confuiMakeItemColorButton (configui_t *confui, GtkWidget *vbox, GtkSizeGroup *sg,
    char *txt, int widx, int bdjoptIdx, char *color)
{
  GtkWidget   *hbox;
  GtkWidget   *widget;
  GdkRGBA     rgba;


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
}

static GtkWidget *
confuiMakeItemSpinboxText (configui_t *confui, GtkWidget *vbox, GtkSizeGroup *sg,
    char *txt, int widx, int bdjoptIdx, ssize_t value)
{
  GtkWidget   *hbox;
  GtkWidget   *widget;
  nlist_t     *list;
  size_t      maxWidth;


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

  return widget;
}

static void
confuiMakeItemSpinboxTime (configui_t *confui, GtkWidget *vbox,
    GtkSizeGroup *sg, char *txt, int widx, int bdjoptIdx, ssize_t value)
{
  GtkWidget   *hbox;
  GtkWidget   *widget;


  confui->uiitem [widx].basetype = CONFUI_SPINBOX_TIME;
  confui->uiitem [widx].outtype = CONFUI_OUT_NUM;
  hbox = confuiMakeItemLabel (vbox, sg, txt);
  widget = uiutilsSpinboxTimeCreate (&confui->uiitem [widx].u.spinbox, confui);
  uiutilsSpinboxTimeSetValue (&confui->uiitem [widx].u.spinbox, value);
  gtk_widget_set_margin_start (widget, 8);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  confui->uiitem [widx].bdjoptIdx = bdjoptIdx;
}

static GtkWidget *
confuiMakeItemSpinboxInt (configui_t *confui, GtkWidget *vbox, GtkSizeGroup *sg,
    char *txt, int widx, int bdjoptIdx, int min, int max, int value)
{
  GtkWidget   *hbox;
  GtkWidget   *widget;


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
  return widget;
}

static void
confuiMakeItemSpinboxDouble (configui_t *confui, GtkWidget *vbox, GtkSizeGroup *sg,
    char *txt, int widx, int bdjoptIdx, double min, double max, double value)
{
  GtkWidget   *hbox;
  GtkWidget   *widget;


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
}

static GtkWidget *
confuiMakeItemSwitch (configui_t *confui, GtkWidget *vbox, GtkSizeGroup *sg,
    char *txt, int widx, int bdjoptIdx, int value)
{
  GtkWidget   *hbox;
  GtkWidget   *widget;


  confui->uiitem [widx].basetype = CONFUI_SWITCH;
  confui->uiitem [widx].outtype = CONFUI_OUT_BOOL;
  hbox = confuiMakeItemLabel (vbox, sg, txt);
  widget = uiutilsCreateSwitch (value);
  gtk_widget_set_margin_start (widget, 8);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  confui->uiitem [widx].u.widget = widget;
  confui->uiitem [widx].bdjoptIdx = bdjoptIdx;
  return widget;
}

static GtkWidget *
confuiMakeItemLabelDisp (configui_t *confui, GtkWidget *vbox, GtkSizeGroup *sg,
    char *txt, int widx, int bdjoptIdx)
{
  GtkWidget   *hbox;
  GtkWidget   *widget;


  confui->uiitem [widx].basetype = CONFUI_NONE;
  confui->uiitem [widx].outtype = CONFUI_OUT_NONE;
  hbox = confuiMakeItemLabel (vbox, sg, txt);
  widget = uiutilsCreateLabel ("");
  gtk_widget_set_margin_start (widget, 8);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  confui->uiitem [widx].u.widget = widget;
  confui->uiitem [widx].bdjoptIdx = bdjoptIdx;
  return widget;
}

static void
confuiMakeItemCheckButton (configui_t *confui, GtkWidget *vbox, GtkSizeGroup *sg,
    const char *txt, int widx, int bdjoptIdx, int value)
{
  GtkWidget   *widget;


  confui->uiitem [widx].basetype = CONFUI_CHECK_BUTTON;
  confui->uiitem [widx].outtype = CONFUI_OUT_BOOL;
  widget = uiutilsCreateCheckButton (txt, value);
  gtk_widget_set_margin_start (widget, 8);
  gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);
  confui->uiitem [widx].u.widget = widget;
  confui->uiitem [widx].bdjoptIdx = bdjoptIdx;
}

static GtkWidget *
confuiMakeItemLabel (GtkWidget *vbox, GtkSizeGroup *sg, char *txt)
{
  GtkWidget   *hbox;
  GtkWidget   *widget;

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  if (*txt == '\0') {
    widget = uiutilsCreateLabel (txt);
  } else {
    widget = uiutilsCreateColonLabel (txt);
  }
  gtk_label_set_xalign (GTK_LABEL (widget), 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
  gtk_size_group_add_widget (sg, widget);
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
  GtkTreeSelection  *sel;

  mhbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  assert (mhbox != NULL);
  gtk_widget_set_halign (mhbox, GTK_ALIGN_START);
  gtk_widget_set_margin_top (mhbox, 4);
  gtk_widget_set_hexpand (mhbox, TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), mhbox, FALSE, FALSE, 0);

  widget = uiutilsCreateScrolledWindow ();
  gtk_scrolled_window_set_min_content_height (GTK_SCROLLED_WINDOW (widget), 200);
  gtk_box_pack_start (GTK_BOX (mhbox), widget, FALSE, FALSE, 0);

  tree = gtk_tree_view_new ();
  confui->tables [id].tree = tree;
  confui->tables [id].flags = flags;
  gtk_widget_set_margin_top (tree, 2);
  gtk_widget_set_margin_start (tree, 16);
  gtk_tree_view_set_activate_on_single_click (GTK_TREE_VIEW (tree), TRUE);
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tree), TRUE);
  sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
  gtk_tree_selection_set_mode (sel, GTK_SELECTION_SINGLE);
  gtk_widget_set_halign (tree, GTK_ALIGN_START);
  gtk_widget_set_hexpand (tree, FALSE);
  gtk_widget_set_vexpand (tree, FALSE);
  gtk_container_add (GTK_CONTAINER (widget), tree);

  bvbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  assert (bvbox != NULL);
  gtk_widget_set_margin_top (bvbox, 2);
  gtk_widget_set_margin_start (bvbox, 8);
  gtk_widget_set_valign (bvbox, GTK_ALIGN_CENTER);
  gtk_box_pack_start (GTK_BOX (mhbox), bvbox, FALSE, FALSE, 0);

  if ((flags & CONFUI_TABLE_NO_UP_DOWN) != CONFUI_TABLE_NO_UP_DOWN) {
    widget = uiutilsCreateButton (_("Move Up"), "button_up", NULL, confui);
    g_signal_connect (widget, "clicked",
        G_CALLBACK (confuiTableMoveUp), confui);
    gtk_box_pack_start (GTK_BOX (bvbox), widget, FALSE, FALSE, 0);

    widget = uiutilsCreateButton (_("Move Down"), "button_down", NULL, confui);
    g_signal_connect (widget, "clicked",
        G_CALLBACK (confuiTableMoveDown), confui);
    gtk_box_pack_start (GTK_BOX (bvbox), widget, FALSE, FALSE, 0);
  }

  widget = uiutilsCreateButton (_("Delete"), "button_remove", NULL, confui);
  g_signal_connect (widget, "clicked",
      G_CALLBACK (confuiTableRemove), confui);
  gtk_box_pack_start (GTK_BOX (bvbox), widget, FALSE, FALSE, 0);

  widget = uiutilsCreateButton (_("Add New"), "button_add", NULL, confui);
  g_signal_connect (widget, "clicked",
      G_CALLBACK (confuiTableAdd), confui);
  gtk_box_pack_start (GTK_BOX (bvbox), widget, FALSE, FALSE, 0);

  return vbox;
}

static nlist_t *
confuiGetThemeList (void)
{
  slist_t     *filelist = NULL;
  nlist_t     *themelist = NULL;
  char        tbuff [MAXPATHLEN];

  themelist = nlistAlloc ("cu-themes", LIST_ORDERED, free);

  if (isWindows ()) {
    snprintf (tbuff, sizeof (tbuff), "%s/plocal/share/themes",
        sysvarsGetStr (SV_BDJ4MAINDIR));
    filelist = filemanipRecursiveDirList (tbuff, FILEMANIP_DIRS);
    confuiGetThemeNames (themelist, filelist);
    slistFree (filelist);
  } else {
    filelist = filemanipRecursiveDirList ("/usr/share/themes", FILEMANIP_DIRS);
    confuiGetThemeNames (themelist, filelist);
    slistFree (filelist);

    snprintf (tbuff, sizeof (tbuff), "%s/.themes", sysvarsGetStr (SV_HOME));
    filelist = filemanipRecursiveDirList (tbuff, FILEMANIP_DIRS);
    confuiGetThemeNames (themelist, filelist);
    slistFree (filelist);
  }

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

  if (filelist == NULL) {
    return NULL;
  }

  count = nlistGetCount (themelist);

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


  tlist = nlistAlloc ("cu-html-list", LIST_ORDERED, free);
  llist = nlistAlloc ("cu-html-list-l", LIST_ORDERED, free);

  pathbldMakePath (tbuff, sizeof (tbuff), "",
      "html-list", ".txt", PATHBLD_MP_TEMPLATEDIR);
  df = datafileAllocParse ("conf-html-list", DFTYPE_KEY_VAL, tbuff,
      NULL, 0, DATAFILE_NO_LOOKUP);
  list = datafileGetList (df);

  slistStartIterator (list, &iteridx);
  count = 0;
  while ((key = slistIterateKey (list, &iteridx)) != NULL) {
    data = slistGetStr (list, key);
    if (strcmp (key, bdjoptGetStr (OPT_G_REMCONTROLHTML)) == 0) {
      confui->rchtmlidx = count;
    }
    nlistSetStr (tlist, count, key);
    nlistSetStr (llist, count, data);
    ++count;
  }
  datafileFree (df);

  confui->uiitem [CONFUI_SPINBOX_RC_HTML_TEMPLATE].list = tlist;
  confui->uiitem [CONFUI_SPINBOX_RC_HTML_TEMPLATE].sblookuplist = llist;
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


  tlist = nlistAlloc ("cu-locale-list", LIST_ORDERED, free);
  llist = nlistAlloc ("cu-locale-list-l", LIST_ORDERED, free);

  pathbldMakePath (tbuff, sizeof (tbuff), "",
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
      confui->localeidx = count;
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
    confui->localeidx = shortidx;
  } else if (! found) {
    confui->localeidx = engbidx;
  }
  datafileFree (df);

  confui->uiitem [CONFUI_SPINBOX_LOCALE].list = tlist;
  confui->uiitem [CONFUI_SPINBOX_LOCALE].sblookuplist = llist;
}


static void
confuiLoadDanceTypeList (configui_t *confui)
{
  nlist_t       *tlist = NULL;
  dnctype_t     *dnctypes;
  slistidx_t    iteridx;
  char          *key;
  int           count;


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
}

static gboolean
confuiFadeTypeTooltip (GtkWidget *w, gint x, gint y, gboolean kbmode,
    GtkTooltip *tt, void *udata)
{
  configui_t  *confui = udata;
  GdkPixbuf   *pixbuf;

  pixbuf = gtk_image_get_pixbuf (GTK_IMAGE (confui->fadetypeImage));
  gtk_tooltip_set_icon (tt, pixbuf);
  return TRUE;
}

static void
confuiOrgPathSelect (GtkTreeView *tv, GtkTreePath *path,
    GtkTreeViewColumn *column, gpointer udata)
{
  configui_t        *confui = udata;
  char              *sval;

  sval = confuiComboboxSelect (confui, path, CONFUI_COMBOBOX_AO_PATHFMT);
  confuiUpdateOrgExamples (confui, sval);
}

static char *
confuiComboboxSelect (configui_t *confui, GtkTreePath *path, int widx)
{
  uiutilsdropdown_t *dd = NULL;
  ssize_t           idx;
  char              *sval;

  dd = &confui->uiitem [widx].u.dropdown;
  idx = uiutilsDropDownSelectionGet (dd, path);
  sval = slistGetDataByIdx (confui->uiitem [widx].list, idx);
  confui->uiitem [widx].listidx = idx;
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

    ip = webclientGetLocalIP ();
    snprintf (uri, sizeof (uri), "http://%s:%zd",
        ip, bdjoptGetNum (OPT_P_MOBILEMQPORT));
  }

  if (type != MOBILEMQ_OFF) {
    qruri = confuiMakeQRCodeFile (confui, _("Mobile Marquee"), uri);
  }

  widget = confui->uiitem [CONFUI_WIDGET_MMQ_QR_CODE].u.widget;
  gtk_link_button_set_uri (GTK_LINK_BUTTON (widget), qruri);
  gtk_button_set_label (GTK_BUTTON (widget), uri);
  if (*qruri) {
    free (qruri);
  }
}

static void
confuiMobmqTypeChg (GtkSpinButton *sb, gpointer udata)
{
  configui_t    *confui = udata;
  GtkAdjustment *adjustment;
  double        value;
  ssize_t       nval;

  adjustment = gtk_spin_button_get_adjustment (sb);
  value = gtk_adjustment_get_value (adjustment);
  nval = (ssize_t) value;
  bdjoptSetNum (OPT_P_MOBILEMARQUEE, nval);
  confuiUpdateMobmqQrcode (confui);
}

static void
confuiMobmqPortChg (GtkSpinButton *sb, gpointer udata)
{
  configui_t    *confui = udata;
  GtkAdjustment *adjustment;
  double        value;
  ssize_t       nval;

  adjustment = gtk_spin_button_get_adjustment (sb);
  value = gtk_adjustment_get_value (adjustment);
  nval = (ssize_t) value;
  bdjoptSetNum (OPT_P_MOBILEMQPORT, nval);
  confuiUpdateMobmqQrcode (confui);
}

static bool
confuiMobmqNameChg (void *edata, void *udata)
{
  uiutilsentry_t  *entry = edata;
  configui_t    *confui = udata;
  const char      *sval;

  sval = uiutilsEntryGetValue (entry);
  bdjoptSetStr (OPT_P_MOBILEMQTAG, sval);
  confuiUpdateMobmqQrcode (confui);
  return true;
}

static bool
confuiMobmqTitleChg (void *edata, void *udata)
{
  uiutilsentry_t  *entry = edata;
  configui_t      *confui = udata;
  const char      *sval;

  sval = uiutilsEntryGetValue (entry);
  bdjoptSetStr (OPT_P_MOBILEMQTITLE, sval);
  confuiUpdateMobmqQrcode (confui);
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


  onoff = (bdjmobilemq_t) bdjoptGetNum (OPT_P_REMOTECONTROL);

  if (onoff == 0) {
    *tbuff = '\0';
    *uri = '\0';
    qruri = "";
  }
  if (onoff == 1) {
    char *ip;

    ip = webclientGetLocalIP ();
    snprintf (uri, sizeof (uri), "http://%s:%zd",
        ip, bdjoptGetNum (OPT_P_REMCONTROLPORT));
  }

  if (onoff == 1) {
    qruri = confuiMakeQRCodeFile (confui, _("Mobile Remote Control"), uri);
  }

  widget = confui->uiitem [CONFUI_WIDGET_RC_QR_CODE].u.widget;
  gtk_link_button_set_uri (GTK_LINK_BUTTON (widget), qruri);
  gtk_button_set_label (GTK_BUTTON (widget), uri);
  if (*qruri) {
    free (qruri);
  }
}

static gboolean
confuiRemctrlChg (GtkSwitch *sw, gboolean value, gpointer udata)
{
  configui_t  *confui = udata;

  bdjoptSetNum (OPT_P_REMOTECONTROL, value);
  confuiUpdateRemctrlQrcode (confui);
  return FALSE;
}

static void
confuiRemctrlPortChg (GtkSpinButton *sb, gpointer udata)
{
  configui_t    *confui = udata;
  GtkAdjustment *adjustment;
  double        value;
  ssize_t       nval;

  adjustment = gtk_spin_button_get_adjustment (sb);
  value = gtk_adjustment_get_value (adjustment);
  nval = (ssize_t) value;
  bdjoptSetNum (OPT_P_REMCONTROLPORT, nval);
  confuiUpdateRemctrlQrcode (confui);
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

  qruri = malloc (MAXPATHLEN);

  pathbldMakePath (baseuri, sizeof (baseuri), "",
      "", "", PATHBLD_MP_TEMPLATEDIR);
  pathbldMakePath (tbuff, sizeof (tbuff), "",
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

  pathbldMakePath (tbuff, sizeof (tbuff), "",
      "qrcode", ".html", PATHBLD_MP_TMPDIR);
  fh = fopen (tbuff, "w");
  fwrite (ndata, dlen, 1, fh);
  fclose (fh);
  snprintf (qruri, MAXPATHLEN, "file://%s/%s",
      sysvarsGetStr (SV_BDJ4DATATOPDIR), tbuff);

  free (data);
  free (ndata);
  return qruri;
}

static void
confuiUpdateOrgExamples (configui_t *confui, char *pathfmt)
{
  char      *data;
  org_t     *org;
  GtkWidget *widget;

  org = orgAlloc (pathfmt);

  data = "FILE\n..none\nDISCNUMBER\n..1\nTRACKNUMBER\n..1\nALBUMARTIST\n..Prandi Sound Orchestra\nARTIST\n..Prandi Sound Orchestra\nDANCE\n..Foxtrot\nGENRE\n..Ballroom Dance\nTITLE\n..Don't Be That Way\n";
  widget = confui->uiitem [CONFUI_WIDGET_AO_EXAMPLE_1].u.widget;
  confuiUpdateOrgExample (confui, org, data, widget);

  data = "FILE\n..none\nDISCNUMBER\n..2\nTRACKNUMBER\n..17\nALBUMARTIST\n..Dancehouse\nARTIST\n..John Painter Feat. Shannon Lea Smith\nDANCE\n..Quickstep\nGENRE\n..Ballroom Dance\nTITLE\n..You Want a Piece of Me\n";
  widget = confui->uiitem [CONFUI_WIDGET_AO_EXAMPLE_2].u.widget;
  confuiUpdateOrgExample (confui, org, data, widget);

  data = "FILE\n..none\nDISCNUMBER\n..1\nTRACKNUMBER\n..2\nALBUMARTIST\n..Casa Musica\nARTIST\n..Boris Myagkov Big Band\nDANCE\n..Waltz\nTITLE\n..If You Don't Know Me By Now\nALBUM\n..The Music of the German Open Championships 2014\nGENRE\n..Ballroom Dance\n";
  widget = confui->uiitem [CONFUI_WIDGET_AO_EXAMPLE_3].u.widget;
  confuiUpdateOrgExample (confui, org, data, widget);

  data = "FILE\n..none\nDISCNUMBER\n..1\nTRACKNUMBER\n..3\nCOMPOSER\n..Beethoven\nCONDUCTOR\n..Herbert von Karajan\nALBUMARTIST\n..Berliner Philharmonker\nnDANCE\n..\nTITLE\n..Symphonie No.5 C-minor, Op.67: I Allegro con brio\nALBUM\n..Beethoven Symphonien 5 & 6\nGENRE\n..Classical\n";
  widget = confui->uiitem [CONFUI_WIDGET_AO_EXAMPLE_4].u.widget;
  confuiUpdateOrgExample (confui, org, data, widget);

  orgFree (org);
}

static void
confuiUpdateOrgExample (configui_t *config, org_t *org, char *data, GtkWidget *widget)
{
  song_t    *song;
  char      *tdata;
  char      *disp;

  tdata = strdup (data);
  song = songAlloc ();
  songParse (song, tdata, 0);
  disp = orgMakeSongPath (org, song);
  gtk_label_set_text (GTK_LABEL (widget), disp);
  songFree (song);
  free (disp);
  free (tdata);
}

/* table editing */
static void
confuiTableMoveUp (GtkButton *b, gpointer udata)
{
  confuiTableMove (udata, CONFUI_TABLE_MOVE_PREV);
}

static void
confuiTableMoveDown (GtkButton *b, gpointer udata)
{
  confuiTableMove (udata, CONFUI_TABLE_MOVE_NEXT);
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

  tree = confui->tables [confui->tablecurr].tree;
  flags = confui->tables [confui->tablecurr].flags;
  sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
  count = gtk_tree_selection_count_selected_rows (sel);
  if (count != 1) {
    return;
  }
  gtk_tree_selection_get_selected (sel, &model, &iter);

  path = gtk_tree_model_get_path (model, &iter);
  pathstr = gtk_tree_path_to_string (path);
  sscanf (pathstr, "%d", &idx);
  free (pathstr);
  gtk_tree_path_free (path);
  if (idx == 1 &&
      dir == CONFUI_TABLE_MOVE_PREV &&
      (flags & CONFUI_TABLE_KEEP_FIRST) == CONFUI_TABLE_KEEP_FIRST) {
    return;
  }
  if (idx == confui->tables [confui->tablecurr].currcount - 1 &&
      dir == CONFUI_TABLE_MOVE_PREV &&
      (flags & CONFUI_TABLE_KEEP_LAST) == CONFUI_TABLE_KEEP_LAST) {
    return;
  }
  if (idx == confui->tables [confui->tablecurr].currcount - 2 &&
      dir == CONFUI_TABLE_MOVE_NEXT &&
      (flags & CONFUI_TABLE_KEEP_LAST) == CONFUI_TABLE_KEEP_LAST) {
    return;
  }
  if (idx == 0 &&
      dir == CONFUI_TABLE_MOVE_NEXT &&
      (flags & CONFUI_TABLE_KEEP_FIRST) == CONFUI_TABLE_KEEP_FIRST) {
    return;
  }

  memcpy (&citer, &iter, sizeof (iter));
  if (dir == CONFUI_TABLE_MOVE_PREV) {
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
//  confuiTableView (tree);
}

static void
confuiTableRemove (GtkButton *b, gpointer udata)
{
  configui_t        *confui = udata;
  GtkTreeSelection  *sel;
  GtkWidget         *tree;
  GtkTreeModel      *model;
  GtkTreeIter       iter;
  GtkTreePath       *path;
  char              *pathstr;
  int               idx;
  int               count;
  int               flags;

  tree = confui->tables [confui->tablecurr].tree;
  flags = confui->tables [confui->tablecurr].flags;
  sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
  count = gtk_tree_selection_count_selected_rows (sel);
  if (count != 1) {
    return;
  }
  gtk_tree_selection_get_selected (sel, &model, &iter);

  path = gtk_tree_model_get_path (model, &iter);
  pathstr = gtk_tree_path_to_string (path);
  sscanf (pathstr, "%d", &idx);
  free (pathstr);
  gtk_tree_path_free (path);
  if (idx == 0 &&
      (flags & CONFUI_TABLE_KEEP_FIRST) == CONFUI_TABLE_KEEP_FIRST) {
    return;
  }
  if (idx == confui->tables [confui->tablecurr].currcount - 1 &&
      (flags & CONFUI_TABLE_KEEP_LAST) == CONFUI_TABLE_KEEP_LAST) {
    return;
  }

  gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
  confui->tables [confui->tablecurr].changed = true;
  confui->tables [confui->tablecurr].currcount -= 1;
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
  int               count;
  int               valid;
  int               flags;


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
      valid = gtk_tree_model_iter_next (model, &iter);
    }
  }

  if (titer == NULL) {
    gtk_list_store_append (GTK_LIST_STORE (model), &niter);
  } else {
    gtk_list_store_insert_before (GTK_LIST_STORE (model), &niter, &iter);
  }

  switch (confui->tablecurr) {
    case CONFUI_ID_NONE:
    case CONFUI_ID_MAX:
    {
      break;
    }

    case CONFUI_ID_DANCE:
    {
      break;
    }

    case CONFUI_ID_GENRES:
    {
      confuiGenreSet (GTK_LIST_STORE (model), &niter, TRUE, "", 0);
      break;
    }

    case CONFUI_ID_RATINGS:
    {
      confuiRatingSet (GTK_LIST_STORE (model), &niter, TRUE, "", 0);
      break;
    }

    case CONFUI_ID_LEVELS:
    {
      confuiLevelSet (GTK_LIST_STORE (model), &niter, TRUE, "", 0, 0);
      break;
    }

    case CONFUI_ID_STATUS:
    {
      confuiStatusSet (GTK_LIST_STORE (model), &niter, TRUE, "", 0);
      break;
    }
  }

  confui->tables [confui->tablecurr].changed = true;
  confui->tables [confui->tablecurr].currcount += 1;
}

static void
confuiSwitchTable (GtkNotebook *nb, GtkWidget *page, guint pagenum, gpointer udata)
{
  configui_t        *confui = udata;
  GtkWidget         *tree;
  int               count;
  GtkTreeSelection  *sel;

  logProcBegin (LOG_PROC, "confuiSwitchTable");
  if (pagenum >= (unsigned int) confui->tabcount) {
    logProcEnd (LOG_PROC, "confuiSwitchTable", "bad-pagenum");
    return;
  }

  confui->tablecurr = confui->tableidents [pagenum];
  if (confui->tablecurr == CONFUI_ID_NONE) {
    logProcEnd (LOG_PROC, "confuiSwitchTable", "no-table");
    return;
  }

  tree = confui->tables [confui->tablecurr].tree;
  sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
  count = 0;
  if (sel != NULL) {
    count = gtk_tree_selection_count_selected_rows (sel);
  }

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

  logProcEnd (LOG_PROC, "confuiSwitchTable", "");
}

static void
confuiCreateDanceTable (configui_t *confui)
{
  GtkTreeIter       iter;
  GtkListStore      *store = NULL;
  GtkCellRenderer   *renderer = NULL;
  GtkTreeViewColumn *column = NULL;
  ilistidx_t        iteridx;
  ilistidx_t        key;
  dance_t           *dances;
  GtkWidget         *tree;

  logProcBegin (LOG_PROC, "confuiCreateDancesTable");

  dances = bdjvarsdfGet (BDJVDF_DANCES);

  store = gtk_list_store_new (CONFUI_DANCE_COL_MAX,
      G_TYPE_STRING, G_TYPE_STRING, G_TYPE_ULONG);
  assert (store != NULL);

  danceStartIterator (dances, &iteridx);

  while ((key = danceIterate (dances, &iteridx)) >= 0) {
    char        *dancedisp;

    dancedisp = danceGetStr (dances, key, DANCE_DANCE);

    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter,
        CONFUI_DANCE_COL_DANCE, dancedisp,
        CONFUI_DANCE_COL_SB_PAD, "    ",
        CONFUI_DANCE_COL_DANCE_IDX, key,
        -1);
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

  logProcBegin (LOG_PROC, "confuiCreateRatingsTable");

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
  gtk_tree_view_column_set_title (column, _("Weight"));
  g_signal_connect (renderer, "edited", G_CALLBACK (confuiTableEditSpinbox), confui);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

  gtk_tree_view_set_model (GTK_TREE_VIEW (tree), GTK_TREE_MODEL (store));
  g_object_unref (store);
  logProcEnd (LOG_PROC, "confuiCreateRatingsTable", "");
}

static void
confuiRatingSet (GtkListStore *store, GtkTreeIter *iter,
    int editable, char *ratingdisp, ssize_t weight)
{
  GtkAdjustment     *adjustment;

  adjustment = gtk_adjustment_new (weight, 0.0, 100.0, 1.0, 5.0, 0.0);
  gtk_list_store_set (store, iter,
      CONFUI_RATING_COL_R_EDITABLE, editable,
      CONFUI_RATING_COL_W_EDITABLE, TRUE,
      CONFUI_RATING_COL_RATING, ratingdisp,
      CONFUI_RATING_COL_WEIGHT, weight,
      CONFUI_RATING_COL_ADJUST, adjustment,
      CONFUI_RATING_COL_DIGITS, 0,
      -1);
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
  gtk_list_store_set (store, iter,
      CONFUI_STATUS_COL_EDITABLE, editable,
      CONFUI_STATUS_COL_STATUS, statusdisp,
      CONFUI_STATUS_COL_PLAY_FLAG, playflag,
      -1);
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
  int               editable;

  logProcBegin (LOG_PROC, "confuiCreateLevelTable");

  levels = bdjvarsdfGet (BDJVDF_LEVELS);

  store = gtk_list_store_new (CONFUI_LEVEL_COL_MAX,
      G_TYPE_ULONG, G_TYPE_STRING, G_TYPE_ULONG, G_TYPE_BOOLEAN,
      G_TYPE_OBJECT, G_TYPE_ULONG);
  assert (store != NULL);

  levelStartIterator (levels, &iteridx);

  editable = FALSE;
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
    editable = TRUE;
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
  gtk_tree_view_column_set_title (column, _("Default"));
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

  gtk_tree_view_set_model (GTK_TREE_VIEW (tree), GTK_TREE_MODEL (store));
  g_object_unref (store);
  logProcEnd (LOG_PROC, "confuiCreatelevelsTable", "");
}

static void
confuiLevelSet (GtkListStore *store, GtkTreeIter *iter,
    int editable, char *leveldisp, ssize_t weight, int def)
{
  GtkAdjustment     *adjustment;

  adjustment = gtk_adjustment_new (weight, 0.0, 100.0, 1.0, 5.0, 0.0);
  gtk_list_store_set (store, iter,
      CONFUI_LEVEL_COL_EDITABLE, editable,
      CONFUI_LEVEL_COL_LEVEL, leveldisp,
      CONFUI_LEVEL_COL_WEIGHT, weight,
      CONFUI_LEVEL_COL_ADJUST, adjustment,
      CONFUI_LEVEL_COL_DIGITS, 0,
      CONFUI_LEVEL_COL_DEFAULT, def,
      -1);
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
  gtk_list_store_set (store, iter,
      CONFUI_GENRE_COL_EDITABLE, editable,
      CONFUI_GENRE_COL_GENRE, genredisp,
      CONFUI_GENRE_COL_CLASSICAL, clflag,
      -1);
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

  model = gtk_tree_view_get_model (
      GTK_TREE_VIEW (confui->tables [confui->tablecurr].tree));
  path = gtk_tree_path_new_from_string (spath);
  if (gtk_tree_model_get_iter (model, &iter, path) == FALSE) {
    return;
  }
  gtk_tree_path_free (path);
  col = confui->tables [confui->tablecurr].togglecol;
  gtk_tree_model_get (model, &iter, col, &val, -1);
  gtk_list_store_set (GTK_LIST_STORE (model), &iter, col, !val, -1);
  confui->tables [confui->tablecurr].changed = true;
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
}

static void
confuiTableEditText (GtkCellRendererText* r, const gchar* path,
    const gchar* ntext, gpointer udata)
{
  configui_t    *confui = udata;

  confuiTableEdit (confui, r, path, ntext, CONFUI_TABLE_TEXT);
}

static void
confuiTableEditSpinbox (GtkCellRendererText* r, const gchar* path,
    const gchar* ntext, gpointer udata)
{
  configui_t    *confui = udata;

  confuiTableEdit (confui, r, path, ntext, CONFUI_TABLE_NUM);
}

static void
confuiTableEdit (configui_t *confui, GtkCellRendererText* r,
    const gchar* path, const gchar* ntext, int type)
{
  GtkWidget     *tree;
  GtkTreeModel  *model;
  GtkTreeIter   iter;
  int           col;

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
}

static void
confuiTableSave (configui_t *confui, confuiident_t id)
{
  GtkWidget     *tree;
  GtkTreeModel  *model;
  savefunc_t    savefunc;

  if (confui->tables [id].changed == false) {
    return;
  }
  if (confui->tables [id].savefunc == NULL ||
      confui->tables [id].listcreatefunc == NULL) {
    return;
  }

  tree = confui->tables [id].tree;
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree));
  confui->tables [id].savelist = ilistAlloc ("cu-table-save", LIST_ORDERED);
  confui->tables [id].saveidx = 0;
  gtk_tree_model_foreach (model, confui->tables [id].listcreatefunc, confui);
  savefunc = confui->tables [id].savefunc;
  savefunc (confui);
}

static gboolean
confuiRatingListCreate (GtkTreeModel *model, GtkTreePath *path,
    GtkTreeIter *iter, gpointer udata)
{
  configui_t  *confui = udata;
  char        *ratingdisp;
  gulong      weight;

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
  return FALSE;
}

static gboolean
confuiStatusListCreate (GtkTreeModel *model, GtkTreePath *path,
    GtkTreeIter *iter, gpointer udata)
{
  configui_t  *confui = udata;
  char        *statusdisp;
  gboolean    playflag;

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
  return FALSE;
}

static gboolean
confuiGenreListCreate (GtkTreeModel *model, GtkTreePath *path,
    GtkTreeIter *iter, gpointer udata)
{
  configui_t  *confui = udata;
  char        *genredisp;
  gboolean    clflag;

  gtk_tree_model_get (model, iter,
      CONFUI_GENRE_COL_GENRE, &genredisp,
      CONFUI_GENRE_COL_CLASSICAL, &clflag,
      -1);
  ilistSetStr (confui->tables [CONFUI_ID_GENRES].savelist,
      confui->tables [CONFUI_ID_GENRES].saveidx, GENRE_GENRE, genredisp);
  ilistSetNum (confui->tables [CONFUI_ID_GENRES].savelist,
      confui->tables [CONFUI_ID_GENRES].saveidx, GENRE_CLASSICAL_FLAG, clflag);
  free (genredisp);
  confui->tables [CONFUI_ID_GENRES].saveidx += 1;
  return FALSE;
}

static void
confuiRatingSave (configui_t *confui)
{
  rating_t    *ratings;

  ratings = bdjvarsdfGet (BDJVDF_RATINGS);
  ratingSave (ratings, confui->tables [CONFUI_ID_RATINGS].savelist);
}

static void
confuiLevelSave (configui_t *confui)
{
  level_t    *levels;

  levels = bdjvarsdfGet (BDJVDF_LEVELS);
  levelSave (levels, confui->tables [CONFUI_ID_LEVELS].savelist);
}

static void
confuiStatusSave (configui_t *confui)
{
  status_t    *status;

  status = bdjvarsdfGet (BDJVDF_STATUS);
  statusSave (status, confui->tables [CONFUI_ID_STATUS].savelist);
}

static void
confuiGenreSave (configui_t *confui)
{
  genre_t    *genres;

  genres = bdjvarsdfGet (BDJVDF_GENRES);
  genreSave (genres, confui->tables [CONFUI_ID_GENRES].savelist);
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

  model = gtk_tree_view_get_model (tv);
  if (! gtk_tree_model_get_iter (model, &iter, path)) {
    return;
  }
  gtk_tree_model_get (model, &iter, CONFUI_DANCE_COL_DANCE_IDX, &idx, -1);
  key = (ilistidx_t) idx;

  dances = bdjvarsdfGet (BDJVDF_DANCES);

  sval = danceGetStr (dances, key, DANCE_DANCE);
  widx = CONFUI_ENTRY_DANCE_DANCE;
  uiutilsEntrySetValue (&confui->uiitem [widx].u.entry, sval);

  slist = danceGetList (dances, key, DANCE_TAGS);
  conv.u.list = slist;
  conv.valuetype = VALUE_LIST;
  convTextList (&conv);
  sval = conv.u.str;
  widx = CONFUI_ENTRY_DANCE_TAGS;
  uiutilsEntrySetValue (&confui->uiitem [widx].u.entry, sval);

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
}
