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

uiplayer_t *
uiplayerInit (progstate_t *progstate, conn_t *conn)
{
  uiplayer_t    *uiplayer;

  logProcBegin (LOG_PROC, "uiplayerInit");
  uiplayer = malloc (sizeof (uiplayer_t));
  assert (uiplayer != NULL);
  uiplayer->progstate = progstate;
  uiplayer->conn = conn;

  progstateSetCallback (uiplayer->progstate, STATE_CONNECTING, uiplayerInitCallback, uiplayer);
  progstateSetCallback (uiplayer->progstate, STATE_CLOSING, uiplayerClosingCallback, uiplayer);

  logProcEnd (LOG_PROC, "uiplayerInit", "");
  return uiplayer;
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
uiplayerActivate (uiplayer_t *uiplayer)
{
  char            tbuff [MAXPATHLEN];
  GtkWidget       *image = NULL;
  GtkAdjustment   *adjustment = NULL;
  GtkWidget       *hbox;
  GtkWidget       *tbox;
  GtkWidget       *widget;
  GtkSizeGroup    *sgA;
  GtkSizeGroup    *sgB;
  GtkSizeGroup    *sgC;
  GtkSizeGroup    *sgD;
  GtkSizeGroup    *sgE;

  logProcBegin (LOG_PROC, "uiplayerActivate");

  sgA = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  sgB = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  sgC = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  sgD = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  sgE = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  uiplayer->vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  assert (uiplayer->vbox != NULL);
  gtk_widget_set_hexpand (uiplayer->vbox, TRUE);
  gtk_widget_set_vexpand (uiplayer->vbox, FALSE);

  /* song display */

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  assert (hbox != NULL);
  gtk_widget_set_hexpand (hbox, TRUE);
  gtk_box_pack_start (GTK_BOX (uiplayer->vbox), hbox,
      FALSE, FALSE, 0);

  /* size group E */
  tbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  assert (tbox != NULL);
  gtk_box_pack_start (GTK_BOX (hbox), tbox,
      FALSE, FALSE, 0);
  gtk_size_group_add_widget (sgE, tbox);

  uiplayer->statusImg = gtk_image_new ();
  assert (uiplayer->statusImg != NULL);
  pathbldMakePath (tbuff, sizeof (tbuff), "", "button_stop", ".svg",
      PATHBLD_MP_IMGDIR);
  image = gtk_image_new_from_file (tbuff);
  uiplayer->stopImg = gtk_image_get_pixbuf (GTK_IMAGE (image));
  g_object_ref_sink (G_OBJECT (uiplayer->stopImg));
  gtk_image_set_from_pixbuf (GTK_IMAGE (uiplayer->statusImg), uiplayer->stopImg);
  gtk_widget_set_size_request (uiplayer->statusImg, 18, -1);
  gtk_widget_set_margin_start (uiplayer->statusImg, 2);
  gtk_box_pack_start (GTK_BOX (tbox), uiplayer->statusImg,
      FALSE, FALSE, 0);

  pathbldMakePath (tbuff, sizeof (tbuff), "", "button_play", ".svg",
      PATHBLD_MP_IMGDIR);
  image = gtk_image_new_from_file (tbuff);
  uiplayer->playImg = gtk_image_get_pixbuf (GTK_IMAGE (image));
  g_object_ref_sink (G_OBJECT (uiplayer->playImg));

  pathbldMakePath (tbuff, sizeof (tbuff), "", "button_pause", ".svg",
      PATHBLD_MP_IMGDIR);
  image = gtk_image_new_from_file (tbuff);
  uiplayer->pauseImg = gtk_image_get_pixbuf (GTK_IMAGE (image));
  g_object_ref_sink (G_OBJECT (uiplayer->pauseImg));

  uiplayer->repeatImg = gtk_image_new ();
  assert (uiplayer->repeatImg != NULL);
  gtk_image_clear (GTK_IMAGE (uiplayer->repeatImg));
  gtk_widget_set_size_request (uiplayer->repeatImg, 18, -1);
  gtk_widget_set_margin_start (uiplayer->repeatImg, 2);
  gtk_box_pack_start (GTK_BOX (tbox), uiplayer->repeatImg,
      FALSE, FALSE, 0);

  uiplayer->danceLab = gtk_label_new ("");
  assert (uiplayer->danceLab != NULL);
  gtk_widget_set_margin_start (uiplayer->danceLab, 2);
  gtk_box_pack_start (GTK_BOX (hbox), uiplayer->danceLab,
      FALSE, FALSE, 0);

  widget = gtk_label_new (" : ");
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

  uiplayer->artistLab = gtk_label_new ("");
  assert (uiplayer->artistLab != NULL);
  gtk_box_pack_start (GTK_BOX (hbox), uiplayer->artistLab,
      FALSE, FALSE, 0);

  widget = gtk_label_new (" : ");
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

  uiplayer->titleLab = gtk_label_new ("");
  assert (uiplayer->titleLab != NULL);
  gtk_box_pack_start (GTK_BOX (hbox), uiplayer->titleLab,
      FALSE, FALSE, 0);

  widget = gtk_label_new ("");
  assert (widget != NULL);
  gtk_widget_set_hexpand (widget, TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

  /* size group A */
  widget = gtk_label_new ("%");
  gtk_box_pack_end (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
  gtk_widget_set_margin_start (widget, 1);
  gtk_size_group_add_widget (sgA, widget);

  /* size group B */
  uiplayer->speedDisplayLab = gtk_label_new ("100");
  assert (uiplayer->speedDisplayLab != NULL);
  gtk_widget_set_size_request (uiplayer->speedDisplayLab, 24, -1);
  gtk_widget_set_halign (uiplayer->speedDisplayLab, GTK_ALIGN_END);
  gtk_label_set_xalign (GTK_LABEL (uiplayer->speedDisplayLab), 1.0);
  gtk_box_pack_end (GTK_BOX (hbox), uiplayer->speedDisplayLab,
      FALSE, FALSE, 0);
  gtk_size_group_add_widget (sgB, uiplayer->speedDisplayLab);

  /* size group C */
  adjustment = gtk_adjustment_new (0.0, 70.0, 130.0, 0.1, 1.0, 0.0);
  uiplayer->speedScale = gtk_scale_new (GTK_ORIENTATION_HORIZONTAL, adjustment);
  assert (uiplayer->speedScale != NULL);
  gtk_widget_set_size_request (uiplayer->speedScale, 200, 5);
  gtk_scale_set_draw_value (GTK_SCALE (uiplayer->speedScale), FALSE);
  gtk_scale_set_has_origin (GTK_SCALE (uiplayer->speedScale), TRUE);
  gtk_range_set_value (GTK_RANGE (uiplayer->speedScale), 100.0);
  uiutilsSetCss (uiplayer->speedScale,
      "scale, trough { min-height: 5px; }");
  gtk_box_pack_end (GTK_BOX (hbox), uiplayer->speedScale,
      FALSE, FALSE, 0);
  gtk_size_group_add_widget (sgC, uiplayer->speedScale);
  g_signal_connect (uiplayer->speedScale, "change-value", G_CALLBACK (uiplayerSpeedProcess), uiplayer);

  /* size group D */
  widget = uiutilsCreateColonLabel (_("Speed"));
  gtk_widget_set_halign (widget, GTK_ALIGN_END);
  gtk_label_set_xalign (GTK_LABEL (widget), 1.0);
  gtk_box_pack_end (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
  gtk_size_group_add_widget (sgD, widget);

  /* position controls / display */

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  assert (hbox != NULL);
  gtk_widget_set_hexpand (hbox, TRUE);
  gtk_box_pack_start (GTK_BOX (uiplayer->vbox), hbox,
      FALSE, FALSE, 0);

  /* size group E */
  widget = gtk_label_new ("");
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
  gtk_size_group_add_widget (sgE, widget);

  uiplayer->countdownTimerLab = gtk_label_new (" 0:00");
  assert (uiplayer->countdownTimerLab != NULL);
  gtk_widget_set_size_request (uiplayer->countdownTimerLab, 40, -1);
  gtk_widget_set_margin_start (uiplayer->countdownTimerLab, 2);
  gtk_box_pack_start (GTK_BOX (hbox), uiplayer->countdownTimerLab,
      FALSE, FALSE, 0);

  widget = gtk_label_new (" / ");
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

  uiplayer->durationLab = gtk_label_new (" 3:00");
  assert (uiplayer->durationLab != NULL);
  gtk_widget_set_size_request (uiplayer->countdownTimerLab, 40, -1);
  gtk_box_pack_start (GTK_BOX (hbox), uiplayer->durationLab,
      FALSE, FALSE, 0);

  widget = gtk_label_new ("");
  assert (widget != NULL);
  gtk_widget_set_hexpand (widget, TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

  /* size group A */
  widget = gtk_label_new ("");
  gtk_box_pack_end (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
  gtk_size_group_add_widget (sgA, widget);

  /* size group B */
  uiplayer->seekDisplayLab = gtk_label_new ("3:00");
  assert (uiplayer->seekDisplayLab != NULL);
  gtk_widget_set_size_request (uiplayer->seekDisplayLab, 40, -1);
  gtk_widget_set_halign (uiplayer->seekDisplayLab, GTK_ALIGN_END);
  gtk_size_group_add_widget (sgB, uiplayer->seekDisplayLab);
  gtk_box_pack_end (GTK_BOX (hbox), uiplayer->seekDisplayLab,
      FALSE, FALSE, 0);

  /* size group C */
  adjustment = gtk_adjustment_new (0.0, 0.0, 180000.0, 100.0, 1000.0, 0.0);
  uiplayer->seekScale = gtk_scale_new (GTK_ORIENTATION_HORIZONTAL, adjustment);
  assert (uiplayer->seekScale != NULL);
  gtk_widget_set_size_request (uiplayer->seekScale, 200, 5);
  gtk_scale_set_draw_value (GTK_SCALE (uiplayer->seekScale), FALSE);
  gtk_scale_set_has_origin (GTK_SCALE (uiplayer->seekScale), TRUE);
  gtk_range_set_value (GTK_RANGE (uiplayer->seekScale), 0.0);
  uiutilsSetCss (uiplayer->seekScale,
      "scale, trough { min-height: 5px; }");
  gtk_box_pack_end (GTK_BOX (hbox), uiplayer->seekScale,
      FALSE, FALSE, 0);
  gtk_size_group_add_widget (sgC, uiplayer->seekScale);
  g_signal_connect (uiplayer->seekScale, "change-value", G_CALLBACK (uiplayerSeekProcess), uiplayer);

  /* size group D */
  widget = uiutilsCreateColonLabel (_("Position"));
  gtk_box_pack_end (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
  gtk_widget_set_halign (widget, GTK_ALIGN_END);
  gtk_label_set_xalign (GTK_LABEL (widget), 1.0);
  gtk_size_group_add_widget (sgD, widget);

  /* main controls */

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  assert (hbox != NULL);
  gtk_widget_set_hexpand (hbox, TRUE);
  gtk_box_pack_start (GTK_BOX (uiplayer->vbox), hbox,
      FALSE, FALSE, 0);

  /* size group E */
  widget = gtk_label_new ("");
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
  gtk_size_group_add_widget (sgE, widget);

  widget = uiutilsCreateButton (_("Fade"), NULL,
      uiplayerFadeProcess, uiplayer);
  gtk_box_pack_start (GTK_BOX (hbox), widget,
      FALSE, FALSE, 0);

  snprintf (tbuff, sizeof (tbuff), "%s / %s", _("Play"), _("Pause"));
  widget = uiutilsCreateButton (tbuff, "button_playpause",
      uiplayerPlayPauseProcess, uiplayer);
  gtk_box_pack_start (GTK_BOX (hbox), widget,
      FALSE, FALSE, 0);

  uiplayer->repeatButton = gtk_toggle_button_new ();
  assert (uiplayer->repeatButton != NULL);
  pathbldMakePath (tbuff, sizeof (tbuff), "", "button_repeat", ".svg",
      PATHBLD_MP_IMGDIR);
  image = gtk_image_new_from_file (tbuff);
  gtk_button_set_image (GTK_BUTTON (uiplayer->repeatButton), image);
  gtk_widget_set_tooltip_text (uiplayer->repeatButton, _("Toggle Repeat"));
  gtk_widget_set_margin_start (uiplayer->repeatButton, 2);
  gtk_box_pack_start (GTK_BOX (hbox), uiplayer->repeatButton,
      FALSE, FALSE, 0);
  g_signal_connect (uiplayer->repeatButton, "toggled", G_CALLBACK (uiplayerRepeatProcess), uiplayer);

  widget = uiutilsCreateButton (_("Return to beginning of song"),
      "button_begin", uiplayerSongBeginProcess, uiplayer);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

  widget = uiutilsCreateButton (_("Next Song"), "button_nextsong",
      uiplayerNextSongProcess, uiplayer);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

  uiplayer->pauseatendButton = gtk_toggle_button_new ();
  gtk_button_set_label (GTK_BUTTON (uiplayer->pauseatendButton), _("Pause at End"));
  assert (uiplayer->pauseatendButton != NULL);

  pathbldMakePath (tbuff, sizeof (tbuff), "", "led_off", ".svg",
      PATHBLD_MP_IMGDIR);
  uiplayer->ledoffImg = gtk_image_new_from_file (tbuff);
  g_object_ref_sink (G_OBJECT (uiplayer->ledoffImg));

  pathbldMakePath (tbuff, sizeof (tbuff), "", "led_on", ".svg",
      PATHBLD_MP_IMGDIR);
  uiplayer->ledonImg = gtk_image_new_from_file (tbuff);
  g_object_ref_sink (G_OBJECT (uiplayer->ledonImg));

  gtk_button_set_image (GTK_BUTTON (uiplayer->pauseatendButton), uiplayer->ledoffImg);
  gtk_button_set_image_position (GTK_BUTTON (uiplayer->pauseatendButton), GTK_POS_RIGHT);
  gtk_button_set_always_show_image (GTK_BUTTON (uiplayer->pauseatendButton), TRUE);
  gtk_widget_set_margin_start (uiplayer->pauseatendButton, 2);
  gtk_box_pack_start (GTK_BOX (hbox), uiplayer->pauseatendButton,
      FALSE, FALSE, 0);
  g_signal_connect (uiplayer->pauseatendButton, "toggled", G_CALLBACK (uiplayerPauseatendProcess), uiplayer);

  /* volume controls / display */

  /* size group A */
  widget = gtk_label_new ("%");
  gtk_box_pack_end (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
  gtk_widget_set_margin_start (widget, 1);
  gtk_size_group_add_widget (sgA, widget);

  /* size group B */
  uiplayer->volumeDisplayLab = gtk_label_new ("100");
  assert (uiplayer->volumeDisplayLab != NULL);
  gtk_widget_set_size_request (uiplayer->volumeDisplayLab, 24, -1);
  gtk_widget_set_halign (uiplayer->volumeDisplayLab, GTK_ALIGN_END);
  gtk_label_set_xalign (GTK_LABEL (uiplayer->volumeDisplayLab), 1.0);
  gtk_box_pack_end (GTK_BOX (hbox), uiplayer->volumeDisplayLab,
      FALSE, FALSE, 0);
  gtk_size_group_add_widget (sgB, uiplayer->volumeDisplayLab);

  /* size group C */
  adjustment = gtk_adjustment_new (0.0, 0.0, 100.0, 0.1, 1.0, 0.0);
  uiplayer->volumeScale = gtk_scale_new (GTK_ORIENTATION_HORIZONTAL, adjustment);
  assert (uiplayer->volumeScale != NULL);
  gtk_widget_set_size_request (uiplayer->volumeScale, 200, 5);
  gtk_scale_set_draw_value (GTK_SCALE (uiplayer->volumeScale), FALSE);
  gtk_scale_set_has_origin (GTK_SCALE (uiplayer->volumeScale), TRUE);
  gtk_range_set_value (GTK_RANGE (uiplayer->volumeScale), 0.0);
  uiutilsSetCss (uiplayer->volumeScale,
      "scale, trough { min-height: 5px; }");
  gtk_box_pack_end (GTK_BOX (hbox), uiplayer->volumeScale,
      FALSE, FALSE, 0);
  gtk_size_group_add_widget (sgC, uiplayer->volumeScale);
  g_signal_connect (uiplayer->volumeScale, "change-value", G_CALLBACK (uiplayerVolumeProcess), uiplayer);

  /* size group D */
  widget = uiutilsCreateColonLabel (_("Volume"));
  gtk_box_pack_end (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
  gtk_widget_set_halign (widget, GTK_ALIGN_END);
  gtk_label_set_xalign (GTK_LABEL (widget), 1.0);
  gtk_size_group_add_widget (sgD, widget);

  logProcEnd (LOG_PROC, "uiplayerActivate", "");
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

  switch (route) {
    case ROUTE_NONE:
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
  g_object_unref (G_OBJECT (uiplayer->stopImg));
  g_object_unref (G_OBJECT (uiplayer->playImg));
  g_object_unref (G_OBJECT (uiplayer->pauseImg));
  g_object_unref (G_OBJECT (uiplayer->ledonImg));
  g_object_unref (G_OBJECT (uiplayer->ledoffImg));
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
    gtk_widget_set_sensitive (uiplayer->volumeScale, FALSE);
    gtk_widget_set_sensitive (uiplayer->seekScale, FALSE);
    gtk_widget_set_sensitive (uiplayer->speedScale, FALSE);
  } else {
    gtk_widget_set_sensitive (uiplayer->volumeScale, TRUE);
    gtk_widget_set_sensitive (uiplayer->seekScale, TRUE);
    gtk_widget_set_sensitive (uiplayer->speedScale, TRUE);
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
  if (uiplayer->repeatImg != NULL) {
    uiplayer->repeatLock = true;
    if (atol (p)) {
      pathbldMakePath (tbuff, sizeof (tbuff), "", "button_repeat", ".svg",
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
    gtk_label_set_label (GTK_LABEL (uiplayer->volumeDisplayLab), p);
    dval = atof (p);
    gtk_range_set_value (GTK_RANGE (uiplayer->volumeScale), dval);
  }

  /* speed */
  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  if (! uiplayer->speedLock) {
    snprintf (tbuff, sizeof (tbuff), "%3s", p);
    gtk_label_set_label (GTK_LABEL (uiplayer->speedDisplayLab), p);
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
    gtk_label_set_label (GTK_LABEL (uiplayer->durationLab), tbuff);
    gtk_range_set_range (GTK_RANGE (uiplayer->seekScale), 0.0, ddur);
    uiplayer->lastdur = dur;
  }

  if (! uiplayer->seekLock) {
    tmutilToMS (position, tbuff, sizeof (tbuff));
    gtk_label_set_label (GTK_LABEL (uiplayer->seekDisplayLab), tbuff);

    timeleft = dur - position;
    tmutilToMSD (timeleft, tbuff, sizeof (tbuff));
    gtk_label_set_label (GTK_LABEL (uiplayer->countdownTimerLab), tbuff);

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

  song = dbGetByIdx (dbidx);
  if (song == NULL) {
    logProcEnd (LOG_PROC, "uiplayerProcessMusicqStatusData", "null-song");
    return;
  }

  if (uiplayer->danceLab != NULL) {
    danceIdx = songGetNum (song, TAG_DANCE);
    data = danceGetData (dances, danceIdx, DANCE_DANCE);
    gtk_label_set_label (GTK_LABEL (uiplayer->danceLab), data);
  }

  /* artist */
  if (uiplayer->artistLab != NULL) {
    data = songGetData (song, TAG_ARTIST);
    gtk_label_set_label (GTK_LABEL (uiplayer->artistLab), data);
  }

  /* title */
  if (uiplayer->titleLab != NULL) {
    data = songGetData (song, TAG_TITLE);
    gtk_label_set_label (GTK_LABEL (uiplayer->titleLab), data);
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
  snprintf (tbuff, sizeof (tbuff), "%3.0f", value);
  gtk_label_set_label (GTK_LABEL (uiplayer->speedDisplayLab), tbuff);
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
  position = (ssize_t) round (value);

  tmutilToMS (position, tbuff, sizeof (tbuff));
  gtk_label_set_label (GTK_LABEL (uiplayer->seekDisplayLab), tbuff);

  timeleft = uiplayer->lastdur - position;
  tmutilToMSD (timeleft, tbuff, sizeof (tbuff));
  gtk_label_set_label (GTK_LABEL (uiplayer->countdownTimerLab), tbuff);
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
  snprintf (tbuff, sizeof (tbuff), "%3.0f", value);
  gtk_label_set_label (GTK_LABEL (uiplayer->volumeDisplayLab), tbuff);
  logProcEnd (LOG_PROC, "uiplayerVolumeProcess", "");
  return FALSE;
}

