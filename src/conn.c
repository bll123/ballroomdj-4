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
#include "sock.h"
#include "sockh.h"

static uint16_t connports [ROUTE_MAX];

/* note that connInit() must be called after bdjvarsInit() */
conn_t *
connInit (bdjmsgroute_t routefrom)
{
  conn_t     *conn;

  conn = malloc (sizeof (conn_t) * ROUTE_MAX);
  assert (conn != NULL);

  assert (bdjvarsIsInitialized() == true);

  connports [ROUTE_NONE] = 0;
  connports [ROUTE_MAIN] = bdjvarsl [BDJVL_MAIN_PORT];
  connports [ROUTE_PLAYERUI] = bdjvarsl [BDJVL_PLAYERUI_PORT];
  connports [ROUTE_MANAGEUI] = bdjvarsl [BDJVL_MANAGEUI_PORT];
  connports [ROUTE_CONFIGUI] = bdjvarsl [BDJVL_CONFIGUI_PORT];
  connports [ROUTE_PLAYER] = bdjvarsl [BDJVL_PLAYER_PORT];
  connports [ROUTE_CLICOMM] = 0;
  connports [ROUTE_MOBILEMQ] = bdjvarsl [BDJVL_MOBILEMQ_PORT];
  connports [ROUTE_REMCTRL] = bdjvarsl [BDJVL_REMCTRL_PORT];
  connports [ROUTE_MARQUEE] = bdjvarsl [BDJVL_MARQUEE_PORT];

  for (bdjmsgroute_t i = ROUTE_NONE; i < ROUTE_MAX; ++i) {
    conn [i].sock = INVALID_SOCKET;
    conn [i].port = 0;
    conn [i].routefrom = routefrom;
    conn [i].connected = false;
    conn [i].handshake = false;
  }

  return conn;
}

void
connFree (conn_t *conn)
{
  if (conn != NULL) {
    free (conn);
  }
}

void
connConnect (conn_t *conn, bdjmsgroute_t route)
{
  int         err;

  if (route >= ROUTE_MAX) {
    return;
  }

  if (conn [route].sock == INVALID_SOCKET &&
      connports [route] != 0) {
    conn [route].sock = sockConnect (connports [route], &err, 1000);
  }

  if (conn [route].sock != INVALID_SOCKET) {
    sockhSendMessage (conn [route].sock, conn [route].routefrom, route,
        MSG_HANDSHAKE, NULL);
    conn [route].connected = true;
  }
}

void
connDisconnect (conn_t *conn, bdjmsgroute_t route)
{
  if (route >= ROUTE_MAX) {
    return;
  }

  if (conn [route].sock != INVALID_SOCKET) {
    sockhSendMessage (conn [route].sock, conn [route].routefrom, route,
        MSG_REMOVE_HANDSHAKE, NULL);
    sockhSendMessage (conn [route].sock, conn [route].routefrom, route,
        MSG_SOCKET_CLOSE, NULL);
    sockClose (conn [route].sock);
  }

  conn [route].sock = INVALID_SOCKET;
  conn [route].connected = false;
}

void
connDisconnectAll (conn_t *conn)
{
  for (bdjmsgroute_t i = ROUTE_NONE; i < ROUTE_MAX; ++i) {
    connDisconnect (conn, i);
  }
}

void
connReconnect (conn_t *conn, bdjmsgroute_t route)
{
  if (route >= ROUTE_MAX) {
    return;
  }

/* untested */

  connDisconnect (conn, route);
  connConnect (conn, route);
}

void
connProcessHandshake (conn_t *conn, bdjmsgroute_t routefrom)
{
  if (routefrom >= ROUTE_MAX) {
    return;
  }

  conn [routefrom].handshake = true;
}

void
connSendMessage (conn_t *conn, bdjmsgroute_t route,
    bdjmsgmsg_t msg, char *args)
{
  int         rc;

  if (route >= ROUTE_MAX) {
    return;
  }

  if (conn [route].sock == INVALID_SOCKET) {
    return;
  }

  rc = sockhSendMessage (conn [route].sock, conn [route].routefrom,
      route, msg, args);
  if (rc < 0) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "lost connection to %d", route);
    sockClose (conn [route].sock);
    conn [route].sock = INVALID_SOCKET;
    conn [route].connected = false;
    conn [route].handshake = false;
  }
}

void
connConnectResponse (conn_t *conn, bdjmsgroute_t route)
{
  if (connHaveHandshake (conn, route) &&
      ! connIsConnected (conn, route)) {
    connConnect (conn, route);
  }
}


void
connClearHandshake (conn_t *conn, bdjmsgroute_t route)
{
  if (route >= ROUTE_MAX) {
    return;
  }

  conn [route].handshake = false;
}

inline bool
connIsConnected (conn_t *conn, bdjmsgroute_t route)
{
  if (route >= ROUTE_MAX) {
    return false;
  }

  return conn [route].connected;
}

inline bool
connHaveHandshake (conn_t *conn, bdjmsgroute_t route)
{
  if (route >= ROUTE_MAX) {
    return false;
  }

  return conn [route].handshake;
}

