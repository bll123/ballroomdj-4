/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
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
#include "mdebug.h"
#include "musicdb.h"
#include "pathbld.h"
#include "player.h"
#include "progstate.h"
#include "song.h"
#include "tagdef.h"
#include "ui.h"
#include "callback.h"
#include "uiplayer.h"

/* there are all sorts of latency issues making the sliders work nicely */
/* it will take at least 100ms and at most 200ms for the message to get */
/* back.  Then there are the latency issues on this end. */
enum {
  UIPLAYER_LOCK_TIME_WAIT = 300,
  UIPLAYER_LOCK_TIME_SEND = 30,
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
  UIPL_CB_MAX,
};

enum {
  UIPL_BUTTON_FADE,
  UIPL_BUTTON_PLAYPAUSE,
  UIPL_BUTTON_BEGSONG,
  UIPL_BUTTON_NEXTSONG,
  UIPL_BUTTON_MAX,
};

enum {
  UIPL_IMG_STATUS,
  UIPL_IMG_REPEAT,
  UIPL_PIX_PLAY,
  UIPL_PIX_STOP,
  UIPL_PIX_PAUSE,
  UIPL_PIX_REPEAT,
  UIPL_IMG_LED_OFF,
  UIPL_IMG_LED_ON,
  UIPL_IMG_MAX,
};

typedef struct uiplayer {
  progstate_t     *progstate;
  conn_t          *conn;
  playerstate_t   playerState;
  musicdb_t       *musicdb;
  callback_t      *callbacks [UIPL_CB_MAX];
  dbidx_t         curr_dbidx;
  /* song display */
  uiwcont_t       *vbox;
  uiwcont_t       *images [UIPL_IMG_MAX];
  uiwcont_t       danceLab;
  uiwcont_t       artistLab;
  uiwcont_t       titleLab;
  uibutton_t      *buttons [UIPL_BUTTON_MAX];
  /* speed controls / display */
  uiwcont_t       *speedScale;
  uiwcont_t       speedDisplayLab;
  bool            speedLock;
  mstime_t        speedLockTimeout;
  mstime_t        speedLockSend;
  /* position controls / display */
  uiwcont_t       countdownTimerLab;
  uiwcont_t       durationLab;
  uiwcont_t       *seekScale;
  uiwcont_t       seekDisplayLab;
  ssize_t         lastdur;
  bool            seekLock;
  mstime_t        seekLockTimeout;
  mstime_t        seekLockSend;
  /* main controls */
  uiwcont_t       *repeatButton;
  uiwcont_t       songbeginButton;
  uiwcont_t       *pauseatendButton;
  bool            repeatLock;
  bool            pauseatendLock;
  bool            pauseatendstate;
  /* volume controls / display */
  uiwcont_t       *volumeScale;
  bool            volumeLock;
  mstime_t        volumeLockTimeout;
  mstime_t        volumeLockSend;
  uiwcont_t      volumeDisplayLab;
  bool            uibuilt;
} uiplayer_t;

static bool  uiplayerInitCallback (void *udata, programstate_t programState);
static bool  uiplayerClosingCallback (void *udata, programstate_t programState);

static void     uiplayerProcessPauseatend (uiplayer_t *uiplayer, int on);
static void     uiplayerProcessPlayerState (uiplayer_t *uiplayer, int playerState);
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

uiplayer_t *
uiplayerInit (progstate_t *progstate, conn_t *conn, musicdb_t *musicdb)
{
  uiplayer_t    *uiplayer;

  logProcBegin (LOG_PROC, "uiplayerInit");
  uiplayer = mdmalloc (sizeof (uiplayer_t));
  assert (uiplayer != NULL);
  uiplayer->progstate = progstate;
  uiplayer->conn = conn;
  uiplayer->musicdb = musicdb;
  uiplayer->uibuilt = false;
  uiplayer->curr_dbidx = -1;

  uiplayer->vbox = NULL;
  uiwcontInit (&uiplayer->danceLab);
  uiwcontInit (&uiplayer->artistLab);
  uiwcontInit (&uiplayer->titleLab);
  uiwcontInit (&uiplayer->countdownTimerLab);
  uiwcontInit (&uiplayer->durationLab);
  uiwcontInit (&uiplayer->speedDisplayLab);
  uiwcontInit (&uiplayer->seekDisplayLab);
  uiplayer->speedScale = NULL;
  uiplayer->seekScale = NULL;
  uiplayer->repeatButton = NULL;
  uiwcontInit (&uiplayer->songbeginButton);
  uiplayer->pauseatendButton = NULL;
  uiwcontInit (&uiplayer->volumeDisplayLab);
  uiplayer->volumeScale = NULL;
  for (int i = 0; i < UIPL_BUTTON_MAX; ++i) {
    uiplayer->buttons [i] = NULL;
  }
  for (int i = 0; i < UIPL_CB_MAX; ++i) {
    uiplayer->callbacks [i] = NULL;
  }
  for (int i = 0; i < UIPL_IMG_MAX; ++i) {
    uiplayer->images [i] = NULL;
  }

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
    for (int i = 0; i < UIPL_CB_MAX; ++i) {
      callbackFree (uiplayer->callbacks [i]);
    }
    for (int i = 0; i < UIPL_BUTTON_MAX; ++i) {
      uiButtonFree (uiplayer->buttons [i]);
    }
    for (int i = 0; i < UIPL_IMG_MAX; ++i) {
      uiwcontFree (uiplayer->images [i]);
    }
    uiwcontFree (uiplayer->volumeScale);
    uiwcontFree (uiplayer->seekScale);
    uiwcontFree (uiplayer->speedScale);
    uiwcontFree (uiplayer->vbox);
    uiwcontFree (uiplayer->repeatButton);
    uiwcontFree (uiplayer->pauseatendButton);
    mdfree (uiplayer);
  }
  logProcEnd (LOG_PROC, "uiplayerFree", "");
}

