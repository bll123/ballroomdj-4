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

#include "bdj4intl.h"
#include "bdjvarsdf.h"
#include "conn.h"
#include "dance.h"
#include "ilist.h"
#include "musicdb.h"
#include "pathbld.h"
#include "portability.h"
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

  uiplayer = malloc (sizeof (uiplayer_t));
  assert (uiplayer != NULL);
  uiplayer->progstate = progstate;
  uiplayer->conn = conn;

  progstateSetCallback (uiplayer->progstate, STATE_CONNECTING, uiplayerInitCallback, uiplayer);
  progstateSetCallback (uiplayer->progstate, STATE_CLOSING, uiplayerClosingCallback, uiplayer);

  return uiplayer;
}

void
uiplayerFree (uiplayer_t *uiplayer)
{
  if (uiplayer != NULL) {
    free (uiplayer);
  }
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


  sgA = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  sgB = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  sgC = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  sgD = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  sgE = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  uiplayer->vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  assert (uiplayer->vbox != NULL);
  gtk_widget_set_hexpand (GTK_WIDGET (uiplayer->vbox), TRUE);
  gtk_widget_set_vexpand (GTK_WIDGET (uiplayer->vbox), FALSE);

  /* song display */

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  assert (hbox != NULL);
  gtk_widget_set_hexpand (GTK_WIDGET (hbox), TRUE);
  gtk_box_pack_start (GTK_BOX (uiplayer->vbox), GTK_WIDGET (hbox),
      FALSE, FALSE, 0);

  /* size group E */
  tbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  assert (tbox != NULL);
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (tbox),
      FALSE, FALSE, 0);
  gtk_size_group_add_widget (sgE, GTK_WIDGET (tbox));

  uiplayer->statusImg = gtk_image_new ();
  assert (uiplayer->statusImg != NULL);
  pathbldMakePath (tbuff, sizeof (tbuff), "", "button_stop", ".svg",
      PATHBLD_MP_IMGDIR);
  image = gtk_image_new_from_file (tbuff);
  uiplayer->stopImg = gtk_image_get_pixbuf (GTK_IMAGE (image));
  g_object_ref_sink (G_OBJECT (uiplayer->stopImg));
  gtk_image_set_from_pixbuf (GTK_IMAGE (uiplayer->statusImg), uiplayer->stopImg);
  gtk_widget_set_size_request (GTK_WIDGET (uiplayer->statusImg), 18, -1);
  gtk_widget_set_margin_start (GTK_WIDGET (uiplayer->statusImg), 2);
  gtk_box_pack_start (GTK_BOX (tbox), GTK_WIDGET (uiplayer->statusImg),
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
  gtk_widget_set_size_request (GTK_WIDGET (uiplayer->repeatImg), 18, -1);
  gtk_widget_set_margin_start (GTK_WIDGET (uiplayer->repeatImg), 2);
  gtk_box_pack_start (GTK_BOX (tbox), GTK_WIDGET (uiplayer->repeatImg),
      FALSE, FALSE, 0);

  uiplayer->danceLab = gtk_label_new ("");
  assert (uiplayer->danceLab != NULL);
  gtk_widget_set_margin_start (GTK_WIDGET (uiplayer->danceLab), 2);
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (uiplayer->danceLab),
      FALSE, FALSE, 0);

  widget = gtk_label_new (" : ");
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (widget), FALSE, FALSE, 0);

  uiplayer->artistLab = gtk_label_new ("");
  assert (uiplayer->artistLab != NULL);
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (uiplayer->artistLab),
      FALSE, FALSE, 0);

  widget = gtk_label_new (" : ");
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (widget), FALSE, FALSE, 0);

  uiplayer->titleLab = gtk_label_new ("");
  assert (uiplayer->titleLab != NULL);
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (uiplayer->titleLab),
      FALSE, FALSE, 0);

  widget = gtk_label_new ("");
  assert (widget != NULL);
  gtk_widget_set_hexpand (GTK_WIDGET (widget), TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (widget), FALSE, FALSE, 0);

  /* size group A */
  widget = gtk_label_new ("%");
  gtk_box_pack_end (GTK_BOX (hbox), GTK_WIDGET (widget), FALSE, FALSE, 0);
  gtk_widget_set_margin_start (GTK_WIDGET (widget), 1);
  gtk_size_group_add_widget (sgA, GTK_WIDGET (widget));

  /* size group B */
  uiplayer->speedDisplayLab = gtk_label_new ("100");
  assert (uiplayer->speedDisplayLab != NULL);
  gtk_widget_set_size_request (GTK_WIDGET (uiplayer->speedDisplayLab), 24, -1);
  gtk_widget_set_halign (GTK_WIDGET (uiplayer->speedDisplayLab), GTK_ALIGN_END);
  gtk_label_set_xalign (GTK_LABEL (uiplayer->speedDisplayLab), 1.0);
  gtk_box_pack_end (GTK_BOX (hbox), GTK_WIDGET (uiplayer->speedDisplayLab),
      FALSE, FALSE, 0);
  gtk_size_group_add_widget (sgB, GTK_WIDGET (uiplayer->speedDisplayLab));

  /* size group C */
  adjustment = gtk_adjustment_new (0.0, 70.0, 130.0, 0.1, 1.0, 0.0);
  uiplayer->speedScale = gtk_scale_new (GTK_ORIENTATION_HORIZONTAL, adjustment);
  assert (uiplayer->speedScale != NULL);
  gtk_widget_set_size_request (GTK_WIDGET (uiplayer->speedScale), 200, 5);
  gtk_scale_set_draw_value (GTK_SCALE (uiplayer->speedScale), FALSE);
  gtk_scale_set_has_origin (GTK_SCALE (uiplayer->speedScale), TRUE);
  gtk_range_set_value (GTK_RANGE (uiplayer->speedScale), 100.0);
  uiutilsSetCss (GTK_WIDGET (uiplayer->speedScale),
      "scale, trough { min-height: 5px; }");
  gtk_box_pack_end (GTK_BOX (hbox), GTK_WIDGET (uiplayer->speedScale),
      FALSE, FALSE, 0);
  gtk_size_group_add_widget (sgC, GTK_WIDGET (uiplayer->speedScale));
  g_signal_connect (uiplayer->speedScale, "change-value", G_CALLBACK (uiplayerSpeedProcess), uiplayer);

  /* size group D */
  widget = gtk_label_new ("Speed:");
  gtk_box_pack_end (GTK_BOX (hbox), GTK_WIDGET (widget), FALSE, FALSE, 0);
  gtk_widget_set_halign (GTK_WIDGET (widget), GTK_ALIGN_END);
  gtk_label_set_xalign (GTK_LABEL (widget), 1.0);
  gtk_size_group_add_widget (sgD, GTK_WIDGET (widget));

  /* position controls / display */

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  assert (hbox != NULL);
  gtk_widget_set_hexpand (GTK_WIDGET (hbox), TRUE);
  gtk_box_pack_start (GTK_BOX (uiplayer->vbox), GTK_WIDGET (hbox),
      FALSE, FALSE, 0);

  /* size group E */
  widget = gtk_label_new ("");
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (widget), FALSE, FALSE, 0);
  gtk_size_group_add_widget (sgE, GTK_WIDGET (widget));

  uiplayer->countdownTimerLab = gtk_label_new (" 0:00");
  assert (uiplayer->countdownTimerLab != NULL);
  gtk_widget_set_size_request (GTK_WIDGET (uiplayer->countdownTimerLab), 40, -1);
  gtk_widget_set_margin_start (GTK_WIDGET (uiplayer->countdownTimerLab), 2);
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (uiplayer->countdownTimerLab),
      FALSE, FALSE, 0);

  widget = gtk_label_new (" / ");
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (widget), FALSE, FALSE, 0);

  uiplayer->durationLab = gtk_label_new (" 3:00");
  assert (uiplayer->durationLab != NULL);
  gtk_widget_set_size_request (GTK_WIDGET (uiplayer->countdownTimerLab), 40, -1);
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (uiplayer->durationLab),
      FALSE, FALSE, 0);

  widget = gtk_label_new ("");
  assert (widget != NULL);
  gtk_widget_set_hexpand (GTK_WIDGET (widget), TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (widget), FALSE, FALSE, 0);

  /* size group A */
  widget = gtk_label_new ("");
  gtk_box_pack_end (GTK_BOX (hbox), GTK_WIDGET (widget), FALSE, FALSE, 0);
  gtk_size_group_add_widget (sgA, GTK_WIDGET (widget));

  /* size group B */
  widget = gtk_label_new ("");
  gtk_box_pack_end (GTK_BOX (hbox), GTK_WIDGET (widget), FALSE, FALSE, 0);
  gtk_size_group_add_widget (sgB, GTK_WIDGET (widget));

  /* size group C */
  adjustment = gtk_adjustment_new (0.0, 0.0, 180000.0, 100.0, 1000.0, 0.0);
  uiplayer->seekScale = gtk_scale_new (GTK_ORIENTATION_HORIZONTAL, adjustment);
  assert (uiplayer->seekScale != NULL);
  gtk_widget_set_size_request (GTK_WIDGET (uiplayer->seekScale), 200, 5);
  gtk_scale_set_draw_value (GTK_SCALE (uiplayer->seekScale), FALSE);
  gtk_scale_set_has_origin (GTK_SCALE (uiplayer->seekScale), TRUE);
  gtk_range_set_value (GTK_RANGE (uiplayer->seekScale), 0.0);
  uiutilsSetCss (GTK_WIDGET (uiplayer->seekScale),
      "scale, trough { min-height: 5px; }");
  gtk_box_pack_end (GTK_BOX (hbox), GTK_WIDGET (uiplayer->seekScale),
      FALSE, FALSE, 0);
  gtk_size_group_add_widget (sgC, GTK_WIDGET (uiplayer->seekScale));
  g_signal_connect (uiplayer->seekScale, "change-value", G_CALLBACK (uiplayerSeekProcess), uiplayer);

  /* size group D */
  widget = gtk_label_new ("Position:");
  gtk_box_pack_end (GTK_BOX (hbox), GTK_WIDGET (widget), FALSE, FALSE, 0);
  gtk_widget_set_halign (GTK_WIDGET (widget), GTK_ALIGN_END);
  gtk_label_set_xalign (GTK_LABEL (widget), 1.0);
  gtk_size_group_add_widget (sgD, GTK_WIDGET (widget));

  /* main controls */

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  assert (hbox != NULL);
  gtk_widget_set_hexpand (GTK_WIDGET (hbox), TRUE);
  gtk_box_pack_start (GTK_BOX (uiplayer->vbox), GTK_WIDGET (hbox),
      FALSE, FALSE, 0);

  /* size group E */
  widget = gtk_label_new ("");
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (widget), FALSE, FALSE, 0);
  gtk_size_group_add_widget (sgE, GTK_WIDGET (widget));

  widget = gtk_button_new ();
  assert (widget != NULL);
  gtk_button_set_label (GTK_BUTTON (widget), "Fade");
  gtk_widget_set_margin_start (GTK_WIDGET (widget), 2);
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (widget),
      FALSE, FALSE, 0);
  g_signal_connect (widget, "clicked", G_CALLBACK (uiplayerFadeProcess), uiplayer);

  widget = gtk_button_new ();
  assert (widget != NULL);
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (widget),
      FALSE, FALSE, 0);
  pathbldMakePath (tbuff, sizeof (tbuff), "", "button_playpause", ".svg",
      PATHBLD_MP_IMGDIR);
  image = gtk_image_new_from_file (tbuff);
  gtk_button_set_image (GTK_BUTTON (widget), image);
  gtk_widget_set_tooltip_text (widget, "Play / Pause");
  gtk_widget_set_margin_start (GTK_WIDGET (widget), 2);
  g_signal_connect (widget, "clicked", G_CALLBACK (uiplayerPlayPauseProcess), uiplayer);

  uiplayer->repeatButton = gtk_toggle_button_new ();
  assert (uiplayer->repeatButton != NULL);
  pathbldMakePath (tbuff, sizeof (tbuff), "", "button_repeat", ".svg",
      PATHBLD_MP_IMGDIR);
  image = gtk_image_new_from_file (tbuff);
  gtk_button_set_image (GTK_BUTTON (uiplayer->repeatButton), image);
  gtk_widget_set_tooltip_text (uiplayer->repeatButton, "Toggle Repeat");
  gtk_widget_set_margin_start (GTK_WIDGET (uiplayer->repeatButton), 2);
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (uiplayer->repeatButton),
      FALSE, FALSE, 0);
  g_signal_connect (uiplayer->repeatButton, "toggled", G_CALLBACK (uiplayerRepeatProcess), uiplayer);

  widget = gtk_button_new ();
  assert (widget != NULL);
  pathbldMakePath (tbuff, sizeof (tbuff), "", "button_begin", ".svg",
      PATHBLD_MP_IMGDIR);
  image = gtk_image_new_from_file (tbuff);
  gtk_button_set_image (GTK_BUTTON (widget), image);
  gtk_widget_set_tooltip_text (widget, "Return to beginning of song");
  gtk_widget_set_margin_start (GTK_WIDGET (widget), 2);
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (widget), FALSE, FALSE, 0);
  g_signal_connect (widget, "clicked", G_CALLBACK (uiplayerSongBeginProcess), uiplayer);

  widget = gtk_button_new ();
  assert (widget != NULL);
  pathbldMakePath (tbuff, sizeof (tbuff), "", "button_nextsong", ".svg",
      PATHBLD_MP_IMGDIR);
  image = gtk_image_new_from_file (tbuff);
  gtk_button_set_image (GTK_BUTTON (widget), image);
  gtk_widget_set_tooltip_text (widget, "Next Song");
  gtk_widget_set_margin_start (GTK_WIDGET (widget), 2);
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (widget), FALSE, FALSE, 0);
  g_signal_connect (widget, "clicked", G_CALLBACK (uiplayerNextSongProcess), uiplayer);

  uiplayer->pauseatendButton = gtk_toggle_button_new ();
  gtk_button_set_label (GTK_BUTTON (uiplayer->pauseatendButton), "Pause At End");
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
  gtk_widget_set_margin_start (GTK_WIDGET (uiplayer->pauseatendButton), 2);
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (uiplayer->pauseatendButton),
      FALSE, FALSE, 0);
  g_signal_connect (uiplayer->pauseatendButton, "toggled", G_CALLBACK (uiplayerPauseatendProcess), uiplayer);

  /* volume controls / display */

  /* size group A */
  widget = gtk_label_new ("%");
  gtk_box_pack_end (GTK_BOX (hbox), GTK_WIDGET (widget), FALSE, FALSE, 0);
  gtk_widget_set_margin_start (GTK_WIDGET (widget), 1);
  gtk_size_group_add_widget (sgA, GTK_WIDGET (widget));

  /* size group B */
  uiplayer->volumeDisplayLab = gtk_label_new ("100");
  assert (uiplayer->volumeDisplayLab != NULL);
  gtk_widget_set_size_request (GTK_WIDGET (uiplayer->volumeDisplayLab), 24, -1);
  gtk_widget_set_halign (GTK_WIDGET (uiplayer->volumeDisplayLab), GTK_ALIGN_END);
  gtk_label_set_xalign (GTK_LABEL (uiplayer->volumeDisplayLab), 1.0);
  gtk_box_pack_end (GTK_BOX (hbox), GTK_WIDGET (uiplayer->volumeDisplayLab),
      FALSE, FALSE, 0);
  gtk_size_group_add_widget (sgB, GTK_WIDGET (uiplayer->volumeDisplayLab));

  /* size group C */
  adjustment = gtk_adjustment_new (0.0, 0.0, 100.0, 0.1, 1.0, 0.0);
  uiplayer->volumeScale = gtk_scale_new (GTK_ORIENTATION_HORIZONTAL, adjustment);
  assert (uiplayer->volumeScale != NULL);
  gtk_widget_set_size_request (GTK_WIDGET (uiplayer->volumeScale), 200, 5);
  gtk_scale_set_draw_value (GTK_SCALE (uiplayer->volumeScale), FALSE);
  gtk_scale_set_has_origin (GTK_SCALE (uiplayer->volumeScale), TRUE);
  gtk_range_set_value (GTK_RANGE (uiplayer->volumeScale), 0.0);
  uiutilsSetCss (GTK_WIDGET (uiplayer->volumeScale),
      "scale, trough { min-height: 5px; }");
  gtk_box_pack_end (GTK_BOX (hbox), GTK_WIDGET (uiplayer->volumeScale),
      FALSE, FALSE, 0);
  gtk_size_group_add_widget (sgC, GTK_WIDGET (uiplayer->volumeScale));
  g_signal_connect (uiplayer->volumeScale, "change-value", G_CALLBACK (uiplayerVolumeProcess), uiplayer);

  /* size group D */
  widget = gtk_label_new ("Volume:");
  gtk_box_pack_end (GTK_BOX (hbox), GTK_WIDGET (widget), FALSE, FALSE, 0);
  gtk_widget_set_halign (GTK_WIDGET (widget), GTK_ALIGN_END);
  gtk_label_set_xalign (GTK_LABEL (widget), 1.0);
  gtk_size_group_add_widget (sgD, GTK_WIDGET (widget));

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

  return 0;
}

