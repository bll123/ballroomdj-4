#ifndef INC_UIPLAYER_H
#define INC_UIPLAYER_H

#include <stdbool.h>

#include "bdj4.h"
#include "conn.h"
#include "progstate.h"
#include "ui.h"

enum {
  UIPLAYER_CB_FADE,
  UIPLAYER_CB_PLAYPAUSE,
  UIPLAYER_CB_BEGSONG,
  UIPLAYER_CB_NEXTSONG,
  UIPLAYER_CB_PAUSEATEND,
  UIPLAYER_CB_REPEAT,
  UIPLAYER_CB_MAX,
};

typedef struct {
  progstate_t     *progstate;
  conn_t          *conn;
  playerstate_t   playerState;
  musicdb_t       *musicdb;
  UICallback      callbacks [UIPLAYER_CB_MAX];
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
  UIWidget        songbeginButton;
  UIWidget        pauseatendButton;
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

