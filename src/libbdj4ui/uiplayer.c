#include "config.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>
#include <math.h>

#include "bdj4.h"
#include "bdj4intl.h"
#include "bdjvarsdf.h"
#include "conn.h"
#include "dance.h"
#include "ilist.h"
#include "log.h"
#include "musicdb.h"
#include "pathbld.h"
#include "progstate.h"
#include "song.h"
#include "tagdef.h"
#include "uiplayer.h"
#include "ui.h"

/* there are all sorts of latency issues making the sliders work nicely */
/* it will take at least 100ms and at most 200ms for the message to get */
/* back.  Then there are the latency issues on this end. */
#define UIPLAYER_LOCK_TIME_WAIT   300
#define UIPLAYER_LOCK_TIME_SEND   30

static bool  uiplayerInitCallback (void *udata, programstate_t programState);
static bool  uiplayerClosingCallback (void *udata, programstate_t programState);

static void     uiplayerProcessPauseatend (uiplayer_t *uiplayer, int on);
static void     uiplayerProcessPlayerState (uiplayer_t *uiplayer, int playerState);
static void     uiplayerProcessPlayerStatusData (uiplayer_t *uiplayer, char *args);
static void     uiplayerProcessMusicqStatusData (uiplayer_t *uiplayer, char *args);
static bool     uiplayerFadeProcess (void *udata);
static bool     uiplayerPlayPauseProcess (void *udata);
static bool     uiplayerRepeatProcess (GtkButton *b, void *udata);
static bool     uiplayerSongBeginProcess (void *udata);
static bool     uiplayerNextSongProcess (void *udata);
static void     uiplayerPauseatendProcess (GtkButton *b, void *udata);
static gboolean uiplayerSpeedProcess (GtkRange *range, GtkScrollType *scroll, gdouble value, gpointer udata);
static gboolean uiplayerSeekProcess (GtkRange *range, GtkScrollType *scroll, gdouble value, gpointer udata);
static gboolean uiplayerVolumeProcess (GtkRange *range, GtkScrollType *scroll, gdouble value, gpointer udata);
static void     uiplayerClearDisplay (uiplayer_t *uiplayer);

uiplayer_t *
uiplayerInit (progstate_t *progstate, conn_t *conn, musicdb_t *musicdb)
{
  uiplayer_t    *uiplayer;

  logProcBegin (LOG_PROC, "uiplayerInit");
  uiplayer = malloc (sizeof (uiplayer_t));
  assert (uiplayer != NULL);
  uiplayer->progstate = progstate;
  uiplayer->conn = conn;
  uiplayer->musicdb = musicdb;

  uiplayer->vbox = NULL;
  uiutilsUIWidgetInit (&uiplayer->statusImg);
  uiutilsUIWidgetInit (&uiplayer->repeatImg);
  uiplayer->danceLab = NULL;
  uiplayer->artistLab = NULL;
  uiplayer->titleLab = NULL;
  uiplayer->speedScale = NULL;
  uiplayer->speedDisplayLab = NULL;
  uiplayer->countdownTimerLab = NULL;
  uiplayer->durationLab = NULL;
  uiplayer->seekScale = NULL;
  uiplayer->seekDisplayLab = NULL;
  uiplayer->repeatButton = NULL;
  uiplayer->pauseatendButton = NULL;
  uiutilsUIWidgetInit (&uiplayer->playPixbuf);
  uiutilsUIWidgetInit (&uiplayer->stopPixbuf);
  uiutilsUIWidgetInit (&uiplayer->pausePixbuf);
  uiutilsUIWidgetInit (&uiplayer->repeatPixbuf);
  uiutilsUIWidgetInit (&uiplayer->ledoffImg);
  uiutilsUIWidgetInit (&uiplayer->ledonImg);
  uiplayer->volumeScale = NULL;
  uiplayer->volumeDisplayLab = NULL;

  progstateSetCallback (uiplayer->progstate, STATE_CONNECTING, uiplayerInitCallback, uiplayer);
  progstateSetCallback (uiplayer->progstate, STATE_CLOSING, uiplayerClosingCallback, uiplayer);

  logProcEnd (LOG_PROC, "uiplayerInit", "");
  return uiplayer;
}

void
uiplayerSetDatabase (uiplayer_t *uiplayer, musicdb_t *musicdb)
{
  uiplayer->musicdb = musicdb;
}

void
uiplayerFree (uiplayer_t *uiplayer)
{
  logProcBegin (LOG_PROC, "uiplayerFree");
  if (uiplayer != NULL) {
    free (uiplayer);
  }
  logProcEnd (LOG_PROC, "uiplayerFree", "");
}