/* internal routines */

static bool
uiplayerInitCallback (void *udata, programstate_t programState)
{
  uiplayer_t *uiplayer = udata;

  uiplayer->repeatLock = false;
  uiplayer->pauseatendLock = false;
  uiplayer->lastdur = 180000.0;
  uiplayer->speedLock = false;
  mstimeset (&uiplayer->speedLockTimeout, 3600000);
  mstimeset (&uiplayer->speedLockSend, 3600000);
  uiplayer->seekLock = false;
  mstimeset (&uiplayer->seekLockTimeout, 3600000);
  mstimeset (&uiplayer->seekLockSend, 3600000);
  uiplayer->volumeLock = false;
  mstimeset (&uiplayer->volumeLockTimeout, 3600000);
  mstimeset (&uiplayer->volumeLockSend, 3600000);

  return true;
}

static bool
uiplayerClosingCallback (void *udata, programstate_t programState)
{
  uiplayer_t      *uiplayer = udata;

  g_object_unref (G_OBJECT (uiplayer->stopImg));
  g_object_unref (G_OBJECT (uiplayer->playImg));
  g_object_unref (G_OBJECT (uiplayer->pauseImg));
  g_object_unref (G_OBJECT (uiplayer->ledonImg));
  g_object_unref (G_OBJECT (uiplayer->ledoffImg));
  return true;
}

