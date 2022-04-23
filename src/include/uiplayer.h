#ifndef INC_UIPLAYER_H
#define INC_UIPLAYER_H

#include <stdbool.h>
#include <gtk/gtk.h>

#include "bdj4.h"
#include "conn.h"
#include "progstate.h"

typedef struct {
  progstate_t     *progstate;
  conn_t          *conn;
  playerstate_t   playerState;
  musicdb_t       *musicdb;
  /* song display */
  GtkWidget       *vbox;
  GtkWidget       *statusImg;
  GtkWidget       *repeatImg;
  GtkWidget       *danceLab;
  GtkWidget       *artistLab;
  GtkWidget       *titleLab;
  /* speed controls / display */
  GtkWidget       *speedScale;
  GtkWidget       *speedDisplayLab;
  bool            speedLock;
  mstime_t        speedLockTimeout;
  mstime_t        speedLockSend;
  /* position controls / display */
  GtkWidget       *countdownTimerLab;
  GtkWidget       *durationLab;
  GtkWidget       *seekScale;
  GtkWidget       *seekDisplayLab;
  ssize_t         lastdur;
  bool            seekLock;
  mstime_t        seekLockTimeout;
  mstime_t        seekLockSend;
  /* main controls */
  GtkWidget       *repeatButton;
  bool            repeatLock;
  GtkWidget       *pauseatendButton;
  bool            pauseatendLock;
  GdkPixbuf       *playImg;
  GdkPixbuf       *stopImg;
  GdkPixbuf       *pauseImg;
  GtkWidget       *ledoffImg;
  GtkWidget       *ledonImg;
  /* volume controls / display */
  GtkWidget       *volumeScale;
  bool            volumeLock;
  mstime_t        volumeLockTimeout;
  mstime_t        volumeLockSend;
  GtkWidget       *volumeDisplayLab;
} uiplayer_t;

uiplayer_t  * uiplayerInit (progstate_t *progstate, conn_t *conn, musicdb_t *musicdb);
void        uiplayerFree (uiplayer_t *uiplayer);
GtkWidget   * uiplayerActivate (uiplayer_t *uiplayer);
void        uiplayerMainLoop (uiplayer_t *uiplayer);
int         uiplayerProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
                bdjmsgmsg_t msg, char *args, void *udata);

#endif /* INC_UIPLAYER_H */

