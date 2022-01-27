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
#include "bdjmsg.h"
#include "bdjopt.h"
#include "bdjstring.h"
#include "bdjvars.h"
#include "lock.h"
#include "log.h"
#include "pathbld.h"
#include "process.h"
#include "sock.h"
#include "sockh.h"
#include "sysvars.h"
#include "tmutil.h"

typedef struct {
  programstate_t  programState;
  Sock_t          mainSock;
  sockserver_t    *sockserver;
  char            *mqfont;
  mstime_t        tm;
  GtkWidget       *window;
  GtkWidget       *vbox;
  GtkWidget       *pbar;
  GtkWidget       *danceLab;
  GtkWidget       *countdownTimerLab;
  GtkWidget       *infoLab;
  GtkWidget       *sep;
  GtkWidget       **marqueeLabs;
  int             marginTotal;
  gulong          sizeSignal;
  gulong          unmaxSignal;
  int             newFontSize;
  int             setFontSize;
  int             mqLen;
  int             lastHeight;
  int             priorSize;
  bool            isMaximized : 1;
  bool            unMaximize : 1;
  bool            isHidden : 1;
  bool            inResize : 1;
  bool            doubleClickMax : 1;
  bool            inMax : 1;
  bool            setPrior : 1;
  bool            mqShowInfo : 1;
  bool            mainHandshake : 1;
} marquee_t;

static int  marqueeCreateGui (marquee_t *marquee, int argc, char *argv []);
static void marqueeActivate (GApplication *app, gpointer userdata);
gboolean    marqueeMainLoop (void *tmarquee);
static int  marqueeProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
    bdjmsgmsg_t msg, char *args, void *udata);
static gboolean marqueeCloseWin (GtkWidget *window, GdkEvent *event, gpointer userdata);
static gboolean marqueeToggleFullscreen (GtkWidget *window,
    GdkEventButton *event, gpointer userdata);
static gboolean marqueeResized (GtkWidget *window, GdkEventConfigure *event,
    gpointer userdata);
static gboolean marqueeWinState (GtkWidget *window, GdkEventWindowState *event,
    gpointer userdata);
static void marqueeStateChg (GtkWidget *w, GtkStateType flags, gpointer userdata);
static void marqueeSigHandler (int sig);
static void marqueeSetFontSize (marquee_t *marquee, GtkWidget *lab, char *style, int sz);
static void marqueeAdjustFontSizes (marquee_t *marquee, int sz);
static void marqueeAdjustFontCallback (GtkWidget *w, GtkAllocation *retAllocSize, gpointer userdata);
static void marqueeSetCss (GtkWidget *w, char *style);
static void marqueePopulate (marquee_t *marquee, char *args);
static void marqueeSetTimer (marquee_t *marquee, char *args);
static void marqueeUnmaxCallback (GtkWidget *w, GtkAllocation *retAllocSize, gpointer userdata);

static int gKillReceived = 0;

