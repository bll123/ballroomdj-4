/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <string.h>
#include <inttypes.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#if __has_include (<winsock2.h>)
# include <winsock2.h>
#endif

#include "bdj4.h"
#include "bdjvars.h"
#include "conn.h"
#include "log.h"
#include "mdebug.h"
#include "nodiscard.h"
#include "progstate.h"
#include "sock.h"
#include "sockh.h"
#include "tmutil.h"

typedef struct conn {
  Sock_t        sock;
  uint16_t      port;
  bdjmsgroute_t routefrom;
  mstime_t      connchk;
  bool          handshakesent;
  bool          handshakerecv;
  bool          handshake;
  bool          connected;
} conn_t;

static volatile atomic_flag initialized = ATOMIC_FLAG_INIT;
static uint16_t connports [ROUTE_MAX];

/**
 * Check and see if there are any connections active.
 * @param[in] conn The connection.
 * @return true if all connections are disconnected.  false otherwise.
 */
static bool connCheckAll (conn_t *conn);
static Sock_t connTryConnect (uint16_t port, int *connerr, Sock_t sock);
static void connBaseInit (void);

/* note that connInit() must be called after bdjvarsInit() */
BDJ_NODISCARD
conn_t *
connInit (bdjmsgroute_t routefrom)
{
  conn_t     *conn;

  conn = mdmalloc (sizeof (conn_t) * ROUTE_MAX);

  if (bdjvarsIsInitialized () != true) {
    fprintf (stderr, "ERR: connInit called before bdjvars-init\n");
    return NULL;
  }

  connBaseInit ();

  for (bdjmsgroute_t i = ROUTE_NONE; i < ROUTE_MAX; ++i) {
    conn [i].sock = INVALID_SOCKET;
    conn [i].port = 0;
    conn [i].routefrom = routefrom;
    conn [i].handshakesent = false;
    conn [i].handshakerecv = false;
    conn [i].handshake = false;
    conn [i].connected = false;
    mstimeset (&conn [i].connchk, 0);
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
    mdfree (conn);
    atomic_flag_clear (&initialized);
  }
}