GtkWidget *
uiplayerBuildUI (uiplayer_t *uiplayer)
{
  char            tbuff [MAXPATHLEN];
  UIWidget        uiwidget;
  GtkWidget       *hbox;
  GtkWidget       *tbox;
  GtkWidget       *widget;
  UIWidget        sgA;
  UIWidget        sgB;
  UIWidget        sgC;
  UIWidget        sgD;
  UIWidget        sgE;

  logProcBegin (LOG_PROC, "uiplayerBuildUI");

  uiCreateSizeGroupHoriz (&sgA);
  uiCreateSizeGroupHoriz (&sgB);
  uiCreateSizeGroupHoriz (&sgC);
  uiCreateSizeGroupHoriz (&sgD);
  uiCreateSizeGroupHoriz (&sgE);

  uiplayer->vbox = uiCreateVertBoxWW ();
  assert (uiplayer->vbox != NULL);
  uiWidgetExpandHorizW (uiplayer->vbox);

  /* song display */

  hbox = uiCreateHorizBoxWW ();
  assert (hbox != NULL);
  uiWidgetExpandHorizW (hbox);
  uiBoxPackStartWW (uiplayer->vbox, hbox);

  /* size group E */
  tbox = uiCreateHorizBoxWW ();
  assert (tbox != NULL);
  uiBoxPackStartWW (hbox, tbox);
  uiSizeGroupAddW (&sgE, tbox);

  uiImageNew (&uiplayer->statusImg);

  pathbldMakePath (tbuff, sizeof (tbuff), "button_stop", ".svg",
      PATHBLD_MP_IMGDIR);
  uiImageFromFile (&uiplayer->stopPixbuf, tbuff);
  uiImageGetPixbuf (&uiplayer->stopPixbuf);
  uiWidgetMakePersistent (&uiplayer->stopPixbuf);

  uiImageSetFromPixbuf (&uiplayer->statusImg, &uiplayer->stopPixbuf);
  uiWidgetSetSizeRequest (&uiplayer->statusImg, 18, -1);
  uiWidgetSetMarginStartW (uiplayer->statusImg.widget, uiBaseMarginSz);
  uiBoxPackStartWW (tbox, uiplayer->statusImg.widget);

  pathbldMakePath (tbuff, sizeof (tbuff), "button_play", ".svg",
      PATHBLD_MP_IMGDIR);
  uiImageFromFile (&uiplayer->playPixbuf, tbuff);
  uiImageGetPixbuf (&uiplayer->playPixbuf);
  uiWidgetMakePersistent (&uiplayer->playPixbuf);

  pathbldMakePath (tbuff, sizeof (tbuff), "button_pause", ".svg",
      PATHBLD_MP_IMGDIR);
  uiImageFromFile (&uiplayer->pausePixbuf, tbuff);
  uiImageGetPixbuf (&uiplayer->pausePixbuf);
  uiWidgetMakePersistent (&uiplayer->pausePixbuf);

  pathbldMakePath (tbuff, sizeof (tbuff), "button_repeat", ".svg",
      PATHBLD_MP_IMGDIR);
  uiImageFromFile (&uiplayer->repeatPixbuf, tbuff);
  uiImageGetPixbuf (&uiplayer->repeatPixbuf);
  uiWidgetMakePersistent (&uiplayer->repeatPixbuf);

  uiImageNew (&uiplayer->repeatImg);
  uiImageClear (&uiplayer->repeatImg);
  uiWidgetSetSizeRequest (&uiplayer->repeatImg, 18, -1);
  uiWidgetSetMarginStartW (uiplayer->repeatImg.widget, uiBaseMarginSz);
  uiBoxPackStartWW (tbox, uiplayer->repeatImg.widget);

  uiplayer->danceLab = uiCreateLabelW ("");
  uiBoxPackStartWW (hbox, uiplayer->danceLab);

  widget = uiCreateLabelW (" : ");
  uiWidgetSetMarginStartW (widget, 0);
  uiBoxPackStartWW (hbox, widget);

  widget = uiCreateLabelW ("");
  uiWidgetSetMarginStartW (widget, 0);
  uiLabelEllipsizeOnW (widget);
  uiBoxPackStartWW (hbox, widget);
  uiplayer->artistLab = widget;

  widget = uiCreateLabelW (" : ");
  uiWidgetSetMarginStartW (widget, 0);
  uiBoxPackStartWW (hbox, widget);

  widget = uiCreateLabelW ("");
  uiWidgetSetMarginStartW (widget, 0);
  uiLabelEllipsizeOnW (widget);
  uiBoxPackStartWW (hbox, widget);
  uiplayer->titleLab = widget;

  widget = uiCreateLabelW ("");
  uiWidgetExpandHorizW (widget);
  uiBoxPackStartWW (hbox, widget);

  /* size group A */
  widget = uiCreateLabelW ("%");
  uiBoxPackEndWW (hbox, widget);
  uiSizeGroupAddW (&sgA, widget);

  /* size group B */
  uiplayer->speedDisplayLab = uiCreateLabelW ("100");
  assert (uiplayer->speedDisplayLab != NULL);
  gtk_widget_set_size_request (uiplayer->speedDisplayLab, 24, -1);
  gtk_widget_set_halign (uiplayer->speedDisplayLab, GTK_ALIGN_END);
  gtk_label_set_xalign (GTK_LABEL (uiplayer->speedDisplayLab), 1.0);
  uiBoxPackEndWW (hbox, uiplayer->speedDisplayLab);
  uiSizeGroupAddW (&sgB, uiplayer->speedDisplayLab);

  /* size group C */
  uiplayer->speedScale = uiCreateScale (70.0, 130.0, 0.1, 1.0, 100.0);
  uiBoxPackEndWW (hbox, uiplayer->speedScale);
  uiSizeGroupAddW (&sgC, uiplayer->speedScale);
  g_signal_connect (uiplayer->speedScale, "change-value", G_CALLBACK (uiplayerSpeedProcess), uiplayer);

  /* size group D */
  /* CONTEXT: the current speed for song playback */
  widget = uiCreateColonLabelW (_("Speed"));
  gtk_widget_set_halign (widget, GTK_ALIGN_END);
  gtk_label_set_xalign (GTK_LABEL (widget), 1.0);
  uiBoxPackEndWW (hbox, widget);
  uiSizeGroupAddW (&sgD, widget);

  /* position controls / display */

  hbox = uiCreateHorizBoxWW ();
  assert (hbox != NULL);
  uiWidgetExpandHorizW (hbox);
  uiBoxPackStartWW (uiplayer->vbox, hbox);

  /* size group E */
  widget = uiCreateLabelW ("");
  uiBoxPackStartWW (hbox, widget);
  uiSizeGroupAddW (&sgE, widget);

  uiplayer->countdownTimerLab = uiCreateLabelW (" 0:00");
  gtk_label_set_xalign (GTK_LABEL (uiplayer->countdownTimerLab), 1.0);
  gtk_widget_set_size_request (uiplayer->countdownTimerLab, 40, -1);
  uiBoxPackStartWW (hbox, uiplayer->countdownTimerLab);

  widget = uiCreateLabelW (" / ");
  uiWidgetSetMarginStartW (widget, 0);
  uiBoxPackStartWW (hbox, widget);

  uiplayer->durationLab = uiCreateLabelW (" 3:00");
  gtk_label_set_xalign (GTK_LABEL (uiplayer->durationLab), 1.0);
  gtk_widget_set_size_request (uiplayer->countdownTimerLab, 40, -1);
  uiBoxPackStartWW (hbox, uiplayer->durationLab);

  widget = uiCreateLabelW ("");
  uiWidgetExpandHorizW (widget);
  uiBoxPackStartWW (hbox, widget);

  /* size group A */
  widget = uiCreateLabelW ("");
  uiBoxPackEndWW (hbox, widget);
  uiSizeGroupAddW (&sgA, widget);

  /* size group B */
  uiplayer->seekDisplayLab = uiCreateLabelW ("3:00");
  assert (uiplayer->seekDisplayLab != NULL);
  gtk_widget_set_size_request (uiplayer->seekDisplayLab, 40, -1);
  gtk_widget_set_halign (uiplayer->seekDisplayLab, GTK_ALIGN_END);
  uiSizeGroupAddW (&sgB, uiplayer->seekDisplayLab);
  uiBoxPackEndWW (hbox, uiplayer->seekDisplayLab);

  /* size group C */
  uiplayer->seekScale = uiCreateScale (0.0, 180000.0, 100.0, 1000.0, 0.0);
  uiBoxPackEndWW (hbox, uiplayer->seekScale);
  uiSizeGroupAddW (&sgC, uiplayer->seekScale);
  g_signal_connect (uiplayer->seekScale, "change-value", G_CALLBACK (uiplayerSeekProcess), uiplayer);

  /* size group D */
  /* CONTEXT: the current position of the song during song playback */
  widget = uiCreateColonLabelW (_("Position"));
  uiBoxPackEndWW (hbox, widget);
  gtk_widget_set_halign (widget, GTK_ALIGN_END);
  gtk_label_set_xalign (GTK_LABEL (widget), 1.0);
  uiSizeGroupAddW (&sgD, widget);

  /* main controls */

  hbox = uiCreateHorizBoxWW ();
  assert (hbox != NULL);
  uiWidgetExpandHorizW (hbox);
  uiBoxPackStartWW (uiplayer->vbox, hbox);

  /* size group E */
  widget = uiCreateLabelW ("");
  uiBoxPackStartWW (hbox, widget);
  uiSizeGroupAddW (&sgE, widget);

  uiutilsUICallbackInit (&uiplayer->callbacks [UIPLAYER_CALLBACK_FADE],
      uiplayerFadeProcess, uiplayer);
  uiCreateButton (&uiwidget,
      &uiplayer->callbacks [UIPLAYER_CALLBACK_FADE],
      /* CONTEXT: button: fade out the song and stop playing it */
      _("Fade"), NULL, NULL, NULL);
  uiBoxPackStartWW (hbox, uiwidget.widget);

  /* CONTEXT: button: play or pause the song */
  snprintf (tbuff, sizeof (tbuff), "%s / %s", _("Play"), _("Pause"));
  uiutilsUICallbackInit (&uiplayer->callbacks [UIPLAYER_CALLBACK_PLAYPAUSE],
      uiplayerPlayPauseProcess, uiplayer);
  uiCreateButton (&uiwidget,
      &uiplayer->callbacks [UIPLAYER_CALLBACK_PLAYPAUSE],
      tbuff, "button_playpause", NULL, NULL);
  uiBoxPackStartWW (hbox, uiwidget.widget);

  pathbldMakePath (tbuff, sizeof (tbuff), "button_repeat", ".svg",
      PATHBLD_MP_IMGDIR);
  uiplayer->repeatButton = uiCreateToggleButton ("",
      /* CONTEXT: button: tooltip: toggle the repeat song on and off */
      tbuff, _("Toggle Repeat"), NULL, 0);
  assert (uiplayer->repeatButton != NULL);
  uiBoxPackStartWW (hbox, uiplayer->repeatButton);
  g_signal_connect (uiplayer->repeatButton, "toggled", G_CALLBACK (uiplayerRepeatProcess), uiplayer);

  uiutilsUICallbackInit (&uiplayer->callbacks [UIPLAYER_CALLBACK_BEGSONG],
      uiplayerSongBeginProcess, uiplayer);
  uiCreateButton (&uiwidget,
      &uiplayer->callbacks [UIPLAYER_CALLBACK_BEGSONG],
      /* CONTEXT: button: tooltip: return to the beginning of the song */
      _("Return to beginning of song"), "button_begin", NULL, NULL);
  uiBoxPackStartWW (hbox, uiwidget.widget);

  uiutilsUICallbackInit (&uiplayer->callbacks [UIPLAYER_CALLBACK_NEXTSONG],
      uiplayerNextSongProcess, uiplayer);
  uiCreateButton (&uiwidget,
      &uiplayer->callbacks [UIPLAYER_CALLBACK_NEXTSONG],
      /* CONTEXT: button: tooltip: start playing the next song (immediate) */
      _("Next Song"), "button_nextsong", NULL, NULL);
  uiBoxPackStartWW (hbox, uiwidget.widget);

  pathbldMakePath (tbuff, sizeof (tbuff), "led_on", ".svg",
      PATHBLD_MP_IMGDIR);
  uiImageFromFile (&uiplayer->ledonImg, tbuff);
  uiWidgetMakePersistent (&uiplayer->ledonImg);

  pathbldMakePath (tbuff, sizeof (tbuff), "led_off", ".svg",
      PATHBLD_MP_IMGDIR);
  uiImageFromFile (&uiplayer->ledoffImg, tbuff);
  uiWidgetMakePersistent (&uiplayer->ledoffImg);

  /* CONTEXT: button: pause at the end of the song (toggle) */
  uiplayer->pauseatendButton = uiCreateToggleButton (_("Pause at End"),
      NULL, NULL, &uiplayer->ledoffImg, 0);
  assert (uiplayer->pauseatendButton != NULL);
  uiBoxPackStartWW (hbox, uiplayer->pauseatendButton);
  g_signal_connect (uiplayer->pauseatendButton, "toggled", G_CALLBACK (uiplayerPauseatendProcess), uiplayer);

  /* volume controls / display */

  /* size group A */
  widget = uiCreateLabelW ("%");
  uiBoxPackEndWW (hbox, widget);
  uiSizeGroupAddW (&sgA, widget);

  /* size group B */
  uiplayer->volumeDisplayLab = uiCreateLabelW ("100");
  assert (uiplayer->volumeDisplayLab != NULL);
  gtk_widget_set_size_request (uiplayer->volumeDisplayLab, 24, -1);
  gtk_widget_set_halign (uiplayer->volumeDisplayLab, GTK_ALIGN_END);
  gtk_label_set_xalign (GTK_LABEL (uiplayer->volumeDisplayLab), 1.0);
  uiBoxPackEndWW (hbox, uiplayer->volumeDisplayLab);
  uiSizeGroupAddW (&sgB, uiplayer->volumeDisplayLab);

  /* size group C */
  uiplayer->volumeScale = uiCreateScale (0.0, 100.0, 0.1, 1.0, 0.0);
  uiBoxPackEndWW (hbox, uiplayer->volumeScale);
  uiSizeGroupAddW (&sgC, uiplayer->volumeScale);
  g_signal_connect (uiplayer->volumeScale, "change-value", G_CALLBACK (uiplayerVolumeProcess), uiplayer);

  /* size group D */
  /* CONTEXT: The current volume of the song */
  widget = uiCreateColonLabelW (_("Volume"));
  uiBoxPackEndWW (hbox, widget);
  gtk_widget_set_halign (widget, GTK_ALIGN_END);
  gtk_label_set_xalign (GTK_LABEL (widget), 1.0);
  uiSizeGroupAddW (&sgD, widget);

  logProcEnd (LOG_PROC, "uiplayerBuildUI", "");
  return uiplayer->vbox;
}

