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
#include "bdjvarsdfload.h"
#include "conn.h"
#include "datafile.h"
#include "fileop.h"
#include "filemanip.h"
#include "localeutil.h"
#include "lock.h"
#include "log.h"
#include "nlist.h"
#include "orgopt.h"
#include "osuiutils.h"
#include "pathbld.h"
#include "pathutil.h"
#include "procutil.h"
#include "progstate.h"
#include "slist.h"
#include "sock.h"
#include "sockh.h"
#include "sysvars.h"
#include "tmutil.h"
#include "uiutils.h"
#include "volume.h"


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
  CONFUI_WIDGET_AO_CHG_SPACE,
  CONFUI_WIDGET_AUTO_ORGANIZE,
  CONFUI_WIDGET_DB_LOAD_FROM_GENRE,
  CONFUI_WIDGET_DEBUG_1,
  CONFUI_WIDGET_DEBUG_1024,
  CONFUI_WIDGET_DEBUG_128,
  CONFUI_WIDGET_DEBUG_131072,
  CONFUI_WIDGET_DEBUG_16,
  CONFUI_WIDGET_DEBUG_16384,
  CONFUI_WIDGET_DEBUG_2,
  CONFUI_WIDGET_DEBUG_2048,
  CONFUI_WIDGET_DEBUG_256,
  CONFUI_WIDGET_DEBUG_32,
  CONFUI_WIDGET_DEBUG_32768,
  CONFUI_WIDGET_DEBUG_4,
  CONFUI_WIDGET_DEBUG_4096,
  CONFUI_WIDGET_DEBUG_512,
  CONFUI_WIDGET_DEBUG_64,
  CONFUI_WIDGET_DEBUG_65536,
  CONFUI_WIDGET_DEBUG_8,
  CONFUI_WIDGET_DEBUG_8192,
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
  nlist_t             *list;
  nlist_t             *sblookuplist;
} confuiitem_t;

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
  int               orgpathidx;
  volume_t          *volume;
  GtkWidget         *fadetypeImage;
  /* gtk stuff */
  GtkApplication    *app;
  GtkWidget         *window;
  GtkWidget         *vbox;
  GtkWidget         *notebook;
  /* options */
  datafile_t        *optiondf;
  nlist_t           *options;
} configui_t;

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
static void confuiSelectFileDialog (configui_t *confui, int widx);

static GtkWidget * confuiMakeNotebookTab (GtkWidget *notebook, char *txt);
static GtkWidget * confuiMakeItemEntry (configui_t *confui, GtkWidget *vbox, GtkSizeGroup *sg, char *txt, int widx, int bdjoptIdx, char *disp);
static void confuiMakeItemEntryChooser (configui_t *confui, GtkWidget *vbox, GtkSizeGroup *sg, char *txt, int widx, int bdjoptIdx, char *disp, void *dialogFunc);
static GtkWidget * confuiMakeItemCombobox (configui_t *confui, GtkWidget *vbox, GtkSizeGroup *sg, char *txt, int widx, int bdjoptIdx, void *ddcb, char *value);
static void confuiMakeItemLink (configui_t *confui, GtkWidget *vbox, GtkSizeGroup *sg, char *txt, int widx, char *disp);
static void confuiMakeItemFontButton (configui_t *confui, GtkWidget *vbox, GtkSizeGroup *sg, char *txt, int widx, int bdjoptIdx, char *fontname);
static void confuiMakeItemColorButton (configui_t *confui, GtkWidget *vbox, GtkSizeGroup *sg, char *txt, int widx, int bdjoptIdx, char *color);
static GtkWidget *confuiMakeItemSpinboxText (configui_t *confui, GtkWidget *vbox, GtkSizeGroup *sg, char *txt, int widx, int bdjoptIdx, ssize_t value);
static void confuiMakeItemSpinboxTime (configui_t *confui, GtkWidget *vbox, GtkSizeGroup *sg, char *txt, int widx, int bdjoptIdx, ssize_t value);
static void confuiMakeItemSpinboxInt (configui_t *confui, GtkWidget *vbox, GtkSizeGroup *sg, char *txt, int widx, int bdjoptIdx, int min, int max, int value);
static void confuiMakeItemSpinboxDouble (configui_t *confui, GtkWidget *vbox, GtkSizeGroup *sg, char *txt, int widx, int bdjoptIdx, double min, double max, double value);
static void confuiMakeItemSwitch (configui_t *confui, GtkWidget *vbox, GtkSizeGroup *sg, char *txt, int widx, int bdjoptIdx, int value);
static void confuiMakeItemCheckButton (configui_t *confui, GtkWidget *vbox, GtkSizeGroup *sg, const char *txt, int widx, int bdjoptIdx, int value);
static GtkWidget * confuiMakeItemLabel (GtkWidget *vbox, GtkSizeGroup *sg, char *txt);

