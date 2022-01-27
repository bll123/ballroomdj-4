#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <getopt.h>
#include <unistd.h>

#include <gtk/gtk.h>

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
  sockserver_t  *sockserver;
  mstime_t      tm;
  GtkWidget     *vbox;
  GtkWidget     *window;
  GtkWidget     *danceLab;
  GtkWidget     *countdownTimerLab;
  GtkWidget     **marqueeLabs;
  int           mqLen;
  int           lastHeight;
  int           priorSize;
  bool          isMaximized : 1;
  bool          isHidden : 1;
  bool          inResize : 1;
  bool          doubleClickMax : 1;
  bool          inMax : 1;
  bool          setPrior : 1;
} marquee_t;

static int  marqueeCreateGui (marquee_t *marquee, int argc, char *argv []);
static void marqueeActivate (GApplication *app, gpointer data);
gboolean    marqueeMainLoop (void *tmarquee);
static int  marqueeProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
    bdjmsgmsg_t msg, char *args, void *udata);
static gboolean marqueeCloseWin (GtkWidget *window, GdkEvent *event, gpointer data);
static gboolean marqueeToggleFullscreen (GtkWidget *window,
    GdkEventButton *event, gpointer data);
static gboolean marqueeResized (GtkWidget *window, GdkEventConfigure *event,
    gpointer data);
static gboolean marqueeWinState (GtkWidget *window, GdkEventWindowState *event,
    gpointer data);
static void marqueeStateChg (GtkWidget *w, GtkStateType flags, gpointer data);
static void marqueeSigHandler (int sig);
static void marqueeSetFontSize (GtkWidget *lab, char *style, int sz);
static void marqueeAdjustFontSizes (marquee_t *marquee, int sz);

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
  marquee.sockserver = NULL;
  marquee.window = NULL;
  marquee.danceLab = NULL;
  marquee.countdownTimerLab = NULL;
  marquee.marqueeLabs = NULL;
  marquee.lastHeight = 0;
  marquee.priorSize = 0;
  marquee.isMaximized = false;
  marquee.isHidden = false;
  marquee.inResize = false;
  marquee.doubleClickMax = false;
  marquee.inMax = false;
  marquee.setPrior = false;

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
  marquee.mqLen = bdjoptGetNum (OPT_G_PLAYERQLEN);

  marquee.sockserver = sockhStartServer (listenPort);

  status = marqueeCreateGui (&marquee, 0, NULL);

  sockhCloseServer (marquee.sockserver);
  bdjoptFree ();
  bdjvarsCleanup ();

  lockRelease (MARQUEE_LOCK_FN, PATHBLD_MP_USEIDX);
  logMsg (LOG_SESS, LOG_IMPORTANT, "time-to-end: %ld ms", mstimeend (&marquee.tm));
  logEnd ();
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

  g_timeout_add (5, marqueeMainLoop, marquee);
  status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);
  return status;
}

static void
marqueeActivate (GApplication *app, gpointer data)
{
  marquee_t             *marquee = data;
  GtkWidget             *window;
  GtkWidget             *hbox;
  char                  tbuff [40];
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
  gtk_widget_set_margin_end (marquee->vbox, 10);
  gtk_widget_set_margin_start (marquee->vbox, 10);
  gtk_container_add (GTK_CONTAINER (window), marquee->vbox);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_container_add (GTK_CONTAINER (marquee->vbox), hbox);

  marquee->danceLab = gtk_label_new ("Dance");
  gtk_widget_set_hexpand (marquee->danceLab, TRUE);
  gtk_widget_set_halign (marquee->danceLab, GTK_ALIGN_START);
  gtk_container_add (GTK_CONTAINER (hbox), marquee->danceLab);

  marquee->countdownTimerLab = gtk_label_new ("2:30");
  gtk_widget_set_halign (marquee->countdownTimerLab, GTK_ALIGN_END);
  gtk_label_set_max_width_chars (GTK_LABEL (marquee->countdownTimerLab), 6);
  gtk_container_add (GTK_CONTAINER (hbox), marquee->countdownTimerLab);

  marquee->marqueeLabs = malloc (sizeof (GtkWidget *) * marquee->mqLen);

  for (int i = 1; i <= marquee->mqLen; ++i) {
    snprintf (tbuff, sizeof (tbuff), "lab %d", i);
    marquee->marqueeLabs [i] = gtk_label_new (tbuff);
    gtk_widget_set_halign (marquee->marqueeLabs [i], GTK_ALIGN_START);
    gtk_container_add (GTK_CONTAINER (marquee->vbox), marquee->marqueeLabs [i]);
  }

  marqueeAdjustFontSizes (marquee, 0);

  marquee->inResize = true;
  gtk_widget_show_all (GTK_WIDGET (window));
  marquee->inResize = false;

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
  return 0;
}


