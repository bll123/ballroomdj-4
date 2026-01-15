/*
 * Copyright 2026 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>

#include "bdjopt.h"
#include "callback.h"
#include "conn.h"
#include "continst.h"
#include "controller.h"
#include "mdebug.h"
#include "progstate.h"
#include "uiplayer.h"

enum {
  CI_CB_CONTROLLER,
  CI_CB_CONT_URI,
  CI_CB_CONT_URI_CB,
  CI_CB_MAX,
};

typedef struct continst {
  controller_t    *controller;
  conn_t          *conn;
  uiplayer_t      *uiplayer;
  callback_t      *callbacks [CI_CB_MAX];
} continst_t;

static bool contInstanceCallback (void *udata, int32_t cmd, int32_t val);
static bool contInstanceURICallback (void *udata, const char *uri, int32_t cmd);

continst_t *
contInstanceInit (conn_t *conn, uiplayer_t *uiplayer)
{
  continst_t    *ci;

  ci = mdmalloc (sizeof (continst_t));
  ci->controller = NULL;
  ci->conn = conn;
  ci->uiplayer = uiplayer;
  for (int i = 0; i < CI_CB_MAX; ++i) {
    ci->callbacks [i] = NULL;
  }

  return ci;
}

void
contInstanceFree (continst_t *ci)
{
  if (ci == NULL) {
    return;
  }

  for (int i = 0; i < CI_CB_MAX; ++i) {
    callbackFree (ci->callbacks [i]);
  }

  controllerFree (ci->controller);
  mdfree (ci);
}


int
contInstanceSetup (continst_t *ci)
{
  bool          rc = STATE_NOT_FINISH;

  if (ci->controller == NULL) {
    const char  *val;

    val = bdjoptGetStr (OPT_M_CONTROLLER_INTFC);
    if (val != NULL && *val) {
      ci->controller = controllerInit (val);
      if (ci->controller != NULL) {
        ci->callbacks [CI_CB_CONTROLLER] =
            callbackInitII (contInstanceCallback, ci);
        ci->callbacks [CI_CB_CONT_URI] =
            callbackInitSI (contInstanceURICallback, ci);
        controllerSetCallbacks (ci->controller,
            ci->callbacks [CI_CB_CONTROLLER],
            ci->callbacks [CI_CB_CONT_URI]);
      }
    } else {
      rc = STATE_FINISHED;
    }
  }

  if (ci->controller != NULL &&
      controllerCheckReady (ci->controller)) {
    controllerSetup (ci->controller);
    uiplayerSetController (ci->uiplayer, ci->controller);
    rc = STATE_FINISHED;
  }

  return rc;
}

void
contInstanceSetUIPlayer (continst_t *ci, uiplayer_t *uiplayer)
{
  if (ci == NULL || uiplayer == NULL) {
    return;
  }

  ci->uiplayer = uiplayer;
}

void
contInstanceSetURICallback (continst_t *ci, callback_t *cburi)
{
  if (ci == NULL || cburi == NULL) {
    return;
  }

  ci->callbacks [CI_CB_CONT_URI_CB] = cburi;
}

controller_t *
contInstanceGetController (continst_t *ci)
{
  if (ci == NULL) {
    return NULL;
  }

  return ci->controller;
}

/* internal routines */

static bool
contInstanceCallback (void *udata, int32_t cmd, int32_t val)
{
  continst_t    *ci = udata;
  bool          rc = false;

  switch (cmd) {
    case CONTROLLER_PLAY: {
      connSendMessage (ci->conn, ROUTE_MAIN, MSG_CMD_PLAY, NULL);
      break;
    }
    case CONTROLLER_PAUSE: {
      connSendMessage (ci->conn, ROUTE_MAIN, MSG_CMD_PLAYPAUSE, NULL);
      break;
    }
    case CONTROLLER_PLAYPAUSE: {
      connSendMessage (ci->conn, ROUTE_MAIN, MSG_CMD_PLAYPAUSE, NULL);
      break;
    }
    case CONTROLLER_STOP: {
      connSendMessage (ci->conn, ROUTE_PLAYER, MSG_PLAY_STOP, NULL);
      break;
    }
    case CONTROLLER_NEXT: {
      connSendMessage (ci->conn, ROUTE_PLAYER, MSG_PLAY_NEXTSONG, NULL);
      break;
    }
    case CONTROLLER_SEEK: {
      char    tmp [40];

      snprintf (tmp, sizeof (tmp), "%d", val);
      connSendMessage (ci->conn, ROUTE_PLAYER, MSG_PLAY_SEEK, tmp);
      break;
    }
    case CONTROLLER_SEEK_SKIP: {
      char    tmp [40];

      snprintf (tmp, sizeof (tmp), "%d", val);
      connSendMessage (ci->conn, ROUTE_PLAYER, MSG_PLAY_SEEK_SKIP, tmp);
      break;
    }
    case CONTROLLER_VOLUME: {
      char    tmp [40];

      snprintf (tmp, sizeof (tmp), "%d", val);
      connSendMessage (ci->conn, ROUTE_PLAYER, MSG_PLAYER_VOLUME, tmp);
      break;
    }
    case CONTROLLER_REPEAT: {
      bool    repflag = false;

      repflag = uiplayerGetRepeat (ci->uiplayer);
      if (val != repflag) {
        /* only toggle repeat if it does not match the current setting */
        connSendMessage (ci->conn, ROUTE_PLAYER, MSG_PLAY_REPEAT, NULL);
      }
      break;
    }
    case CONTROLLER_QUIT: {
      /* do not have route-from */
      connSendMessage (ci->conn, ROUTE_PLAYERUI, MSG_EXIT_REQUEST, NULL);
      connSendMessage (ci->conn, ROUTE_MANAGEUI, MSG_EXIT_REQUEST, NULL);
      break;
    }
  }

  return rc;
}

static bool
contInstanceURICallback (void *udata, const char *uri, int32_t cmd)
{
  continst_t      *ci = udata;

  if (ci == NULL) {
    return false;
  }

  if (ci->callbacks [CI_CB_CONT_URI_CB] != NULL) {
    callbackHandlerSI (ci->callbacks [CI_CB_CONT_URI_CB], uri, cmd);
  }

  return true;
}