void
uiplayerMainLoop (uiplayer_t *uiplayer)
{
  if (mstimeCheck (&uiplayer->volumeLockTimeout)) {
    mstimeset (&uiplayer->volumeLockTimeout, 3600000);
    uiplayer->volumeLock = false;
  }

  if (mstimeCheck (&uiplayer->volumeLockSend)) {
    double        value;
    char          tbuff [40];

    value = gtk_range_get_value (GTK_RANGE (uiplayer->volumeScale));
    snprintf (tbuff, sizeof (tbuff), "%.0f", value);
    connSendMessage (uiplayer->conn, ROUTE_PLAYER, MSG_PLAYER_VOLUME, tbuff);
    if (uiplayer->volumeLock) {
      mstimeset (&uiplayer->volumeLockSend, UIPLAYER_LOCK_TIME_SEND);
    } else {
      mstimeset (&uiplayer->volumeLockSend, 3600000);
    }
  }

  if (mstimeCheck (&uiplayer->speedLockTimeout)) {
    mstimeset (&uiplayer->speedLockTimeout, 3600000);
    uiplayer->speedLock = false;
  }

  if (mstimeCheck (&uiplayer->speedLockSend)) {
    double        value;
    char          tbuff [40];

    value = gtk_range_get_value (GTK_RANGE (uiplayer->speedScale));
    snprintf (tbuff, sizeof (tbuff), "%.0f", value);
    connSendMessage (uiplayer->conn, ROUTE_PLAYER, MSG_PLAY_SPEED, tbuff);
    if (uiplayer->speedLock) {
      mstimeset (&uiplayer->speedLockSend, UIPLAYER_LOCK_TIME_SEND);
    } else {
      mstimeset (&uiplayer->speedLockSend, 3600000);
    }
  }

  if (mstimeCheck (&uiplayer->seekLockTimeout)) {
    mstimeset (&uiplayer->seekLockTimeout, 3600000);
    uiplayer->seekLock = false;
  }

  if (mstimeCheck (&uiplayer->seekLockSend)) {
    double        value;
    char          tbuff [40];

    value = gtk_range_get_value (GTK_RANGE (uiplayer->seekScale));
    snprintf (tbuff, sizeof (tbuff), "%.0f", round (value));
    connSendMessage (uiplayer->conn, ROUTE_PLAYER, MSG_PLAY_SEEK, tbuff);
    if (uiplayer->seekLock) {
      mstimeset (&uiplayer->seekLockSend, UIPLAYER_LOCK_TIME_SEND);
    } else {
      mstimeset (&uiplayer->seekLockSend, 3600000);
    }
  }
}

