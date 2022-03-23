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

enum {
  CONFUI_ENTRY_MUSIC_DIR,
  CONFUI_ENTRY_PROFILE_NAME,
  CONFUI_ENTRY_STARTUP,
  CONFUI_ENTRY_SHUTDOWN,
  CONFUI_ENTRY_QUEUE_NM_A,
  CONFUI_ENTRY_QUEUE_NM_B,
  CONFUI_ENTRY_RC_USER_ID,
  CONFUI_ENTRY_RC_PASS,
  CONFUI_ENTRY_RC_PORT,
  CONFUI_ENTRY_MM_PORT,
  CONFUI_ENTRY_MM_NAME,
  CONFUI_ENTRY_MM_TITLE,
  CONFUI_ENTRY_MAX,
};

enum {
  CONFUI_SPINBOX_WRITE_AUDIO_FILE_TAGS,
  CONFUI_SPINBOX_LOCALE,
  CONFUI_SPINBOX_PLAYER,
  CONFUI_SPINBOX_AUDIO,
  CONFUI_SPINBOX_AUDIO_OUTPUT,
  CONFUI_SPINBOX_FADE_TYPE,
  CONFUI_SPINBOX_MQ_THEME,
  CONFUI_SPINBOX_UI_THEME,
  CONFUI_SPINBOX_RC_HTML_TEMPLATE,
  CONFUI_SPINBOX_MAX_PLAY_TIME,
  CONFUI_SPINBOX_MAX,
};

enum {
  CONFUI_WIDGET_DB_LOAD_FROM_GENRE,
  CONFUI_WIDGET_ENABLE_ITUNES,
  CONFUI_WIDGET_DEFAULT_VOL,
  CONFUI_WIDGET_FADE_IN_TIME,
  CONFUI_WIDGET_FADE_OUT_TIME,
  CONFUI_WIDGET_GAP,
  CONFUI_WIDGET_PL_QUEUE_LEN,
  CONFUI_WIDGET_MQ_ACCENT_COLOR,
  CONFUI_WIDGET_MQ_FONT,
  CONFUI_WIDGET_MQ_QUEUE_LEN,
  CONFUI_WIDGET_MQ_SHOW_SONG_INFO,
  CONFUI_WIDGET_UI_FONT,
  CONFUI_WIDGET_UI_LISTING_FONT,
  CONFUI_WIDGET_UI_ACCENT_COLOR,
  CONFUI_WIDGET_RC_ENABLE,
  CONFUI_WIDGET_RC_QR_CODE,
  CONFUI_WIDGET_MMQ_ENABLE,
  CONFUI_WIDGET_MMQ_QR_CODE,
  CONFUI_WIDGET_MAX,
};



