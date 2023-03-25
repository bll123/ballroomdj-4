/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>

#include "bdjmsg.h"
#include "log.h"
#include "mdebug.h"
#include "sock.h"
#include "sockh.h"
#include "tmutil.h"

static int   sockhProcessMain (sockserver_t *sockserver, sockhProcessMsg_t msgProc, void *userData);

void
sockhMainLoop (uint16_t listenPort, sockhProcessMsg_t msgFunc,
    sockhProcessFunc_t processFunc, void *userData)
{
  int           done = 0;
  int           tdone = 0;
  sockserver_t  *sockserver;
  Sock_t        msgsock = INVALID_SOCKET;

  sockserver = sockhStartServer (listenPort);

  while (! done) {
    tdone = sockhProcessMain (sockserver, msgFunc, userData);
    if (tdone) {
      ++done;
    }
    tdone = processFunc (userData);
    if (tdone) {
      ++done;
    }

    msgsock = sockCheck (sockserver->si);
    /* if there is more data, don't sleep */
    if (msgsock == 0 || socketInvalid (msgsock)) {
      mssleep (SOCKH_MAINLOOP_TIMEOUT);
    }
  } /* wait for a message */

  sockhCloseServer (sockserver);
  logProcEnd (LOG_PROC, "sockhMainLoop", "");
}

sockserver_t *
sockhStartServer (uint16_t listenPort)
{
  int           err = 0;
  sockserver_t  *sockserver;

  sockserver = mdmalloc (sizeof (sockserver_t));
  sockserver->listenSock = INVALID_SOCKET;
  sockserver->si = NULL;

  logProcBegin (LOG_PROC, "sockhMainLoop");
  sockserver->listenSock = sockServer (listenPort, &err);
  sockserver->si = sockAddCheck (sockserver->si, sockserver->listenSock);
  logMsg (LOG_DBG, LOG_SOCKET, "add listen sock %"PRId64,
      (int64_t) sockserver->listenSock);
  return sockserver;
}

void
sockhCloseServer (sockserver_t *sockserver)
{
  if (sockserver != NULL) {
    sockhCloseClients (sockserver->si);
    sockClose (sockserver->listenSock);
    sockFreeCheck (sockserver->si);
    mdfree (sockserver);
  }
}

int
sockhSendMessage (Sock_t sock, bdjmsgroute_t routefrom,
    bdjmsgroute_t route, bdjmsgmsg_t msg, char *args)
{
  char        msgbuff [BDJMSG_MAX];   // keep static to reduce latency
  size_t      len;
  int         rc;

  if (sock == INVALID_SOCKET) {
    return -1;
  }

  /* this is only to keep the log clean */
  if (msg != MSG_MUSICQ_STATUS_DATA && msg != MSG_PLAYER_STATUS_DATA) {
    logMsg (LOG_DBG, LOG_SOCKET, "route:%d/%s msg:%d/%s args:%s",
        route, msgRouteDebugText (route), msg, msgDebugText (msg), args);
  }
  len = msgEncode (routefrom, route, msg, args, msgbuff, sizeof (msgbuff));
  rc = sockWriteBinary (sock, msgbuff, len);
  logMsg (LOG_DBG, LOG_SOCKET, "sent: msg:%d/%s to %d/%s len:%"PRIu64" rc:%d",
      msg, msgDebugText (msg), route, msgRouteDebugText (route), (uint64_t) len, rc);
  return rc;
}

void
sockhCloseClients (sockinfo_t *sockinfo)
{
  if (sockinfo == NULL) {
    return;
  }

  for (size_t i = 0; i < (size_t) sockinfo->count; ++i) {
    Sock_t tsock = sockinfo->socklist[i];
    if (socketInvalid (tsock)) {
      continue;
    }

    sockRemoveCheck (sockinfo, tsock);
    sockClose (tsock);
  }

  return;
}

/* internal routines */

static int
sockhProcessMain (sockserver_t *sockserver, sockhProcessMsg_t msgFunc,
    void *userData)
{
  Sock_t      msgsock = INVALID_SOCKET;
  char        msgbuff [BDJMSG_MAX];
  size_t      len = 0;
  bdjmsgroute_t routefrom = ROUTE_NONE;
  bdjmsgroute_t route = ROUTE_NONE;
  bdjmsgmsg_t msg = MSG_NULL;
  char        args [BDJMSG_MAX];
  Sock_t      clsock;
  int         done = 0;
  int         err = 0;


  msgsock = sockCheck (sockserver->si);
  if (socketInvalid (msgsock)) {
    return done;
  }

  if (msgsock != 0) {
    if (msgsock == sockserver->listenSock) {
      logMsg (LOG_DBG, LOG_SOCKET, "got connection request");
      clsock = sockAccept (sockserver->listenSock, &err);
      if (! socketInvalid (clsock)) {
        logMsg (LOG_DBG, LOG_SOCKET, "connected");
        sockserver->si = sockAddCheck (sockserver->si, clsock);
        logMsg (LOG_DBG, LOG_SOCKET, "add client sock %"PRId64, (int64_t) clsock);
      }
    } else {
      char *rval = sockReadBuff (msgsock, &len, msgbuff, sizeof (msgbuff));

      if (rval == NULL) {
        /* either an indicator that the code is mucked up,
         * the message buffer is too short,
         * or that the socket has been closed.
         */
        logMsg (LOG_DBG, LOG_SOCKET, "remove sock %"PRId64, (int64_t) msgsock);
        sockRemoveCheck (sockserver->si, msgsock);
        sockClose (msgsock);
        return done;
      }
      logMsg (LOG_DBG, LOG_SOCKET, "rcvd message: %s", rval);

      msgDecode (msgbuff, &routefrom, &route, &msg, args, sizeof (args));
      logMsg (LOG_DBG, LOG_SOCKET,
          "sockh: from: %d/%s route:%d/%s msg:%d/%s args:%s",
          routefrom, msgRouteDebugText (routefrom),
          route, msgRouteDebugText (route), msg, msgDebugText (msg), args);
      switch (msg) {
        case MSG_NULL: {
          break;
        }
        case MSG_SOCKET_CLOSE: {
          logMsg (LOG_DBG, LOG_SOCKET, "rcvd close socket");
          sockRemoveCheck (sockserver->si, msgsock);
          /* the caller will close the socket */
          done = msgFunc (routefrom, route, msg, args, userData);
          break;
        }
        default: {
          done = msgFunc (routefrom, route, msg, args, userData);
        }
      } /* switch */
    } /* msg from client */
  } /* have message */

  return done;
}

