/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIPLAYER_H
#define INC_UIPLAYER_H

#include <stdbool.h>

#include "bdjmsg.h"
#include "conn.h"
#include "controller.h"
#include "dispsel.h"
#include "musicdb.h"
#include "progstate.h"
#include "uiwcont.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef struct uiplayer uiplayer_t;

uiplayer_t  * uiplayerInit (const char *tag, progstate_t *progstate, conn_t *conn, musicdb_t *musicdb, dispsel_t *dispsel);
void  uiplayerSetDatabase (uiplayer_t *uiplayer, musicdb_t *musicdb);
void  uiplayerSetController (uiplayer_t *uiplayer, controller_t *controller);
void  uiplayerFree (uiplayer_t *uiplayer);
uiwcont_t     *uiplayerBuildUI (uiplayer_t *uiplayer);
void  uiplayerMainLoop (uiplayer_t *uiplayer);
int   uiplayerProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route, bdjmsgmsg_t msg, char *args, void *udata);
dbidx_t uiplayerGetCurrSongIdx (uiplayer_t *uiplayer);
void  uiplayerDisableSpeed (uiplayer_t *uiplayer);
void  uiplayerDisableSeek (uiplayer_t *uiplayer);
void  uiplayerGetVolumeSpeed (uiplayer_t *uiplayer, int *baseVolume, double *volume, double *speed);
bool  uiplayerGetRepeat (uiplayer_t *uiplayer);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_UIPLAYER_H */