static gboolean
marqueeCloseWin (GtkWidget *window, GdkEvent *event, gpointer data)
{
  marquee_t   *marquee = data;

  gtk_widget_hide (GTK_WIDGET (window));
  marquee->isHidden = true;
  return TRUE;
}

static gboolean
marqueeToggleFullscreen (GtkWidget *window, GdkEventButton *event, gpointer data)
{
  marquee_t   *marquee = data;

  if (event->type != GDK_DOUBLE_BUTTON_PRESS) {
    return TRUE;
  }

  marquee->doubleClickMax = true;
  if (marquee->isMaximized) {
    marquee->inMax = true;
    marquee->isMaximized = false;
    marqueeAdjustFontSizes (marquee, marquee->priorSize);
    marquee->setPrior = true;
    gtk_window_unmaximize (GTK_WINDOW (window));
    marquee->inMax = false;
    gtk_window_set_decorated (GTK_WINDOW (window), TRUE);
  } else {
    marquee->inMax = true;
    marquee->isMaximized = true;
    gtk_window_set_decorated (GTK_WINDOW (window), FALSE);
    marquee->inMax = false;
    gtk_window_maximize (GTK_WINDOW (window));
  }

  return TRUE;
}

/* resize gets called multiple times; it's difficult to get the font  */
/* size reset back to the normal.  not working quite right yet.       */
static gboolean
marqueeResized (GtkWidget *window, GdkEventConfigure *event, gpointer data)
{
  marquee_t   *marquee = data;
  int         sz = 0;

  if (marquee->inMax) {
    return FALSE;
  }
  if (marquee->inResize) {
    return FALSE;
  }

  if (event->height == marquee->lastHeight) {
    return FALSE;
  }

  if (! marquee->isMaximized) {
    /* this has to be run again after the font sizes have been adjusted */
    gtk_window_unmaximize (GTK_WINDOW (window));
  }

  if (marquee->setPrior) {
    sz = marquee->priorSize;
    marquee->setPrior = false;
  }
  marqueeAdjustFontSizes (marquee, sz);
  marquee->lastHeight = event->height;

  return FALSE;
}

static gboolean
marqueeWinState (GtkWidget *window, GdkEventWindowState *event, gpointer data)
{
  marquee_t   *marquee = data;


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
      marqueeAdjustFontSizes (marquee, 0);
    }
  }

  return FALSE;
}

static void
marqueeStateChg (GtkWidget *window, GtkStateType flags, gpointer data)
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
marqueeSetFontSize (GtkWidget *lab, char *style, int sz)
{
  PangoFontDescription  *font_desc;
  PangoAttribute        *attr;
  PangoAttrList         *attrlist;
  char                  tbuff [50];


  attrlist = pango_attr_list_new ();
  snprintf (tbuff, sizeof (tbuff), "%s condensed", style);
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

  if (marquee->inResize) {
    return;
  }

  gtk_window_get_size (GTK_WINDOW (marquee->window), &width, &height);

  marquee->inResize = true;
  gtk_widget_set_vexpand (GTK_WIDGET (marquee->window), FALSE);
  gtk_widget_set_vexpand (GTK_WIDGET (marquee->vbox), FALSE);

  if (sz == 0) {
    spacing = gtk_box_get_spacing (GTK_BOX (marquee->vbox));
    newsz = (height - 20 - (spacing * marquee->mqLen)) / (marquee->mqLen + 1);
    newsz -= (newsz / 16) * (marquee->mqLen + 1);
    spacing = newsz / 10;
    newsz = (height - 20 - (spacing * marquee->mqLen)) / (marquee->mqLen + 1);
    newsz -= (newsz / 16) * (marquee->mqLen + 1);
  } else {
    newsz = sz;
    spacing = newsz / 10;
  }
  gtk_box_set_spacing (GTK_BOX (marquee->vbox), spacing);

  marqueeSetFontSize (marquee->danceLab, "bold", newsz);
  marqueeSetFontSize (marquee->countdownTimerLab, "bold", newsz);

  for (int i = 1; i <= marquee->mqLen; ++i) {
    marqueeSetFontSize (marquee->marqueeLabs [i], "", newsz);
  }
  gtk_widget_set_vexpand (GTK_WIDGET (marquee->vbox), TRUE);
  gtk_widget_set_vexpand (GTK_WIDGET (marquee->window), TRUE);
  /* only save the prior size once */
  if (! marquee->isMaximized && marquee->priorSize == 0) {
    marquee->priorSize = newsz;
  }
  marquee->inResize = false;
}
