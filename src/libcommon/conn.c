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
    for (bdjmsgroute_t i = ROUTE_NONE; i < ROUTE_MAX; ++i) {
      conn [i].sock = INVALID_SOCKET;
      conn [i].port = 0;
      conn [i].connected = false;
      conn [i].handshake = false;
    }
    free (conn);
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
  int         err;

  if (route >= ROUTE_MAX) {
    return;
  }

  if (socketInvalid (conn [route].sock) &&
      connports [route] != 0) {
    conn [route].sock = sockConnect (connports [route], &err, 1000);
  }

  if (! socketInvalid (conn [route].sock)) {
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

  if (! socketInvalid (conn [route].sock)) {
    sockhSendMessage (conn [route].sock, conn [route].routefrom, route,
        MSG_SOCKET_CLOSE, NULL);
    sockClose (conn [route].sock);
  }

  conn [route].sock = INVALID_SOCKET;
  conn [route].connected = false;
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
fprintf (stderr, "%d still connected\n", i);
      rc = false;
      break;
    }
  }

  return rc;
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

  if (conn == NULL) {
    return;
  }
  if (route >= ROUTE_MAX) {
    return;
  }
  if (socketInvalid (conn [route].sock)) {
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
  if (conn == NULL) {
    return;
  }

  if (connHaveHandshake (conn, route) &&
      ! connIsConnected (conn, route)) {
    connConnect (conn, route);
  }
}


void
connClearHandshake (conn_t *conn, bdjmsgroute_t route)
{
  if (conn == NULL) {
    return;
  }
  if (route >= ROUTE_MAX) {
    return;
  }

  conn [route].handshake = false;
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