typedef struct {
  progstate_t       *progstate;
  char              *locknm;
  procutil_t        *processes [ROUTE_MAX];
  conn_t            *conn;
  sockserver_t      *sockserver;
  int               dbgflags;
  uiutilsentry_t    uientry [CONFUI_ENTRY_MAX];
  uiutilsspinbox_t  uispinbox [CONFUI_SPINBOX_MAX];
  nlist_t           * uilists [CONFUI_SPINBOX_MAX];
  nlist_t           * lookuplists [CONFUI_SPINBOX_MAX];
  int               uithemeidx;
  int               mqthemeidx;
  int               rchtmlidx;
  int               localeidx;
  /* gtk stuff */
  GtkApplication    *app;
  GtkWidget         *window;
  GtkWidget         *vbox;
  GtkWidget         *notebook;
  GtkWidget         *uiwidgets [CONFUI_WIDGET_MAX];
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

static bool     confuiListeningCallback (void *udata, programstate_t programState);
static bool     confuiConnectingCallback (void *udata, programstate_t programState);
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

static GtkWidget * confuiMakeNotebookTab (GtkWidget *notebook, char *txt);
static GtkWidget * confuiMakeItemEntry (configui_t *confui, GtkWidget *vbox, GtkSizeGroup *sg, char *txt, int entryIdx, char *disp);
static void confuiMakeItemEntryChooser (configui_t *confui, GtkWidget *vbox, GtkSizeGroup *sg, char *txt, int entryIdx, char *disp);
static void confuiMakeItemLink (configui_t *confui, GtkWidget *vbox, GtkSizeGroup *sg, int widx, char *txt, char *disp);
static void confuiMakeItemFontButton (configui_t *confui, GtkWidget *vbox, GtkSizeGroup *sg, int widx, char *txt, char *fontname);
static void confuiMakeItemColorButton (configui_t *confui, GtkWidget *vbox, GtkSizeGroup *sg, int widx, char *txt, char *color);
static void confuiMakeItemSpinboxText (configui_t *confui, GtkWidget *vbox, GtkSizeGroup *sg, int sbidx, char *txt, ssize_t value);
static void confuiMakeItemSpinboxTime (configui_t *confui, GtkWidget *vbox, GtkSizeGroup *sg, int sbidx, char *txt, ssize_t value);
static void confuiMakeItemSpinboxInt (configui_t *confui, GtkWidget *vbox, GtkSizeGroup *sg, int widx, char *txt, int min, int max, int value);
static void confuiMakeItemSpinboxDouble (configui_t *confui, GtkWidget *vbox, GtkSizeGroup *sg, int widx, char *txt, double min, double max, double value);
static void confuiMakeItemSwitch (configui_t *confui, GtkWidget *vbox, GtkSizeGroup *sg, int widx, char *txt, int value);
static GtkWidget * confuiMakeItemLabel (GtkWidget *vbox, GtkSizeGroup *sg, char *txt);

static nlist_t  * confuiGetThemeList (void);
static nlist_t  * confuiGetThemeNames (nlist_t *themelist, slist_t *filelist);
static void     confuiLoadHTMLList (configui_t *confui);
static void     confuiLoadLocaleList (configui_t *confui);

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


  confui.notebook = NULL;
  confui.progstate = progstateInit ("configui");
  progstateSetCallback (confui.progstate, STATE_LISTENING,
      confuiListeningCallback, &confui);
  progstateSetCallback (confui.progstate, STATE_CONNECTING,
      confuiConnectingCallback, &confui);
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

  for (int i = 0; i < CONFUI_SPINBOX_MAX; ++i) {
    confui.uilists [i] = NULL;
    confui.lookuplists [i] = NULL;
    uiutilsSpinboxTextInit (&confui.uispinbox [i]);
  }

  for (int i = 0; i < CONFUI_WIDGET_MAX; ++i) {
    confui.uiwidgets [i] = NULL;
  }

  uiutilsEntryInit (&confui.uientry [CONFUI_ENTRY_MUSIC_DIR], 50, 100);
  uiutilsEntryInit (&confui.uientry [CONFUI_ENTRY_PROFILE_NAME], 20, 30);
  uiutilsEntryInit (&confui.uientry [CONFUI_ENTRY_STARTUP], 50, 100);
  uiutilsEntryInit (&confui.uientry [CONFUI_ENTRY_SHUTDOWN], 50, 100);
  uiutilsEntryInit (&confui.uientry [CONFUI_ENTRY_QUEUE_NM_A], 20, 30);
  uiutilsEntryInit (&confui.uientry [CONFUI_ENTRY_QUEUE_NM_B], 20, 30);
  uiutilsEntryInit (&confui.uientry [CONFUI_ENTRY_RC_USER_ID], 10, 30);
  uiutilsEntryInit (&confui.uientry [CONFUI_ENTRY_RC_PASS], 10, 20);
  uiutilsEntryInit (&confui.uientry [CONFUI_ENTRY_RC_PORT], 10, 10);
  uiutilsEntryInit (&confui.uientry [CONFUI_ENTRY_MM_PORT], 10, 10);
  uiutilsEntryInit (&confui.uientry [CONFUI_ENTRY_MM_NAME], 10, 40);
  uiutilsEntryInit (&confui.uientry [CONFUI_ENTRY_MM_TITLE], 20, 100);

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

  tlist = nlistAlloc ("cu-writetags", LIST_UNORDERED, free);
  llist = nlistAlloc ("cu-writetags-l", LIST_ORDERED, free);
  nlistSetStr (tlist, WRITE_TAGS_ALL, _("All Tags"));
  nlistSetStr (tlist, WRITE_TAGS_BDJ_ONLY, _("BDJ Tags Only"));
  nlistSetStr (tlist, WRITE_TAGS_NONE, _("Don't Write"));
  confui.uilists [CONFUI_SPINBOX_WRITE_AUDIO_FILE_TAGS] = tlist;

  tlist = nlistAlloc ("cu-fadetype", LIST_UNORDERED, free);
  nlistSetStr (tlist, FADETYPE_TRIANGLE, _("Triangle"));
  nlistSetStr (tlist, FADETYPE_QUARTER_SINE, _("Quarter Sine Wave"));
  nlistSetStr (tlist, FADETYPE_HALF_SINE, _("Half Sine Wave"));
  nlistSetStr (tlist, FADETYPE_LOGARITHMIC, _("Logarithmic"));
  nlistSetStr (tlist, FADETYPE_INVERTED_PARABOLA, _("Inverted Parabola"));
  confui.uilists [CONFUI_SPINBOX_FADE_TYPE] = tlist;

  tlist = nlistAlloc ("cu-player", LIST_UNORDERED, free);
  nlistSetStr (tlist, 0, _("Integrated VLC"));
  nlistSetStr (tlist, 1, _("Null Player"));
  confui.uilists [CONFUI_SPINBOX_PLAYER] = tlist;

  tlist = nlistAlloc ("cu-audio", LIST_UNORDERED, free);
  llist = nlistAlloc ("cu-audio-l", LIST_ORDERED, free);
  count = 0;
  if (isWindows ()) {
    nlistSetStr (tlist, count, "Windows");
    nlistSetStr (llist, count, "volwin");
    ++count;
  }
  if (isMacOS ()) {
    nlistSetStr (tlist, count, "MacOS");
    nlistSetStr (llist, count, "volmac");
    ++count;
  }
  if (isLinux ()) {
    nlistSetStr (tlist, count, "Pulse Audio");
    nlistSetStr (llist, count, "volpa");
    ++count;
    nlistSetStr (tlist, count, "ALSA");
    nlistSetStr (llist, count, "volalsa");
    ++count;
  }
  nlistSetStr (tlist, count++, _("Null Audio"));
  nlistSetStr (llist, count, "volnull");
  ++count;
  confui.uilists [CONFUI_SPINBOX_AUDIO] = tlist;
  confui.lookuplists [CONFUI_SPINBOX_AUDIO] = llist;

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
  confui.uilists [CONFUI_SPINBOX_UI_THEME] = tlist;
  confui.uilists [CONFUI_SPINBOX_MQ_THEME] = tlist;

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
  datafileSaveKeyVal (fn, configuidfkeys, CONFUI_KEY_MAX, confui->options);

  sockhCloseServer (confui->sockserver);

  bdj4shutdown (ROUTE_CONFIGUI);

  for (int i = 0; i < CONFUI_SPINBOX_MAX; ++i) {
    uiutilsSpinboxTextFree (&confui->uispinbox [i]);
    /* the mq and ui theme share a list */
    if (i != CONFUI_SPINBOX_UI_THEME && confui->uilists [i] != NULL) {
      nlistFree (confui->uilists [i]);
    }
    if (i != CONFUI_SPINBOX_UI_THEME && confui->lookuplists [i] != NULL) {
      nlistFree (confui->lookuplists [i]);
    }
    confui->uilists [i] = NULL;
  }

  for (int i = 0; i < CONFUI_ENTRY_MAX; ++i) {
    uiutilsEntryFree (&confui->uientry [i]);
  }

  if (confui->options != datafileGetList (confui->optiondf)) {
    nlistFree (confui->options);
  }
  datafileFree (confui->optiondf);

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

  /* gtk somehow manages to screw up the localization; re-bind the text domain */
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
  GtkSizeGroup  *sg;
  char          imgbuff [MAXPATHLEN];
  char          tbuff [40];
  gint          x, y;

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

  vbox = confuiMakeNotebookTab (confui->notebook, _("General Options"));
  sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  /* general options */
  confuiMakeItemEntryChooser (confui, vbox, sg, _("Music Folder"),
      CONFUI_ENTRY_MUSIC_DIR, bdjoptGetStr (OPT_M_DIR_MUSIC));
  confuiMakeItemEntry (confui, vbox, sg, _("Profile Name"),
      CONFUI_ENTRY_PROFILE_NAME, bdjoptGetStr (OPT_P_PROFILENAME));

  /* database */
  confuiMakeItemSpinboxText (confui, vbox, sg, CONFUI_SPINBOX_WRITE_AUDIO_FILE_TAGS,
      _("Write Audio File Tags"), bdjoptGetNum (OPT_G_WRITETAGS));
  confuiMakeItemSwitch (confui, vbox, sg, CONFUI_WIDGET_DB_LOAD_FROM_GENRE,
      _("Database Loads Dance From Genre"),
      bdjoptGetNum (OPT_G_LOADDANCEFROMGENRE));
  confuiMakeItemSwitch (confui, vbox, sg, CONFUI_WIDGET_ENABLE_ITUNES,
      _("Enable iTunes Support"),
      bdjoptGetNum (OPT_G_ITUNESSUPPORT));

  /* bdj4 */
  confuiMakeItemSpinboxText (confui, vbox, sg, CONFUI_SPINBOX_LOCALE,
      _("Locale"), 0);
  confuiMakeItemEntryChooser (confui, vbox, sg, _("Startup Script"),
      CONFUI_ENTRY_STARTUP, bdjoptGetStr (OPT_M_STARTUPSCRIPT));
  confuiMakeItemEntryChooser (confui, vbox, sg, _("Shutdown Script"),
      CONFUI_ENTRY_SHUTDOWN, bdjoptGetStr (OPT_M_SHUTDOWNSCRIPT));

  vbox = confuiMakeNotebookTab (confui->notebook, _("Player Options"));
  sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  /* player options */
  confuiMakeItemSpinboxText (confui, vbox, sg, CONFUI_SPINBOX_PLAYER,
      _("Player"), 0);
  confuiMakeItemSpinboxText (confui, vbox, sg, CONFUI_SPINBOX_AUDIO,
      _("Audio"), 0);
  confuiMakeItemSpinboxText (confui, vbox, sg, CONFUI_SPINBOX_AUDIO_OUTPUT,
      _("Audio Output"), 0);
  confuiMakeItemSpinboxInt (confui, vbox, sg, CONFUI_WIDGET_DEFAULT_VOL,
      _("Default Volume"), 10, 100,
      bdjoptGetNum (OPT_P_DEFAULTVOLUME));
  confuiMakeItemSpinboxDouble (confui, vbox, sg, CONFUI_WIDGET_FADE_IN_TIME,
      _("Fade In Time"), 0.0, 2.0,
      (double) bdjoptGetNum (OPT_P_FADEINTIME) / 1000.0);
  confuiMakeItemSpinboxDouble (confui, vbox, sg, CONFUI_WIDGET_FADE_OUT_TIME,
      _("Fade Out Time"), 0.0, 10.0,
      (double) bdjoptGetNum (OPT_P_FADEOUTTIME) / 1000.0);
  confuiMakeItemSpinboxText (confui, vbox, sg, CONFUI_SPINBOX_FADE_TYPE,
      _("Fade Type"), bdjoptGetNum (OPT_P_FADETYPE));
  confuiMakeItemSpinboxDouble (confui, vbox, sg, CONFUI_WIDGET_GAP,
      _("Gap Between Songs"), 0.0, 60.0,
      (double) bdjoptGetNum (OPT_P_GAP) / 1000.0);
  confuiMakeItemSpinboxTime (confui, vbox, sg, CONFUI_SPINBOX_MAX_PLAY_TIME,
      _("Maximum Play Time"), bdjoptGetNum (OPT_P_MAXPLAYTIME));
  confuiMakeItemSpinboxInt (confui, vbox, sg, CONFUI_WIDGET_PL_QUEUE_LEN,
      _("Queue Length"), 20, 400,
      bdjoptGetNum (OPT_G_PLAYERQLEN));

  confuiMakeItemEntry (confui, vbox, sg, _("Queue A Name"),
      CONFUI_ENTRY_QUEUE_NM_A, bdjoptGetStr (OPT_P_QUEUE_NAME_A));
  confuiMakeItemEntry (confui, vbox, sg, _("Queue B Name"),
      CONFUI_ENTRY_QUEUE_NM_B, bdjoptGetStr (OPT_P_QUEUE_NAME_B));

  vbox = confuiMakeNotebookTab (confui->notebook, _("Marquee Options"));
  sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  /* marquee options */
  confuiMakeItemSpinboxText (confui, vbox, sg, CONFUI_SPINBOX_MQ_THEME,
      _("Marquee Theme"), confui->mqthemeidx);
  confuiMakeItemFontButton (confui, vbox, sg, CONFUI_WIDGET_MQ_FONT,
      _("Marquee Font"),
      bdjoptGetStr (OPT_MP_MQFONT));
  confuiMakeItemSpinboxInt (confui, vbox, sg, CONFUI_WIDGET_MQ_QUEUE_LEN,
      _("Queue Length"), 1, 20,
      bdjoptGetNum (OPT_P_MQQLEN));
  confuiMakeItemSwitch (confui, vbox, sg, CONFUI_WIDGET_MQ_SHOW_SONG_INFO,
      _("Show Song Information"),
      bdjoptGetNum (OPT_P_MQ_SHOW_INFO));
  confuiMakeItemColorButton (confui, vbox, sg, CONFUI_WIDGET_MQ_ACCENT_COLOR,
      _("Accent Color"),
      bdjoptGetStr (OPT_P_MQ_ACCENT_COL));

  vbox = confuiMakeNotebookTab (confui->notebook, _("User Interface"));
  sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  /* user infterface */
  confuiMakeItemSpinboxText (confui, vbox, sg, CONFUI_SPINBOX_UI_THEME,
      _("Theme"), confui->uithemeidx);
  confuiMakeItemFontButton (confui, vbox, sg, CONFUI_WIDGET_UI_FONT,
      _("Font"),
      bdjoptGetStr (OPT_MP_UIFONT));
  confuiMakeItemFontButton (confui, vbox, sg, CONFUI_WIDGET_UI_LISTING_FONT,
      _("Listing Font"),
      bdjoptGetStr (OPT_MP_LISTING_FONT));
  confuiMakeItemColorButton (confui, vbox, sg, CONFUI_WIDGET_UI_ACCENT_COLOR,
      _("Accent Color"),
      bdjoptGetStr (OPT_P_UI_ACCENT_COL));

  vbox = confuiMakeNotebookTab (confui->notebook, _("Organization"));
  sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

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

  vbox = confuiMakeNotebookTab (confui->notebook, _("Mobile Remote Control"));
  sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  /* mobile remote control */
  confuiMakeItemSwitch (confui, vbox, sg, CONFUI_WIDGET_RC_ENABLE,
      _("Enable Remote Control"),
      bdjoptGetNum (OPT_P_REMOTECONTROL));
  confuiMakeItemSpinboxText (confui, vbox, sg, CONFUI_SPINBOX_RC_HTML_TEMPLATE,
      _("HTML Template"), confui->rchtmlidx);
  confuiMakeItemEntry (confui, vbox, sg, _("User ID"),
      CONFUI_ENTRY_RC_USER_ID, bdjoptGetStr (OPT_P_REMCONTROLUSER));
  confuiMakeItemEntry (confui, vbox, sg, _("Password"),
      CONFUI_ENTRY_RC_PASS, bdjoptGetStr (OPT_P_REMCONTROLPASS));
  snprintf (tbuff, sizeof (tbuff), "%zd", bdjoptGetNum (OPT_P_REMCONTROLPORT));
  confuiMakeItemEntry (confui, vbox, sg, _("Port Number"),
      CONFUI_ENTRY_RC_PORT, tbuff);
  snprintf (tbuff, sizeof (tbuff), "http://%s:%zd",
      "localhost", bdjoptGetNum (OPT_P_REMCONTROLPORT));
  confuiMakeItemLink (confui, vbox, sg, CONFUI_WIDGET_RC_QR_CODE,
      _("QR Code"), tbuff);

  vbox = confuiMakeNotebookTab (confui->notebook, _("Mobile Marquee"));
  sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  /* mobile marquee */
  confuiMakeItemSwitch (confui, vbox, sg, CONFUI_WIDGET_MMQ_ENABLE,
      _("Mobile Marquee"),
      bdjoptGetNum (OPT_P_MOBILEMARQUEE));
  snprintf (tbuff, sizeof (tbuff), "%zd", bdjoptGetNum (OPT_P_MOBILEMQPORT));
  confuiMakeItemEntry (confui, vbox, sg, _("Port"),
      CONFUI_ENTRY_MM_PORT, tbuff);
  confuiMakeItemEntry (confui, vbox, sg, _("Name"),
      CONFUI_ENTRY_MM_NAME, bdjoptGetStr (OPT_P_MOBILEMQTAG));
  confuiMakeItemEntry (confui, vbox, sg, _("Title"),
      CONFUI_ENTRY_MM_TITLE, bdjoptGetStr (OPT_P_MOBILEMQTITLE));
  snprintf (tbuff, sizeof (tbuff), "http://%s:%zd",
      "localhost", bdjoptGetNum (OPT_P_MOBILEMQPORT));
  confuiMakeItemLink (confui, vbox, sg, CONFUI_WIDGET_MMQ_QR_CODE,
      _("QR Code"), tbuff);

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

  if (gKillReceived) {
    logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
    progstateShutdownProcess (confui->progstate);
  }
  return cont;
}