int
uiplayerProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
    bdjmsgmsg_t msg, char *args, void *udata)
{
  uiplayer_t    *uiplayer = udata;
  bool          disp = false;
  char          *targs = NULL;

  logProcBegin (LOG_PROC, "uiplayerProcessMsg");
  if (args != NULL) {
    targs = strdup (args);
  }

  switch (route) {
    case ROUTE_NONE:
    case ROUTE_MANAGEUI:
    case ROUTE_PLAYERUI: {
      switch (msg) {
        case MSG_PLAYER_STATE: {
          uiplayerProcessPlayerState (uiplayer, atol (targs));
          disp = true;
          break;
        }
        case MSG_PLAY_PAUSEATEND_STATE: {
          uiplayerProcessPauseatend (uiplayer, atol (targs));
          disp = true;
          break;
        }
        case MSG_PLAYER_STATUS_DATA: {
          uiplayerProcessPlayerStatusData (uiplayer, targs);
          // disp = true;
          break;
        }
        case MSG_MUSICQ_STATUS_DATA: {
          uiplayerProcessMusicqStatusData (uiplayer, targs);
          disp = true;
          break;
        }
        case MSG_FINISHED: {
          uiplayerClearDisplay (uiplayer);
          disp = true;
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

  if (disp) {
    logMsg (LOG_DBG, LOG_MSGS, "uiplayer: got: from:%d/%s route:%d/%s msg:%d/%s args:%s",
        routefrom, msgRouteDebugText (routefrom),
        route, msgRouteDebugText (route), msg, msgDebugText (msg), args);
  }
  if (args != NULL) {
    free (targs);
  }

  logProcEnd (LOG_PROC, "uiplayerProcessMsg", "");
  return 0;
}

/* internal routines */

static bool
uiplayerInitCallback (void *udata, programstate_t programState)
{
  uiplayer_t *uiplayer = udata;

  logProcBegin (LOG_PROC, "uiplayerInitCallback");

  uiplayer->repeatLock = false;
  uiplayer->pauseatendLock = false;
  uiplayer->lastdur = 180000;
  uiplayer->speedLock = false;
  mstimeset (&uiplayer->speedLockTimeout, 3600000);
  mstimeset (&uiplayer->speedLockSend, 3600000);
  uiplayer->seekLock = false;
  mstimeset (&uiplayer->seekLockTimeout, 3600000);
  mstimeset (&uiplayer->seekLockSend, 3600000);
  uiplayer->volumeLock = false;
  mstimeset (&uiplayer->volumeLockTimeout, 3600000);
  mstimeset (&uiplayer->volumeLockSend, 3600000);

  logProcEnd (LOG_PROC, "uiplayerInitCallback", "");
  return STATE_FINISHED;
}

static bool
uiplayerClosingCallback (void *udata, programstate_t programState)
{
  uiplayer_t      *uiplayer = udata;

  logProcBegin (LOG_PROC, "uiplayerClosingCallback");
  uiWidgetClearPersistent (&uiplayer->stopPixbuf);
  uiWidgetClearPersistent (&uiplayer->playPixbuf);
  uiWidgetClearPersistent (&uiplayer->pausePixbuf);
  uiWidgetClearPersistent (&uiplayer->repeatPixbuf);
  uiWidgetClearPersistent (&uiplayer->ledonImg);
  uiWidgetClearPersistent (&uiplayer->ledoffImg);
  logProcEnd (LOG_PROC, "uiplayerClosingCallback", "");
  return STATE_FINISHED;
}

static void
uiplayerProcessPauseatend (uiplayer_t *uiplayer, int on)
{
  logProcBegin (LOG_PROC, "uiplayerProcessPauseatend");

  if (uiplayer->pauseatendButton == NULL) {
    logProcEnd (LOG_PROC, "uiplayerProcessPauseatend", "no-pae-button");
    return;
  }
  uiplayer->pauseatendLock = true;

  if (on) {
    uiToggleButtonSetImage (uiplayer->pauseatendButton, &uiplayer->ledonImg);
    uiToggleButtonSetState (uiplayer->pauseatendButton, TRUE);
  } else {
    uiToggleButtonSetImage (uiplayer->pauseatendButton, &uiplayer->ledoffImg);
    uiToggleButtonSetState (uiplayer->pauseatendButton, FALSE);
  }
  uiplayer->pauseatendLock = false;
  logProcEnd (LOG_PROC, "uiplayerProcessPauseatend", "");
}

static void
uiplayerProcessPlayerState (uiplayer_t *uiplayer, int playerState)
{
  logProcBegin (LOG_PROC, "uiplayerProcessPlayerState");

  uiplayer->playerState = playerState;
  if (playerState == PL_STATE_IN_FADEOUT) {
    uiWidgetDisableW (uiplayer->volumeScale);
    uiWidgetDisableW (uiplayer->seekScale);
    uiWidgetDisableW (uiplayer->speedScale);
  } else {
    uiWidgetEnableW (uiplayer->volumeScale);
    uiWidgetEnableW (uiplayer->seekScale);
    uiWidgetEnableW (uiplayer->speedScale);
  }

  switch (playerState) {
    case PL_STATE_UNKNOWN:
    case PL_STATE_STOPPED: {
      uiImageClear (&uiplayer->statusImg);
      uiImageSetFromPixbuf (&uiplayer->statusImg, &uiplayer->stopPixbuf);
      break;
    }
    case PL_STATE_LOADING:
    case PL_STATE_IN_FADEOUT:
    case PL_STATE_IN_GAP:
    case PL_STATE_PLAYING: {
      uiImageClear (&uiplayer->statusImg);
      uiImageSetFromPixbuf (&uiplayer->statusImg, &uiplayer->playPixbuf);
      break;
    }
    case PL_STATE_PAUSED: {
      uiImageClear (&uiplayer->statusImg);
      uiImageSetFromPixbuf (&uiplayer->statusImg, &uiplayer->pausePixbuf);
      break;
    }
    default: {
      uiImageClear (&uiplayer->statusImg);
      uiImageSetFromPixbuf (&uiplayer->statusImg, &uiplayer->stopPixbuf);
      break;
    }
  }
  logProcEnd (LOG_PROC, "uiplayerProcessPlayerState", "");
}

static void
uiplayerProcessPlayerStatusData (uiplayer_t *uiplayer, char *args)
{
  char          *p;
  char          *tokstr;
  char          tbuff [100];
  double        dval;
  double        ddur;
  ssize_t       timeleft;
  ssize_t       position;
  ssize_t       dur;

  logProcBegin (LOG_PROC, "uiplayerProcessPlayerStatusData");

  /* player state */
  p = strtok_r (args, MSG_ARGS_RS_STR, &tokstr);
  /* this is handled by the MSG_PLAYER_STATE message */

  /* repeat */
  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  if (p != NULL) {
    uiplayer->repeatLock = true;
    if (atol (p)) {
      uiImageClear (&uiplayer->repeatImg);
      uiImageSetFromPixbuf (&uiplayer->repeatImg, &uiplayer->repeatPixbuf);
      uiToggleButtonSetState (uiplayer->repeatButton, TRUE);
    } else {
      uiImageClear (&uiplayer->repeatImg);
      uiToggleButtonSetState (uiplayer->repeatButton, FALSE);
    }
    uiplayer->repeatLock = false;
  }

  /* pauseatend */
  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  uiplayerProcessPauseatend (uiplayer, atol (p));

  /* vol */
  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  if (! uiplayer->volumeLock) {
    snprintf (tbuff, sizeof (tbuff), "%3s", p);
    uiLabelSetTextW (uiplayer->volumeDisplayLab, p);
    dval = atof (p);
    gtk_range_set_value (GTK_RANGE (uiplayer->volumeScale), dval);
  }

  /* speed */
  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  if (! uiplayer->speedLock) {
    snprintf (tbuff, sizeof (tbuff), "%3s", p);
    uiLabelSetTextW (uiplayer->speedDisplayLab, p);
    dval = atof (p);
    gtk_range_set_value (GTK_RANGE (uiplayer->speedScale), dval);
  }

  /* playedtime */
  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  if (! uiplayer->seekLock) {
    position = atol (p);
    dval = atof (p);    // used below
  }

  /* duration */
  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  ddur = atof (p);
  dur = atol (p);
  if (ddur > 0.0 && dur != uiplayer->lastdur) {
    tmutilToMS (dur, tbuff, sizeof (tbuff));
    uiLabelSetTextW (uiplayer->durationLab, tbuff);
    gtk_range_set_range (GTK_RANGE (uiplayer->seekScale), 0.0, ddur);
    uiplayer->lastdur = dur;
  }

  if (! uiplayer->seekLock) {
    tmutilToMS (position, tbuff, sizeof (tbuff));
    uiLabelSetTextW (uiplayer->seekDisplayLab, tbuff);

    timeleft = dur - position;
    tmutilToMS (timeleft, tbuff, sizeof (tbuff));
    uiLabelSetTextW (uiplayer->countdownTimerLab, tbuff);

    if (ddur == 0.0) {
      gtk_range_set_value (GTK_RANGE (uiplayer->seekScale), 0.0);
    } else {
      gtk_range_set_value (GTK_RANGE (uiplayer->seekScale), dval);
    }
  }
  logProcEnd (LOG_PROC, "uiplayerProcessPlayerStatusData", "");
}

static void
uiplayerProcessMusicqStatusData (uiplayer_t *uiplayer, char *args)
{
  char          *p;
  char          *tokstr;
  dbidx_t       dbidx = -1;
  song_t        *song = NULL;
  char          *data = NULL;
  ilistidx_t    danceIdx;
  dance_t       *dances;

  logProcBegin (LOG_PROC, "uiplayerProcessMusicqStatusData");

  dances = bdjvarsdfGet (BDJVDF_DANCES);

  p = strtok_r (args, MSG_ARGS_RS_STR, &tokstr);
  dbidx = atol (p);
  if (dbidx < 0) {
    logProcEnd (LOG_PROC, "uiplayerProcessMusicqStatusData", "bad-idx");
    return;
  }

  song = dbGetByIdx (uiplayer->musicdb, dbidx);
  if (song == NULL) {
    logProcEnd (LOG_PROC, "uiplayerProcessMusicqStatusData", "null-song");
    return;
  }

  if (uiplayer->danceLab != NULL) {
    danceIdx = songGetNum (song, TAG_DANCE);
    data = danceGetStr (dances, danceIdx, DANCE_DANCE);
    uiLabelSetTextW (uiplayer->danceLab, data);
  }

  /* artist */
  if (uiplayer->artistLab != NULL) {
    data = songGetStr (song, TAG_ARTIST);
    uiLabelSetTextW (uiplayer->artistLab, data);
  }

  /* title */
  if (uiplayer->titleLab != NULL) {
    data = songGetStr (song, TAG_TITLE);
    uiLabelSetTextW (uiplayer->titleLab, data);
  }
  logProcEnd (LOG_PROC, "uiplayerProcessMusicqStatusData", "");
}

static bool
uiplayerFadeProcess (void *udata)
{
  uiplayer_t      *uiplayer = udata;

  logProcBegin (LOG_PROC, "uiplayerFadeProcess");
  connSendMessage (uiplayer->conn, ROUTE_PLAYER, MSG_PLAY_FADE, NULL);
  logProcEnd (LOG_PROC, "uiplayerFadeProcess", "");
  return UICB_CONT;
}

static bool
uiplayerPlayPauseProcess (void *udata)
{
  uiplayer_t      *uiplayer = udata;

  logProcBegin (LOG_PROC, "uiplayerPlayPauseProcess");
  connSendMessage (uiplayer->conn, ROUTE_MAIN, MSG_CMD_PLAYPAUSE, NULL);
  logProcEnd (LOG_PROC, "uiplayerPlayPauseProcess", "");
  return UICB_CONT;
}

static bool
uiplayerRepeatProcess (GtkButton *b, void *udata)
{
  uiplayer_t      *uiplayer = udata;

  logProcBegin (LOG_PROC, "uiplayerRepeatProcess");

  if (uiplayer->repeatLock) {
    logProcEnd (LOG_PROC, "uiplayerRepeatProcess", "repeat-lock");
    return UICB_CONT;
  }

  connSendMessage (uiplayer->conn, ROUTE_PLAYER, MSG_PLAY_REPEAT, NULL);
  logProcEnd (LOG_PROC, "uiplayerRepeatProcess", "");
  return UICB_CONT;
}

static bool
uiplayerSongBeginProcess (void *udata)
{
  uiplayer_t      *uiplayer = udata;

  logProcBegin (LOG_PROC, "uiplayerSongBeginProcess");
  connSendMessage (uiplayer->conn, ROUTE_PLAYER, MSG_PLAY_SONG_BEGIN, NULL);
  logProcEnd (LOG_PROC, "uiplayerSongBeginProcess", "");
  return UICB_CONT;
}

static bool
uiplayerNextSongProcess (void *udata)
{
  uiplayer_t      *uiplayer = udata;

  logProcBegin (LOG_PROC, "uiplayerNextSongProcess");
  connSendMessage (uiplayer->conn, ROUTE_PLAYER, MSG_PLAY_NEXTSONG, NULL);
  logProcEnd (LOG_PROC, "uiplayerNextSongProcess", "");
  return UICB_CONT;
}

static void
uiplayerPauseatendProcess (GtkButton *b, void *udata)
{
  uiplayer_t      *uiplayer = udata;

  logProcBegin (LOG_PROC, "uiplayerPauseatendProcess");

  if (uiplayer->pauseatendLock) {
    logProcEnd (LOG_PROC, "uiplayerPauseatendProcess", "pae-lock");
    return;
  }
  connSendMessage (uiplayer->conn, ROUTE_PLAYER, MSG_PLAY_PAUSEATEND, NULL);
  logProcEnd (LOG_PROC, "uiplayerPauseatendProcess", "");
}

static gboolean
uiplayerSpeedProcess (GtkRange *range, GtkScrollType *scroll, gdouble value, gpointer udata)
{
  uiplayer_t    *uiplayer = udata;
  char          tbuff [40];

  logProcBegin (LOG_PROC, "uiplayerSpeedProcess");

  if (! uiplayer->speedLock) {
    mstimeset (&uiplayer->speedLockSend, UIPLAYER_LOCK_TIME_SEND);
  }
  uiplayer->speedLock = true;
  mstimeset (&uiplayer->speedLockTimeout, UIPLAYER_LOCK_TIME_WAIT);
  value = uiScaleEnforceMax (uiplayer->speedScale, value);
  snprintf (tbuff, sizeof (tbuff), "%3.0f", value);
  uiLabelSetTextW (uiplayer->speedDisplayLab, tbuff);
  logProcEnd (LOG_PROC, "uiplayerSpeedProcess", "");
  return FALSE;
}

static gboolean
uiplayerSeekProcess (GtkRange *range, GtkScrollType *scroll, gdouble value, gpointer udata)
{
  uiplayer_t    *uiplayer = udata;
  char          tbuff [40];
  ssize_t       position;
  ssize_t       timeleft;

  logProcBegin (LOG_PROC, "uiplayerSeekProcess");

  if (! uiplayer->seekLock) {
    mstimeset (&uiplayer->seekLockSend, UIPLAYER_LOCK_TIME_SEND);
  }
  uiplayer->seekLock = true;
  mstimeset (&uiplayer->seekLockTimeout, UIPLAYER_LOCK_TIME_WAIT);

  value = uiScaleEnforceMax (uiplayer->seekScale, value);
  position = (ssize_t) round (value);

  tmutilToMS (position, tbuff, sizeof (tbuff));
  uiLabelSetTextW (uiplayer->seekDisplayLab, tbuff);

  timeleft = uiplayer->lastdur - position;
  tmutilToMSD (timeleft, tbuff, sizeof (tbuff));
  uiLabelSetTextW (uiplayer->countdownTimerLab, tbuff);
  logProcEnd (LOG_PROC, "uiplayerSeekProcess", "");
  return FALSE;
}

static gboolean
uiplayerVolumeProcess (GtkRange *range, GtkScrollType *scroll, gdouble value, gpointer udata)
{
  uiplayer_t    *uiplayer = udata;
  char          tbuff [40];

  logProcBegin (LOG_PROC, "uiplayerVolumeProcess");

  if (! uiplayer->volumeLock) {
    mstimeset (&uiplayer->volumeLockSend, UIPLAYER_LOCK_TIME_SEND);
  }
  uiplayer->volumeLock = true;
  mstimeset (&uiplayer->volumeLockTimeout, UIPLAYER_LOCK_TIME_WAIT);

  value = uiScaleEnforceMax (uiplayer->volumeScale, value);
  snprintf (tbuff, sizeof (tbuff), "%3.0f", value);
  uiLabelSetTextW (uiplayer->volumeDisplayLab, tbuff);
  logProcEnd (LOG_PROC, "uiplayerVolumeProcess", "");
  return FALSE;
}

static void
uiplayerClearDisplay (uiplayer_t *uiplayer)
{
  uiLabelSetTextW (uiplayer->danceLab, "");
  uiLabelSetTextW (uiplayer->artistLab, "");
  uiLabelSetTextW (uiplayer->titleLab, "");
}
