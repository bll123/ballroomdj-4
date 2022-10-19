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
#include "inline.h"
#include "log.h"
#include "musicdb.h"
#include "pathbld.h"
#include "player.h"
#include "progstate.h"
#include "song.h"
#include "tagdef.h"
#include "uiplayer.h"
#include "ui.h"

/* there are all sorts of latency issues making the sliders work nicely */
/* it will take at least 100ms and at most 200ms for the message to get */
/* back.  Then there are the latency issues on this end. */
enum {
  UIPLAYER_LOCK_TIME_WAIT = 300,
  UIPLAYER_LOCK_TIME_SEND = 30,
};

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
  uiplayer = malloc (sizeof (uiplayer_t));
  assert (uiplayer != NULL);
  uiplayer->progstate = progstate;
  uiplayer->conn = conn;
  uiplayer->musicdb = musicdb;
  uiplayer->uibuilt = false;

  uiutilsUIWidgetInit (&uiplayer->vbox);
  uiutilsUIWidgetInit (&uiplayer->statusImg);
  uiutilsUIWidgetInit (&uiplayer->repeatImg);
  uiutilsUIWidgetInit (&uiplayer->danceLab);
  uiutilsUIWidgetInit (&uiplayer->artistLab);
  uiutilsUIWidgetInit (&uiplayer->titleLab);
  uiutilsUIWidgetInit (&uiplayer->countdownTimerLab);
  uiutilsUIWidgetInit (&uiplayer->durationLab);
  uiutilsUIWidgetInit (&uiplayer->speedDisplayLab);
  uiutilsUIWidgetInit (&uiplayer->seekDisplayLab);
  uiutilsUIWidgetInit (&uiplayer->speedScale);
  uiutilsUIWidgetInit (&uiplayer->seekScale);
  uiutilsUIWidgetInit (&uiplayer->repeatButton);
  uiutilsUIWidgetInit (&uiplayer->songbeginButton);
  uiutilsUIWidgetInit (&uiplayer->pauseatendButton);
  uiutilsUIWidgetInit (&uiplayer->playPixbuf);
  uiutilsUIWidgetInit (&uiplayer->stopPixbuf);
  uiutilsUIWidgetInit (&uiplayer->pausePixbuf);
  uiutilsUIWidgetInit (&uiplayer->repeatPixbuf);
  uiutilsUIWidgetInit (&uiplayer->ledoffImg);
  uiutilsUIWidgetInit (&uiplayer->ledonImg);
  uiutilsUIWidgetInit (&uiplayer->volumeDisplayLab);
  uiutilsUIWidgetInit (&uiplayer->volumeScale);

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