static bool
confuiListeningCallback (void *udata, programstate_t programState)
{
  configui_t    *confui = udata;

  return true;
}

static bool
confuiConnectingCallback (void *udata, programstate_t programState)
{
  configui_t   *confui = udata;

  return true;
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
    char *txt, int entryIdx, char *disp)
{
  GtkWidget   *hbox;
  GtkWidget   *widget;

  hbox = confuiMakeItemLabel (vbox, sg, txt);
  widget = uiutilsEntryCreate (&confui->uientry [entryIdx]);
  gtk_widget_set_margin_start (widget, 8);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  if (disp != NULL) {
    uiutilsEntrySetValue (&confui->uientry [entryIdx], disp);
  } else {
    uiutilsEntrySetValue (&confui->uientry [entryIdx], "");
  }
  return hbox;
}

static void
confuiMakeItemEntryChooser (configui_t *confui, GtkWidget *vbox,
    GtkSizeGroup *sg, char *txt, int entryIdx, char *disp)
{
  GtkWidget   *hbox;
  GtkWidget   *widget;
  GtkWidget   *image;

  hbox = confuiMakeItemEntry (confui, vbox, sg, txt, entryIdx, disp);

  widget = uiutilsCreateButton ("", NULL, NULL, NULL);
  image = gtk_image_new_from_icon_name ("folder", GTK_ICON_SIZE_BUTTON);
  gtk_button_set_image (GTK_BUTTON (widget), image);
  gtk_button_set_always_show_image (GTK_BUTTON (widget), TRUE);
  gtk_widget_set_margin_start (widget, 0);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
}

