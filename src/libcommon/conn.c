#include "config.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>

#include "bdjvars.h"
#include "conn.h"
#include "log.h"
#include "progstate.h"
#include "sock.h"
#include "sockh.h"

typedef struct conn {
  Sock_t        sock;
  uint16_t      port;
  bdjmsgroute_t routefrom;
  bool          handshakesent : 1;
  bool          handshakerecv : 1;
  bool          handshake : 1;
  bool          connected : 1;
} conn_t;

static bool     initialized = false;
static uint16_t connports [ROUTE_MAX];

/* note that connInit() must be called after bdjvarsInit() */
conn_t *
connInit (bdjmsgroute_t routefrom)
{
  conn_t     *conn;

  conn = malloc (sizeof (conn_t) * ROUTE_MAX);
  assert (conn != NULL);

  assert (bdjvarsIsInitialized () == true);

  if (! initialized) {
    connports [ROUTE_NONE] = 0;
    connports [ROUTE_MAIN] = bdjvarsGetNum (BDJVL_MAIN_PORT);
    connports [ROUTE_PLAYERUI] = bdjvarsGetNum (BDJVL_PLAYERUI_PORT);
    connports [ROUTE_MANAGEUI] = bdjvarsGetNum (BDJVL_MANAGEUI_PORT);
    connports [ROUTE_CONFIGUI] = bdjvarsGetNum (BDJVL_CONFIGUI_PORT);
    connports [ROUTE_PLAYER] = bdjvarsGetNum (BDJVL_PLAYER_PORT);
    connports [ROUTE_MOBILEMQ] = bdjvarsGetNum (BDJVL_MOBILEMQ_PORT);
    connports [ROUTE_REMCTRL] = bdjvarsGetNum (BDJVL_REMCTRL_PORT);
    connports [ROUTE_MARQUEE] = bdjvarsGetNum (BDJVL_MARQUEE_PORT);
    connports [ROUTE_STARTERUI] = bdjvarsGetNum (BDJVL_STARTERUI_PORT);
    connports [ROUTE_DBUPDATE] = bdjvarsGetNum (BDJVL_DBUPDATE_PORT);
    connports [ROUTE_DBTAG] = bdjvarsGetNum (BDJVL_DBTAG_PORT);
    connports [ROUTE_RAFFLE] = bdjvarsGetNum (BDJVL_RAFFLE_PORT);
    connports [ROUTE_HELPERUI] = bdjvarsGetNum (BDJVL_HELPERUI_PORT);
    connports [ROUTE_BPM_COUNTER] = bdjvarsGetNum (BDJVL_BPM_COUNTER_PORT);
    initialized = true;
  }

  for (bdjmsgroute_t i = ROUTE_NONE; i < ROUTE_MAX; ++i) {
    conn [i].sock = INVALID_SOCKET;
    conn [i].port = 0;
    conn [i].routefrom = routefrom;
    conn [i].handshakesent = false;
    conn [i].handshakerecv = false;
    conn [i].handshake = false;
    conn [i].connected = false;
  }

  return conn;
}

void
connFree (conn_t *conn)
{
  if (conn != NULL) {
    for (bdjmsgroute_t i = ROUTE_NONE; i < ROUTE_MAX; ++i) {
      conn [i].sock = INVALID_SOCKET;
      conn [i].port = 0;
      conn [i].connected = false;
      conn [i].handshakesent = false;
      conn [i].handshakerecv = false;
      conn [i].handshake = false;
    }
    free (conn);
    initialized = false;
  }
}

inline uint16_t
connPort (conn_t *conn, bdjmsgroute_t route)
{
  return connports [route];
}

void
connConnect (conn_t *conn, bdjmsgroute_t route)
{
  int         connerr = SOCK_CONN_OK;

  if (route >= ROUTE_MAX) {
    return;
  }

  if (connports [route] != 0 && ! conn [route].connected) {
    logMsg (LOG_DBG, LOG_SOCKET, "connect %d/%s to:%d/%s",
        conn [route].routefrom, msgRouteDebugText (conn [route].routefrom),
        route, msgRouteDebugText (route));
    conn [route].sock = sockConnect (connports [route], &connerr, conn [route].sock);
    if (connerr != SOCK_CONN_OK && connerr != SOCK_CONN_IN_PROGRESS) {
      sockClose (conn [route].sock);
      conn [route].sock = INVALID_SOCKET;
    }
  }

  if (connerr == SOCK_CONN_OK &&
      ! socketInvalid (conn [route].sock)) {
    if (sockhSendMessage (conn [route].sock, conn [route].routefrom, route,
        MSG_HANDSHAKE, NULL) < 0) {
      logMsg (LOG_DBG, LOG_SOCKET, "connect-send-handshake-fail %d/%s to:%d/%s",
          conn [route].routefrom, msgRouteDebugText (conn [route].routefrom),
          route, msgRouteDebugText (route));
      sockClose (conn [route].sock);
      conn [route].sock = INVALID_SOCKET;
    } else {
      logMsg (LOG_DBG, LOG_SOCKET, "connect ok %d/%s to:%d/%s",
          conn [route].routefrom, msgRouteDebugText (conn [route].routefrom),
          route, msgRouteDebugText (route));
      conn [route].handshakesent = true;
      if (conn [route].handshakerecv) {
        conn [route].handshake = true;
      }
      conn [route].connected = true;
    }
  }
}

