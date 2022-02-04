#include "config.h"


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "sockh.h"
#include "sock.h"
#include "tmutil.h"
#include "log.h"
#include "bdjmsg.h"

void
sockhMainLoop (uint16_t listenPort, sockProcessMsg_t msgProc,
            sockOtherProcessing_t otherProc, void *userData)
{
  int           done = 0;
  int           tdone = 0;
  sockserver_t  *sockserver;
  char          args [BDJMSG_MAX];

  sockserver = sockhStartServer (listenPort);

  while (done < SOCKH_EXIT_WAIT_COUNT) {
    tdone = sockhProcessMain (sockserver, msgProc, userData);
    if (tdone || done) {
      /* wait for close messages to come in */
      ++done;
    }

    tdone = otherProc (userData);
    if (tdone) {
      args [0] = '\0';
      tdone = msgProc (ROUTE_NONE, ROUTE_NONE, MSG_EXIT_REQUEST, args, userData);
      ++done;
    }
    mssleep (SOCKH_MAINLOOP_TIMEOUT);
  } /* wait for a message */

  sockhCloseServer (sockserver);
  logProcEnd (LOG_PROC, "sockhMainLoop", "");
}

sockserver_t *
sockhStartServer (uint16_t listenPort)
{
  int           err = 0;
  sockserver_t  *sockserver;

  sockserver = malloc (sizeof (sockserver_t));
  assert (sockserver != NULL);
  sockserver->listenSock = INVALID_SOCKET;
  sockserver->si = NULL;

  logProcBegin (LOG_PROC, "sockhMainLoop");
  sockserver->listenSock = sockServer (listenPort, &err);
  sockserver->si = sockAddCheck (sockserver->si, sockserver->listenSock);
  logMsg (LOG_DBG, LOG_SOCKET, "add listen sock %zd",
      (size_t) sockserver->listenSock);
  return sockserver;
}

void
sockhCloseServer (sockserver_t *sockserver)
{
  if (sockserver != NULL) {
    sockhCloseClients (sockserver->si);
    sockFreeCheck (sockserver->si);
    sockClose (sockserver->listenSock);
    free (sockserver);
  }
}

int
sockhProcessMain (sockserver_t *sockserver, sockProcessMsg_t msgProc,
    void *userData)
{
  Sock_t      msgsock = INVALID_SOCKET;
  char        msgbuff [1024];
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
        logMsg (LOG_DBG, LOG_SOCKET, "add client sock %zd", (size_t) clsock);
      }
    } else {
      char *rval = sockReadBuff (msgsock, &len, msgbuff, sizeof (msgbuff));

      if (rval == NULL) {
        /* either an indicator that the code is mucked up,
         * or that the socket has been closed.
         */
        logMsg (LOG_DBG, LOG_SOCKET, "remove sock %zd", (size_t) msgsock);
        sockRemoveCheck (sockserver->si, msgsock);
        sockClose (msgsock);
        return done;
      }
      logMsg (LOG_DBG, LOG_SOCKET, "got message: %s", rval);

      msgDecode (msgbuff, &routefrom, &route, &msg, args, sizeof (args));
      logMsg (LOG_DBG, LOG_SOCKET, "got: route: %ld msg:%ld args:%s", route, msg, args);
      switch (msg) {
        case MSG_NULL: {
          break;
        }
        case MSG_SOCKET_CLOSE: {
          logMsg (LOG_DBG, LOG_SOCKET, "got: close socket");
          sockRemoveCheck (sockserver->si, msgsock);
          /* the caller will close the socket */
          done = msgProc (routefrom, route, msg, args, userData);
          break;
        }
        default: {
          done = msgProc (routefrom, route, msg, args, userData);
        }
      } /* switch */
    } /* msg from client */
  } /* have message */

  return done;
}


int
sockhSendMessage (Sock_t sock, bdjmsgroute_t routefrom,
    bdjmsgroute_t route, bdjmsgmsg_t msg, char *args)
{
  char        msgbuff [BDJMSG_MAX];
  size_t      len;
  int         rc;

  if (sock == INVALID_SOCKET) {
    return -1;
  }

  /* this is only to keep the log clean */
  if (msg != MSG_MUSICQ_STATUS_DATA && msg != MSG_PLAYER_STATUS_DATA) {
    logMsg (LOG_DBG, LOG_SOCKET, "route:%d msg:%d args:%s", route, msg, args);
  }
  len = msgEncode (routefrom, route, msg, args, msgbuff, sizeof (msgbuff));
  rc = sockWriteBinary (sock, msgbuff, len);
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

