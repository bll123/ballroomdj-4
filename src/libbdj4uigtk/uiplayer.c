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
#include "uiutils.h"

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
static void     uiplayerFadeProcess (GtkButton *b, gpointer udata);
static void     uiplayerPlayPauseProcess (GtkButton *b, gpointer udata);
static void     uiplayerRepeatProcess (GtkButton *b, gpointer udata);
static void     uiplayerSongBeginProcess (GtkButton *b, gpointer udata);
static void     uiplayerNextSongProcess (GtkButton *b, gpointer udata);
static void     uiplayerPauseatendProcess (GtkButton *b, gpointer udata);
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
  uiplayer->statusImg = NULL;
  uiplayer->repeatImg = NULL;
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
  uiplayer->playImg = NULL;
  uiplayer->stopImg = NULL;
  uiplayer->pauseImg = NULL;
  uiplayer->ledoffImg = NULL;
  uiplayer->ledonImg = NULL;
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
  GtkWidget       *image = NULL;
  GtkWidget       *hbox;
  GtkWidget       *tbox;
  GtkWidget       *widget;
  UIWidget        sgA;
  UIWidget        sgB;
  UIWidget        sgC;
  UIWidget        sgD;
  UIWidget        sgE;

  logProcBegin (LOG_PROC, "uiplayerBuildUI");

  uiutilsCreateSizeGroupHoriz (&sgA);
  uiutilsCreateSizeGroupHoriz (&sgB);
  uiutilsCreateSizeGroupHoriz (&sgC);
  uiutilsCreateSizeGroupHoriz (&sgD);
  uiutilsCreateSizeGroupHoriz (&sgE);

  uiplayer->vbox = uiutilsCreateVertBox ();
  assert (uiplayer->vbox != NULL);
  uiutilsWidgetExpandHoriz (uiplayer->vbox);

  /* song display */

  hbox = uiutilsCreateHorizBox ();
  assert (hbox != NULL);
  uiutilsWidgetExpandHoriz (hbox);
  uiutilsBoxPackStart (uiplayer->vbox, hbox);

  /* size group E */
  tbox = uiutilsCreateHorizBox ();
  assert (tbox != NULL);
  uiutilsBoxPackStart (hbox, tbox);
  uiutilsSizeGroupAdd (&sgE, tbox);

  uiplayer->statusImg = gtk_image_new ();
  assert (uiplayer->statusImg != NULL);

  pathbldMakePath (tbuff, sizeof (tbuff), "button_stop", ".svg",
      PATHBLD_MP_IMGDIR);
  image = gtk_image_new_from_file (tbuff);
  uiplayer->stopImg = gtk_image_get_pixbuf (GTK_IMAGE (image));
  if (G_IS_OBJECT (uiplayer->stopImg)) {
    g_object_ref_sink (G_OBJECT (uiplayer->stopImg));
  }

  gtk_image_set_from_pixbuf (GTK_IMAGE (uiplayer->statusImg), uiplayer->stopImg);
  gtk_widget_set_size_request (uiplayer->statusImg, 18, -1);
  gtk_widget_set_margin_start (uiplayer->statusImg, 2);
  uiutilsBoxPackStart (tbox, uiplayer->statusImg);

  pathbldMakePath (tbuff, sizeof (tbuff), "button_play", ".svg",
      PATHBLD_MP_IMGDIR);
  image = gtk_image_new_from_file (tbuff);
  uiplayer->playImg = gtk_image_get_pixbuf (GTK_IMAGE (image));
  if (G_IS_OBJECT (uiplayer->playImg)) {
    g_object_ref_sink (G_OBJECT (uiplayer->playImg));
  }

  pathbldMakePath (tbuff, sizeof (tbuff), "button_pause", ".svg",
      PATHBLD_MP_IMGDIR);
  image = gtk_image_new_from_file (tbuff);
  uiplayer->pauseImg = gtk_image_get_pixbuf (GTK_IMAGE (image));
  if (G_IS_OBJECT (uiplayer->pauseImg)) {
    g_object_ref_sink (G_OBJECT (uiplayer->pauseImg));
  }

  uiplayer->repeatImg = gtk_image_new ();
  assert (uiplayer->repeatImg != NULL);
  gtk_image_clear (GTK_IMAGE (uiplayer->repeatImg));
  gtk_widget_set_size_request (uiplayer->repeatImg, 18, -1);
  gtk_widget_set_margin_start (uiplayer->repeatImg, 2);
  uiutilsBoxPackStart (tbox, uiplayer->repeatImg);

  uiplayer->danceLab = uiutilsCreateLabel ("");
  uiutilsBoxPackStart (hbox, uiplayer->danceLab);

  widget = uiutilsCreateLabel (" : ");
  gtk_widget_set_margin_start (widget, 0);
  uiutilsBoxPackStart (hbox, widget);

  widget = uiutilsCreateLabel ("");
  gtk_widget_set_margin_start (widget, 0);
  uiutilsLabelEllipsizeOn (widget);
  uiutilsBoxPackStart (hbox, widget);
  uiplayer->artistLab = widget;

  widget = uiutilsCreateLabel (" : ");
  gtk_widget_set_margin_start (widget, 0);
  uiutilsBoxPackStart (hbox, widget);

  widget = uiutilsCreateLabel ("");
  gtk_widget_set_margin_start (widget, 0);
  uiutilsLabelEllipsizeOn (widget);
  uiutilsBoxPackStart (hbox, widget);
  uiplayer->titleLab = widget;

  widget = uiutilsCreateLabel ("");
  uiutilsWidgetExpandHoriz (widget);
  uiutilsBoxPackStart (hbox, widget);

  /* size group A */
  widget = uiutilsCreateLabel ("%");
  uiutilsBoxPackEnd (hbox, widget);
  uiutilsSizeGroupAdd (&sgA, widget);

  /* size group B */
  uiplayer->speedDisplayLab = uiutilsCreateLabel ("100");
  assert (uiplayer->speedDisplayLab != NULL);
  gtk_widget_set_size_request (uiplayer->speedDisplayLab, 24, -1);
  gtk_widget_set_halign (uiplayer->speedDisplayLab, GTK_ALIGN_END);
  gtk_label_set_xalign (GTK_LABEL (uiplayer->speedDisplayLab), 1.0);
  uiutilsBoxPackEnd (hbox, uiplayer->speedDisplayLab);
  uiutilsSizeGroupAdd (&sgB, uiplayer->speedDisplayLab);

  /* size group C */
  uiplayer->speedScale = uiutilsCreateScale (70.0, 130.0, 0.1, 1.0, 100.0);
  uiutilsBoxPackEnd (hbox, uiplayer->speedScale);
  uiutilsSizeGroupAdd (&sgC, uiplayer->speedScale);
  g_signal_connect (uiplayer->speedScale, "change-value", G_CALLBACK (uiplayerSpeedProcess), uiplayer);

  /* size group D */
  /* CONTEXT: the current speed for song playback */
  widget = uiutilsCreateColonLabel (_("Speed"));
  gtk_widget_set_halign (widget, GTK_ALIGN_END);
  gtk_label_set_xalign (GTK_LABEL (widget), 1.0);
  uiutilsBoxPackEnd (hbox, widget);
  uiutilsSizeGroupAdd (&sgD, widget);

  /* position controls / display */

  hbox = uiutilsCreateHorizBox ();
  assert (hbox != NULL);
  uiutilsWidgetExpandHoriz (hbox);
  uiutilsBoxPackStart (uiplayer->vbox, hbox);

  /* size group E */
  widget = uiutilsCreateLabel ("");
  uiutilsBoxPackStart (hbox, widget);
  uiutilsSizeGroupAdd (&sgE, widget);

  uiplayer->countdownTimerLab = uiutilsCreateLabel (" 0:00");
  gtk_label_set_xalign (GTK_LABEL (uiplayer->countdownTimerLab), 1.0);
  gtk_widget_set_size_request (uiplayer->countdownTimerLab, 40, -1);
  uiutilsBoxPackStart (hbox, uiplayer->countdownTimerLab);

  widget = uiutilsCreateLabel (" / ");
  gtk_widget_set_margin_start (widget, 0);
  uiutilsBoxPackStart (hbox, widget);

  uiplayer->durationLab = uiutilsCreateLabel (" 3:00");
  gtk_label_set_xalign (GTK_LABEL (uiplayer->durationLab), 1.0);
  gtk_widget_set_size_request (uiplayer->countdownTimerLab, 40, -1);
  uiutilsBoxPackStart (hbox, uiplayer->durationLab);

  widget = uiutilsCreateLabel ("");
  uiutilsWidgetExpandHoriz (widget);
  uiutilsBoxPackStart (hbox, widget);

  /* size group A */
  widget = uiutilsCreateLabel ("");
  uiutilsBoxPackEnd (hbox, widget);
  uiutilsSizeGroupAdd (&sgA, widget);

  /* size group B */
  uiplayer->seekDisplayLab = uiutilsCreateLabel ("3:00");
  assert (uiplayer->seekDisplayLab != NULL);
  gtk_widget_set_size_request (uiplayer->seekDisplayLab, 40, -1);
  gtk_widget_set_halign (uiplayer->seekDisplayLab, GTK_ALIGN_END);
  uiutilsSizeGroupAdd (&sgB, uiplayer->seekDisplayLab);
  uiutilsBoxPackEnd (hbox, uiplayer->seekDisplayLab);

  /* size group C */
  uiplayer->seekScale = uiutilsCreateScale (0.0, 180000.0, 100.0, 1000.0, 0.0);
  uiutilsBoxPackEnd (hbox, uiplayer->seekScale);
  uiutilsSizeGroupAdd (&sgC, uiplayer->seekScale);
  g_signal_connect (uiplayer->seekScale, "change-value", G_CALLBACK (uiplayerSeekProcess), uiplayer);

  /* size group D */
  /* CONTEXT: the current position of the song during song playback */
  widget = uiutilsCreateColonLabel (_("Position"));
  uiutilsBoxPackEnd (hbox, widget);
  gtk_widget_set_halign (widget, GTK_ALIGN_END);
  gtk_label_set_xalign (GTK_LABEL (widget), 1.0);
  uiutilsSizeGroupAdd (&sgD, widget);

  /* main controls */

  hbox = uiutilsCreateHorizBox ();
  assert (hbox != NULL);
  uiutilsWidgetExpandHoriz (hbox);
  uiutilsBoxPackStart (uiplayer->vbox, hbox);

  /* size group E */
  widget = uiutilsCreateLabel ("");
  uiutilsBoxPackStart (hbox, widget);
  uiutilsSizeGroupAdd (&sgE, widget);

  /* CONTEXT: button: fade out the song and stop playing it */
  widget = uiutilsCreateButton (_("Fade"), NULL,
      uiplayerFadeProcess, uiplayer);
  uiutilsBoxPackStart (hbox, widget);

  /* CONTEXT: button: play or pause the song */
  snprintf (tbuff, sizeof (tbuff), "%s / %s", _("Play"), _("Pause"));
  widget = uiutilsCreateButton (tbuff, "button_playpause",
      uiplayerPlayPauseProcess, uiplayer);
  uiutilsBoxPackStart (hbox, widget);

  uiplayer->repeatButton = gtk_toggle_button_new ();
  assert (uiplayer->repeatButton != NULL);
  gtk_widget_set_margin_top (uiplayer->repeatButton, 2);
  gtk_widget_set_margin_start (uiplayer->repeatButton, 2);

  pathbldMakePath (tbuff, sizeof (tbuff), "button_repeat", ".svg",
      PATHBLD_MP_IMGDIR);
  image = gtk_image_new_from_file (tbuff);
  gtk_button_set_image (GTK_BUTTON (uiplayer->repeatButton), image);
  /* CONTEXT: button: toggle the repeat song on and off */
  gtk_widget_set_tooltip_text (uiplayer->repeatButton, _("Toggle Repeat"));
  uiutilsBoxPackStart (hbox, uiplayer->repeatButton);
  g_signal_connect (uiplayer->repeatButton, "toggled", G_CALLBACK (uiplayerRepeatProcess), uiplayer);

  /* CONTEXT: button: return to the beginning of the song */
  widget = uiutilsCreateButton (_("Return to beginning of song"),
      "button_begin", uiplayerSongBeginProcess, uiplayer);
  uiutilsBoxPackStart (hbox, widget);

  /* CONTEXT: button: start playing the next song (immediate) */
  widget = uiutilsCreateButton (_("Next Song"), "button_nextsong",
      uiplayerNextSongProcess, uiplayer);
  uiutilsBoxPackStart (hbox, widget);

  uiplayer->pauseatendButton = gtk_toggle_button_new ();
  assert (uiplayer->pauseatendButton != NULL);
  /* CONTEXT: button: pause at the end of the song (toggle) */
  gtk_button_set_label (GTK_BUTTON (uiplayer->pauseatendButton),
      _("Pause at End"));
  gtk_widget_set_margin_top (uiplayer->pauseatendButton, 2);
  gtk_widget_set_margin_start (uiplayer->pauseatendButton, 2);

  pathbldMakePath (tbuff, sizeof (tbuff), "led_off", ".svg",
      PATHBLD_MP_IMGDIR);
  uiplayer->ledoffImg = gtk_image_new_from_file (tbuff);
  if (G_IS_OBJECT (uiplayer->ledoffImg)) {
    g_object_ref_sink (G_OBJECT (uiplayer->ledoffImg));
  }

  pathbldMakePath (tbuff, sizeof (tbuff), "led_on", ".svg",
      PATHBLD_MP_IMGDIR);
  uiplayer->ledonImg = gtk_image_new_from_file (tbuff);
  if (G_IS_OBJECT (uiplayer->ledonImg)) {
    g_object_ref_sink (G_OBJECT (uiplayer->ledonImg));
  }

  gtk_button_set_image (GTK_BUTTON (uiplayer->pauseatendButton), uiplayer->ledoffImg);
  gtk_button_set_image_position (GTK_BUTTON (uiplayer->pauseatendButton), GTK_POS_RIGHT);
  gtk_button_set_always_show_image (GTK_BUTTON (uiplayer->pauseatendButton), TRUE);
  uiutilsBoxPackStart (hbox, uiplayer->pauseatendButton);
  g_signal_connect (uiplayer->pauseatendButton, "toggled", G_CALLBACK (uiplayerPauseatendProcess), uiplayer);

  /* volume controls / display */

  /* size group A */
  widget = uiutilsCreateLabel ("%");
  uiutilsBoxPackEnd (hbox, widget);
  uiutilsSizeGroupAdd (&sgA, widget);

  /* size group B */
  uiplayer->volumeDisplayLab = uiutilsCreateLabel ("100");
  assert (uiplayer->volumeDisplayLab != NULL);
  gtk_widget_set_size_request (uiplayer->volumeDisplayLab, 24, -1);
  gtk_widget_set_halign (uiplayer->volumeDisplayLab, GTK_ALIGN_END);
  gtk_label_set_xalign (GTK_LABEL (uiplayer->volumeDisplayLab), 1.0);
  uiutilsBoxPackEnd (hbox, uiplayer->volumeDisplayLab);
  uiutilsSizeGroupAdd (&sgB, uiplayer->volumeDisplayLab);

  /* size group C */
  uiplayer->volumeScale = uiutilsCreateScale (0.0, 100.0, 0.1, 1.0, 0.0);
  uiutilsBoxPackEnd (hbox, uiplayer->volumeScale);
  uiutilsSizeGroupAdd (&sgC, uiplayer->volumeScale);
  g_signal_connect (uiplayer->volumeScale, "change-value", G_CALLBACK (uiplayerVolumeProcess), uiplayer);

  /* size group D */
  /* CONTEXT: The current volume of the song */
  widget = uiutilsCreateColonLabel (_("Volume"));
  uiutilsBoxPackEnd (hbox, widget);
  gtk_widget_set_halign (widget, GTK_ALIGN_END);
  gtk_label_set_xalign (GTK_LABEL (widget), 1.0);
  uiutilsSizeGroupAdd (&sgD, widget);

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

  logProcBegin (LOG_PROC, "uiplayerProcessMsg");

  logMsg (LOG_DBG, LOG_MSGS, "uiplayer: got: from:%d/%s route:%d/%s msg:%d/%s args:%s",
      routefrom, msgRouteDebugText (routefrom),
      route, msgRouteDebugText (route), msg, msgDebugText (msg), args);

  switch (route) {
    case ROUTE_NONE:
    case ROUTE_MANAGEUI:
    case ROUTE_PLAYERUI: {
      switch (msg) {
        case MSG_PLAYER_STATE: {
          uiplayerProcessPlayerState (uiplayer, atol (args));
          break;
        }
        case MSG_PLAY_PAUSEATEND_STATE: {
          uiplayerProcessPauseatend (uiplayer, atol (args));
          break;
        }
        case MSG_PLAYER_STATUS_DATA: {
          uiplayerProcessPlayerStatusData (uiplayer, args);
          break;
        }
        case MSG_MUSICQ_STATUS_DATA: {
          uiplayerProcessMusicqStatusData (uiplayer, args);
          break;
        }
        case MSG_FINISHED: {
          uiplayerClearDisplay (uiplayer);
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
  return true;
}

static bool
uiplayerClosingCallback (void *udata, programstate_t programState)
{
  uiplayer_t      *uiplayer = udata;

  logProcBegin (LOG_PROC, "uiplayerClosingCallback");
  if (G_IS_OBJECT (uiplayer->stopImg)) {
    g_object_unref (G_OBJECT (uiplayer->stopImg));
  }
  if (G_IS_OBJECT (uiplayer->playImg)) {
    g_object_unref (G_OBJECT (uiplayer->playImg));
  }
  if (G_IS_OBJECT (uiplayer->pauseImg)) {
    g_object_unref (G_OBJECT (uiplayer->pauseImg));
  }
  if (G_IS_OBJECT (uiplayer->ledonImg)) {
    g_object_unref (G_OBJECT (uiplayer->ledonImg));
  }
  if (G_IS_OBJECT (uiplayer->ledoffImg)) {
    g_object_unref (G_OBJECT (uiplayer->ledoffImg));
  }
  logProcEnd (LOG_PROC, "uiplayerClosingCallback", "");
  return true;
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
    gtk_button_set_image (GTK_BUTTON (uiplayer->pauseatendButton), uiplayer->ledonImg);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (uiplayer->pauseatendButton), TRUE);
  } else {
    gtk_button_set_image (GTK_BUTTON (uiplayer->pauseatendButton), uiplayer->ledoffImg);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (uiplayer->pauseatendButton), FALSE);
  }
  uiplayer->pauseatendLock = false;
  logProcEnd (LOG_PROC, "uiplayerProcessPauseatend", "");
}

