#ifndef INC_UIPLAYER_H
#define INC_UIPLAYER_H

#include <stdbool.h>

#include "bdjmsg.h"
#include "conn.h"
#include "musicdb.h"
#include "progstate.h"
#include "ui.h"

typedef struct uiplayer uiplayer_t;

uiplayer_t  * uiplayerInit (progstate_t *progstate, conn_t *conn, musicdb_t *musicdb);
void        uiplayerSetDatabase (uiplayer_t *uiplayer, musicdb_t *musicdb);
void        uiplayerFree (uiplayer_t *uiplayer);
UIWidget    *uiplayerBuildUI (uiplayer_t *uiplayer);
void        uiplayerMainLoop (uiplayer_t *uiplayer);
int         uiplayerProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
                bdjmsgmsg_t msg, char *args, void *udata);

#endif /* INC_UIPLAYER_H */