UIWidget *
uiplayerBuildUI (uiplayer_t *uiplayer)
{
  char            tbuff [MAXPATHLEN];
  UIWidget        uiwidget;
  UIWidget        hbox;
  UIWidget        tbox;
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

  uiCreateVertBox (&uiplayer->vbox);
  uiWidgetExpandHoriz (&uiplayer->vbox);

  /* song display */

  uiCreateHorizBox (&hbox);
  uiWidgetExpandHoriz (&hbox);
  uiBoxPackStart (&uiplayer->vbox, &hbox);

  /* size group E */
  uiCreateHorizBox (&tbox);
  uiBoxPackStart (&hbox, &tbox);
  uiSizeGroupAdd (&sgE, &tbox);

  uiImageNew (&uiplayer->statusImg);

  pathbldMakePath (tbuff, sizeof (tbuff), "button_stop", ".svg",
      PATHBLD_MP_IMGDIR | PATHBLD_MP_USEIDX);
  uiImageFromFile (&uiplayer->stopPixbuf, tbuff);
  uiImageGetPixbuf (&uiplayer->stopPixbuf);
  uiWidgetMakePersistent (&uiplayer->stopPixbuf);

  uiImageSetFromPixbuf (&uiplayer->statusImg, &uiplayer->stopPixbuf);
  uiWidgetSetSizeRequest (&uiplayer->statusImg, 18, -1);
  uiWidgetSetMarginStart (&uiplayer->statusImg, uiBaseMarginSz);
  uiBoxPackStart (&tbox, &uiplayer->statusImg);

  pathbldMakePath (tbuff, sizeof (tbuff), "button_play", ".svg",
      PATHBLD_MP_IMGDIR | PATHBLD_MP_USEIDX);
  uiImageFromFile (&uiplayer->playPixbuf, tbuff);
  uiImageGetPixbuf (&uiplayer->playPixbuf);
  uiWidgetMakePersistent (&uiplayer->playPixbuf);

  pathbldMakePath (tbuff, sizeof (tbuff), "button_pause", ".svg",
      PATHBLD_MP_IMGDIR | PATHBLD_MP_USEIDX);
  uiImageFromFile (&uiplayer->pausePixbuf, tbuff);
  uiImageGetPixbuf (&uiplayer->pausePixbuf);
  uiWidgetMakePersistent (&uiplayer->pausePixbuf);

  pathbldMakePath (tbuff, sizeof (tbuff), "button_repeat", ".svg",
      PATHBLD_MP_IMGDIR | PATHBLD_MP_USEIDX);
  uiImageFromFile (&uiplayer->repeatPixbuf, tbuff);
  uiImageGetPixbuf (&uiplayer->repeatPixbuf);
  uiWidgetMakePersistent (&uiplayer->repeatPixbuf);

  uiImageNew (&uiplayer->repeatImg);
  uiImageClear (&uiplayer->repeatImg);
  uiWidgetSetSizeRequest (&uiplayer->repeatImg, 18, -1);
  uiWidgetSetMarginStart (&uiplayer->repeatImg, uiBaseMarginSz);
  uiBoxPackStart (&tbox, &uiplayer->repeatImg);

  uiCreateLabel (&uiplayer->danceLab, "");
  uiBoxPackStart (&hbox, &uiplayer->danceLab);

  uiCreateLabel (&uiwidget, " : ");
  uiWidgetSetMarginStart (&uiwidget, 0);
  uiBoxPackStart (&hbox, &uiwidget);

  uiCreateLabel (&uiplayer->artistLab, "");
  uiWidgetSetMarginStart (&uiplayer->artistLab, 0);
  uiLabelEllipsizeOn (&uiplayer->artistLab);
  uiBoxPackStart (&hbox, &uiplayer->artistLab);

  uiCreateLabel (&uiwidget, " : ");
  uiWidgetSetMarginStart (&uiwidget, 0);
  uiBoxPackStart (&hbox, &uiwidget);

  uiCreateLabel (&uiplayer->titleLab, "");
  uiWidgetSetMarginStart (&uiplayer->titleLab, 0);
  uiLabelEllipsizeOn (&uiplayer->titleLab);
  uiBoxPackStart (&hbox, &uiplayer->titleLab);

  /* expanding label to take space */
  uiCreateLabel (&uiwidget, "");
  uiWidgetExpandHoriz (&uiwidget);
  uiBoxPackStart (&hbox, &uiwidget);

  /* size group A */
  uiCreateLabel (&uiwidget, "%");
  uiBoxPackEnd (&hbox, &uiwidget);
  uiSizeGroupAdd (&sgA, &uiwidget);

  /* size group B */
  uiCreateLabel (&uiplayer->speedDisplayLab, "100");
  uiLabelAlignEnd (&uiplayer->speedDisplayLab);
  uiBoxPackEnd (&hbox, &uiplayer->speedDisplayLab);
  uiSizeGroupAdd (&sgB, &uiplayer->speedDisplayLab);

  /* size group C */
  uiCreateScale (&uiplayer->speedScale, 70.0, 130.0, 1.0, 10.0, 100.0, 0);
  uiBoxPackEnd (&hbox, &uiplayer->speedScale);
  uiSizeGroupAdd (&sgC, &uiplayer->speedScale);
  uiutilsUICallbackDoubleInit (&uiplayer->speedcb,
      uiplayerSpeedCallback, uiplayer);
  uiScaleSetCallback (&uiplayer->speedScale, &uiplayer->speedcb);

  /* size group D */
  /* CONTEXT: playerui: the current speed for song playback */
  uiCreateColonLabel (&uiwidget, _("Speed"));
  uiLabelAlignEnd (&uiwidget);
  uiWidgetSetMarginEnd (&uiwidget, uiBaseMarginSz);
  uiBoxPackEnd (&hbox, &uiwidget);
  uiSizeGroupAdd (&sgD, &uiwidget);

  /* position controls / display */

  uiCreateHorizBox (&hbox);
  uiWidgetExpandHoriz (&hbox);
  uiBoxPackStart (&uiplayer->vbox, &hbox);

  /* size group E */
  uiCreateLabel (&uiwidget, "");
  uiBoxPackStart (&hbox, &uiwidget);
  uiSizeGroupAdd (&sgE, &uiwidget);

  uiCreateLabel (&uiplayer->countdownTimerLab, " 0:00");
  uiLabelAlignEnd (&uiplayer->countdownTimerLab);
  uiBoxPackStart (&hbox, &uiplayer->countdownTimerLab);

  uiCreateLabel (&uiwidget, " / ");
  uiWidgetSetMarginStart (&uiwidget, 0);
  uiBoxPackStart (&hbox, &uiwidget);

  uiCreateLabel (&uiplayer->durationLab, " 3:00");
  uiLabelAlignEnd (&uiplayer->durationLab);
  uiBoxPackStart (&hbox, &uiplayer->durationLab);

  uiCreateLabel (&uiwidget, "");
  uiWidgetExpandHoriz (&uiwidget);
  uiBoxPackStart (&hbox, &uiwidget);

  /* size group A */
  uiCreateLabel (&uiwidget, "");
  uiBoxPackEnd (&hbox, &uiwidget);
  uiSizeGroupAdd (&sgA, &uiwidget);

  /* size group B */
  uiCreateLabel (&uiplayer->seekDisplayLab, "3:00");
  uiLabelAlignEnd (&uiplayer->seekDisplayLab);
  uiSizeGroupAdd (&sgB, &uiplayer->seekDisplayLab);
  uiBoxPackEnd (&hbox, &uiplayer->seekDisplayLab);

  /* size group C */
  uiCreateScale (&uiplayer->seekScale, 0.0, 180000.0, 1000.0, 10000.0, 0.0, 0);
  uiBoxPackEnd (&hbox, &uiplayer->seekScale);
  uiSizeGroupAdd (&sgC, &uiplayer->seekScale);
  uiutilsUICallbackDoubleInit (&uiplayer->seekcb,
      uiplayerSeekCallback, uiplayer);
  uiScaleSetCallback (&uiplayer->seekScale, &uiplayer->seekcb);

  /* size group D */
  /* CONTEXT: playerui: the current position of the song during song playback */
  uiCreateColonLabel (&uiwidget, _("Position"));
  uiLabelAlignEnd (&uiwidget);
  uiWidgetSetMarginEnd (&uiwidget, uiBaseMarginSz);
  uiBoxPackEnd (&hbox, &uiwidget);
  uiSizeGroupAdd (&sgD, &uiwidget);

  /* main controls */

  uiCreateHorizBox (&hbox);
  uiWidgetExpandHoriz (&hbox);
  uiBoxPackStart (&uiplayer->vbox, &hbox);

  /* size group E */
  uiCreateLabel (&uiwidget, "");
  uiBoxPackStart (&hbox, &uiwidget);
  uiSizeGroupAdd (&sgE, &uiwidget);

  uiutilsUICallbackInit (&uiplayer->callbacks [UIPLAYER_CB_FADE],
      uiplayerFadeProcess, uiplayer, NULL);
  uiCreateButton (&uiwidget,
      &uiplayer->callbacks [UIPLAYER_CB_FADE],
      /* CONTEXT: playerui: button: fade out the song and stop playing it */
      _("Fade"), NULL);
  uiBoxPackStart (&hbox, &uiwidget);

  uiutilsUICallbackInit (&uiplayer->callbacks [UIPLAYER_CB_PLAYPAUSE],
      uiplayerPlayPauseProcess, uiplayer, NULL);
  uiCreateButton (&uiwidget,
      &uiplayer->callbacks [UIPLAYER_CB_PLAYPAUSE],
      /* CONTEXT: playerui: button: tooltip: play or pause the song */
      _("Play / Pause"), "button_playpause");
  uiBoxPackStart (&hbox, &uiwidget);

  pathbldMakePath (tbuff, sizeof (tbuff), "button_repeat", ".svg",
      PATHBLD_MP_IMGDIR | PATHBLD_MP_USEIDX);
  uiutilsUICallbackInit (&uiplayer->callbacks [UIPLAYER_CB_REPEAT],
      uiplayerRepeatCallback, uiplayer, NULL);
  uiCreateToggleButton (&uiplayer->repeatButton, "",
      /* CONTEXT: playerui: button: tooltip: toggle the repeat song on and off */
      tbuff, _("Toggle Repeat"), NULL, 0);
  uiBoxPackStart (&hbox, &uiplayer->repeatButton);
  uiToggleButtonSetCallback (&uiplayer->repeatButton,
      &uiplayer->callbacks [UIPLAYER_CB_REPEAT]);

  uiutilsUICallbackInit (&uiplayer->callbacks [UIPLAYER_CB_BEGSONG],
      uiplayerSongBeginProcess, uiplayer, NULL);
  uiCreateButton (&uiwidget,
      &uiplayer->callbacks [UIPLAYER_CB_BEGSONG],
      /* CONTEXT: playerui: button: tooltip: return to the beginning of the song */
      _("Return to beginning of song"), "button_begin");
  uiBoxPackStart (&hbox, &uiwidget);

  uiutilsUICallbackInit (&uiplayer->callbacks [UIPLAYER_CB_NEXTSONG],
      uiplayerNextSongProcess, uiplayer, NULL);
  uiCreateButton (&uiwidget,
      &uiplayer->callbacks [UIPLAYER_CB_NEXTSONG],
      /* CONTEXT: playerui: button: tooltip: start playing the next song (immediate) */
      _("Next Song"), "button_nextsong");
  uiBoxPackStart (&hbox, &uiwidget);

  pathbldMakePath (tbuff, sizeof (tbuff), "led_on", ".svg",
      PATHBLD_MP_IMGDIR);
  uiImageFromFile (&uiplayer->ledonImg, tbuff);
  uiWidgetMakePersistent (&uiplayer->ledonImg);

  pathbldMakePath (tbuff, sizeof (tbuff), "led_off", ".svg",
      PATHBLD_MP_IMGDIR);
  uiImageFromFile (&uiplayer->ledoffImg, tbuff);
  uiWidgetMakePersistent (&uiplayer->ledoffImg);

  /* CONTEXT: playerui: button: pause at the end of the song (toggle) */
  uiutilsUICallbackInit (&uiplayer->callbacks [UIPLAYER_CB_PAUSEATEND],
      uiplayerPauseatendCallback, uiplayer, NULL);
  uiCreateToggleButton (&uiplayer->pauseatendButton, _("Pause at End"),
      NULL, NULL, &uiplayer->ledoffImg, 0);
  uiBoxPackStart (&hbox, &uiplayer->pauseatendButton);
  uiToggleButtonSetCallback (&uiplayer->pauseatendButton,
      &uiplayer->callbacks [UIPLAYER_CB_PAUSEATEND]);

  /* volume controls / display */

  /* size group A */
  uiCreateLabel (&uiwidget, "%");
  uiBoxPackEnd (&hbox, &uiwidget);
  uiSizeGroupAdd (&sgA, &uiwidget);

  /* size group B */
  uiCreateLabel (&uiplayer->volumeDisplayLab, "100");
  uiLabelAlignEnd (&uiplayer->volumeDisplayLab);
  uiBoxPackEnd (&hbox, &uiplayer->volumeDisplayLab);
  uiSizeGroupAdd (&sgB, &uiplayer->volumeDisplayLab);

  /* size group C */
  uiCreateScale (&uiplayer->volumeScale, 0.0, 100.0, 1.0, 10.0, 0.0, 0);
  uiBoxPackEnd (&hbox, &uiplayer->volumeScale);
  uiSizeGroupAdd (&sgC, &uiplayer->volumeScale);
  uiutilsUICallbackDoubleInit (&uiplayer->volumecb,
      uiplayerVolumeCallback, uiplayer);
  uiScaleSetCallback (&uiplayer->volumeScale, &uiplayer->volumecb);

  /* size group D */
  /* CONTEXT: playerui: The current volume of the song */
  uiCreateColonLabel (&uiwidget, _("Volume"));
  uiLabelAlignEnd (&uiwidget);
  uiWidgetSetMarginEnd (&uiwidget, uiBaseMarginSz);
  uiBoxPackEnd (&hbox, &uiwidget);
  uiSizeGroupAdd (&sgD, &uiwidget);

  uiplayer->uibuilt = true;

  logProcEnd (LOG_PROC, "uiplayerBuildUI", "");
  return &uiplayer->vbox;
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

    value = uiScaleGetValue (&uiplayer->volumeScale);
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

    value = uiScaleGetValue (&uiplayer->speedScale);
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

    value = uiScaleGetValue (&uiplayer->seekScale);
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
  bool          dbgdisp = false;
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
          dbgdisp = true;
          break;
        }
        case MSG_PLAY_PAUSEATEND_STATE: {
          uiplayerProcessPauseatend (uiplayer, atol (targs));
          dbgdisp = true;
          break;
        }
        case MSG_PLAYER_STATUS_DATA: {
          uiplayerProcessPlayerStatusData (uiplayer, targs);
          // dbgdisp = true;
          break;
        }
        case MSG_MUSICQ_STATUS_DATA: {
          uiplayerProcessMusicqStatusData (uiplayer, targs);
          dbgdisp = true;
          break;
        }
        case MSG_FINISHED: {
          uiplayerClearDisplay (uiplayer);
          dbgdisp = true;
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

  if (dbgdisp) {
    logMsg (LOG_DBG, LOG_MSGS, "uiplayer: rcvd: from:%d/%s route:%d/%s msg:%d/%s args:%s",
        routefrom, msgRouteDebugText (routefrom),
        route, msgRouteDebugText (route), msg, msgDebugText (msg), args);
  }
  dataFree (targs);

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

  if (! uiplayer->uibuilt) {
    logProcEnd (LOG_PROC, "uiplayerProcessPauseatend", "no-ui");
    return;
  }
  uiplayer->pauseatendLock = true;

  if (on) {
    uiToggleButtonSetImage (&uiplayer->pauseatendButton, &uiplayer->ledonImg);
    uiToggleButtonSetState (&uiplayer->pauseatendButton, TRUE);
  } else {
    uiToggleButtonSetImage (&uiplayer->pauseatendButton, &uiplayer->ledoffImg);
    uiToggleButtonSetState (&uiplayer->pauseatendButton, FALSE);
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
    uiWidgetDisable (&uiplayer->volumeScale);
    uiWidgetDisable (&uiplayer->speedScale);
    uiWidgetDisable (&uiplayer->seekScale);
    uiWidgetDisable (&uiplayer->songbeginButton);
  } else {
    uiWidgetEnable (&uiplayer->volumeScale);
    uiWidgetEnable (&uiplayer->speedScale);
    uiWidgetEnable (&uiplayer->seekScale);
    uiWidgetEnable (&uiplayer->songbeginButton);
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
  ssize_t       timeleft = 0;
  ssize_t       position = 0;
  ssize_t       dur = 0;

  logProcBegin (LOG_PROC, "uiplayerProcessPlayerStatusData");

  /* repeat */
  p = strtok_r (args, MSG_ARGS_RS_STR, &tokstr);
  if (p != NULL) {
    uiplayer->repeatLock = true;
    if (atol (p)) {
      uiImageClear (&uiplayer->repeatImg);
      uiImageSetFromPixbuf (&uiplayer->repeatImg, &uiplayer->repeatPixbuf);
      uiToggleButtonSetState (&uiplayer->repeatButton, TRUE);
    } else {
      uiImageClear (&uiplayer->repeatImg);
      uiToggleButtonSetState (&uiplayer->repeatButton, FALSE);
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
    uiScaleSetValue (&uiplayer->volumeScale, dval);
  }

  /* speed */
  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  if (! uiplayer->speedLock) {
    snprintf (tbuff, sizeof (tbuff), "%3s", p);
    uiLabelSetText (&uiplayer->speedDisplayLab, p);
    dval = atof (p);
    uiScaleSetValue (&uiplayer->speedScale, dval);
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
    uiScaleSetRange (&uiplayer->seekScale, 0.0, ddur);
    uiplayer->lastdur = dur;
  }

  if (! uiplayer->seekLock) {
    tmutilToMS (position, tbuff, sizeof (tbuff));
    uiLabelSetText (&uiplayer->seekDisplayLab, tbuff);

    timeleft = dur - position;
    tmutilToMS (timeleft, tbuff, sizeof (tbuff));
    uiLabelSetText (&uiplayer->countdownTimerLab, tbuff);

    if (ddur == 0.0) {
      uiScaleSetValue (&uiplayer->seekScale, 0.0);
    } else {
      uiScaleSetValue (&uiplayer->seekScale, dval);
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

  if (! uiplayer->uibuilt) {
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
  value = uiScaleEnforceMax (&uiplayer->speedScale, value);
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

  value = uiScaleEnforceMax (&uiplayer->seekScale, value);
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

  value = uiScaleEnforceMax (&uiplayer->volumeScale, value);
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