int
main (int argc, char *argv[])
{
  int             status = 0;
  int             c = 0;
  int             rc = 0;
  int             option_index = 0;
  uint16_t        listenPort;
  loglevel_t      loglevel = LOG_IMPORTANT | LOG_MAIN;
  marquee_t       marquee;

  static struct option bdj_options [] = {
    { "debug",      required_argument,  NULL,   'd' },
    { "profile",    required_argument,  NULL,   'p' },
    { "marquee",    no_argument,        NULL,   0 },
    { NULL,         0,                  NULL,   0 }
  };

  mstimestart (&marquee.tm);
  marquee.programState = STATE_INITIALIZING;
  marquee.mainSock = INVALID_SOCKET;
  marquee.sockserver = NULL;
  marquee.window = NULL;
  marquee.pbar = NULL;
  marquee.danceLab = NULL;
  marquee.countdownTimerLab = NULL;
  marquee.infoLab = NULL;
  marquee.sep = NULL;
  marquee.marqueeLabs = NULL;
  marquee.lastHeight = 0;
  marquee.priorSize = 0;
  marquee.isMaximized = false;
  marquee.unMaximize = false;
  marquee.isHidden = false;
  marquee.inResize = false;
  marquee.doubleClickMax = false;
  marquee.inMax = false;
  marquee.setPrior = false;
  marquee.mqfont = "";
  marquee.marginTotal = 0;
  marquee.sizeSignal = 0;
  marquee.unmaxSignal = 0;
  marquee.newFontSize = 0;
  marquee.setFontSize = 0;
  marquee.mainHandshake = false;

#if _define_SIGHUP
  processCatchSignal (marqueeSigHandler, SIGHUP);
#endif
  processCatchSignal (marqueeSigHandler, SIGINT);
  processDefaultSignal (SIGTERM);
#if _define_SIGCHLD
  processDefaultSignal (SIGCHLD);
#endif

  sysvarsInit (argv[0]);

  while ((c = getopt_long (argc, argv, "", bdj_options, &option_index)) != -1) {
    switch (c) {
      case 'd': {
        if (optarg) {
          loglevel = (loglevel_t) atoi (optarg);
        }
        break;
      }
      case 'p': {
        if (optarg) {
          sysvarSetNum (SVL_BDJIDX, atol (optarg));
        }
        break;
      }
      default: {
        break;
      }
    }
  }

  logStartAppend ("bdj4marquee", "mq", loglevel);
  logMsg (LOG_SESS, LOG_IMPORTANT, "Using profile %ld", lsysvars [SVL_BDJIDX]);

  rc = lockAcquire (MARQUEE_LOCK_FN, PATHBLD_MP_USEIDX);
  if (rc < 0) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: marquee: unable to acquire lock: profile: %zd", lsysvars [SVL_BDJIDX]);
    logMsg (LOG_SESS, LOG_IMPORTANT, "ERR: marquee: unable to acquire lock: profile: %zd", lsysvars [SVL_BDJIDX]);
    logEnd ();
    exit (0);
  }

  bdjvarsInit ();
  bdjoptInit ();

  listenPort = bdjvarsl [BDJVL_MARQUEE_PORT];
  marquee.mqLen = bdjoptGetNum (OPT_P_MQQLEN);
  marquee.mqShowInfo = bdjoptGetNum (OPT_P_MQ_SHOW_INFO);
  marquee.mqfont = strdup (bdjoptGetData (OPT_MP_MQFONT));
  if (marquee.mqfont == NULL) {
    marquee.mqfont = "";
  }

  marquee.sockserver = sockhStartServer (listenPort);
  g_timeout_add (5, marqueeMainLoop, &marquee);

  status = marqueeCreateGui (&marquee, 0, NULL);

  sockhCloseServer (marquee.sockserver);
  bdjoptFree ();
  bdjvarsCleanup ();

  lockRelease (MARQUEE_LOCK_FN, PATHBLD_MP_USEIDX);
  logMsg (LOG_SESS, LOG_IMPORTANT, "time-to-end: %ld ms", mstimeend (&marquee.tm));
  logEnd ();

  if (marquee.mqfont != NULL && *marquee.mqfont != '\0') {
    free (marquee.mqfont);
  }
  if (marquee.marqueeLabs != NULL) {
    free (marquee.marqueeLabs);
  }
  return status;
}

/* internal routines */

static int
marqueeCreateGui (marquee_t *marquee, int argc, char *argv [])
{
  int             status;
  GtkApplication  *app;

  app = gtk_application_new (
      "org.ballroomdj.BallroomDJ",
      G_APPLICATION_FLAGS_NONE
  );
  g_signal_connect (app, "activate", G_CALLBACK (marqueeActivate), marquee);

  status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);
  return status;
}