static nlist_t  * confuiGetThemeList (void);
static nlist_t  * confuiGetThemeNames (nlist_t *themelist, slist_t *filelist);
static void     confuiLoadHTMLList (configui_t *confui);
static void     confuiLoadLocaleList (configui_t *confui);
static gboolean confuiFadeTypeTooltip (GtkWidget *, gint, gint, gboolean, GtkTooltip *, void *);
static void     confuiOrgPathSelect (GtkTreeView *tv, GtkTreePath *path,
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
  confui.progstate = progstateInit ("configui");
  progstateSetCallback (confui.progstate, STATE_WAIT_HANDSHAKE,
      confuiHandshakeCallback, &confui);
  confui.sockserver = NULL;
  confui.window = NULL;
  confui.uithemeidx = 0;
  confui.mqthemeidx = 0;
  confui.rchtmlidx = 0;
  confui.localeidx = 0;

  for (bdjmsgroute_t i = ROUTE_NONE; i < ROUTE_MAX; ++i) {
    confui.processes [i] = NULL;
  }

  for (int i = 0; i < CONFUI_ITEM_MAX; ++i) {
    confui.uiitem [i].list = NULL;
    confui.uiitem [i].sblookuplist = NULL;
    confui.uiitem [i].basetype = CONFUI_NONE;
    confui.uiitem [i].outtype = CONFUI_OUT_NONE;
    if (i > CONFUI_BEGIN && i < CONFUI_COMBOBOX_MAX) {
      uiutilsDropDownInit (&confui.uiitem [i].u.dropdown);
    }
    if (i > CONFUI_ENTRY_MAX && i < CONFUI_SPINBOX_MAX) {
      uiutilsSpinboxTextInit (&confui.uiitem [i].u.spinbox);
    }
  }

  uiutilsEntryInit (&confui.uiitem [CONFUI_ENTRY_MUSIC_DIR].u.entry, 50, 100);
  uiutilsEntryInit (&confui.uiitem [CONFUI_ENTRY_PROFILE_NAME].u.entry, 20, 30);
  uiutilsEntryInit (&confui.uiitem [CONFUI_ENTRY_STARTUP].u.entry, 50, 100);
  uiutilsEntryInit (&confui.uiitem [CONFUI_ENTRY_SHUTDOWN].u.entry, 50, 100);
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
      confui.orgpathidx = count;
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

  confuiLoadHTMLList (&confui);
  confuiLoadLocaleList (&confui);

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
    /* the mq and ui theme share both list and sblookuplist */
    /* ui_theme > mq_theme */
    if (i == CONFUI_SPINBOX_UI_THEME) {
      continue;
    }
    if (confui->uiitem [i].list != NULL) {
      nlistFree (confui->uiitem [i].list);
    }
    if (i != CONFUI_SPINBOX_MQ_THEME && confui->uiitem [i].sblookuplist != NULL) {
      nlistFree (confui->uiitem [i].sblookuplist);
    }
    confui->uiitem [i].list = NULL;
  }

  if (confui->options != datafileGetList (confui->optiondf)) {
    nlistFree (confui->options);
  }
  datafileFree (confui->optiondf);

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

  confui->notebook = gtk_notebook_new ();
  assert (confui->notebook != NULL);
  gtk_notebook_set_show_border (GTK_NOTEBOOK (confui->notebook), TRUE);
  gtk_widget_set_margin_top (confui->notebook, 4);
  gtk_widget_set_hexpand (confui->notebook, TRUE);
  gtk_widget_set_vexpand (confui->notebook, FALSE);
  gtk_notebook_set_tab_pos (GTK_NOTEBOOK (confui->notebook), GTK_POS_LEFT);
  uiutilsSetCss (confui->notebook,
      "notebook tab:checked { background-color: #111111; }");
  gtk_box_pack_start (GTK_BOX (confui->vbox), confui->notebook,
      TRUE, TRUE, 0);

  /* general options */
  vbox = confuiMakeNotebookTab (confui->notebook, _("General Options"));
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
  vbox = confuiMakeNotebookTab (confui->notebook, _("Player Options"));
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
  vbox = confuiMakeNotebookTab (confui->notebook, _("Marquee Options"));
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
  vbox = confuiMakeNotebookTab (confui->notebook, _("User Interface"));
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

  vbox = confuiMakeNotebookTab (confui->notebook, _("Display Settings"));
  sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  vbox = confuiMakeNotebookTab (confui->notebook, _("Organisation"));
  sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  confuiMakeItemCombobox (confui, vbox, sg, _("Organisation Path"),
      CONFUI_COMBOBOX_AO_PATHFMT, OPT_G_AO_PATHFMT,
      confuiOrgPathSelect, bdjoptGetStr (OPT_G_AO_PATHFMT));
  confuiMakeItemSwitch (confui, vbox, sg, _("Auto Organise"),
      CONFUI_WIDGET_AUTO_ORGANIZE, OPT_G_AUTOORGANIZE,
      bdjoptGetNum (OPT_G_AUTOORGANIZE));
  confuiMakeItemSwitch (confui, vbox, sg, _("Remove Spaces"),
      CONFUI_WIDGET_AO_CHG_SPACE, OPT_G_AO_CHANGESPACE,
      bdjoptGetNum (OPT_G_AO_CHANGESPACE));

  vbox = confuiMakeNotebookTab (confui->notebook, _("Edit Dances"));
  sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  vbox = confuiMakeNotebookTab (confui->notebook, _("Edit Genres"));
  sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  vbox = confuiMakeNotebookTab (confui->notebook, _("Edit Levels"));
  sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  vbox = confuiMakeNotebookTab (confui->notebook, _("Edit Ratings"));
  sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  vbox = confuiMakeNotebookTab (confui->notebook, _("Edit Status"));
  sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  /* mobile remote control */
  vbox = confuiMakeNotebookTab (confui->notebook, _("Mobile Remote Control"));
  sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  confuiMakeItemSwitch (confui, vbox, sg, _("Enable Remote Control"),
      CONFUI_WIDGET_RC_ENABLE, OPT_P_REMOTECONTROL,
      bdjoptGetNum (OPT_P_REMOTECONTROL));
  confuiMakeItemSpinboxText (confui, vbox, sg, _("HTML Template"),
      CONFUI_SPINBOX_RC_HTML_TEMPLATE, OPT_G_REMCONTROLHTML,
      confui->rchtmlidx);
  confuiMakeItemEntry (confui, vbox, sg, _("User ID"),
      CONFUI_ENTRY_RC_USER_ID,  OPT_P_REMCONTROLUSER,
      bdjoptGetStr (OPT_P_REMCONTROLUSER));
  confuiMakeItemEntry (confui, vbox, sg, _("Password"),
      CONFUI_ENTRY_RC_PASS, OPT_P_REMCONTROLPASS,
      bdjoptGetStr (OPT_P_REMCONTROLPASS));

  confuiMakeItemSpinboxInt (confui, vbox, sg, _("Port Number"),
      CONFUI_WIDGET_RC_PORT, OPT_P_REMCONTROLPORT,
      8000, 30000, bdjoptGetNum (OPT_P_REMCONTROLPORT));

  snprintf (tbuff, sizeof (tbuff), "http://%s:%zd",
      "localhost", bdjoptGetNum (OPT_P_REMCONTROLPORT));
  confuiMakeItemLink (confui, vbox, sg, _("QR Code"),
      CONFUI_WIDGET_RC_QR_CODE, tbuff);

  /* mobile marquee */
  vbox = confuiMakeNotebookTab (confui->notebook, _("Mobile Marquee"));
  sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  confuiMakeItemSpinboxText (confui, vbox, sg, _("Mobile Marquee"),
      CONFUI_SPINBOX_MOBILE_MQ, OPT_P_MOBILEMARQUEE,
      bdjoptGetNum (OPT_P_MOBILEMARQUEE));

  confuiMakeItemSpinboxInt (confui, vbox, sg, _("Port"),
      CONFUI_WIDGET_MMQ_PORT, OPT_P_MOBILEMQPORT,
      8000, 30000, bdjoptGetNum (OPT_P_MOBILEMQPORT));

  confuiMakeItemEntry (confui, vbox, sg, _("Name"),
      CONFUI_ENTRY_MM_NAME, OPT_P_MOBILEMQTAG,
      bdjoptGetStr (OPT_P_MOBILEMQTAG));
  confuiMakeItemEntry (confui, vbox, sg, _("Title"),
      CONFUI_ENTRY_MM_TITLE, OPT_P_MOBILEMQTITLE,
      bdjoptGetStr (OPT_P_MOBILEMQTITLE));

  snprintf (tbuff, sizeof (tbuff), "http://%s:%zd",
      "localhost", bdjoptGetNum (OPT_P_MOBILEMQPORT));
  confuiMakeItemLink (confui, vbox, sg, _("QR Code"),
      CONFUI_WIDGET_MMQ_QR_CODE, tbuff);

  vbox = confuiMakeNotebookTab (confui->notebook, _("Debug Options"));
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
        sval = slistGetDataByIdx (confui->uiitem [i].list, confui->orgpathidx);
        outtype = CONFUI_OUT_STR;
        break;
      }
    }

    switch (outtype) {
      case CONFUI_OUT_NONE: {
        break;
      }
      case CONFUI_OUT_STR: {
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

    if (i == CONFUI_SPINBOX_LOCALE) {
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

    }

    if (i == CONFUI_ENTRY_MUSIC_DIR) {
      strlcpy (tbuff, bdjoptGetStr (confui->uiitem [i].bdjoptIdx), sizeof (tbuff));
      pathNormPath (tbuff, sizeof (tbuff));
      bdjoptSetStr (confui->uiitem [i].bdjoptIdx, tbuff);
    }

    if (i == CONFUI_SPINBOX_UI_THEME) {
      FILE    *fh;

      pathbldMakePath (tbuff, sizeof (tbuff), "",
          "theme", ".txt", PATHBLD_MP_NONE);
      fh = fopen (tbuff, "w");
      sval = bdjoptGetStr (confui->uiitem [i].bdjoptIdx);
      if (sval != NULL) {
        fprintf (fh, "%s\n", sval);
      }
      fclose (fh);
    }

    if (i == CONFUI_WIDGET_UI_ACCENT_COLOR) {
    }

    if (i == CONFUI_WIDGET_MQ_ACCENT_COLOR) {
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

  confuiSelectFileDialog (confui, CONFUI_ENTRY_STARTUP);
}

static void
confuiSelectShutdown (GtkButton *b, gpointer udata)
{
  configui_t            *confui = udata;

  confuiSelectFileDialog (confui, CONFUI_ENTRY_SHUTDOWN);
}

static void
confuiSelectFileDialog (configui_t *confui, int widx)
{
  char                  *fn = NULL;
  uiutilsselect_t       selectdata;

  selectdata.label = _("Select file");
  selectdata.window = confui->window;
  fn = uiutilsSelectFileDialog (&selectdata);
  if (fn != NULL) {
    gtk_entry_buffer_set_text (confui->uiitem [widx].u.entry.buffer,
        fn, -1);
    free (fn);
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "selected loc: %s", fn);
  }
}

static GtkWidget *
confuiMakeNotebookTab (GtkWidget *notebook, char *txt)
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
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox, tablabel);

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

static void
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

static void
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
  widget = uiutilsCreateColonLabel (txt);
  gtk_label_set_xalign (GTK_LABEL (widget), 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
  gtk_size_group_add_widget (sg, widget);
  return hbox;
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
  uiutilsdropdown_t *dd = NULL;
  ssize_t           idx;

  logProcBegin (LOG_PROC, "confuiOrgPathSelect");

  dd = &confui->uiitem [CONFUI_COMBOBOX_AO_PATHFMT].u.dropdown;
  idx = uiutilsDropDownSelectionGet (dd, path);
  confui->orgpathidx = idx;
  logProcEnd (LOG_PROC, "confuiOrgPathSelect", "");
}