static void
uiplayerProcessPauseatend (uiplayer_t *uiplayer, int on)
{
  if (uiplayer->pauseatendButton == NULL) {
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
}

static void
uiplayerProcessPlayerState (uiplayer_t *uiplayer, int playerState)
{
  if (uiplayer->statusImg == NULL) {
    return;
  }

  uiplayer->playerState = playerState;

  if (playerState == PL_STATE_IN_FADEOUT) {
    gtk_widget_set_sensitive (GTK_WIDGET (uiplayer->volumeScale), FALSE);
    gtk_widget_set_sensitive (GTK_WIDGET (uiplayer->seekScale), FALSE);
    gtk_widget_set_sensitive (GTK_WIDGET (uiplayer->speedScale), FALSE);
  } else {
    gtk_widget_set_sensitive (GTK_WIDGET (uiplayer->volumeScale), TRUE);
    gtk_widget_set_sensitive (GTK_WIDGET (uiplayer->seekScale), TRUE);
    gtk_widget_set_sensitive (GTK_WIDGET (uiplayer->speedScale), TRUE);
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
}

static void
uiplayerProcessPlayerStatusData (uiplayer_t *uiplayer, char *args)
{
  char          *p;
  char          *tokstr;
  char          tbuff [100];
  double        dval;
  double        ddur;
  GtkAdjustment *adjustment;

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
    tmutilToMSD (atol (p), tbuff, sizeof (tbuff));
    gtk_label_set_label (GTK_LABEL (uiplayer->countdownTimerLab), tbuff);
    dval = atof (p);    // used below
  }

  /* duration */
  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  ddur = atof (p);
  if (ddur > 0.0 && ddur != uiplayer->lastdur) {
    tmutilToMS (atol (p), tbuff, sizeof (tbuff));
    gtk_label_set_label (GTK_LABEL (uiplayer->durationLab), tbuff);
    adjustment = gtk_adjustment_new (0.0, 0.0, ddur, 100.0, 1000.0, 0.0);
    gtk_range_set_adjustment (GTK_RANGE (uiplayer->seekScale), adjustment);
    uiplayer->lastdur = ddur;
  }
  if (! uiplayer->seekLock) {
    if (ddur == 0.0) {
      gtk_range_set_value (GTK_RANGE (uiplayer->seekScale), 0.0);
    } else {
      gtk_range_set_value (GTK_RANGE (uiplayer->seekScale), dval);
    }
  }
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

  dances = bdjvarsdfGet (BDJVDF_DANCES);

  p = strtok_r (args, MSG_ARGS_RS_STR, &tokstr);
  dbidx = atol (p);
  if (dbidx < 0) {
    return;
  }

  song = dbGetByIdx (dbidx);
  if (song == NULL) {
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
}

static void
uiplayerFadeProcess (GtkButton *b, gpointer udata)
{
  uiplayer_t      *uiplayer = udata;

  connSendMessage (uiplayer->conn, ROUTE_PLAYER, MSG_PLAY_FADE, NULL);
}

static void
uiplayerPlayPauseProcess (GtkButton *b, gpointer udata)
{
  uiplayer_t      *uiplayer = udata;

  connSendMessage (uiplayer->conn, ROUTE_MAIN, MSG_PLAY_PLAYPAUSE, NULL);
}

static void
uiplayerRepeatProcess (GtkButton *b, gpointer udata)
{
  uiplayer_t      *uiplayer = udata;

  if (uiplayer->repeatLock) {
    return;
  }

  connSendMessage (uiplayer->conn, ROUTE_PLAYER, MSG_PLAY_REPEAT, NULL);
}

static void
uiplayerSongBeginProcess (GtkButton *b, gpointer udata)
{
  uiplayer_t      *uiplayer = udata;

  connSendMessage (uiplayer->conn, ROUTE_PLAYER, MSG_PLAY_SONG_BEGIN, NULL);
}

static void
uiplayerNextSongProcess (GtkButton *b, gpointer udata)
{
  uiplayer_t      *uiplayer = udata;

  connSendMessage (uiplayer->conn, ROUTE_PLAYER, MSG_PLAY_NEXTSONG, NULL);
}

static void
uiplayerPauseatendProcess (GtkButton *b, gpointer udata)
{
  uiplayer_t      *uiplayer = udata;

  if (uiplayer->pauseatendLock) {
    return;
  }
  connSendMessage (uiplayer->conn, ROUTE_PLAYER, MSG_PLAY_PAUSEATEND, NULL);
}

static gboolean
uiplayerSpeedProcess (GtkRange *range, GtkScrollType *scroll, gdouble value, gpointer udata)
{
  uiplayer_t    *uiplayer = udata;
  char          tbuff [40];

  if (! uiplayer->speedLock) {
    mstimeset (&uiplayer->speedLockSend, UIPLAYER_LOCK_TIME_SEND);
  }
  uiplayer->speedLock = true;
  mstimeset (&uiplayer->speedLockTimeout, UIPLAYER_LOCK_TIME_WAIT);
  snprintf (tbuff, sizeof (tbuff), "%3.0f", value);
  gtk_label_set_label (GTK_LABEL (uiplayer->speedDisplayLab), tbuff);
  return FALSE;
}

static gboolean
uiplayerSeekProcess (GtkRange *range, GtkScrollType *scroll, gdouble value, gpointer udata)
{
  uiplayer_t    *uiplayer = udata;
  char          tbuff [40];
  ssize_t       position;

  if (! uiplayer->seekLock) {
    mstimeset (&uiplayer->seekLockSend, UIPLAYER_LOCK_TIME_SEND);
  }
  uiplayer->seekLock = true;
  mstimeset (&uiplayer->seekLockTimeout, UIPLAYER_LOCK_TIME_WAIT);
  position = (ssize_t) round (value);
  tmutilToMSD (position, tbuff, sizeof (tbuff));
  gtk_label_set_label (GTK_LABEL (uiplayer->countdownTimerLab), tbuff);
  return FALSE;
}

static gboolean
uiplayerVolumeProcess (GtkRange *range, GtkScrollType *scroll, gdouble value, gpointer udata)
{
  uiplayer_t    *uiplayer = udata;
  char          tbuff [40];

  if (! uiplayer->volumeLock) {
    mstimeset (&uiplayer->volumeLockSend, UIPLAYER_LOCK_TIME_SEND);
  }
  uiplayer->volumeLock = true;
  mstimeset (&uiplayer->volumeLockTimeout, UIPLAYER_LOCK_TIME_WAIT);
  snprintf (tbuff, sizeof (tbuff), "%3.0f", value);
  gtk_label_set_label (GTK_LABEL (uiplayer->volumeDisplayLab), tbuff);
  return FALSE;
}

