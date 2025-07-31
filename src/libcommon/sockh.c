/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>

#if __has_include (<winsock2.h>)
# include <winsock2.h>
#endif

#include "bdjmsg.h"
#include "log.h"
#include "mdebug.h"
#include "sock.h"
#include "sockh.h"
#include "tmutil.h"

enum {
  MAIN_NO_DATA,
  MAIN_HAD_DATA,
  MAIN_FINISH,
};

static sockserver_t * sockhStartServer (uint16_t listenPort);
static void  sockhCloseServer (sockserver_t *sockserver);
static int   sockhProcessMain (sockserver_t *sockserver, sockhProcessMsg_t msgProc, void *userData);

void
sockhMainLoop (uint16_t listenPort, sockhProcessMsg_t msgFunc,
    sockhProcessFunc_t processFunc, void *userData)
{
  int           done = 0;
  sockserver_t  *sockserver;

  sockserver = sockhStartServer (listenPort);

  while (done != MAIN_FINISH) {
    int     rc;
    int     tdone = 0;

    tdone = sockhProcessMain (sockserver, msgFunc, userData);
    if (tdone == MAIN_FINISH) {
      rc = sockWaitClosed (sockserver->si);
      if (rc) {
        done = MAIN_FINISH;
      }
    }
    tdone = processFunc (userData);
    if (tdone == SOCKH_STOP) {
      rc = sockWaitClosed (sockserver->si);
      if (rc) {
        done = MAIN_FINISH;
      }
    }

    /* if there was data, don't sleep */
    if (done == MAIN_NO_DATA) {
      mssleep (SOCKH_MAINLOOP_TIMEOUT);
    }
  } /* wait for a message */

  sockhCloseServer (sockserver);
  logProcEnd ("");
}

int
sockhSendMessage (Sock_t sock, bdjmsgroute_t routefrom,
    bdjmsgroute_t route, bdjmsgmsg_t msg, const char *args)
{
  char        msgbuff [BDJMSG_MAX_PFX];
  size_t      pfxlen;
  int         rc;
  size_t      alen;

  if (sock == INVALID_SOCKET) {
    return -1;
  }

  /* this is only to keep the log clean */
  pfxlen = msgEncode (routefrom, route, msg, msgbuff, sizeof (msgbuff));
  if (args != NULL) {
    /* if args is specified, do not send the null byte */
    pfxlen -= 1;
  }
  alen = 0;
  if (args != NULL) {
    /* write out the null byte also.  the args string must be terminated */
    alen = strlen (args) + 1;
  }
  rc = sockWriteBinary (sock, msgbuff, pfxlen, args, alen);
  if (rc == 0 &&
      msg != MSG_MUSICQ_STATUS_DATA && msg != MSG_PLAYER_STATUS_DATA) {
    logMsg (LOG_DBG, LOG_SOCKET, "sent: msg:%d/%s to %d/%s rc:%d pfx:%s args:%s",
        msg, msgDebugText (msg), route, msgRouteDebugText (route), rc, msgbuff, args);
  }
  return rc;
}

/* internal routines */

static sockserver_t *
sockhStartServer (uint16_t listenPort)
{
  int           err = 0;
  sockserver_t  *sockserver;

  sockserver = mdmalloc (sizeof (sockserver_t));
  sockserver->listenSock = INVALID_SOCKET;
  sockserver->si = NULL;

  logProcBegin ();
  sockserver->listenSock = sockServer (listenPort, &err);
  sockserver->si = sockAddCheck (sockserver->si, sockserver->listenSock);
  logMsg (LOG_DBG, LOG_SOCKET, "add listen sock %" PRId64,
      (int64_t) sockserver->listenSock);
  return sockserver;
}

static void
sockhCloseServer (sockserver_t *sockserver)
{
  if (sockserver != NULL) {
    logMsg (LOG_DBG, LOG_SOCKET, "close listen sock %" PRId64, (int64_t) sockserver->listenSock);
    sockClose (sockserver->listenSock);
    sockFreeCheck (sockserver->si);
    mdfree (sockserver);
  }
}

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
  char        *args;
  Sock_t      clsock;
  int         rc = MAIN_NO_DATA;
  int         trc;
  int         err = 0;


  msgsock = sockCheck (sockserver->si);
  if (socketInvalid (msgsock) || msgsock == 0) {
    return rc;
  }
  rc = MAIN_HAD_DATA;

  if (msgsock == sockserver->listenSock) {
    logMsg (LOG_DBG, LOG_SOCKET, "got connection request");
    clsock = sockAccept (sockserver->listenSock, &err);
    if (! socketInvalid (clsock)) {
      logMsg (LOG_DBG, LOG_SOCKET, "connected");
      sockserver->si = sockAddCheck (sockserver->si, clsock);
      logMsg (LOG_DBG, LOG_SOCKET, "add accept sock %" PRId64, (int64_t) clsock);
    }
  } else {
    char *rval = sockReadBuff (msgsock, &len, msgbuff, sizeof (msgbuff));

    if (rval == NULL) {
      /* either an indicator that the code is mucked up,
       * the message buffer is too short,
       * or that the socket has been closed.
       */
      logMsg (LOG_DBG, LOG_SOCKET, "remove sock %" PRId64, (int64_t) msgsock);
      sockRemoveCheck (sockserver->si, msgsock);
      sockDecrActive (sockserver->si);
      logMsg (LOG_DBG, LOG_SOCKET, "close sock %" PRId64, (int64_t) msgsock);
      sockClose (msgsock);
      return rc;
    }

    logMsg (LOG_DBG, LOG_SOCKET, "rcvd message: %s", rval);

    msgDecode (msgbuff, &routefrom, &route, &msg, &args);
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
        sockDecrActive (sockserver->si);
        /* the caller will close the socket */
        trc = msgFunc (routefrom, route, msg, args, userData);
        if (trc) {
          rc = MAIN_FINISH;
        }
        logMsg (LOG_DBG, LOG_SOCKET, "close sock %" PRId64, (int64_t) msgsock);
        sockClose (msgsock);
        break;
      }
      /* extra handshake handling so that the active */
      /* counter can be incremented */
      case MSG_HANDSHAKE: {
        sockIncrActive (sockserver->si);
        trc = msgFunc (routefrom, route, msg, args, userData);
        if (trc) {
          rc = MAIN_FINISH;
        }
        break;
      }
      default: {
        trc = msgFunc (routefrom, route, msg, args, userData);
        if (trc) {
          rc = MAIN_FINISH;
        }
      }
    } /* switch */
  } /* msg from client */

  return rc;
}