uiwcont_t *
uiplayerBuildUI (uiplayer_t *uiplayer)
{
  char            tbuff [MAXPATHLEN];
  uiwcont_t       uiwidget;
  uibutton_t      *uibutton;
  uiwcont_t       *uiwidgetp;
  uiwcont_t       *hbox;
  uiwcont_t       *tbox;
  uiwcont_t       *szgrpA;
  uiwcont_t       *szgrpB;
  uiwcont_t       *szgrpC;
  uiwcont_t       *szgrpD;
  uiwcont_t       *szgrpE;

  logProcBegin (LOG_PROC, "uiplayerBuildUI");

  szgrpA = uiCreateSizeGroupHoriz ();
  szgrpB = uiCreateSizeGroupHoriz ();
  szgrpC = uiCreateSizeGroupHoriz ();
  szgrpD = uiCreateSizeGroupHoriz ();
  szgrpE = uiCreateSizeGroupHoriz ();

  uiplayer->vbox = uiCreateVertBox ();
  uiWidgetExpandHoriz (uiplayer->vbox);

  /* song display */

  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (uiplayer->vbox, hbox);

  /* size group E */
  tbox = uiCreateHorizBox ();
  uiBoxPackStart (hbox, tbox);
  uiSizeGroupAdd (szgrpE, tbox);

  uiplayer->images [UIPL_IMG_STATUS] = uiImageNew ();

  pathbldMakePath (tbuff, sizeof (tbuff), "button_stop", ".svg",
      PATHBLD_MP_DREL_IMG | PATHBLD_MP_USEIDX);
  uiplayer->images [UIPL_PIX_STOP] = uiImageFromFile (tbuff);
  uiImageConvertToPixbuf (uiplayer->images [UIPL_PIX_STOP]);
  uiWidgetMakePersistent (uiplayer->images [UIPL_PIX_STOP]);

  uiImageSetFromPixbuf (uiplayer->images [UIPL_IMG_STATUS], uiplayer->images [UIPL_PIX_STOP]);
  uiWidgetSetSizeRequest (uiplayer->images [UIPL_IMG_STATUS], 18, -1);
  uiWidgetSetMarginStart (uiplayer->images [UIPL_IMG_STATUS], 1);
  uiBoxPackStart (tbox, uiplayer->images [UIPL_IMG_STATUS]);

  pathbldMakePath (tbuff, sizeof (tbuff), "button_play", ".svg",
      PATHBLD_MP_DREL_IMG | PATHBLD_MP_USEIDX);
  uiplayer->images [UIPL_PIX_PLAY] = uiImageFromFile (tbuff);
  uiImageConvertToPixbuf (uiplayer->images [UIPL_PIX_PLAY]);
  uiWidgetMakePersistent (uiplayer->images [UIPL_PIX_PLAY]);

  pathbldMakePath (tbuff, sizeof (tbuff), "button_pause", ".svg",
      PATHBLD_MP_DREL_IMG | PATHBLD_MP_USEIDX);
  uiplayer->images [UIPL_PIX_PAUSE] = uiImageFromFile (tbuff);
  uiImageConvertToPixbuf (uiplayer->images [UIPL_PIX_PAUSE]);
  uiWidgetMakePersistent (uiplayer->images [UIPL_PIX_PAUSE]);

  pathbldMakePath (tbuff, sizeof (tbuff), "button_repeat", ".svg",
      PATHBLD_MP_DREL_IMG | PATHBLD_MP_USEIDX);
  uiplayer->images [UIPL_PIX_REPEAT] = uiImageFromFile (tbuff);
  uiImageConvertToPixbuf (uiplayer->images [UIPL_PIX_REPEAT]);
  uiWidgetMakePersistent (uiplayer->images [UIPL_PIX_REPEAT]);

  uiplayer->images [UIPL_IMG_REPEAT] = uiImageNew ();
  uiImageClear (uiplayer->images [UIPL_IMG_REPEAT]);
  uiWidgetSetSizeRequest (uiplayer->images [UIPL_IMG_REPEAT], 18, -1);
  uiWidgetSetMarginStart (uiplayer->images [UIPL_IMG_REPEAT], 1);
  uiBoxPackStart (tbox, uiplayer->images [UIPL_IMG_REPEAT]);

  uiCreateLabelOld (&uiplayer->danceLab, "");
  uiBoxPackStart (hbox, &uiplayer->danceLab);

  uiCreateLabelOld (&uiwidget, " : ");
  uiWidgetSetMarginStart (&uiwidget, 0);
  uiBoxPackStart (hbox, &uiwidget);

  uiCreateLabelOld (&uiplayer->artistLab, "");
  uiWidgetSetMarginStart (&uiplayer->artistLab, 0);
  uiLabelEllipsizeOn (&uiplayer->artistLab);
  uiBoxPackStart (hbox, &uiplayer->artistLab);

  uiCreateLabelOld (&uiwidget, " : ");
  uiWidgetSetMarginStart (&uiwidget, 0);
  uiBoxPackStart (hbox, &uiwidget);

  uiCreateLabelOld (&uiplayer->titleLab, "");
  uiWidgetSetMarginStart (&uiplayer->titleLab, 0);
  uiLabelEllipsizeOn (&uiplayer->titleLab);
  uiBoxPackStart (hbox, &uiplayer->titleLab);

  /* expanding label to take space */
  uiCreateLabelOld (&uiwidget, "");
  uiWidgetExpandHoriz (&uiwidget);
  uiBoxPackStart (hbox, &uiwidget);

  /* size group A */
  uiCreateLabelOld (&uiwidget, "%");
  uiBoxPackEnd (hbox, &uiwidget);
  uiSizeGroupAdd (szgrpA, &uiwidget);

  /* size group B */
  uiCreateLabelOld (&uiplayer->speedDisplayLab, "100");
  uiLabelAlignEnd (&uiplayer->speedDisplayLab);
  uiBoxPackEnd (hbox, &uiplayer->speedDisplayLab);
  uiSizeGroupAdd (szgrpB, &uiplayer->speedDisplayLab);

  /* size group C */
  uiplayer->speedScale = uiCreateScale (70.0, 130.0, 1.0, 10.0, 100.0, 0);
  uiBoxPackEnd (hbox, uiplayer->speedScale);
  uiSizeGroupAdd (szgrpC, uiplayer->speedScale);
  uiplayer->callbacks [UIPL_CB_SPEED] = callbackInitDouble (
      uiplayerSpeedCallback, uiplayer);
  uiScaleSetCallback (uiplayer->speedScale, uiplayer->callbacks [UIPL_CB_SPEED]);

  /* size group D */
  /* CONTEXT: playerui: the current speed for song playback */
  uiCreateColonLabelOld (&uiwidget, _("Speed"));
  uiLabelAlignEnd (&uiwidget);
  uiWidgetSetMarginEnd (&uiwidget, 1);
  uiBoxPackEnd (hbox, &uiwidget);
  uiSizeGroupAdd (szgrpD, &uiwidget);

  /* position controls / display */

  uiwcontFree (hbox);
  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (uiplayer->vbox, hbox);

  /* size group E */
  uiCreateLabelOld (&uiwidget, "");
  uiBoxPackStart (hbox, &uiwidget);
  uiSizeGroupAdd (szgrpE, &uiwidget);

  uiCreateLabelOld (&uiplayer->countdownTimerLab, " 0:00");
  uiLabelAlignEnd (&uiplayer->countdownTimerLab);
  uiBoxPackStart (hbox, &uiplayer->countdownTimerLab);

  uiCreateLabelOld (&uiwidget, " / ");
  uiWidgetSetMarginStart (&uiwidget, 0);
  uiBoxPackStart (hbox, &uiwidget);

  uiCreateLabelOld (&uiplayer->durationLab, " 0:00");
  uiLabelAlignEnd (&uiplayer->durationLab);
  uiBoxPackStart (hbox, &uiplayer->durationLab);

  uiCreateLabelOld (&uiwidget, "");
  uiWidgetExpandHoriz (&uiwidget);
  uiBoxPackStart (hbox, &uiwidget);

  /* size group A */
  uiCreateLabelOld (&uiwidget, "");
  uiBoxPackEnd (hbox, &uiwidget);
  uiSizeGroupAdd (szgrpA, &uiwidget);

  /* size group B */
  uiCreateLabelOld (&uiplayer->seekDisplayLab, "0:00");
  uiLabelAlignEnd (&uiplayer->seekDisplayLab);
  uiSizeGroupAdd (szgrpB, &uiplayer->seekDisplayLab);
  uiBoxPackEnd (hbox, &uiplayer->seekDisplayLab);

  /* size group C */
  uiplayer->seekScale = uiCreateScale (0.0, 180000.0, 1000.0, 10000.0, 0.0, 0);
  uiBoxPackEnd (hbox, uiplayer->seekScale);
  uiSizeGroupAdd (szgrpC, uiplayer->seekScale);
  uiplayer->callbacks [UIPL_CB_SEEK] = callbackInitDouble (
      uiplayerSeekCallback, uiplayer);
  uiScaleSetCallback (uiplayer->seekScale, uiplayer->callbacks [UIPL_CB_SEEK]);

  /* size group D */
  /* CONTEXT: playerui: the current position of the song during song playback */
  uiCreateColonLabelOld (&uiwidget, _("Position"));
  uiLabelAlignEnd (&uiwidget);
  uiWidgetSetMarginEnd (&uiwidget, 1);
  uiBoxPackEnd (hbox, &uiwidget);
  uiSizeGroupAdd (szgrpD, &uiwidget);

  /* main controls */

  uiwcontFree (hbox);
  hbox = uiCreateHorizBox ();
  uiWidgetExpandHoriz (hbox);
  uiBoxPackStart (uiplayer->vbox, hbox);

  /* size group E */
  uiCreateLabelOld (&uiwidget, "");
  uiBoxPackStart (hbox, &uiwidget);
  uiSizeGroupAdd (szgrpE, &uiwidget);

  uiplayer->callbacks [UIPL_CB_FADE] = callbackInit (
      uiplayerFadeProcess, uiplayer, NULL);
  uibutton = uiCreateButton (
      uiplayer->callbacks [UIPL_CB_FADE],
      /* CONTEXT: playerui: button: fade out the song and stop playing it */
      _("Fade"), NULL);
  uiplayer->buttons [UIPL_BUTTON_FADE] = uibutton;
  uiwidgetp = uiButtonGetWidgetContainer (uibutton);
  uiBoxPackStart (hbox, uiwidgetp);

  uiplayer->callbacks [UIPL_CB_PLAYPAUSE] = callbackInit (
      uiplayerPlayPauseProcess, uiplayer, NULL);
  uibutton = uiCreateButton (
      uiplayer->callbacks [UIPL_CB_PLAYPAUSE],
      /* CONTEXT: playerui: button: tooltip: play or pause the song */
      _("Play / Pause"), "button_playpause");
  uiplayer->buttons [UIPL_BUTTON_PLAYPAUSE] = uibutton;
  uiwidgetp = uiButtonGetWidgetContainer (uibutton);
  uiBoxPackStart (hbox, uiwidgetp);

  pathbldMakePath (tbuff, sizeof (tbuff), "button_repeat", ".svg",
      PATHBLD_MP_DREL_IMG | PATHBLD_MP_USEIDX);
  uiplayer->callbacks [UIPL_CB_REPEAT] = callbackInit (
      uiplayerRepeatCallback, uiplayer, NULL);
  uiplayer->repeatButton = uiCreateToggleButton ("", tbuff,
      /* CONTEXT: playerui: button: tooltip: toggle the repeat song on and off */
      _("Toggle Repeat"), NULL, 0);
  uiBoxPackStart (hbox, uiplayer->repeatButton);
  uiToggleButtonSetCallback (uiplayer->repeatButton,
      uiplayer->callbacks [UIPL_CB_REPEAT]);

  uiplayer->callbacks [UIPL_CB_BEGSONG] = callbackInit (
      uiplayerSongBeginProcess, uiplayer, NULL);
  uibutton = uiCreateButton (
      uiplayer->callbacks [UIPL_CB_BEGSONG],
      /* CONTEXT: playerui: button: tooltip: return to the beginning of the song */
      _("Return to beginning of song"), "button_begin");
  uiplayer->buttons [UIPL_BUTTON_BEGSONG] = uibutton;
  uiwidgetp = uiButtonGetWidgetContainer (uibutton);
  uiBoxPackStart (hbox, uiwidgetp);

  uiplayer->callbacks [UIPL_CB_NEXTSONG] = callbackInit (
      uiplayerNextSongProcess, uiplayer, NULL);
  uibutton = uiCreateButton (
      uiplayer->callbacks [UIPL_CB_NEXTSONG],
      /* CONTEXT: playerui: button: tooltip: start playing the next song (immediate) */
      _("Next Song"), "button_nextsong");
  uiplayer->buttons [UIPL_BUTTON_NEXTSONG] = uibutton;
  uiwidgetp = uiButtonGetWidgetContainer (uibutton);
  uiBoxPackStart (hbox, uiwidgetp);

  pathbldMakePath (tbuff, sizeof (tbuff), "led_on", ".svg",
      PATHBLD_MP_DIR_IMG);
  uiplayer->images [UIPL_IMG_LED_ON] = uiImageFromFile (tbuff);
  uiWidgetMakePersistent (uiplayer->images [UIPL_IMG_LED_ON]);

  pathbldMakePath (tbuff, sizeof (tbuff), "led_off", ".svg",
      PATHBLD_MP_DIR_IMG);
  uiplayer->images [UIPL_IMG_LED_OFF] = uiImageFromFile (tbuff);
  uiWidgetMakePersistent (uiplayer->images [UIPL_IMG_LED_OFF]);

  uiplayer->callbacks [UIPL_CB_PAUSEATEND] = callbackInit (
      uiplayerPauseatendCallback, uiplayer, NULL);
  /* CONTEXT: playerui: button: pause at the end of the song (toggle) */
  uiplayer->pauseatendButton = uiCreateToggleButton (_("Pause at End"),
      NULL, NULL, uiplayer->images [UIPL_IMG_LED_OFF], 0);
  uiBoxPackStart (hbox, uiplayer->pauseatendButton);
  uiToggleButtonSetCallback (uiplayer->pauseatendButton,
      uiplayer->callbacks [UIPL_CB_PAUSEATEND]);

  /* volume controls / display */

  /* size group A */
  uiCreateLabelOld (&uiwidget, "%");
  uiBoxPackEnd (hbox, &uiwidget);
  uiSizeGroupAdd (szgrpA, &uiwidget);

  /* size group B */
  uiCreateLabelOld (&uiplayer->volumeDisplayLab, "100");
  uiLabelAlignEnd (&uiplayer->volumeDisplayLab);
  uiBoxPackEnd (hbox, &uiplayer->volumeDisplayLab);
  uiSizeGroupAdd (szgrpB, &uiplayer->volumeDisplayLab);

  /* size group C */
  uiplayer->volumeScale = uiCreateScale (0.0, 100.0, 1.0, 10.0, 0.0, 0);
  uiBoxPackEnd (hbox, uiplayer->volumeScale);
  uiSizeGroupAdd (szgrpC, uiplayer->volumeScale);
  uiplayer->callbacks [UIPL_CB_VOLUME] = callbackInitDouble (
      uiplayerVolumeCallback, uiplayer);
  uiScaleSetCallback (uiplayer->volumeScale, uiplayer->callbacks [UIPL_CB_VOLUME]);

  /* size group D */
  /* CONTEXT: playerui: The current volume of the song */
  uiCreateColonLabelOld (&uiwidget, _("Volume"));
  uiLabelAlignEnd (&uiwidget);
  uiWidgetSetMarginEnd (&uiwidget, 1);
  uiBoxPackEnd (hbox, &uiwidget);
  uiSizeGroupAdd (szgrpD, &uiwidget);

  uiplayer->uibuilt = true;

  uiwcontFree (tbox);
  uiwcontFree (hbox);
  uiwcontFree (szgrpA);
  uiwcontFree (szgrpB);
  uiwcontFree (szgrpC);
  uiwcontFree (szgrpD);
  uiwcontFree (szgrpE);

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

    value = uiScaleGetValue (uiplayer->volumeScale);
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

    value = uiScaleGetValue (uiplayer->speedScale);
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

    value = uiScaleGetValue (uiplayer->seekScale);
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
  char          *targs = NULL;

  logProcBegin (LOG_PROC, "uiplayerProcessMsg");
  if (args != NULL) {
    targs = mdstrdup (args);
  }

  switch (route) {
    case ROUTE_NONE:
    case ROUTE_MANAGEUI:
    case ROUTE_PLAYERUI: {
      switch (msg) {
        case MSG_PLAYER_STATE: {
          uiplayerProcessPlayerState (uiplayer, atol (targs));
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
        case MSG_FINISHED: {
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

  logProcEnd (LOG_PROC, "uiplayerProcessMsg", "");
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

/* internal routines */

static bool
uiplayerInitCallback (void *udata, programstate_t programState)
{
  uiplayer_t *uiplayer = udata;

  logProcBegin (LOG_PROC, "uiplayerInitCallback");

  uiplayer->repeatLock = false;
  uiplayer->pauseatendLock = false;
  uiplayer->pauseatendstate = false;
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
  uiWidgetClearPersistent (uiplayer->images [UIPL_PIX_STOP]);
  uiWidgetClearPersistent (uiplayer->images [UIPL_PIX_PLAY]);
  uiWidgetClearPersistent (uiplayer->images [UIPL_PIX_PAUSE]);
  uiWidgetClearPersistent (uiplayer->images [UIPL_PIX_REPEAT]);
  uiWidgetClearPersistent (uiplayer->images [UIPL_IMG_LED_ON]);
  uiWidgetClearPersistent (uiplayer->images [UIPL_IMG_LED_OFF]);
  logProcEnd (LOG_PROC, "uiplayerClosingCallback", "");
  return STATE_FINISHED;
}

static void
uiplayerProcessPauseatend (uiplayer_t *uiplayer, int on)
{
  logProcBegin (LOG_PROC, "uiplayerProcessPauseatend");

  if (! uiplayer->uibuilt) {
    logProcEnd (LOG_PROC, "uiplayerProcessPauseatend", "no-ui");
    return;
  }
  uiplayer->pauseatendLock = true;

  if (on && ! uiplayer->pauseatendstate) {
    uiToggleButtonSetImage (uiplayer->pauseatendButton, uiplayer->images [UIPL_IMG_LED_ON]);
    uiToggleButtonSetState (uiplayer->pauseatendButton, UI_TOGGLE_BUTTON_ON);
  }
  if (! on && uiplayer->pauseatendstate) {
    uiToggleButtonSetImage (uiplayer->pauseatendButton, uiplayer->images [UIPL_IMG_LED_OFF]);
    uiToggleButtonSetState (uiplayer->pauseatendButton, UI_TOGGLE_BUTTON_OFF);
  }
  uiplayer->pauseatendLock = false;
  uiplayer->pauseatendstate = on;
  logProcEnd (LOG_PROC, "uiplayerProcessPauseatend", "");
}

static void
uiplayerProcessPlayerState (uiplayer_t *uiplayer, int playerState)
{
  int   state;

  logProcBegin (LOG_PROC, "uiplayerProcessPlayerState");

  uiplayer->playerState = playerState;

  state = UIWIDGET_ENABLE;
  if (playerState == PL_STATE_IN_FADEOUT) {
    state = UIWIDGET_DISABLE;
  }

  uiWidgetSetState (uiplayer->volumeScale, state);
  uiWidgetSetState (uiplayer->speedScale, state);
  uiWidgetSetState (uiplayer->seekScale, state);
  uiWidgetSetState (&uiplayer->songbeginButton, state);

  switch (playerState) {
    case PL_STATE_UNKNOWN:
    case PL_STATE_STOPPED: {
      uiImageClear (uiplayer->images [UIPL_IMG_STATUS]);
      uiImageSetFromPixbuf (uiplayer->images [UIPL_IMG_STATUS], uiplayer->images [UIPL_PIX_STOP]);
      break;
    }
    case PL_STATE_LOADING:
    case PL_STATE_IN_FADEOUT:
    case PL_STATE_IN_GAP:
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
  ssize_t       timeleft = 0;
  ssize_t       position = 0;
  ssize_t       dur = 0;

  logProcBegin (LOG_PROC, "uiplayerProcessPlayerStatusData");

  /* repeat */
  p = strtok_r (args, MSG_ARGS_RS_STR, &tokstr);
  if (p != NULL) {
    uiplayer->repeatLock = true;
    if (atol (p)) {
      uiImageClear (uiplayer->images [UIPL_IMG_REPEAT]);
      uiImageSetFromPixbuf (uiplayer->images [UIPL_IMG_REPEAT], uiplayer->images [UIPL_PIX_REPEAT]);
      uiToggleButtonSetState (uiplayer->repeatButton, UI_TOGGLE_BUTTON_ON);
    } else {
      uiImageClear (uiplayer->images [UIPL_IMG_REPEAT]);
      uiToggleButtonSetState (uiplayer->repeatButton, UI_TOGGLE_BUTTON_OFF);
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
    uiLabelSetText (&uiplayer->volumeDisplayLab, p);
    dval = atof (p);
    uiScaleSetValue (uiplayer->volumeScale, dval);
  }

  /* speed */
  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  if (! uiplayer->speedLock) {
    snprintf (tbuff, sizeof (tbuff), "%3s", p);
    uiLabelSetText (&uiplayer->speedDisplayLab, p);
    dval = atof (p);
    uiScaleSetValue (uiplayer->speedScale, dval);
  }

  /* playedtime */
  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  position = atol (p);
  dval = atof (p);    // used below

  /* duration */
  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  ddur = atof (p);
  dur = atol (p);
  if (ddur > 0.0 && dur != uiplayer->lastdur) {
    tmutilToMS (dur, tbuff, sizeof (tbuff));
    uiLabelSetText (&uiplayer->durationLab, tbuff);
    uiScaleSetRange (uiplayer->seekScale, 0.0, ddur);
    uiplayer->lastdur = dur;
  }

  if (! uiplayer->seekLock) {
    tmutilToMS (position, tbuff, sizeof (tbuff));
    uiLabelSetText (&uiplayer->seekDisplayLab, tbuff);

    timeleft = dur - position;
    tmutilToMS (timeleft, tbuff, sizeof (tbuff));
    uiLabelSetText (&uiplayer->countdownTimerLab, tbuff);

    if (ddur == 0.0) {
      uiScaleSetValue (uiplayer->seekScale, 0.0);
    } else {
      uiScaleSetValue (uiplayer->seekScale, dval);
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

  if (! uiplayer->uibuilt) {
    return;
  }

  p = strtok_r (args, MSG_ARGS_RS_STR, &tokstr);
  dbidx = atol (p);
  uiplayer->curr_dbidx = dbidx;

  if (dbidx < 0) {
    uiplayerClearDisplay (uiplayer);
    logProcEnd (LOG_PROC, "uiplayerProcessMusicqStatusData", "no-dbidx");
    return;
  }

  song = dbGetByIdx (uiplayer->musicdb, dbidx);
  if (song == NULL) {
    uiplayerClearDisplay (uiplayer);
    logProcEnd (LOG_PROC, "uiplayerProcessMusicqStatusData", "null-song");
    return;
  }

  danceIdx = songGetNum (song, TAG_DANCE);
  data = danceGetStr (dances, danceIdx, DANCE_DANCE);
  uiLabelSetText (&uiplayer->danceLab, data);

  /* artist */
  data = songGetStr (song, TAG_ARTIST);
  uiLabelSetText (&uiplayer->artistLab, data);

  /* title */
  data = songGetStr (song, TAG_TITLE);
  uiLabelSetText (&uiplayer->titleLab, data);
  logProcEnd (LOG_PROC, "uiplayerProcessMusicqStatusData", "");
}

static bool
uiplayerFadeProcess (void *udata)
{
  uiplayer_t      *uiplayer = udata;

  logProcBegin (LOG_PROC, "uiplayerFadeProcess");
  logMsg (LOG_DBG, LOG_ACTIONS, "=action: fade button");
  connSendMessage (uiplayer->conn, ROUTE_PLAYER, MSG_PLAY_FADE, NULL);
  logProcEnd (LOG_PROC, "uiplayerFadeProcess", "");
  return UICB_CONT;
}

static bool
uiplayerPlayPauseProcess (void *udata)
{
  uiplayer_t      *uiplayer = udata;

  logProcBegin (LOG_PROC, "uiplayerPlayPauseProcess");
  logMsg (LOG_DBG, LOG_ACTIONS, "=action: play/pause button");
  connSendMessage (uiplayer->conn, ROUTE_MAIN, MSG_CMD_PLAYPAUSE, NULL);
  logProcEnd (LOG_PROC, "uiplayerPlayPauseProcess", "");
  return UICB_CONT;
}

static bool
uiplayerRepeatCallback (void *udata)
{
  uiplayer_t      *uiplayer = udata;

  logProcBegin (LOG_PROC, "uiplayerRepeatCallback");
  logMsg (LOG_DBG, LOG_ACTIONS, "=action: repeat button");

  if (uiplayer->repeatLock) {
    logProcEnd (LOG_PROC, "uiplayerRepeatCallback", "repeat-lock");
    return UICB_CONT;
  }

  connSendMessage (uiplayer->conn, ROUTE_PLAYER, MSG_PLAY_REPEAT, NULL);
  logProcEnd (LOG_PROC, "uiplayerRepeatCallback", "");
  return UICB_CONT;
}

static bool
uiplayerSongBeginProcess (void *udata)
{
  uiplayer_t      *uiplayer = udata;

  logProcBegin (LOG_PROC, "uiplayerSongBeginProcess");
  logMsg (LOG_DBG, LOG_ACTIONS, "=action: song begin button");
  connSendMessage (uiplayer->conn, ROUTE_PLAYER, MSG_PLAY_SONG_BEGIN, NULL);
  logProcEnd (LOG_PROC, "uiplayerSongBeginProcess", "");
  return UICB_CONT;
}

static bool
uiplayerNextSongProcess (void *udata)
{
  uiplayer_t      *uiplayer = udata;

  logProcBegin (LOG_PROC, "uiplayerNextSongProcess");
  logMsg (LOG_DBG, LOG_ACTIONS, "=action: next song button");
  connSendMessage (uiplayer->conn, ROUTE_PLAYER, MSG_PLAY_NEXTSONG, NULL);
  logProcEnd (LOG_PROC, "uiplayerNextSongProcess", "");
  return UICB_CONT;
}

static bool
uiplayerPauseatendCallback (void *udata)
{
  uiplayer_t      *uiplayer = udata;

  logProcBegin (LOG_PROC, "uiplayerPauseatendCallback");
  logMsg (LOG_DBG, LOG_ACTIONS, "=action: pause-at-end button");

  if (uiplayer->pauseatendLock) {
    logProcEnd (LOG_PROC, "uiplayerPauseatendCallback", "pae-lock");
    return UICB_STOP;
  }
  connSendMessage (uiplayer->conn, ROUTE_PLAYER, MSG_PLAY_PAUSEATEND, NULL);
  logProcEnd (LOG_PROC, "uiplayerPauseatendCallback", "");
  return UICB_CONT;
}

static bool
uiplayerSpeedCallback (void *udata, double value)
{
  uiplayer_t    *uiplayer = udata;
  char          tbuff [40];

  logProcBegin (LOG_PROC, "uiplayerSpeedCallback");
  logMsg (LOG_DBG, LOG_ACTIONS, "=action: speed chg");

  if (! uiplayer->speedLock) {
    mstimeset (&uiplayer->speedLockSend, UIPLAYER_LOCK_TIME_SEND);
  }
  uiplayer->speedLock = true;
  mstimeset (&uiplayer->speedLockTimeout, UIPLAYER_LOCK_TIME_WAIT);
  value = uiScaleEnforceMax (uiplayer->speedScale, value);
  snprintf (tbuff, sizeof (tbuff), "%3.0f", value);
  uiLabelSetText (&uiplayer->speedDisplayLab, tbuff);
  logProcEnd (LOG_PROC, "uiplayerSpeedCallback", "");
  return UICB_CONT;
}

static bool
uiplayerSeekCallback (void *udata, double value)
{
  uiplayer_t    *uiplayer = udata;
  char          tbuff [40];
  ssize_t       position;
  ssize_t       timeleft;

  logProcBegin (LOG_PROC, "uiplayerSeekCallback");
  logMsg (LOG_DBG, LOG_ACTIONS, "=action: position chg");

  if (! uiplayer->seekLock) {
    mstimeset (&uiplayer->seekLockSend, UIPLAYER_LOCK_TIME_SEND);
  }
  uiplayer->seekLock = true;
  mstimeset (&uiplayer->seekLockTimeout, UIPLAYER_LOCK_TIME_WAIT);

  value = uiScaleEnforceMax (uiplayer->seekScale, value);
  position = (ssize_t) round (value);

  tmutilToMS (position, tbuff, sizeof (tbuff));
  uiLabelSetText (&uiplayer->seekDisplayLab, tbuff);

  timeleft = uiplayer->lastdur - position;
  tmutilToMS (timeleft, tbuff, sizeof (tbuff));
  uiLabelSetText (&uiplayer->countdownTimerLab, tbuff);
  logProcEnd (LOG_PROC, "uiplayerSeekCallback", "");
  return UICB_CONT;
}

static bool
uiplayerVolumeCallback (void *udata, double value)
{
  uiplayer_t    *uiplayer = udata;
  char          tbuff [40];

  logProcBegin (LOG_PROC, "uiplayerVolumeCallback");
  logMsg (LOG_DBG, LOG_ACTIONS, "=action: volume chg");

  if (! uiplayer->volumeLock) {
    mstimeset (&uiplayer->volumeLockSend, UIPLAYER_LOCK_TIME_SEND);
  }
  uiplayer->volumeLock = true;
  mstimeset (&uiplayer->volumeLockTimeout, UIPLAYER_LOCK_TIME_WAIT);

  value = uiScaleEnforceMax (uiplayer->volumeScale, value);
  snprintf (tbuff, sizeof (tbuff), "%3.0f", value);
  uiLabelSetText (&uiplayer->volumeDisplayLab, tbuff);
  logProcEnd (LOG_PROC, "uiplayerVolumeCallback", "");
  return UICB_CONT;
}

static void
uiplayerClearDisplay (uiplayer_t *uiplayer)
{
  uiLabelSetText (&uiplayer->danceLab, "");
  uiLabelSetText (&uiplayer->artistLab, "");
  uiLabelSetText (&uiplayer->titleLab, "");
}