uint16_t
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

  /* don't check the connection too often */
  if (! mstimeCheck (&conn [route].connchk)) {
    return;
  }

  mstimeset (&conn [route].connchk, 40);
  if (connports [route] != 0 && ! conn [route].connected) {
    conn [route].sock = connTryConnect (connports [route], &connerr, conn [route].sock);
    if (connerr == SOCK_CONN_IN_PROGRESS) {
      mssleep (10);
      conn [route].sock = connTryConnect (connports [route], &connerr, conn [route].sock);
    }
  }

  if (connerr == SOCK_CONN_OK &&
      ! socketInvalid (conn [route].sock)) {
    logMsg (LOG_DBG, LOG_SOCKET, "connect %d/%s to:%d/%s",
        conn [route].routefrom, msgRouteDebugText (conn [route].routefrom),
        route, msgRouteDebugText (route));
    logMsg (LOG_DBG, LOG_SOCKET, "conn sock %" PRId64, (int64_t) conn [route].sock);

    if (sockhSendMessage (conn [route].sock, conn [route].routefrom, route,
        MSG_HANDSHAKE, NULL) < 0) {
      logMsg (LOG_DBG, LOG_SOCKET, "connect-send-handshake-fail %d/%s to:%d/%s",
          conn [route].routefrom, msgRouteDebugText (conn [route].routefrom),
          route, msgRouteDebugText (route));
      sockClose (conn [route].sock);
      conn [route].sock = INVALID_SOCKET;
      conn [route].connected = false;
    } else {
      logMsg (LOG_DBG, LOG_SOCKET, "connect ok %d/%s to:%d/%s",
          conn [route].routefrom, msgRouteDebugText (conn [route].routefrom),
          route, msgRouteDebugText (route));
      conn [route].handshakesent = true;
      if (conn [route].handshakerecv) {
        logMsg (LOG_DBG, LOG_SOCKET, "conn/handshake ok: from:%d/%s route:%d/%s",
            conn [route].routefrom, msgRouteDebugText (conn [route].routefrom),
            route, msgRouteDebugText (route));
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
    logMsg (LOG_DBG, LOG_SOCKET, "close sock %" PRId64, (int64_t) conn [route].sock);
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

void
connProcessHandshake (conn_t *conn, bdjmsgroute_t route)
{
  if (route >= ROUTE_MAX) {
    return;
  }

  conn [route].handshakerecv = true;

  if (conn [route].handshakesent) {
    /* send a null message and see if the connection is valid */
    if (sockhSendMessage (conn [route].sock, conn [route].routefrom, route,
        MSG_NULL, NULL) < 0) {
      logMsg (LOG_DBG, LOG_SOCKET, "handshake fail: from:%d/%s route:%d/%s",
          route, msgRouteDebugText (route),
          conn [route].routefrom, msgRouteDebugText (conn [route].routefrom));
      sockClose (conn [route].sock);
      conn [route].sock = INVALID_SOCKET;
      conn [route].handshakesent = false;
      conn [route].connected = false;
      connConnect (conn, route);
    }
  }

  if (conn [route].handshakesent) {
    logMsg (LOG_DBG, LOG_SOCKET, "handshake ok: from:%d/%s route:%d/%s",
        route, msgRouteDebugText (route),
        conn [route].routefrom, msgRouteDebugText (conn [route].routefrom));
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
    bdjmsgmsg_t msg, const char *args)
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
    logMsg (LOG_DBG, LOG_SOCKET, "close sock %" PRId64, (int64_t) conn [route].sock);
    sockClose (conn [route].sock);
    conn [route].sock = INVALID_SOCKET;
    conn [route].connected = false;
    conn [route].handshakesent = false;
    conn [route].handshakerecv = false;
    conn [route].handshake = false;
  }
}

bool
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

bool
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

/* internal routines */

static bool
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

static Sock_t
connTryConnect (uint16_t port, int *connerr, Sock_t sock)
{
  sock = sockConnect (port, connerr, sock);
  if (*connerr != SOCK_CONN_OK && *connerr != SOCK_CONN_IN_PROGRESS) {
    sockClose (sock);
    sock = INVALID_SOCKET;
  }
  return sock;
}

static void
connBaseInit (void)
{
  if (atomic_flag_test_and_set (&initialized)) {
    return;
  }

  /* if the port is not used, set to 0 */
  connports [ROUTE_NONE] = 0;
  connports [ROUTE_ALTINST] = 0;
  connports [ROUTE_BPM_COUNTER] = bdjvarsGetNum (BDJVL_PORT_BPM_COUNTER);
  connports [ROUTE_CONFIGUI] = bdjvarsGetNum (BDJVL_PORT_CONFIGUI);
  connports [ROUTE_DBUPDATE] = bdjvarsGetNum (BDJVL_PORT_DBUPDATE);
  connports [ROUTE_HELPERUI] = bdjvarsGetNum (BDJVL_PORT_HELPERUI);
  connports [ROUTE_MAIN] = bdjvarsGetNum (BDJVL_PORT_MAIN);
  connports [ROUTE_MANAGEUI] = bdjvarsGetNum (BDJVL_PORT_MANAGEUI);
  connports [ROUTE_MARQUEE] = bdjvarsGetNum (BDJVL_PORT_MARQUEE);
  connports [ROUTE_MOBILEMQ] = bdjvarsGetNum (BDJVL_PORT_MOBILEMQ);
  connports [ROUTE_PLAYER] = bdjvarsGetNum (BDJVL_PORT_PLAYER);
  connports [ROUTE_PLAYERUI] = bdjvarsGetNum (BDJVL_PORT_PLAYERUI);
  connports [ROUTE_PODCASTUPD] = 0;
  connports [ROUTE_REMCTRL] = bdjvarsGetNum (BDJVL_PORT_REMCTRL);
  connports [ROUTE_SERVER] = bdjvarsGetNum (BDJVL_PORT_SERVER);
  connports [ROUTE_STARTERUI] = bdjvarsGetNum (BDJVL_PORT_STARTERUI);
  connports [ROUTE_TEST_SUITE] = bdjvarsGetNum (BDJVL_PORT_TEST_SUITE);
}