void
connDisconnect (conn_t *conn, bdjmsgroute_t route)
{
  if (conn == NULL) {
    return;
  }

  if (route >= ROUTE_MAX) {
    return;
  }

  if (conn [route].connected) {
    logMsg (LOG_DBG, LOG_SOCKET, "disconnect %d/%s from:%d/%s",
        conn [route].routefrom, msgRouteDebugText (conn [route].routefrom),
        route, msgRouteDebugText (route));
    sockhSendMessage (conn [route].sock, conn [route].routefrom, route,
        MSG_SOCKET_CLOSE, NULL);
    sockClose (conn [route].sock);
  }

  conn [route].sock = INVALID_SOCKET;
  conn [route].connected = false;
  conn [route].handshakesent = false;
  conn [route].handshakerecv = false;
  conn [route].handshake = false;
}

void
connDisconnectAll (conn_t *conn)
{
  for (bdjmsgroute_t i = ROUTE_NONE; i < ROUTE_MAX; ++i) {
    connDisconnect (conn, i);
  }
}

bool
connCheckAll (conn_t *conn)
{
  bool      rc = true;

  for (bdjmsgroute_t i = ROUTE_NONE; i < ROUTE_MAX; ++i) {
    if (conn [i].connected) {
      rc = false;
      break;
    }
  }

  return rc;
}

void
connProcessHandshake (conn_t *conn, bdjmsgroute_t route)
{
  if (route >= ROUTE_MAX) {
    return;
  }

  conn [route].handshakerecv = true;
  if (conn [route].handshakesent) {
    conn [route].handshake = true;
  }
}

void
connProcessUnconnected (conn_t *conn)
{
  for (bdjmsgroute_t i = ROUTE_NONE; i < ROUTE_MAX; ++i) {
    if (conn [i].handshakerecv && ! conn [i].connected) {
      connConnect (conn, i);
    }
  }
}

void
connSendMessage (conn_t *conn, bdjmsgroute_t route,
    bdjmsgmsg_t msg, char *args)
{
  int         rc;

  if (conn == NULL) {
    return;
  }
  if (route >= ROUTE_MAX) {
    return;
  }
  if (! conn [route].connected) {
    logMsg (LOG_DBG, LOG_SOCKET, "msg not sent: not connected from:%d/%s route:%d/%s msg:%d/%s args:%s",
        conn [route].routefrom, msgRouteDebugText (conn [route].routefrom),
        route, msgRouteDebugText (route), msg, msgDebugText (msg), args);
    return;
  }
  if (socketInvalid (conn [route].sock)) {
    /* generally, this means the connection hasn't been made yet. */
    logMsg (LOG_DBG, LOG_SOCKET, "msg not sent: bad socket from:%d/%s route:%d/%s msg:%d/%s args:%s",
        conn [route].routefrom, msgRouteDebugText (conn [route].routefrom),
        route, msgRouteDebugText (route), msg, msgDebugText (msg), args);
    return;
  }

  rc = sockhSendMessage (conn [route].sock, conn [route].routefrom,
      route, msg, args);
  if (rc < 0) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "lost connection to %d", route);
    sockClose (conn [route].sock);
    conn [route].sock = INVALID_SOCKET;
    conn [route].connected = false;
    conn [route].handshakesent = false;
    conn [route].handshakerecv = false;
    conn [route].handshake = false;
  }
}

inline bool
connIsConnected (conn_t *conn, bdjmsgroute_t route)
{
  if (conn == NULL) {
    return false;
  }
  if (route >= ROUTE_MAX) {
    return false;
  }

  return conn [route].connected;
}

inline bool
connHaveHandshake (conn_t *conn, bdjmsgroute_t route)
{
  if (conn == NULL) {
    return false;
  }
  if (route >= ROUTE_MAX) {
    return false;
  }

  return conn [route].handshake;
}

bool
connWaitClosed (conn_t *conn, int *stopwaitcount)
{
  bool    crc;
  bool    rc = STATE_NOT_FINISH;

  crc = connCheckAll (conn);
  if (crc) {
    rc = STATE_FINISHED;
  } else {
    rc = STATE_NOT_FINISH;
    ++(*stopwaitcount);
    if (*stopwaitcount > STOP_WAIT_COUNT_MAX) {
      rc = STATE_FINISHED;
    }
  }

  if (rc == STATE_FINISHED) {
    connDisconnectAll (conn);
  }

  return rc;
}