static void
marqueeActivate (GApplication *app, gpointer userdata)
{
  marquee_t             *marquee = userdata;
  GtkWidget             *window;
  GtkWidget             *hbox;
  GtkWidget             *vbox;
  GError                *gerr;


  window = gtk_application_window_new (GTK_APPLICATION (app));
  g_signal_connect (window, "delete-event", G_CALLBACK (marqueeCloseWin), marquee);
  g_signal_connect (window, "button-press-event", G_CALLBACK (marqueeToggleFullscreen), marquee);
  g_signal_connect (window, "configure-event", G_CALLBACK (marqueeResized), marquee);
  g_signal_connect (window, "window-state-event", G_CALLBACK (marqueeWinState), marquee);
  /* the backdrop window state must be intercepted */
  g_signal_connect (window, "state-flags-changed", G_CALLBACK (marqueeStateChg), marquee);

  gtk_window_set_type_hint (GTK_WINDOW (window), GDK_WINDOW_TYPE_HINT_NORMAL);
  gtk_window_set_focus_on_map (GTK_WINDOW (window), FALSE);
  gtk_window_set_keep_above (GTK_WINDOW (window), TRUE);
  gtk_window_set_title (GTK_WINDOW (window), _("Marquee"));
  gtk_window_set_default_icon_from_file ("img/bdj4_icon.svg", &gerr);
  gtk_window_set_default_size (GTK_WINDOW (window), 600, 600);
  marquee->window = window;

  marquee->vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 5);
  gtk_widget_set_margin_top (GTK_WIDGET (marquee->vbox), 20);
  gtk_widget_set_margin_bottom (GTK_WIDGET (marquee->vbox), 10);
  gtk_container_add (GTK_CONTAINER (window), marquee->vbox);
  gtk_widget_set_hexpand (GTK_WIDGET (marquee->vbox), TRUE);
  gtk_widget_set_vexpand (GTK_WIDGET (marquee->vbox), TRUE);
  marquee->marginTotal = 30;

  marquee->pbar = gtk_progress_bar_new ();
  gtk_box_pack_start (GTK_BOX (marquee->vbox), GTK_WIDGET (marquee->pbar),
      FALSE, FALSE, 0);
  gtk_widget_set_halign (GTK_WIDGET (marquee->pbar), GTK_ALIGN_FILL);
  gtk_widget_set_hexpand (GTK_WIDGET (marquee->pbar), TRUE);
  marqueeSetCss (GTK_WIDGET (marquee->pbar),
      "progress, trough { min-height: 25px; } progressbar > trough > progress { background-color: #ffa600; }");

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 5);
  gtk_box_pack_start (GTK_BOX (marquee->vbox), GTK_WIDGET (vbox),
      FALSE, FALSE, 0);
  gtk_widget_set_margin_start (GTK_WIDGET (marquee->vbox), 10);
  gtk_widget_set_margin_end (GTK_WIDGET (marquee->vbox), 10);
  gtk_widget_set_hexpand (GTK_WIDGET (marquee->vbox), TRUE);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (hbox),
      FALSE, FALSE, 0);
  gtk_widget_set_halign (GTK_WIDGET (hbox), GTK_ALIGN_FILL);
  gtk_widget_set_hexpand (GTK_WIDGET (hbox), TRUE);
  gtk_widget_set_margin_end (GTK_WIDGET (hbox), 0);
  gtk_widget_set_margin_start (GTK_WIDGET (hbox), 0);

  marquee->danceLab = gtk_label_new (_("Not Playing"));
  gtk_widget_set_halign (GTK_WIDGET (marquee->danceLab), GTK_ALIGN_START);
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (marquee->danceLab),
      TRUE, TRUE, 0);
  gtk_widget_set_hexpand (GTK_WIDGET (marquee->danceLab), TRUE);
  marqueeSetCss (GTK_WIDGET (marquee->danceLab),
      "label { color: #ffa600; }");

  marquee->countdownTimerLab = gtk_label_new ("0:00");
  gtk_box_pack_end (GTK_BOX (hbox), GTK_WIDGET (marquee->countdownTimerLab),
      TRUE, TRUE, 0);
  gtk_label_set_max_width_chars (GTK_LABEL (marquee->countdownTimerLab), 6);
  gtk_widget_set_halign (GTK_WIDGET (marquee->countdownTimerLab), GTK_ALIGN_END);
  marqueeSetCss (GTK_WIDGET (marquee->countdownTimerLab),
      "label { color: #ffa600; }");

  if (marquee->mqShowInfo) {
    marquee->infoLab = gtk_label_new ("");
    gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (marquee->infoLab),
        TRUE, TRUE, 0);
    gtk_widget_set_halign (GTK_WIDGET (marquee->infoLab), GTK_ALIGN_START);
  }

  marquee->sep = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
  gtk_box_pack_end (GTK_BOX (vbox), GTK_WIDGET (marquee->sep),
      TRUE, TRUE, 0);
  marqueeSetCss (GTK_WIDGET (marquee->sep),
      "separator { min-height: 4px; background-color: #ffa600; }");

  marquee->marqueeLabs = malloc (sizeof (GtkWidget *) * marquee->mqLen);

  for (int i = 0; i < marquee->mqLen; ++i) {
    marquee->marqueeLabs [i] = gtk_label_new ("");
    gtk_widget_set_halign (GTK_WIDGET (marquee->marqueeLabs [i]), GTK_ALIGN_START);
    gtk_widget_set_hexpand (GTK_WIDGET (marquee->marqueeLabs [i]), TRUE);
    gtk_box_pack_start (GTK_BOX (marquee->vbox),
        GTK_WIDGET (marquee->marqueeLabs [i]), FALSE, FALSE, 0);
    gtk_widget_set_margin_start (GTK_WIDGET (marquee->marqueeLabs [i]), 10);
    gtk_widget_set_margin_end (GTK_WIDGET (marquee->marqueeLabs [i]), 10);
  }

  marquee->inResize = true;
  gtk_widget_show_all (GTK_WIDGET (window));
  marquee->inResize = false;

  marqueeAdjustFontSizes (marquee, 0);

  logMsg (LOG_SESS, LOG_IMPORTANT, "running: time-to-start: %ld ms", mstimeend (&marquee->tm));
}