static void
confuiMakeItemLink (configui_t *confui, GtkWidget *vbox, GtkSizeGroup *sg,
    int widx, char *txt, char *disp)
{
  GtkWidget   *hbox;
  GtkWidget   *widget;

  hbox = confuiMakeItemLabel (vbox, sg, txt);
  widget = gtk_link_button_new (disp);
//  gtk_widget_set_margin_start (widget, 8);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  confui->uiwidgets [widx] = widget;
}

static void
confuiMakeItemFontButton (configui_t *confui, GtkWidget *vbox, GtkSizeGroup *sg,
    int widx, char *txt, char *fontname)
{
  GtkWidget   *hbox;
  GtkWidget   *widget;

  hbox = confuiMakeItemLabel (vbox, sg, txt);
  if (fontname != NULL && *fontname) {
    widget = gtk_font_button_new_with_font (fontname);
  } else {
    widget = gtk_font_button_new ();
  }
  gtk_widget_set_margin_start (widget, 8);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  confui->uiwidgets [widx] = widget;
}

static void
confuiMakeItemColorButton (configui_t *confui, GtkWidget *vbox, GtkSizeGroup *sg,
    int widx, char *txt, char *color)
{
  GtkWidget   *hbox;
  GtkWidget   *widget;
  GdkRGBA     rgba;


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
  confui->uiwidgets [widx] = widget;
}

