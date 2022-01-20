#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include "sockh.h"
#include "sock.h"
#include "tmutil.h"
#include "log.h"
#include "bdjmsg.h"

void
sockhMainLoop (uint16_t listenPort, sockProcessMsg_t msgProc,
            sockOtherProcessing_t otherProc, void *userData)
{
  Sock_t      listenSock = INVALID_SOCKET;
  Sock_t      msgsock = INVALID_SOCKET;
  int         err = 0;
  sockinfo_t  *si = NULL;
  char        msgbuff [1024];
  size_t      len = 0;
  int         done = 0;
  int         tdone = 0;
  bdjmsgroute_t routefrom = ROUTE_NONE;
  bdjmsgroute_t route = ROUTE_NONE;
  bdjmsgmsg_t msg = MSG_NULL;
  char        args [BDJMSG_MAX];
  Sock_t      clsock;


  logProcBegin (LOG_PROC, "sockhMainLoop");
  listenSock = sockServer (listenPort, &err);
  si = sockAddCheck (si, listenSock);
  logMsg (LOG_DBG, LOG_SOCKET, "add listen sock %zd", (size_t) listenSock);

  while (done < SOCKH_EXIT_WAIT_COUNT) {
    msgsock = sockCheck (si);
    if (socketInvalid (msgsock)) {
      continue;
    }

    if (msgsock != 0) {
      if (msgsock == listenSock) {
        logMsg (LOG_DBG, LOG_SOCKET, "got connection request");
        clsock = sockAccept (listenSock, &err);
        if (! socketInvalid (clsock)) {
          logMsg (LOG_DBG, LOG_SOCKET, "connected");
          si = sockAddCheck (si, clsock);
          logMsg (LOG_DBG, LOG_SOCKET, "add client sock %zd", (size_t) clsock);
        }
      } else {
        char *rval = sockReadBuff (msgsock, &len, msgbuff, sizeof (msgbuff));

        if (rval == NULL) {
          /* either an indicator that the code is mucked up,
           * or that the socket has been closed.
           */
          logMsg (LOG_DBG, LOG_SOCKET, "remove sock %zd", (size_t) msgsock);
          sockRemoveCheck (si, msgsock);
          sockClose (msgsock);
          continue;
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
            sockRemoveCheck (si, msgsock);
            sockClose (msgsock);
            break;
          }
          default: {
            tdone = msgProc (routefrom, route, msg, args, userData);
            if (tdone) {
              ++done;
            }
          }
        } /* switch */
      } /* msg from client */
    } /* have message */

    if (done) {
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

  sockhCloseClients (si);
  sockFreeCheck (si);
  sockClose (listenSock);
  logProcEnd (LOG_PROC, "sockhMainLoop", "");
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

  logProcBegin (LOG_PROC, "sockhSendMessage");
  logMsg (LOG_DBG, LOG_SOCKET, "route:%d msg:%d args:%s", route, msg, args);
  len = msgEncode (routefrom, route, msg, args, msgbuff, sizeof (msgbuff));
  rc = sockWriteBinary (sock, msgbuff, len);
  logProcEnd (LOG_PROC, "sockhSendMessage", "");
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