gboolean
marqueeMainLoop (void *tmarquee)
{
  marquee_t   *marquee = tmarquee;
  int         tdone = 0;
  gboolean    cont = TRUE;

  tdone = sockhProcessMain (marquee->sockserver, marqueeProcessMsg, marquee);
  if (tdone) {
    cont = FALSE;
  }

  if (marquee->programState == STATE_INITIALIZING) {
    marquee->programState = STATE_CONNECTING;
  }

  if (marquee->programState == STATE_CONNECTING &&
      marquee->mainSock == INVALID_SOCKET) {
    int       err;
    uint16_t  mainPort;

    mainPort = bdjvarsl [BDJVL_MAIN_PORT];
    marquee->mainSock = sockConnect (mainPort, &err, 1000);
    if (marquee->mainSock != INVALID_SOCKET) {
      sockhSendMessage (marquee->mainSock, ROUTE_MOBILEMQ, ROUTE_MAIN,
          MSG_HANDSHAKE, NULL);
      marquee->programState = STATE_WAIT_HANDSHAKE;
    }
  }

  if (marquee->programState == STATE_WAIT_HANDSHAKE) {
    if (marquee->mainHandshake) {
      marquee->programState = STATE_RUNNING;
      logMsg (LOG_SESS, LOG_IMPORTANT, "running: time-to-start: %ld ms", mstimeend (&marquee->tm));
    }
  }

  if (marquee->programState != STATE_RUNNING) {
      /* all of the processing that follows requires a running state */
    if (gKillReceived) {
      logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
      cont = FALSE;
    }
    return cont;
  }

  if (marquee->setFontSize != 0) {
    marquee->inResize = true;
    marqueeSetFontSize (marquee, marquee->danceLab, "bold", marquee->setFontSize);
    marqueeSetFontSize (marquee, marquee->countdownTimerLab, "bold", marquee->setFontSize);
    if (marquee->infoLab != NULL) {
      marqueeSetFontSize (marquee, marquee->infoLab, "", marquee->setFontSize - 10);
    }

    for (int i = 0; i < marquee->mqLen; ++i) {
      marqueeSetFontSize (marquee, marquee->marqueeLabs [i], "", marquee->setFontSize);
    }

    marquee->setFontSize = 0;
    if (marquee->unMaximize) {
      marquee->unmaxSignal = g_signal_connect (marquee->marqueeLabs [marquee->mqLen - 1],
          "size-allocate", G_CALLBACK (marqueeUnmaxCallback), marquee);
    }
    marquee->inResize = false;
  }

  if (gKillReceived) {
    logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
    cont = FALSE;
  }
  return cont;
}