static void
confuiMakeItemSpinboxText (configui_t *confui, GtkWidget *vbox, GtkSizeGroup *sg,
    int sbidx, char *txt, ssize_t value)
{
  GtkWidget   *hbox;
  GtkWidget   *widget;
  nlist_t     *list;
  size_t      maxWidth;


  hbox = confuiMakeItemLabel (vbox, sg, txt);
  widget = uiutilsSpinboxTextCreate (&confui->uispinbox [sbidx], confui);
  list = confui->uilists [sbidx];
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

  uiutilsSpinboxTextSet (&confui->uispinbox [sbidx], 0,
      nlistGetCount (list), maxWidth, list, NULL);
  uiutilsSpinboxTextSetValue (&confui->uispinbox [sbidx], value);
  gtk_widget_set_margin_start (widget, 8);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
}

static void
confuiMakeItemSpinboxTime (configui_t *confui, GtkWidget *vbox,
    GtkSizeGroup *sg, int sbidx, char *txt, ssize_t value)
{
  GtkWidget   *hbox;
  GtkWidget   *widget;


  hbox = confuiMakeItemLabel (vbox, sg, txt);
  widget = uiutilsSpinboxTimeCreate (&confui->uispinbox [sbidx], confui);
  uiutilsSpinboxTimeSetValue (&confui->uispinbox [sbidx], value);
  gtk_widget_set_margin_start (widget, 8);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
}

