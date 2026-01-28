/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <math.h>

#include "audiosrc.h"
#include "bdj4.h"
#include "bdj4intl.h"
#include "bdj4ui.h"
#include "bdjopt.h"
#include "bdjvarsdf.h"
#include "callback.h"
#include "conn.h"
#include "controller.h"
#include "dance.h"
#include "dispsel.h"
#include "genre.h"
#include "ilist.h"
#include "log.h"
#include "mdebug.h"
#include "msgparse.h"
#include "musicdb.h"
#include "pathbld.h"
#include "player.h"
#include "pli.h"
#include "progstate.h"
#include "slist.h"
#include "song.h"
#include "tagdef.h"
#include "ui.h"
#include "uiplayer.h"
#include "uiutils.h"

enum {
  /* for volume and speed */
  UIPLAYER_LOCK_TIME_WAIT = 100,
  UIPLAYER_LOCK_TIME_SEND = 20,
  /* there are all sorts of latency issues making the seek work nicely */
  /* it will take at least 100ms and at most 200ms for the message to get */
  /* back.  Then there are the latency issues on this end. */
  UIPLAYER_SEEK_LOCK_TIME_WAIT = 300,
  UIPLAYER_SEEK_LOCK_TIME_SEND = 50,
};

enum {
  UIPL_CB_FADE,
  UIPL_CB_PLAYPAUSE,
  UIPL_CB_BEGSONG,
  UIPL_CB_NEXTSONG,
  UIPL_CB_PAUSEATEND,
  UIPL_CB_REPEAT,
  UIPL_CB_SPEED,
  UIPL_CB_SEEK,
  UIPL_CB_VOLUME,
  UIPL_CB_SPD_RESET,
  UIPL_CB_MAX,
};

enum {
  UIPL_IMG_STATUS,
  UIPL_IMG_REPEAT,
  UIPL_PIX_PLAY,
  UIPL_PIX_STOP,
  UIPL_PIX_PAUSE,
  UIPL_PIX_TIMER,
  UIPL_PIX_REPEAT,
  UIPL_IMG_LED_OFF,
  UIPL_IMG_LED_ON,
  UIPL_IMG_MAX,
};

enum {
  UIPL_W_BUTTON_FADE,
  UIPL_W_BUTTON_PLAYPAUSE,
  UIPL_W_BUTTON_BEGSONG,
  UIPL_W_BUTTON_NEXTSONG,
  UIPL_W_BUTTON_SPD_RESET,
  UIPL_W_MAIN_VBOX,
  /* the info display must have enough labels for both the display */
  /* and the separators */
  UIPL_W_INFO_DISP_A,
  UIPL_W_INFO_DISP_B,   //
  UIPL_W_INFO_DISP_C,
  UIPL_W_INFO_DISP_D,   //
  UIPL_W_INFO_DISP_E,
  UIPL_W_INFO_DISP_F,   //
  UIPL_W_INFO_DISP_G,
  UIPL_W_INFO_DISP_H,   //
  UIPL_W_INFO_DISP_I,
  UIPL_W_SPEED,
  UIPL_W_SPEED_DISP,
  UIPL_W_COUNTDOWN_TIMER,
  UIPL_W_DURATION,
  UIPL_W_SEEK,
  UIPL_W_SEEK_DISP,
  UIPL_W_REPEAT_B,
  UIPL_W_SONG_BEGIN_B,
  UIPL_W_PAUSE_AT_END_B,
  UIPL_W_VOLUME,
  UIPL_W_VOLUME_DISP,
  UIPL_W_MAX,
};


typedef struct uiplayer {
  const char      *tag;
  progstate_t     *progstate;
  conn_t          *conn;
  playerstate_t   playerState;
  musicdb_t       *musicdb;
  callback_t      *callbacks [UIPL_CB_MAX];
  dbidx_t         curr_dbidx;
  uiwcont_t       *wcont [UIPL_W_MAX];
  int             pliSupported;
  dispsel_t       *dispsel;
  controller_t    *controller;
  /* song display */
  uiwcont_t       *images [UIPL_IMG_MAX];
  /* speed controls / display */
  mstime_t        speedLockTimeout;
  mstime_t        speedLockSend;
  double          speedLastValue;
  /* volume controls / display */
  mstime_t        volumeLockTimeout;
  mstime_t        volumeLockSend;
  double          volumeLastValue;
  int             baseVolume;
  /* position controls / display */
  ssize_t         lastdur;
  mstime_t        seekLockTimeout;
  mstime_t        seekLockSend;
  double          seekLastValue;
  bool            seekLock;
  /* speed controls / display */
  bool            speedLock;
  /* main controls */
  bool            repeatLock;
  bool            pauseatendLock;
  bool            pauseatendstate;
  /* volume controls */
  bool            volumeLock;
  /* display */
  bool            uibuilt;
  /* speed/seek enabled */
  bool            speeddisabled;
  bool            seekdisabled;
  bool            repeat;
} uiplayer_t;

static bool  uiplayerInitCallback (void *udata, programstate_t programState);
static bool  uiplayerClosingCallback (void *udata, programstate_t programState);

static void     uiplayerProcessPauseatend (uiplayer_t *uiplayer, int on);
static void     uiplayerProcessPlayerStateMsg (uiplayer_t *uiplayer, char *data);
static void     uiplayerProcessPlayerState (uiplayer_t *uiplayer);
static void     uiplayerProcessPlayerStatusData (uiplayer_t *uiplayer, char *args);
static void     uiplayerProcessMusicqStatusData (uiplayer_t *uiplayer, char *args);
static bool     uiplayerFadeProcess (void *udata);
static bool     uiplayerPlayPauseProcess (void *udata);
static bool     uiplayerRepeatCallback (void *udata);
static bool     uiplayerSongBeginProcess (void *udata);
static bool     uiplayerNextSongProcess (void *udata);
static bool     uiplayerPauseatendCallback (void *udata);
static bool     uiplayerSpeedCallback (void *udata, double value);
static bool     uiplayerSeekCallback (void *udata, double value);
static bool     uiplayerVolumeCallback (void *udata, double value);
static void     uiplayerClearDisplay (uiplayer_t *uiplayer);
static bool     uiplayerSpdResetCallback (void *udata);

uiplayer_t *
uiplayerInit (const char *tag, progstate_t *progstate,
    conn_t *conn, musicdb_t *musicdb, dispsel_t *dispsel)
{
  uiplayer_t    *uiplayer;

  logProcBegin ();
  uiplayer = mdmalloc (sizeof (uiplayer_t));
  uiplayer->tag = tag;
  uiplayer->progstate = progstate;
  uiplayer->conn = conn;
  uiplayer->musicdb = musicdb;
  uiplayer->dispsel = dispsel;
  uiplayer->controller = NULL;
  for (int i = 0; i < UIPL_CB_MAX; ++i) {
    uiplayer->callbacks [i] = NULL;
  }
  uiplayer->curr_dbidx = -1;
  for (int i = 0; i < UIPL_W_MAX; ++i) {
    uiplayer->wcont [i] = NULL;
  }
  for (int i = 0; i < UIPL_IMG_MAX; ++i) {
    uiplayer->images [i] = NULL;
  }

  uiplayer->uibuilt = false;
  uiplayer->repeat = false;

  progstateSetCallback (uiplayer->progstate, PROGSTATE_CONNECTING, uiplayerInitCallback, uiplayer);
  progstateSetCallback (uiplayer->progstate, PROGSTATE_CLOSING, uiplayerClosingCallback, uiplayer);

  logProcEnd ("");
  return uiplayer;
}