static int
marqueeProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
    bdjmsgmsg_t msg, char *args, void *udata)
{
  marquee_t       *marquee = udata;

  logProcBegin (LOG_PROC, "marqueeProcessMsg");

  logMsg (LOG_DBG, LOG_MSGS, "got: from %d route: %d msg:%d args:%s",
      routefrom, route, msg, args);

  switch (route) {
    case ROUTE_NONE:
    case ROUTE_MARQUEE: {
      switch (msg) {
        case MSG_HANDSHAKE: {
          marquee->mainHandshake = true;
          break;
        }
        case MSG_EXIT_REQUEST: {
          mstimestart (&marquee->tm);
          logMsg (LOG_SESS, LOG_IMPORTANT, "got exit request");
          gKillReceived = 0;
          logMsg (LOG_DBG, LOG_MSGS, "got: req-exit");
          logProcEnd (LOG_PROC, "marqueeProcessMsg", "req-exit");
          return 1;
        }
        case MSG_MARQUEE_SHOW: {
          if (marquee->isHidden) {
            gtk_widget_show (GTK_WIDGET (marquee->window));
            marquee->isHidden = false;
          }
          break;
        }
        case MSG_MARQUEE_DATA: {
          marqueePopulate (marquee, args);
          break;
        }
        case MSG_MARQUEE_TIMER: {
          marqueeSetTimer (marquee, args);
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

  logProcEnd (LOG_PROC, "marqueeProcessMsg", "");

  if (gKillReceived) {
    logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
  }
  return gKillReceived;
}


static gboolean
marqueeCloseWin (GtkWidget *window, GdkEvent *event, gpointer userdata)
{
  marquee_t   *marquee = userdata;

  gtk_widget_hide (GTK_WIDGET (window));
  marquee->isHidden = true;
  return TRUE;
}

static gboolean
marqueeToggleFullscreen (GtkWidget *window, GdkEventButton *event, gpointer userdata)
{
  marquee_t   *marquee = userdata;

  if (gdk_event_get_event_type ((GdkEvent *) event) != GDK_DOUBLE_BUTTON_PRESS) {
    return FALSE;
  }

  marquee->doubleClickMax = true;
  if (marquee->isMaximized) {
    marquee->inMax = true;
    marquee->isMaximized = false;
    marqueeAdjustFontSizes (marquee, marquee->priorSize);
    marquee->setPrior = true;
    gtk_window_unmaximize (GTK_WINDOW (window));
    marquee->unMaximize = true;
    marquee->inMax = false;
    gtk_window_set_decorated (GTK_WINDOW (window), TRUE);
  } else {
    marquee->inMax = true;
    marquee->isMaximized = true;
    gtk_window_set_decorated (GTK_WINDOW (window), FALSE);
    marquee->inMax = false;
    gtk_window_maximize (GTK_WINDOW (window));
  }

  return FALSE;
}

/* resize gets called multiple times; it's difficult to get the font      */
/* size reset back to the normal.  there are all sorts of little glitches */
/* and race conditions that are possible.  this code will very fragile    */
static gboolean
marqueeResized (GtkWidget *window, GdkEventConfigure *event, gpointer userdata)
{
  marquee_t   *marquee = userdata;
  int         sz = 0;
  gint        eheight;

  if (marquee->inMax) {
    return FALSE;
  }
  if (marquee->inResize) {
    return FALSE;
  }

  eheight = event->height;
  if (eheight == marquee->lastHeight) {
    return FALSE;
  }

  if (marquee->setPrior) {
    sz = marquee->priorSize;
    marquee->setPrior = false;
  }
  marqueeAdjustFontSizes (marquee, sz);
  marquee->lastHeight = eheight;

  return FALSE;
}

static gboolean
marqueeWinState (GtkWidget *window, GdkEventWindowState *event, gpointer userdata)
{
  marquee_t         *marquee = userdata;


  if (event->changed_mask == GDK_WINDOW_STATE_MAXIMIZED) {
    /* if the user double-clicked, this is a know maximize change and */
    /* no processing needs to be done here */
    if (marquee->doubleClickMax) {
      marquee->doubleClickMax = false;
      return FALSE;
    }

    /* user selected the maximize button */
    if ((event->new_window_state & GDK_WINDOW_STATE_MAXIMIZED) ==
        GDK_WINDOW_STATE_MAXIMIZED) {
      marquee->isMaximized = true;
      marquee->inMax = true;
      gtk_window_set_decorated (GTK_WINDOW (window), FALSE);
      marquee->inMax = false;
      /* I don't know why this is necessary; did the event propogation stop? */
      gtk_window_maximize (GTK_WINDOW (window));
    }
  }

  return FALSE;
}

static void
marqueeStateChg (GtkWidget *window, GtkStateType flags, gpointer userdata)
{
  /* the marquee is never in a backdrop state */
  gtk_widget_unset_state_flags (GTK_WIDGET (window), GTK_STATE_FLAG_BACKDROP);
}

static void
marqueeSigHandler (int sig)
{
  gKillReceived = 1;
}

static void
marqueeSetFontSize (marquee_t *marquee, GtkWidget *lab, char *style, int sz)
{
  PangoFontDescription  *font_desc;
  PangoAttribute        *attr;
  PangoAttrList         *attrlist;
  char                  tbuff [200];


  attrlist = pango_attr_list_new ();
  snprintf (tbuff, sizeof (tbuff), "%s %s", marquee->mqfont, style);
  font_desc = pango_font_description_from_string (tbuff);
  pango_font_description_set_absolute_size (font_desc, sz * PANGO_SCALE);
  attr = pango_attr_font_desc_new (font_desc);
  pango_attr_list_insert (attrlist, attr);
  gtk_label_set_attributes (GTK_LABEL (lab), attrlist);
}

static void
marqueeAdjustFontSizes (marquee_t *marquee, int sz)
{
  int             newsz;
  int             spacing;
  gint            width;
  gint            height;
  int             pbarHeight;
  int             sepHeight;
  GtkAllocation   allocSize;

  if (marquee->inResize) {
    return;
  }

  marquee->inResize = true;

  gtk_window_get_size (GTK_WINDOW (marquee->window), &width, &height);

  gtk_widget_get_allocation (GTK_WIDGET (marquee->pbar), &allocSize);
  pbarHeight = allocSize.height;
  gtk_widget_get_allocation (GTK_WIDGET (marquee->sep), &allocSize);
  sepHeight = allocSize.height;

  marquee->newFontSize = sz;
  if (sz == 0) {
    spacing = gtk_box_get_spacing (GTK_BOX (marquee->vbox));
    /* 40 = space needed so that the window can be shrunk */
    newsz = (height - 40 - marquee->marginTotal - pbarHeight - sepHeight -
        (spacing * marquee->mqLen)) / (marquee->mqLen + 2);
    spacing = newsz * 0.15;
    newsz = (height - 40 - marquee->marginTotal - pbarHeight - sepHeight -
        (spacing * marquee->mqLen)) / (marquee->mqLen + 2);
  } else {
    /* the old size to restore from before being maximized */
    newsz = sz;
    spacing = newsz * 0.15;
  }

  marqueeSetFontSize (marquee, marquee->danceLab, "bold", newsz);

  marquee->sizeSignal = g_signal_connect (marquee->danceLab, "size-allocate",
      G_CALLBACK (marqueeAdjustFontCallback), marquee);
  marquee->inResize = false;
}

static void
marqueeAdjustFontCallback (GtkWidget *w, GtkAllocation *retAllocSize, gpointer userdata)
{
  int             newsz;
  int             spacing;
  gint            width;
  gint            height;
  int             pbarHeight;
  int             sepHeight;
  GtkAllocation   allocSize;
  marquee_t       *marquee = userdata;
  double          dnewsz;
  double          dheight;

  if (marquee->inResize) {
    return;
  }
  if (marquee->sizeSignal == 0) {
    /* this trap is necessary */
    return;
  }

  marquee->inResize = true;
  g_signal_handler_disconnect (GTK_WIDGET (w), marquee->sizeSignal);
  marquee->sizeSignal = 0;

  gtk_window_get_size (GTK_WINDOW (marquee->window), &width, &height);

  gtk_widget_get_allocation (GTK_WIDGET (marquee->pbar), &allocSize);
  pbarHeight = allocSize.height;
  gtk_widget_get_allocation (GTK_WIDGET (marquee->sep), &allocSize);
  sepHeight = allocSize.height;

  spacing = gtk_box_get_spacing (GTK_BOX (marquee->vbox));

  if (marquee->newFontSize == 0) {
    /* 40 = space needed so that the window can be shrunk */
    newsz = (height - 40 - marquee->marginTotal - pbarHeight - sepHeight -
        (spacing * marquee->mqLen)) / (marquee->mqLen + 2);
    spacing = newsz * 0.15;
    newsz = (height - 40 - marquee->marginTotal - pbarHeight - sepHeight -
        (spacing * marquee->mqLen)) / (marquee->mqLen + 2);
  } else {
    newsz = marquee->newFontSize;
    spacing = newsz * 0.15;
  }

  gtk_box_set_spacing (GTK_BOX (marquee->vbox), spacing);


  /* newsz is the maximum height available */
  /* given the allocation size, determine the actual size available */
  dnewsz = (double) newsz;
  dheight = (double) retAllocSize->height;
  dnewsz = round (dnewsz * (dnewsz / dheight));
  newsz = (int) dnewsz;
  marquee->setFontSize = newsz;

  /* only save the prior size once */
  if (! marquee->isMaximized && marquee->priorSize == 0) {
    marquee->priorSize = newsz;
  }

  marquee->inResize = false;
}

static void
marqueeSetCss (GtkWidget *w, char *style)
{
  GtkCssProvider        *tcss;

  tcss = gtk_css_provider_new ();
  gtk_css_provider_load_from_data (tcss, style, -1, NULL);
  gtk_style_context_add_provider (
      gtk_widget_get_style_context (GTK_WIDGET (w)),
      GTK_STYLE_PROVIDER (tcss), G_MAXUINT);
}

static void
marqueePopulate (marquee_t *marquee, char *args)
{
  char      *p;
  char      *tokptr;

  p = strtok_r (args, MSG_ARGS_RS_STR, &tokptr);
  if (marquee->infoLab != NULL) {
    gtk_label_set_label (GTK_LABEL (marquee->infoLab), p);
  }
  /* first entry is the main dance */
  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokptr);
  gtk_label_set_label (GTK_LABEL (marquee->danceLab), p);

  for (int i = 0; i < marquee->mqLen; ++i) {
    p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokptr);
    if (p != NULL) {
      gtk_label_set_label (GTK_LABEL (marquee->marqueeLabs [i]), p);
    } else {
      gtk_label_set_label (GTK_LABEL (marquee->marqueeLabs [i]), "");
    }
  }
}

static void
marqueeSetTimer (marquee_t *marquee, char *args)
{
  ssize_t     played;
  ssize_t     dur;
  double      dplayed;
  double      ddur;
  double      dratio;
  ssize_t     timeleft;
  char        *p = NULL;
  char        *tokptr = NULL;
  char        tbuff [40];


  p = strtok_r (args, MSG_ARGS_RS_STR, &tokptr);
  played = atol (p);
  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokptr);
  dur = atol (p);

  timeleft = dur - played;
  tmutilToMS (timeleft, tbuff, sizeof (tbuff));
  gtk_label_set_label (GTK_LABEL (marquee->countdownTimerLab), tbuff);

  ddur = (double) dur;
  dplayed = (double) played;
  if (dur != 0.0) {
    dratio = dplayed / ddur;
  } else {
    dratio = 0.0;
  }
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (marquee->pbar), dratio);
}

static void
marqueeUnmaxCallback (GtkWidget *w, GtkAllocation *retAllocSize, gpointer userdata)
{
  marquee_t   *marquee = userdata;

  if (! marquee->unMaximize) {
    return;
  }
  if (marquee->unmaxSignal == 0) {
    return;
  }

  marquee->unMaximize = false;
  g_signal_handler_disconnect (GTK_WIDGET (w), marquee->unmaxSignal);
  marquee->unmaxSignal = 0;
  gtk_window_unmaximize (GTK_WINDOW (marquee->window));
}