static void
uiplayerProcessPlayerState (uiplayer_t *uiplayer, int playerState)
{
  logProcBegin (LOG_PROC, "uiplayerProcessPlayerState");

  if (uiplayer->statusImg == NULL) {
    logProcEnd (LOG_PROC, "uiplayerProcessPlayerState", "no-status-img");
    return;
  }

  uiplayer->playerState = playerState;
  if (playerState == PL_STATE_IN_FADEOUT) {
    uiutilsWidgetDisable (uiplayer->volumeScale);
    uiutilsWidgetDisable (uiplayer->seekScale);
    uiutilsWidgetDisable (uiplayer->speedScale);
  } else {
    uiutilsWidgetEnable (uiplayer->volumeScale);
    uiutilsWidgetEnable (uiplayer->seekScale);
    uiutilsWidgetEnable (uiplayer->speedScale);
  }

  switch (playerState) {
    case PL_STATE_UNKNOWN:
    case PL_STATE_STOPPED: {
      gtk_image_clear (GTK_IMAGE (uiplayer->statusImg));
      gtk_image_set_from_pixbuf (GTK_IMAGE (uiplayer->statusImg), uiplayer->stopImg);
      break;
    }
    case PL_STATE_LOADING:
    case PL_STATE_IN_FADEOUT:
    case PL_STATE_IN_GAP:
    case PL_STATE_PLAYING: {
      gtk_image_clear (GTK_IMAGE (uiplayer->statusImg));
      gtk_image_set_from_pixbuf (GTK_IMAGE (uiplayer->statusImg), uiplayer->playImg);
      break;
    }
    case PL_STATE_PAUSED: {
      gtk_image_clear (GTK_IMAGE (uiplayer->statusImg));
      gtk_image_set_from_pixbuf (GTK_IMAGE (uiplayer->statusImg), uiplayer->pauseImg);
      break;
    }
    default: {
      gtk_image_clear (GTK_IMAGE (uiplayer->statusImg));
      gtk_image_set_from_pixbuf (GTK_IMAGE (uiplayer->statusImg), uiplayer->stopImg);
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
  if (p != NULL && uiplayer->repeatImg != NULL) {
    uiplayer->repeatLock = true;
    if (atol (p)) {
      pathbldMakePath (tbuff, sizeof (tbuff), "button_repeat", ".svg",
          PATHBLD_MP_IMGDIR);
      gtk_image_clear (GTK_IMAGE (uiplayer->repeatImg));
      gtk_image_set_from_file (GTK_IMAGE (uiplayer->repeatImg), tbuff);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (uiplayer->repeatButton), TRUE);
    } else {
      gtk_image_clear (GTK_IMAGE (uiplayer->repeatImg));
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (uiplayer->repeatButton), FALSE);
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
    uiutilsLabelSetText (uiplayer->volumeDisplayLab, p);
    dval = atof (p);
    gtk_range_set_value (GTK_RANGE (uiplayer->volumeScale), dval);
  }

  /* speed */
  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  if (! uiplayer->speedLock) {
    snprintf (tbuff, sizeof (tbuff), "%3s", p);
    uiutilsLabelSetText (uiplayer->speedDisplayLab, p);
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
    uiutilsLabelSetText (uiplayer->durationLab, tbuff);
    gtk_range_set_range (GTK_RANGE (uiplayer->seekScale), 0.0, ddur);
    uiplayer->lastdur = dur;
  }

  if (! uiplayer->seekLock) {
    tmutilToMS (position, tbuff, sizeof (tbuff));
    uiutilsLabelSetText (uiplayer->seekDisplayLab, tbuff);

    timeleft = dur - position;
    tmutilToMS (timeleft, tbuff, sizeof (tbuff));
    uiutilsLabelSetText (uiplayer->countdownTimerLab, tbuff);

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
    uiutilsLabelSetText (uiplayer->danceLab, data);
  }

  /* artist */
  if (uiplayer->artistLab != NULL) {
    data = songGetStr (song, TAG_ARTIST);
    uiutilsLabelSetText (uiplayer->artistLab, data);
  }

  /* title */
  if (uiplayer->titleLab != NULL) {
    data = songGetStr (song, TAG_TITLE);
    uiutilsLabelSetText (uiplayer->titleLab, data);
  }
  logProcEnd (LOG_PROC, "uiplayerProcessMusicqStatusData", "");
}

static void
uiplayerFadeProcess (GtkButton *b, gpointer udata)
{
  uiplayer_t      *uiplayer = udata;

  logProcBegin (LOG_PROC, "uiplayerFadeProcess");
  connSendMessage (uiplayer->conn, ROUTE_PLAYER, MSG_PLAY_FADE, NULL);
  logProcEnd (LOG_PROC, "uiplayerFadeProcess", "");
}

static void
uiplayerPlayPauseProcess (GtkButton *b, gpointer udata)
{
  uiplayer_t      *uiplayer = udata;

  logProcBegin (LOG_PROC, "uiplayerPlayPauseProcess");
  connSendMessage (uiplayer->conn, ROUTE_MAIN, MSG_PLAY_PLAYPAUSE, NULL);
  logProcEnd (LOG_PROC, "uiplayerPlayPauseProcess", "");
}

static void
uiplayerRepeatProcess (GtkButton *b, gpointer udata)
{
  uiplayer_t      *uiplayer = udata;

  logProcBegin (LOG_PROC, "uiplayerRepeatProcess");

  if (uiplayer->repeatLock) {
    logProcEnd (LOG_PROC, "uiplayerRepeatProcess", "repeat-lock");
    return;
  }

  connSendMessage (uiplayer->conn, ROUTE_PLAYER, MSG_PLAY_REPEAT, NULL);
  logProcEnd (LOG_PROC, "uiplayerRepeatProcess", "");
}

static void
uiplayerSongBeginProcess (GtkButton *b, gpointer udata)
{
  uiplayer_t      *uiplayer = udata;

  logProcBegin (LOG_PROC, "uiplayerSongBeginProcess");
  connSendMessage (uiplayer->conn, ROUTE_PLAYER, MSG_PLAY_SONG_BEGIN, NULL);
  logProcEnd (LOG_PROC, "uiplayerSongBeginProcess", "");
}

static void
uiplayerNextSongProcess (GtkButton *b, gpointer udata)
{
  uiplayer_t      *uiplayer = udata;

  logProcBegin (LOG_PROC, "uiplayerNextSongProcess");
  connSendMessage (uiplayer->conn, ROUTE_PLAYER, MSG_PLAY_NEXTSONG, NULL);
  logProcEnd (LOG_PROC, "uiplayerNextSongProcess", "");
}

static void
uiplayerPauseatendProcess (GtkButton *b, gpointer udata)
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
  value = uiutilsScaleEnforceMax (uiplayer->speedScale, value);
  snprintf (tbuff, sizeof (tbuff), "%3.0f", value);
  uiutilsLabelSetText (uiplayer->speedDisplayLab, tbuff);
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

  value = uiutilsScaleEnforceMax (uiplayer->seekScale, value);
  position = (ssize_t) round (value);

  tmutilToMS (position, tbuff, sizeof (tbuff));
  uiutilsLabelSetText (uiplayer->seekDisplayLab, tbuff);

  timeleft = uiplayer->lastdur - position;
  tmutilToMSD (timeleft, tbuff, sizeof (tbuff));
  uiutilsLabelSetText (uiplayer->countdownTimerLab, tbuff);
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

  value = uiutilsScaleEnforceMax (uiplayer->volumeScale, value);
  snprintf (tbuff, sizeof (tbuff), "%3.0f", value);
  uiutilsLabelSetText (uiplayer->volumeDisplayLab, tbuff);
  logProcEnd (LOG_PROC, "uiplayerVolumeProcess", "");
  return FALSE;
}

static void
uiplayerClearDisplay (uiplayer_t *uiplayer)
{
  uiutilsLabelSetText (uiplayer->danceLab, "");
  uiutilsLabelSetText (uiplayer->artistLab, "");
  uiutilsLabelSetText (uiplayer->titleLab, "");
}
