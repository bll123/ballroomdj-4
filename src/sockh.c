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
  int         otherflag = 0;
  long        routefrom = ROUTE_NONE;
  long        route = ROUTE_NONE;
  long        msg = MSG_NONE;
  char        args [BDJMSG_MAX];


  logProcBegin (LOG_SOCKET, "sockhMainLoop");
  listenSock = sockServer (listenPort, &err);
  si = sockAddCheck (si, listenSock);
  logMsg (LOG_DBG, LOG_SOCKET, "add listen sock %zd", (size_t) listenSock);

  while (done < 100) {
    msgsock = sockCheck (si);
    if (socketInvalid (msgsock)) {
      continue;
    }

    if (msgsock != 0) {
      if (msgsock == listenSock) {
        logMsg (LOG_DBG, LOG_SOCKET, "got connection request");
        Sock_t clsock = sockAccept (listenSock, &err);
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
          case MSG_EXIT_FORCE: {
            logMsg (LOG_DBG, LOG_SOCKET, "force exit");
            exit (1);
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

    if (otherflag == 0) {
      tdone = otherProc (userData);
    }
    otherflag = 1 - otherflag;
    if (tdone) {
      args [0] = '\0';
      tdone = msgProc (ROUTE_NONE, ROUTE_NONE, MSG_EXIT_REQUEST, args, userData);
      ++done;
    }
    mssleep (SOCK_MAINLOOP_TIMEOUT);
  } /* wait for a message */

  sockhCloseClients (si);
  sockFreeCheck (si);
  sockClose (listenSock);
  logProcEnd (LOG_SOCKET, "sockhMainLoop", "");
}

int
sockhSendMessage (Sock_t sock, long routefrom, long route,
    long msg, char *args)
{
  char        msgbuff [BDJMSG_MAX];

  logProcBegin (LOG_SOCKET, "sockhSendMessage");
  logMsg (LOG_DBG, LOG_SOCKET, "route:%ld msg:%ld args:%s", route, msg, args);
  size_t len = msgEncode (routefrom, route, msg, args, msgbuff, sizeof (msgbuff));
  int rc = sockWriteBinary (sock, msgbuff, len);
  logProcEnd (LOG_SOCKET, "sockhSendMessage", "");
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