static void
confuiMakeItemSpinboxInt (configui_t *confui, GtkWidget *vbox, GtkSizeGroup *sg,
    int widx, char *txt, int min, int max, int value)
{
  GtkWidget   *hbox;
  GtkWidget   *widget;


  hbox = confuiMakeItemLabel (vbox, sg, txt);
  widget = uiutilsSpinboxIntCreate ();
  uiutilsSpinboxSet (widget, (double) min, (double) max);
  uiutilsSpinboxSetValue (widget, (double) value);
  gtk_widget_set_margin_start (widget, 8);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  confui->uiwidgets [widx] = widget;
}

static void
confuiMakeItemSpinboxDouble (configui_t *confui, GtkWidget *vbox, GtkSizeGroup *sg,
    int widx, char *txt, double min, double max, double value)
{
  GtkWidget   *hbox;
  GtkWidget   *widget;


  hbox = confuiMakeItemLabel (vbox, sg, txt);
  widget = uiutilsSpinboxDoubleCreate ();
  uiutilsSpinboxSet (widget, min, max);
  uiutilsSpinboxSetValue (widget, value);
  gtk_widget_set_margin_start (widget, 8);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  confui->uiwidgets [widx] = widget;
}

static void
confuiMakeItemSwitch (configui_t *confui, GtkWidget *vbox, GtkSizeGroup *sg,
    int widx, char *txt, int value)
{
  GtkWidget   *hbox;
  GtkWidget   *widget;


  hbox = confuiMakeItemLabel (vbox, sg, txt);
  widget = uiutilsCreateSwitch (value);
  gtk_widget_set_margin_start (widget, 8);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  confui->uiwidgets [widx] = widget;
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

  confui->uilists [CONFUI_SPINBOX_RC_HTML_TEMPLATE] = tlist;
  confui->lookuplists [CONFUI_SPINBOX_RC_HTML_TEMPLATE] = llist;
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


  tlist = nlistAlloc ("cu-locale-list", LIST_ORDERED, free);
  llist = nlistAlloc ("cu-locale-list-l", LIST_ORDERED, free);

  pathbldMakePath (tbuff, sizeof (tbuff), "",
      "locales", ".txt", PATHBLD_MP_LOCALEDIR);
  df = datafileAllocParse ("conf-locale-list", DFTYPE_KEY_VAL, tbuff,
      NULL, 0, DATAFILE_NO_LOOKUP);
  list = datafileGetList (df);

  slistStartIterator (list, &iteridx);
  count = 0;
  while ((key = slistIterateKey (list, &iteridx)) != NULL) {
    data = slistGetStr (list, key);
    if (strcmp (data, sysvarsGetStr (SV_LOCALE)) == 0) {
      confui->localeidx = count;
    }
    nlistSetStr (tlist, count, key);
    nlistSetStr (llist, count, data);
    ++count;
  }

  confui->uilists [CONFUI_SPINBOX_LOCALE] = tlist;
  confui->lookuplists [CONFUI_SPINBOX_LOCALE] = llist;
}


