#ifndef INC_UIPLAYER_H
#define INC_UIPLAYER_H

#include <stdbool.h>

#include "bdj4.h"
#include "conn.h"
#include "progstate.h"
#include "uiutils.h"

enum {
  UIPLAYER_CALLBACK_FADE,
  UIPLAYER_CALLBACK_PLAYPAUSE,
  UIPLAYER_CALLBACK_BEGSONG,
  UIPLAYER_CALLBACK_NEXTSONG,
  UIPLAYER_CALLBACK_MAX,
};

typedef struct {
  progstate_t     *progstate;
  conn_t          *conn;
  playerstate_t   playerState;
  musicdb_t       *musicdb;
  UICallback      callbacks [UIPLAYER_CALLBACK_MAX];
  /* song display */
  UIWidget        vbox;
  UIWidget        statusImg;
  UIWidget        repeatImg;
  UIWidget        danceLab;
  UIWidget        artistLab;
  UIWidget        titleLab;
  /* speed controls / display */
  UIWidget        speedScale;
  UICallback      speedcb;
  UIWidget        speedDisplayLab;
  bool            speedLock;
  mstime_t        speedLockTimeout;
  mstime_t        speedLockSend;
  /* position controls / display */
  UIWidget        countdownTimerLab;
  UIWidget        durationLab;
  UIWidget        seekScale;
  UICallback      seekcb;
  UIWidget        seekDisplayLab;
  ssize_t         lastdur;
  bool            seekLock;
  mstime_t        seekLockTimeout;
  mstime_t        seekLockSend;
  /* main controls */
  bool            repeatLock;
  bool            pauseatendLock;
  UIWidget        repeatButton;
  UICallback      repeatcb;
  UIWidget        pauseatendButton;
  UICallback      pauseatendcb;
  UIWidget        playPixbuf;
  UIWidget        stopPixbuf;
  UIWidget        pausePixbuf;
  UIWidget        repeatPixbuf;
  UIWidget        ledoffImg;
  UIWidget        ledonImg;
  /* volume controls / display */
  UIWidget        volumeScale;
  UICallback      volumecb;
  bool            volumeLock;
  mstime_t        volumeLockTimeout;
  mstime_t        volumeLockSend;
  UIWidget        volumeDisplayLab;
  bool            uibuilt;
} uiplayer_t;

uiplayer_t  * uiplayerInit (progstate_t *progstate, conn_t *conn, musicdb_t *musicdb);
void        uiplayerSetDatabase (uiplayer_t *uiplayer, musicdb_t *musicdb);
void        uiplayerFree (uiplayer_t *uiplayer);
UIWidget    *uiplayerBuildUI (uiplayer_t *uiplayer);
void        uiplayerMainLoop (uiplayer_t *uiplayer);
int         uiplayerProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
                bdjmsgmsg_t msg, char *args, void *udata);

#endif /* INC_UIPLAYER_H */