void
uiplayerSetDatabase (uiplayer_t *uiplayer, musicdb_t *musicdb)
{
  if (uiplayer == NULL) {
    return;
  }

  uiplayer->musicdb = musicdb;
}

void
uiplayerSetController (uiplayer_t *uiplayer, controller_t *controller)
{
  if (uiplayer == NULL) {
    return;
  }

  uiplayer->controller = controller;
}

void
uiplayerFree (uiplayer_t *uiplayer)
{
  if (uiplayer == NULL) {
    return;
  }

  logProcBegin ();

  for (int i = 0; i < UIPL_CB_MAX; ++i) {
    callbackFree (uiplayer->callbacks [i]);
  }
  for (int i = 0; i < UIPL_IMG_MAX; ++i) {
    uiwcontFree (uiplayer->images [i]);
  }
  for (int i = 0; i < UIPL_W_MAX; ++i) {
    uiwcontFree (uiplayer->wcont [i]);
  }
  mdfree (uiplayer);
  logProcEnd ("");
}

uiwcont_t *
uiplayerBuildUI (uiplayer_t *uiplayer)
{
  char            tbuff [BDJ4_PATH_MAX];
  uiwcont_t       *uiwidgetp = NULL;
  uiwcont_t       *hbox = NULL;
  uiwcont_t       *statusbox = NULL;
  uiwcont_t       *szgrpScalePerc = NULL;
  uiwcont_t       *szgrpScaleDisp = NULL;
  uiwcont_t       *szgrpScale = NULL;
  uiwcont_t       *szgrpScaleLabel = NULL;
  uiwcont_t       *szgrpScaleButton = NULL;
  uiwcont_t       *szgrpStatus = NULL;
  bool            showspd = false;

  if (uiplayer == NULL) {
    return NULL;
  }

  logProcBegin ();

  showspd = bdjoptGetNum (OPT_P_SHOW_SPD_CONTROL);
  if (showspd) {
    szgrpScaleButton = uiCreateSizeGroupHoriz ();
  }
  szgrpScalePerc = uiCreateSizeGroupHoriz ();
  szgrpScaleDisp = uiCreateSizeGroupHoriz ();
  szgrpScale = uiCreateSizeGroupHoriz ();
  szgrpScaleLabel = uiCreateSizeGroupHoriz ();
  szgrpStatus = uiCreateSizeGroupHoriz ();

  uiplayer->wcont [UIPL_W_MAIN_VBOX] = uiCreateVertBox (NULL);
  uiWidgetExpandHoriz (uiplayer->wcont [UIPL_W_MAIN_VBOX]);

  /* song display */

  hbox = uiCreateHorizBox (NULL);
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (uiplayer->wcont [UIPL_W_MAIN_VBOX], hbox);

  /* size group E */
  statusbox = uiCreateHorizBox (NULL);
  uiBoxPackStart (hbox, statusbox);
  uiWidgetAlignVertCenter (statusbox);
  uiSizeGroupAdd (szgrpStatus, statusbox);

  uiplayer->images [UIPL_IMG_STATUS] = uiImageNew ();

  pathbldMakePath (tbuff, sizeof (tbuff), "button_stop", BDJ4_IMG_SVG_EXT,
      PATHBLD_MP_DREL_IMG | PATHBLD_MP_USEIDX);
  uiplayer->images [UIPL_PIX_STOP] = uiImageFromFile (tbuff);
  uiImageConvertToPixbuf (uiplayer->images [UIPL_PIX_STOP]);
  uiWidgetMakePersistent (uiplayer->images [UIPL_PIX_STOP]);

  uiImageSetFromPixbuf (uiplayer->images [UIPL_IMG_STATUS], uiplayer->images [UIPL_PIX_STOP]);
  uiWidgetSetSizeRequest (uiplayer->images [UIPL_IMG_STATUS], 18, -1);
  uiBoxPackStart (statusbox, uiplayer->images [UIPL_IMG_STATUS]);
  uiWidgetAlignHorizCenter (uiplayer->images [UIPL_IMG_STATUS]);
  uiWidgetAlignVertCenter (uiplayer->images [UIPL_IMG_STATUS]);
  uiWidgetSetMarginStart (uiplayer->images [UIPL_IMG_STATUS], 1);

  pathbldMakePath (tbuff, sizeof (tbuff), "button_play", BDJ4_IMG_SVG_EXT,
      PATHBLD_MP_DREL_IMG | PATHBLD_MP_USEIDX);
  uiplayer->images [UIPL_PIX_PLAY] = uiImageFromFile (tbuff);
  uiImageConvertToPixbuf (uiplayer->images [UIPL_PIX_PLAY]);
  uiWidgetMakePersistent (uiplayer->images [UIPL_PIX_PLAY]);

  pathbldMakePath (tbuff, sizeof (tbuff), "button_pause", BDJ4_IMG_SVG_EXT,
      PATHBLD_MP_DREL_IMG | PATHBLD_MP_USEIDX);
  uiplayer->images [UIPL_PIX_PAUSE] = uiImageFromFile (tbuff);
  uiImageConvertToPixbuf (uiplayer->images [UIPL_PIX_PAUSE]);
  uiWidgetMakePersistent (uiplayer->images [UIPL_PIX_PAUSE]);

  pathbldMakePath (tbuff, sizeof (tbuff), "indicator_timer", BDJ4_IMG_SVG_EXT,
      PATHBLD_MP_DREL_IMG | PATHBLD_MP_USEIDX);
  uiplayer->images [UIPL_PIX_TIMER] = uiImageFromFile (tbuff);
  uiImageConvertToPixbuf (uiplayer->images [UIPL_PIX_TIMER]);
  uiWidgetMakePersistent (uiplayer->images [UIPL_PIX_TIMER]);

  pathbldMakePath (tbuff, sizeof (tbuff), "button_repeat", BDJ4_IMG_SVG_EXT,
      PATHBLD_MP_DREL_IMG | PATHBLD_MP_USEIDX);
  uiplayer->images [UIPL_PIX_REPEAT] = uiImageFromFile (tbuff);
  uiImageConvertToPixbuf (uiplayer->images [UIPL_PIX_REPEAT]);
  uiWidgetMakePersistent (uiplayer->images [UIPL_PIX_REPEAT]);

  uiplayer->images [UIPL_IMG_REPEAT] = uiImageNew ();
  uiImageClear (uiplayer->images [UIPL_IMG_REPEAT]);
  uiWidgetSetSizeRequest (uiplayer->images [UIPL_IMG_REPEAT], 18, -1);
  uiBoxPackStart (statusbox, uiplayer->images [UIPL_IMG_REPEAT]);
  uiWidgetAlignHorizCenter (uiplayer->images [UIPL_IMG_REPEAT]);
  uiWidgetAlignVertCenter (uiplayer->images [UIPL_IMG_REPEAT]);
  uiWidgetSetMarginStart (uiplayer->images [UIPL_IMG_REPEAT], 1);

  for (int i = UIPL_W_INFO_DISP_A; i <= UIPL_W_INFO_DISP_I; ++i) {
    uiwidgetp = uiCreateLabel ("");
    if (i == UIPL_W_INFO_DISP_A) {
      uiWidgetAddClass (uiwidgetp, ACCENT_CLASS);
    }
    if ((i - UIPL_W_INFO_DISP_A) % 2 == 0) {
      uiLabelEllipsizeOn (uiwidgetp);
    }
    uiBoxPackStart (hbox, uiwidgetp);
    uiWidgetAlignVertBaseline (uiwidgetp);
    uiWidgetAlignHorizStart (uiwidgetp);
    uiplayer->wcont [i] = uiwidgetp;
  }

  /* expanding label to take space */
  uiwidgetp = uiCreateLabel ("");
  uiBoxPackStart (hbox, uiwidgetp);
  uiWidgetExpandHoriz (uiwidgetp);
  uiwcontFree (uiwidgetp);

  if (showspd) {
    /* size group F */
    uiplayer->callbacks [UIPL_CB_SPD_RESET] = callbackInit (
        uiplayerSpdResetCallback, uiplayer, "spd-reset");
    uiwidgetp = uiCreateButton ("b-plui-spd-reset",
        uiplayer->callbacks [UIPL_CB_SPD_RESET],
        /* CONTEXT: playerui: button: reset speed to 100% */
        _("100%"), NULL);
    uiWidgetAddClass (uiwidgetp, "bdj-spd-reset");
    uiBoxPackEnd (hbox, uiwidgetp);
    uiSizeGroupAdd (szgrpScaleButton, uiwidgetp);
    uiplayer->wcont [UIPL_W_BUTTON_SPD_RESET] = uiwidgetp;
  }

  /* size group A */
  uiwidgetp = uiCreateLabel ("%");
  uiBoxPackEnd (hbox, uiwidgetp);
  uiSizeGroupAdd (szgrpScalePerc, uiwidgetp);
  uiwcontFree (uiwidgetp);

  /* size group B */
  uiplayer->wcont [UIPL_W_SPEED_DISP] = uiCreateLabel ("100");
  uiBoxPackEnd (hbox, uiplayer->wcont [UIPL_W_SPEED_DISP]);
  uiLabelAlignEnd (uiplayer->wcont [UIPL_W_SPEED_DISP]);
  uiSizeGroupAdd (szgrpScaleDisp, uiplayer->wcont [UIPL_W_SPEED_DISP]);

  /* size group C */
  uiplayer->wcont [UIPL_W_SPEED] = uiCreateScale (
      SPD_LOWER, SPD_UPPER, SPD_INCA, SPD_INCB, 100.0, SPD_DIGITS);
  uiBoxPackEnd (hbox, uiplayer->wcont [UIPL_W_SPEED]);
  uiWidgetSetMarginStart (uiplayer->wcont [UIPL_W_SPEED], 2);
  uiSizeGroupAdd (szgrpScale, uiplayer->wcont [UIPL_W_SPEED]);
  uiplayer->callbacks [UIPL_CB_SPEED] = callbackInitD (
      uiplayerSpeedCallback, uiplayer);
  uiScaleSetCallback (uiplayer->wcont [UIPL_W_SPEED], uiplayer->callbacks [UIPL_CB_SPEED]);

  /* size group D */
  /* CONTEXT: playerui: the current speed for song playback */
  uiwidgetp = uiCreateColonLabel (_("Speed"));
  uiLabelAlignEnd (uiwidgetp);
  uiBoxPackEnd (hbox, uiwidgetp);
  uiWidgetSetMarginEnd (uiwidgetp, 1);
  uiSizeGroupAdd (szgrpScaleLabel, uiwidgetp);
  uiwcontFree (uiwidgetp);

  /* position controls / display */

  uiwcontFree (hbox);
  hbox = uiCreateHorizBox (NULL);
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (uiplayer->wcont [UIPL_W_MAIN_VBOX], hbox);

  /* size group E */
  uiwidgetp = uiCreateLabel ("");
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (szgrpStatus, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiplayer->wcont [UIPL_W_COUNTDOWN_TIMER] = uiCreateLabel (" 0:00");
  uiBoxPackStart (hbox, uiplayer->wcont [UIPL_W_COUNTDOWN_TIMER]);
  uiLabelAlignEnd (uiplayer->wcont [UIPL_W_COUNTDOWN_TIMER]);

  uiwidgetp = uiCreateLabel (" / ");
  uiBoxPackStart (hbox, uiwidgetp);
  uiWidgetSetMarginStart (uiwidgetp, 0);
  uiwcontFree (uiwidgetp);

  uiplayer->wcont [UIPL_W_DURATION] = uiCreateLabel (" 0:00");
  uiBoxPackStart (hbox, uiplayer->wcont [UIPL_W_DURATION]);
  uiLabelAlignEnd (uiplayer->wcont [UIPL_W_DURATION]);

  uiwidgetp = uiCreateLabel ("");
  uiBoxPackStart (hbox, uiwidgetp);
  uiWidgetExpandHoriz (uiwidgetp);
  uiwcontFree (uiwidgetp);

  if (showspd) {
    /* size group F */
    uiwidgetp = uiCreateLabel ("");
    uiBoxPackEnd (hbox, uiwidgetp);
    uiSizeGroupAdd (szgrpScaleButton, uiwidgetp);
    uiwcontFree (uiwidgetp);
  }

  /* size group A */
  uiwidgetp = uiCreateLabel ("");
  uiBoxPackEnd (hbox, uiwidgetp);
  uiSizeGroupAdd (szgrpScalePerc, uiwidgetp);
  uiwcontFree (uiwidgetp);

  /* size group B */
  uiplayer->wcont [UIPL_W_SEEK_DISP] = uiCreateLabel ("0:00");
  uiBoxPackEnd (hbox, uiplayer->wcont [UIPL_W_SEEK_DISP]);
  uiSizeGroupAdd (szgrpScaleDisp, uiplayer->wcont [UIPL_W_SEEK_DISP]);
  uiLabelAlignEnd (uiplayer->wcont [UIPL_W_SEEK_DISP]);

  /* size group C */
  uiplayer->wcont [UIPL_W_SEEK] = uiCreateScale (
      0.0, 180000.0, 1000.0, 10000.0, 0.0, 0);
  uiBoxPackEnd (hbox, uiplayer->wcont [UIPL_W_SEEK]);
  uiWidgetSetMarginStart (uiplayer->wcont [UIPL_W_SEEK], 2);
  uiSizeGroupAdd (szgrpScale, uiplayer->wcont [UIPL_W_SEEK]);
  uiplayer->callbacks [UIPL_CB_SEEK] = callbackInitD (
      uiplayerSeekCallback, uiplayer);
  uiScaleSetCallback (uiplayer->wcont [UIPL_W_SEEK], uiplayer->callbacks [UIPL_CB_SEEK]);

  /* size group D */
  /* CONTEXT: playerui: the current position of the song during song playback */
  uiwidgetp = uiCreateColonLabel (_("Position"));
  uiBoxPackEnd (hbox, uiwidgetp);
  uiLabelAlignEnd (uiwidgetp);
  uiWidgetSetMarginEnd (uiwidgetp, 1);
  uiSizeGroupAdd (szgrpScaleLabel, uiwidgetp);
  uiwcontFree (uiwidgetp);

  /* main controls */

  uiwcontFree (hbox);
  hbox = uiCreateHorizBox (NULL);
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (uiplayer->wcont [UIPL_W_MAIN_VBOX], hbox);

  /* size group E */
  uiwidgetp = uiCreateLabel ("");
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (szgrpStatus, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiplayer->callbacks [UIPL_CB_FADE] = callbackInit (
      uiplayerFadeProcess, uiplayer, "fade");
  uiwidgetp = uiCreateButton ("b-plui-fade",
      uiplayer->callbacks [UIPL_CB_FADE],
      /* CONTEXT: playerui: button: fade out the song and stop playing it */
      _("Fade"), NULL);
  uiBoxPackStart (hbox, uiwidgetp);
  uiplayer->wcont [UIPL_W_BUTTON_FADE] = uiwidgetp;

  uiplayer->callbacks [UIPL_CB_PLAYPAUSE] = callbackInit (
      uiplayerPlayPauseProcess, uiplayer, "play-pause");
  uiwidgetp = uiCreateButton ("b-plui-plpause",
      uiplayer->callbacks [UIPL_CB_PLAYPAUSE],
      /* CONTEXT: playerui: button: tooltip: play or pause the song */
      _("Play / Pause"), "button_playpause");
  uiBoxPackStart (hbox, uiwidgetp);
  uiplayer->wcont [UIPL_W_BUTTON_PLAYPAUSE] = uiwidgetp;

  pathbldMakePath (tbuff, sizeof (tbuff), "button_repeat", BDJ4_IMG_SVG_EXT,
      PATHBLD_MP_DREL_IMG | PATHBLD_MP_USEIDX);
  uiplayer->callbacks [UIPL_CB_REPEAT] = callbackInit (
      uiplayerRepeatCallback, uiplayer, "repeat");
  uiplayer->wcont [UIPL_W_REPEAT_B] = uiCreateToggleButton (NULL, tbuff,
      /* CONTEXT: playerui: button: tooltip: toggle the repeat song on and off */
      _("Toggle Repeat"), NULL, 0);
  uiBoxPackStart (hbox, uiplayer->wcont [UIPL_W_REPEAT_B]);
  uiToggleButtonSetCallback (uiplayer->wcont [UIPL_W_REPEAT_B],
      uiplayer->callbacks [UIPL_CB_REPEAT]);

  uiplayer->callbacks [UIPL_CB_BEGSONG] = callbackInit (
      uiplayerSongBeginProcess, uiplayer, "begin-song");
  uiwidgetp = uiCreateButton ("b-plui-begin",
      uiplayer->callbacks [UIPL_CB_BEGSONG],
      /* CONTEXT: playerui: button: tooltip: return to the beginning of the song */
      _("Return to beginning of song"), "button_begin");
  uiBoxPackStart (hbox, uiwidgetp);
  uiplayer->wcont [UIPL_W_BUTTON_BEGSONG] = uiwidgetp;

  uiplayer->callbacks [UIPL_CB_NEXTSONG] = callbackInit (
      uiplayerNextSongProcess, uiplayer, "next-song");
  uiwidgetp = uiCreateButton ("b-plui-next",
      uiplayer->callbacks [UIPL_CB_NEXTSONG],
      /* CONTEXT: playerui: button: tooltip: start playing the next song (immediate) */
      _("Next Song"), "button_nextsong");
  uiBoxPackStart (hbox, uiwidgetp);
  uiplayer->wcont [UIPL_W_BUTTON_NEXTSONG] = uiwidgetp;

  pathbldMakePath (tbuff, sizeof (tbuff), "led_on", BDJ4_IMG_SVG_EXT,
      PATHBLD_MP_DIR_IMG);
  uiplayer->images [UIPL_IMG_LED_ON] = uiImageFromFile (tbuff);
  uiWidgetMakePersistent (uiplayer->images [UIPL_IMG_LED_ON]);

  pathbldMakePath (tbuff, sizeof (tbuff), "led_off", BDJ4_IMG_SVG_EXT,
      PATHBLD_MP_DIR_IMG);
  uiplayer->images [UIPL_IMG_LED_OFF] = uiImageFromFile (tbuff);
  uiWidgetMakePersistent (uiplayer->images [UIPL_IMG_LED_OFF]);

  uiplayer->callbacks [UIPL_CB_PAUSEATEND] = callbackInit (
      uiplayerPauseatendCallback, uiplayer, "pause-at-end");
  uiplayer->wcont [UIPL_W_PAUSE_AT_END_B] = uiCreateToggleButton (
      /* CONTEXT: playerui: button: pause at the end of the song (toggle) */
      _("Pause at End"), NULL, NULL, uiplayer->images [UIPL_IMG_LED_OFF], 0);
  uiBoxPackStart (hbox, uiplayer->wcont [UIPL_W_PAUSE_AT_END_B]);
  uiToggleButtonSetCallback (uiplayer->wcont [UIPL_W_PAUSE_AT_END_B],
      uiplayer->callbacks [UIPL_CB_PAUSEATEND]);

  /* volume controls / display */

  if (showspd) {
    /* size group F */
    uiwidgetp = uiCreateLabel ("");
    uiBoxPackEnd (hbox, uiwidgetp);
    uiSizeGroupAdd (szgrpScaleButton, uiwidgetp);
    uiwcontFree (uiwidgetp);
  }

  /* size group A */
  uiwidgetp = uiCreateLabel ("%");
  uiBoxPackEnd (hbox, uiwidgetp);
  uiSizeGroupAdd (szgrpScalePerc, uiwidgetp);
  uiwcontFree (uiwidgetp);

  /* size group B */
  uiplayer->wcont [UIPL_W_VOLUME_DISP] = uiCreateLabel ("100");
  uiLabelAlignEnd (uiplayer->wcont [UIPL_W_VOLUME_DISP]);
  uiBoxPackEnd (hbox, uiplayer->wcont [UIPL_W_VOLUME_DISP]);
  uiSizeGroupAdd (szgrpScaleDisp, uiplayer->wcont [UIPL_W_VOLUME_DISP]);

  /* size group C */
  uiplayer->wcont [UIPL_W_VOLUME] = uiCreateScale (
      VOL_LOWER, VOL_UPPER, VOL_INCA, VOL_INCB, 0.0, VOL_DIGITS);
  uiBoxPackEnd (hbox, uiplayer->wcont [UIPL_W_VOLUME]);
  uiWidgetSetMarginStart (uiplayer->wcont [UIPL_W_VOLUME], 2);
  uiSizeGroupAdd (szgrpScale, uiplayer->wcont [UIPL_W_VOLUME]);
  uiplayer->callbacks [UIPL_CB_VOLUME] = callbackInitD (
      uiplayerVolumeCallback, uiplayer);
  uiScaleSetCallback (uiplayer->wcont [UIPL_W_VOLUME], uiplayer->callbacks [UIPL_CB_VOLUME]);

  /* size group D */
  /* CONTEXT: playerui: The current volume of the song */
  uiwidgetp = uiCreateColonLabel (_("Volume"));
  uiLabelAlignEnd (uiwidgetp);
  uiBoxPackEnd (hbox, uiwidgetp);
  uiWidgetSetMarginEnd (uiwidgetp, 1);
  uiSizeGroupAdd (szgrpScaleLabel, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiplayer->uibuilt = true;

  uiwcontFree (statusbox);
  uiwcontFree (hbox);
  if (showspd) {
    uiwcontFree (szgrpScaleButton);
  }
  uiwcontFree (szgrpScalePerc);
  uiwcontFree (szgrpScaleDisp);
  uiwcontFree (szgrpScale);
  uiwcontFree (szgrpScaleLabel);
  uiwcontFree (szgrpStatus);

  logProcEnd ("");
  return uiplayer->wcont [UIPL_W_MAIN_VBOX];
}

void
uiplayerMainLoop (uiplayer_t *uiplayer)
{
  if (uiplayer == NULL) {
    return;
  }

  if (mstimeCheck (&uiplayer->volumeLockTimeout)) {
    mstimeset (&uiplayer->volumeLockTimeout, TM_TIMER_OFF);
    uiplayer->volumeLock = false;
  }

  if (mstimeCheck (&uiplayer->volumeLockSend)) {
    double        value;
    char          tbuff [40];

    value = uiScaleGetValue (uiplayer->wcont [UIPL_W_VOLUME]);
    if (value != uiplayer->volumeLastValue) {
      uiplayer->volumeLastValue = value;
      snprintf (tbuff, sizeof (tbuff), "%.0f", value);
      connSendMessage (uiplayer->conn, ROUTE_PLAYER, MSG_PLAYER_VOLUME, tbuff);
      if (uiplayer->volumeLock) {
        mstimeset (&uiplayer->volumeLockSend, UIPLAYER_LOCK_TIME_SEND);
      } else {
        mstimeset (&uiplayer->volumeLockSend, TM_TIMER_OFF);
        uiplayer->volumeLastValue = -1.0;
      }
    }
    if (! uiplayer->volumeLock) {
      mstimeset (&uiplayer->volumeLockSend, TM_TIMER_OFF);
      uiplayer->volumeLastValue = -1.0;
    }
  }

  if (mstimeCheck (&uiplayer->speedLockTimeout)) {
    mstimeset (&uiplayer->speedLockTimeout, TM_TIMER_OFF);
    uiplayer->speedLock = false;
  }

  if (mstimeCheck (&uiplayer->speedLockSend)) {
    double        value;
    char          tbuff [40];

    value = uiScaleGetValue (uiplayer->wcont [UIPL_W_SPEED]);
    if (value != uiplayer->speedLastValue) {
      uiplayer->speedLastValue = value;
      snprintf (tbuff, sizeof (tbuff), "%.0f", value);
      connSendMessage (uiplayer->conn, ROUTE_PLAYER, MSG_PLAY_SPEED, tbuff);
      if (uiplayer->speedLock) {
        mstimeset (&uiplayer->speedLockSend, UIPLAYER_LOCK_TIME_SEND);
      } else {
        mstimeset (&uiplayer->speedLockSend, TM_TIMER_OFF);
        uiplayer->speedLastValue = -1.0;
      }
    }
  }

  if (mstimeCheck (&uiplayer->seekLockTimeout)) {
    mstimeset (&uiplayer->seekLockTimeout, TM_TIMER_OFF);
    uiplayer->seekLock = false;
  }

  if (mstimeCheck (&uiplayer->seekLockSend)) {
    double        value;
    char          tbuff [40];

    value = uiScaleGetValue (uiplayer->wcont [UIPL_W_SEEK]);
    /* do not keep sending the seek value */
    if (value != uiplayer->seekLastValue) {
      uiplayer->seekLastValue = value;
      snprintf (tbuff, sizeof (tbuff), "%.0f", round (value));
      connSendMessage (uiplayer->conn, ROUTE_PLAYER, MSG_PLAY_SEEK, tbuff);
      if (uiplayer->seekLock) {
        mstimeset (&uiplayer->seekLockSend, UIPLAYER_SEEK_LOCK_TIME_SEND);
      }
    } else {
      mstimeset (&uiplayer->seekLockSend, TM_TIMER_OFF);
      uiplayer->seekLastValue = -1.0;
    }
  }
}

int
uiplayerProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
    bdjmsgmsg_t msg, char *args, void *udata)
{
  uiplayer_t    *uiplayer = udata;
  char          *targs = NULL;

  if (uiplayer == NULL) {
    return 0;
  }

  logProcBegin ();

  /* the management ui has two uiplayer instances */
  /* therefore the original message must be preserved */
  if (args != NULL) {
    targs = mdstrdup (args);
  }

  switch (route) {
    case ROUTE_NONE:
    case ROUTE_MANAGEUI:
    case ROUTE_PLAYERUI: {
      switch (msg) {
        case MSG_PLAYER_STATE: {
          uiplayerProcessPlayerStateMsg (uiplayer, targs);
          break;
        }
        case MSG_PLAY_PAUSEATEND_STATE: {
          uiplayerProcessPauseatend (uiplayer, atol (targs));
          break;
        }
        case MSG_PLAYER_STATUS_DATA: {
          uiplayerProcessPlayerStatusData (uiplayer, targs);
          break;
        }
        case MSG_MUSICQ_STATUS_DATA: {
          uiplayerProcessMusicqStatusData (uiplayer, targs);
          break;
        }
        case MSG_PLAYER_SUPPORT: {
          uiplayer->pliSupported = atoi (targs);
          if (! pliCheckSupport (uiplayer->pliSupported, PLI_SUPPORT_SPEED)) {
            uiplayerDisableSpeed (uiplayer);
          }
          if (! pliCheckSupport (uiplayer->pliSupported, PLI_SUPPORT_SEEK)) {
            uiplayerDisableSeek (uiplayer);
          }
          break;
        }
        case MSG_FINISHED: {
          uiplayer->playerState = PL_STATE_STOPPED;
          uiplayerProcessPlayerState (uiplayer);
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

  dataFree (targs);

  logProcEnd ("");
  return 0;
}

dbidx_t
uiplayerGetCurrSongIdx (uiplayer_t *uiplayer)
{
  if (uiplayer == NULL) {
    return -1;
  }
  return uiplayer->curr_dbidx;
}

void
uiplayerDisableSpeed (uiplayer_t *uiplayer)
{
  if (uiplayer == NULL) {
    return;
  }
  uiWidgetSetState (uiplayer->wcont [UIPL_W_SPEED], UIWIDGET_DISABLE);
  uiplayer->speeddisabled = true;
}

void
uiplayerDisableSeek (uiplayer_t *uiplayer)
{
  if (uiplayer == NULL) {
    return;
  }
  uiWidgetSetState (uiplayer->wcont [UIPL_W_SEEK], UIWIDGET_DISABLE);
  uiplayer->seekdisabled = true;
}

void
uiplayerGetVolumeSpeed (uiplayer_t *uiplayer, int *baseVolume, double *volume, double *speed)
{
  *baseVolume = 0;
  *volume = 0;
  *speed = 100;

  if (uiplayer == NULL) {
    return;
  }

  *baseVolume = uiplayer->baseVolume;
  *volume = uiScaleGetValue (uiplayer->wcont [UIPL_W_VOLUME]);
  *speed = uiScaleGetValue (uiplayer->wcont [UIPL_W_SPEED]);
}

bool
uiplayerGetRepeat (uiplayer_t *uiplayer)
{
  if (uiplayer == NULL) {
    return false;
  }

  return uiplayer->repeat;
}

/* internal routines */

static bool
uiplayerInitCallback (void *udata, programstate_t programState)
{
  uiplayer_t *uiplayer = udata;

  logProcBegin ();

  uiplayer->playerState = PL_STATE_STOPPED;
  mstimeset (&uiplayer->speedLockTimeout, TM_TIMER_OFF);
  mstimeset (&uiplayer->speedLockSend, TM_TIMER_OFF);
  mstimeset (&uiplayer->volumeLockTimeout, TM_TIMER_OFF);
  mstimeset (&uiplayer->volumeLockSend, TM_TIMER_OFF);
  uiplayer->baseVolume = -1;
  uiplayer->lastdur = 180000;
  mstimeset (&uiplayer->seekLockTimeout, TM_TIMER_OFF);
  mstimeset (&uiplayer->seekLockSend, TM_TIMER_OFF);
  uiplayer->seekLastValue = -1.0;
  uiplayer->volumeLastValue = -1.0;
  uiplayer->speedLastValue = 100.0;
  uiplayer->seekLock = false;
  uiplayer->speedLock = false;
  uiplayer->repeatLock = false;
  uiplayer->pauseatendLock = false;
  uiplayer->pauseatendstate = false;
  uiplayer->volumeLock = false;
  uiplayer->speeddisabled = false;
  uiplayer->seekdisabled = false;
  uiplayer->pliSupported = PLI_SUPPORT_NONE;

  logProcEnd ("");
  return STATE_FINISHED;
}

static bool
uiplayerClosingCallback (void *udata, programstate_t programState)
{
  uiplayer_t      *uiplayer = udata;

  logProcBegin ();
  uiWidgetClearPersistent (uiplayer->images [UIPL_PIX_STOP]);
  uiWidgetClearPersistent (uiplayer->images [UIPL_PIX_PLAY]);
  uiWidgetClearPersistent (uiplayer->images [UIPL_PIX_PAUSE]);
  uiWidgetClearPersistent (uiplayer->images [UIPL_PIX_TIMER]);
  uiWidgetClearPersistent (uiplayer->images [UIPL_PIX_REPEAT]);
  uiWidgetClearPersistent (uiplayer->images [UIPL_IMG_LED_ON]);
  uiWidgetClearPersistent (uiplayer->images [UIPL_IMG_LED_OFF]);
  logProcEnd ("");
  return STATE_FINISHED;
}

static void
uiplayerProcessPauseatend (uiplayer_t *uiplayer, int onoff)
{
  logProcBegin ();

  if (! uiplayer->uibuilt) {
    logProcEnd ("no-ui");
    return;
  }
  if (uiplayer->pauseatendLock) {
    logProcEnd ("pae-lock");
    return;
  }

  uiplayer->pauseatendLock = true;

  if (onoff && ! uiplayer->pauseatendstate) {
    uiToggleButtonSetImage (uiplayer->wcont [UIPL_W_PAUSE_AT_END_B], uiplayer->images [UIPL_IMG_LED_ON]);
    uiToggleButtonSetValue (uiplayer->wcont [UIPL_W_PAUSE_AT_END_B], UI_TOGGLE_BUTTON_ON);
  }
  if (! onoff && uiplayer->pauseatendstate) {
    uiToggleButtonSetImage (uiplayer->wcont [UIPL_W_PAUSE_AT_END_B], uiplayer->images [UIPL_IMG_LED_OFF]);
    uiToggleButtonSetValue (uiplayer->wcont [UIPL_W_PAUSE_AT_END_B], UI_TOGGLE_BUTTON_OFF);
  }
  uiplayer->pauseatendstate = onoff;

  uiplayer->pauseatendLock = false;
  logProcEnd ("");
}

static void
uiplayerProcessPlayerStateMsg (uiplayer_t *uiplayer, char *data)
{
  mp_playerstate_t  *ps;

  logProcBegin ();

  ps = msgparsePlayerStateData (data);
  uiplayer->playerState = ps->playerState;

  if (ps->newsong) {
    /* clear any speed lock */
    /* this will force the speed to be reset */
    uiplayer->speedLock = false;
    mstimeset (&uiplayer->speedLockTimeout, TM_TIMER_OFF);
    mstimeset (&uiplayer->speedLockSend, TM_TIMER_OFF);
  }

  uiplayerProcessPlayerState (uiplayer);
  msgparsePlayerStateFree (ps);
}

static void
uiplayerProcessPlayerState (uiplayer_t *uiplayer)
{
  int               state;

  logMsg (LOG_DBG, LOG_INFO, "pl-state: %d/%s",
      uiplayer->playerState, logPlayerState (uiplayer->playerState));

  state = UIWIDGET_ENABLE;
  if (uiplayer->playerState == PL_STATE_IN_FADEOUT) {
    state = UIWIDGET_DISABLE;
  }

  uiWidgetSetState (uiplayer->wcont [UIPL_W_VOLUME], state);
  if (! uiplayer->speeddisabled) {
    uiWidgetSetState (uiplayer->wcont [UIPL_W_SPEED], state);
  }
  if (! uiplayer->seekdisabled) {
    uiWidgetSetState (uiplayer->wcont [UIPL_W_SEEK], state);
  }
  uiWidgetSetState (uiplayer->wcont [UIPL_W_SONG_BEGIN_B], state);

  controllerSetPlayState (uiplayer->controller, uiplayer->playerState);

  switch (uiplayer->playerState) {
    case PL_STATE_UNKNOWN:
    case PL_STATE_STOPPED: {
      uiImageClear (uiplayer->images [UIPL_IMG_STATUS]);
      uiImageSetFromPixbuf (uiplayer->images [UIPL_IMG_STATUS], uiplayer->images [UIPL_PIX_STOP]);
      break;
    }
    case PL_STATE_IN_GAP: {
      uiImageClear (uiplayer->images [UIPL_IMG_STATUS]);
      uiImageSetFromPixbuf (uiplayer->images [UIPL_IMG_STATUS], uiplayer->images [UIPL_PIX_TIMER]);
      break;
    }
    case PL_STATE_LOADING:
    case PL_STATE_IN_FADEOUT:
    case PL_STATE_PLAYING: {
      uiImageClear (uiplayer->images [UIPL_IMG_STATUS]);
      uiImageSetFromPixbuf (uiplayer->images [UIPL_IMG_STATUS], uiplayer->images [UIPL_PIX_PLAY]);
      break;
    }
    case PL_STATE_PAUSED: {
      uiImageClear (uiplayer->images [UIPL_IMG_STATUS]);
      uiImageSetFromPixbuf (uiplayer->images [UIPL_IMG_STATUS], uiplayer->images [UIPL_PIX_PAUSE]);
      break;
    }
    default: {
      uiImageClear (uiplayer->images [UIPL_IMG_STATUS]);
      uiImageSetFromPixbuf (uiplayer->images [UIPL_IMG_STATUS], uiplayer->images [UIPL_PIX_STOP]);
      break;
    }
  }
  logProcEnd ("");
}

static void
uiplayerProcessPlayerStatusData (uiplayer_t *uiplayer, char *args)
{
  char          tbuff [100];
  double        dval;
  double        ddur;
  ssize_t       timeleft = 0;
  ssize_t       position = 0;
  ssize_t       dur = 0;
  mp_playerstatus_t *ps = NULL;

  logProcBegin ();

  ps = msgparsePlayerStatusData (args);

  /* repeat */
  uiplayer->repeatLock = true;
  uiplayer->repeat = ps->repeat;
  if (ps->repeat) {
    uiImageClear (uiplayer->images [UIPL_IMG_REPEAT]);
    uiImageSetFromPixbuf (uiplayer->images [UIPL_IMG_REPEAT], uiplayer->images [UIPL_PIX_REPEAT]);
    uiToggleButtonSetValue (uiplayer->wcont [UIPL_W_REPEAT_B], UI_TOGGLE_BUTTON_ON);
  } else {
    uiImageClear (uiplayer->images [UIPL_IMG_REPEAT]);
    uiToggleButtonSetValue (uiplayer->wcont [UIPL_W_REPEAT_B], UI_TOGGLE_BUTTON_OFF);
  }
  uiplayer->repeatLock = false;
  controllerSetRepeatState (uiplayer->controller, ps->repeat);
  controllerSetPlayState (uiplayer->controller, uiplayer->playerState);

  /* pauseatend */
  uiplayerProcessPauseatend (uiplayer, ps->pauseatend);

  /* current vol */
  if (! uiplayer->volumeLock) {
    snprintf (tbuff, sizeof (tbuff), "%3d", ps->currentVolume);
    uiLabelSetText (uiplayer->wcont [UIPL_W_VOLUME_DISP], tbuff);
    dval = (double) ps->currentVolume;
    uiScaleSetValue (uiplayer->wcont [UIPL_W_VOLUME], dval);
    controllerSetVolume (uiplayer->controller, ps->currentVolume);
  }

  /* speed */
  if (! uiplayer->speedLock) {
    snprintf (tbuff, sizeof (tbuff), "%3d", ps->currentSpeed);
    uiLabelSetText (uiplayer->wcont [UIPL_W_SPEED_DISP], tbuff);
    dval = (double) ps->currentSpeed;
    uiScaleSetValue (uiplayer->wcont [UIPL_W_SPEED], dval);
    uiplayer->speedLastValue = dval;
    controllerSetRate (uiplayer->controller, ps->currentSpeed);
  }

  /* base vol */
  uiplayer->baseVolume = ps->baseVolume;

  /* playedtime */
  position = ps->playedtime;
  dval = (double) ps->playedtime;    // used below for scale
  controllerSetPosition (uiplayer->controller, position);

  /* duration */
  ddur = (double) ps->duration;
  dur = ps->duration;
  if (ddur > 0.0 && dur != uiplayer->lastdur) {
    tmutilToMS (dur, tbuff, sizeof (tbuff));
    uiLabelSetText (uiplayer->wcont [UIPL_W_DURATION], tbuff);
    uiScaleSetRange (uiplayer->wcont [UIPL_W_SEEK], 0.0, ddur);
    uiplayer->lastdur = dur;
  }

  if (! uiplayer->seekLock) {
    tmutilToMS (position, tbuff, sizeof (tbuff));
    uiLabelSetText (uiplayer->wcont [UIPL_W_SEEK_DISP], tbuff);

    timeleft = dur - position;
    tmutilToMS (timeleft, tbuff, sizeof (tbuff));
    uiLabelSetText (uiplayer->wcont [UIPL_W_COUNTDOWN_TIMER], tbuff);

    if (ddur == 0.0) {
      uiScaleSetValue (uiplayer->wcont [UIPL_W_SEEK], 0.0);
    } else {
      uiScaleSetValue (uiplayer->wcont [UIPL_W_SEEK], dval);
    }
  }

  msgparsePlayerStatusFree (ps);

  logProcEnd ("");
}

static void
uiplayerProcessMusicqStatusData (uiplayer_t *uiplayer, char *args)
{
  song_t            *song = NULL;
  ilistidx_t        danceIdx;
  dance_t           *dances;
  slist_t           *sellist;
  const char        *sep = "";
  char              sepstr [20] = { " " };
  int               idx;
  int               tagidx;
  slistidx_t        seliteridx;
  mp_musicqstatus_t mqstatus;
  contmetadata_t    cmetadata;
  char              uri [BDJ4_PATH_MAX];
  genre_t           *genres;

  logProcBegin ();

  dances = bdjvarsdfGet (BDJVDF_DANCES);

  if (! uiplayer->uibuilt) {
    return;
  }

  msgparseMusicQStatus (&mqstatus, args);
  uiplayer->curr_dbidx = mqstatus.dbidx;

  if (mqstatus.dbidx < 0) {
    uiplayerClearDisplay (uiplayer);
    logProcEnd ("no-dbidx");
    return;
  }

  song = dbGetByIdx (uiplayer->musicdb, mqstatus.dbidx);
  if (song == NULL) {
    uiplayerClearDisplay (uiplayer);
    logProcEnd ("null-song");
    return;
  }

  sellist = dispselGetList (uiplayer->dispsel, DISP_SEL_CURRSONG);

  controllerInitMetadata (&cmetadata);
  genres = bdjvarsdfGet (BDJVDF_GENRES);
  audiosrcURI (songGetStr (song, TAG_URI), uri, sizeof (uri), NULL, 0);
  cmetadata.uri = uri;
  cmetadata.imageuri = mqstatus.imguri;
  cmetadata.album = songGetStr (song, TAG_ALBUM);
  cmetadata.albumartist = songGetStr (song, TAG_ALBUMARTIST);
  cmetadata.artist = songGetStr (song, TAG_ARTIST);
  cmetadata.title = songGetStr (song, TAG_TITLE);
  cmetadata.genre = genreGetGenre (genres, songGetNum (song, TAG_GENRE));
  cmetadata.trackid = mqstatus.uniqueidx;
  cmetadata.duration = mqstatus.duration;
  cmetadata.astype = audiosrcGetType (uri);
  controllerSetCurrent (uiplayer->controller, &cmetadata);

  idx = UIPL_W_INFO_DISP_A;
  slistStartIterator (sellist, &seliteridx);
  while ((tagidx = slistIterateValueNum (sellist, &seliteridx)) >= 0) {
    const char  *tstr = NULL;

    if (tagidx == TAG_DANCE) {
      danceIdx = songGetNum (song, TAG_DANCE);
      tstr = danceGetStr (dances, danceIdx, DANCE_DANCE);
    } else {
      tstr = songGetStr (song, tagidx);
    }
    if (tstr == NULL) {
      /* it is possible for there to be no data */
      tstr = "";
    }

    if (*sep) {
      uiLabelSetText (uiplayer->wcont [idx], sepstr);
      ++idx;
    }

    if (idx > UIPL_W_INFO_DISP_I) {
      break;
    }

    uiLabelSetText (uiplayer->wcont [idx], tstr);
    if (tagidx != TAG_DANCE && *tstr) {
      uiWidgetSetTooltip (uiplayer->wcont [idx], tstr);
    }
    if (! *sep) {
      sep = bdjoptGetStr (OPT_P_PLAYER_UI_SEP);
      if (sep != NULL) {
        snprintf (sepstr, sizeof (sepstr), " %s ", sep);
      }
    }

    ++idx;
    if (idx > UIPL_W_INFO_DISP_I) {
      break;
    }
  }

  if (mqstatus.imguri != NULL) {
    mdfree (mqstatus.imguri);
  }

  logProcEnd ("");
}

static bool
uiplayerFadeProcess (void *udata)
{
  uiplayer_t      *uiplayer = udata;

  logProcBegin ();
  connSendMessage (uiplayer->conn, ROUTE_PLAYER, MSG_PLAY_FADE, NULL);
  logProcEnd ("");
  return UICB_CONT;
}

static bool
uiplayerPlayPauseProcess (void *udata)
{
  uiplayer_t      *uiplayer = udata;

  logProcBegin ();
  connSendMessage (uiplayer->conn, ROUTE_MAIN, MSG_CMD_PLAYPAUSE, NULL);
  if (uiplayer->playerState == PL_STATE_STOPPED) {
    /* in case the start-wait timer is set, switch the status display to */
    /* the timer.  The status update from the player will change it */
    uiImageClear (uiplayer->images [UIPL_IMG_STATUS]);
    uiImageSetFromPixbuf (uiplayer->images [UIPL_IMG_STATUS], uiplayer->images [UIPL_PIX_TIMER]);
  }

  logProcEnd ("");
  return UICB_CONT;
}

static bool
uiplayerRepeatCallback (void *udata)
{
  uiplayer_t      *uiplayer = udata;

  logProcBegin ();

  if (uiplayer->repeatLock) {
    logProcEnd ("repeat-lock");
    return UICB_CONT;
  }

  connSendMessage (uiplayer->conn, ROUTE_PLAYER, MSG_PLAY_REPEAT, NULL);
  logProcEnd ("");
  return UICB_CONT;
}

static bool
uiplayerSongBeginProcess (void *udata)
{
  uiplayer_t      *uiplayer = udata;

  logProcBegin ();
  connSendMessage (uiplayer->conn, ROUTE_PLAYER, MSG_PLAY_SONG_BEGIN, NULL);
  logProcEnd ("");
  return UICB_CONT;
}

static bool
uiplayerNextSongProcess (void *udata)
{
  uiplayer_t      *uiplayer = udata;

  logProcBegin ();
  connSendMessage (uiplayer->conn, ROUTE_PLAYER, MSG_PLAY_NEXTSONG, NULL);
  logProcEnd ("");
  return UICB_CONT;
}

static bool
uiplayerPauseatendCallback (void *udata)
{
  uiplayer_t      *uiplayer = udata;

  logProcBegin ();

  if (uiplayer->pauseatendLock) {
    logProcEnd ("pae-lock");
    return UICB_STOP;
  }

  connSendMessage (uiplayer->conn, ROUTE_PLAYER, MSG_PLAY_PAUSEATEND, NULL);
  logProcEnd ("");
  return UICB_CONT;
}

static bool
uiplayerSpeedCallback (void *udata, double value)
{
  uiplayer_t    *uiplayer = udata;
  char          tbuff [40];

  logProcBegin ();
  logMsg (LOG_DBG, LOG_ACTIONS, "%s =action: speed chg", uiplayer->tag);

  if (! uiplayer->speedLock) {
    mstimeset (&uiplayer->speedLockSend, UIPLAYER_LOCK_TIME_SEND);
  }
  uiplayer->speedLock = true;
  mstimeset (&uiplayer->speedLockTimeout, UIPLAYER_LOCK_TIME_WAIT);

  value = uiScaleEnforceMax (uiplayer->wcont [UIPL_W_SPEED], value);
  snprintf (tbuff, sizeof (tbuff), "%3.0f", value);
  uiLabelSetText (uiplayer->wcont [UIPL_W_SPEED_DISP], tbuff);
  logProcEnd ("");
  return UICB_CONT;
}

static bool
uiplayerSeekCallback (void *udata, double value)
{
  uiplayer_t    *uiplayer = udata;
  char          tbuff [40];
  ssize_t       position;
  ssize_t       timeleft;

  logProcBegin ();
  logMsg (LOG_DBG, LOG_ACTIONS, "%s =action: position chg", uiplayer->tag);

  if (! uiplayer->seekLock) {
    mstimeset (&uiplayer->seekLockSend, UIPLAYER_SEEK_LOCK_TIME_SEND);
  }
  uiplayer->seekLock = true;
  mstimeset (&uiplayer->seekLockTimeout, UIPLAYER_SEEK_LOCK_TIME_WAIT);

  value = uiScaleEnforceMax (uiplayer->wcont [UIPL_W_SEEK], value);
  position = (ssize_t) round (value);

  tmutilToMS (position, tbuff, sizeof (tbuff));
  uiLabelSetText (uiplayer->wcont [UIPL_W_SEEK_DISP], tbuff);

  timeleft = uiplayer->lastdur - position;
  tmutilToMS (timeleft, tbuff, sizeof (tbuff));
  uiLabelSetText (uiplayer->wcont [UIPL_W_COUNTDOWN_TIMER], tbuff);
  logProcEnd ("");
  return UICB_CONT;
}

static bool
uiplayerVolumeCallback (void *udata, double value)
{
  uiplayer_t    *uiplayer = udata;
  char          tbuff [40];

  logProcBegin ();
  logMsg (LOG_DBG, LOG_ACTIONS, "%s =action: volume chg", uiplayer->tag);

  if (! uiplayer->volumeLock) {
    mstimeset (&uiplayer->volumeLockSend, UIPLAYER_LOCK_TIME_SEND);
  }
  uiplayer->volumeLock = true;
  mstimeset (&uiplayer->volumeLockTimeout, UIPLAYER_LOCK_TIME_WAIT);

  value = uiScaleEnforceMax (uiplayer->wcont [UIPL_W_VOLUME], value);
  snprintf (tbuff, sizeof (tbuff), "%3.0f", value);
  uiLabelSetText (uiplayer->wcont [UIPL_W_VOLUME_DISP], tbuff);
  logProcEnd ("");
  return UICB_CONT;
}

static void
uiplayerClearDisplay (uiplayer_t *uiplayer)
{
  for (int i = UIPL_W_INFO_DISP_A; i <= UIPL_W_INFO_DISP_I; ++i) {
    uiLabelSetText (uiplayer->wcont [i], "");
  }
}

static bool
uiplayerSpdResetCallback (void *udata)
{
  uiplayer_t    *uiplayer = udata;

  uiScaleSetValue (uiplayer->wcont [UIPL_W_SPEED], 100.0);
  uiLabelSetText (uiplayer->wcont [UIPL_W_SPEED_DISP], "100");

  if (! uiplayer->speedLock) {
    mstimeset (&uiplayer->speedLockSend, UIPLAYER_LOCK_TIME_SEND);
  }
  uiplayer->speedLock = true;
  mstimeset (&uiplayer->speedLockTimeout, UIPLAYER_LOCK_TIME_WAIT);
  return UICB_CONT;
}
