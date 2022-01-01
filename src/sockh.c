#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include "sockh.h"
#include "sock.h"
#include "tmutil.h"
#include "log.h"
#include "bdjmsg.h"

void
sockhMainLoop (uint16_t listenPort, sockProcessMsg_t msgProc,
            sockOtherProcessing_t otherProc)
{
  Sock_t      listenSock = INVALID_SOCKET;
  Sock_t      msgsock = INVALID_SOCKET;
  int         err = 0;
  sockinfo_t  *si = NULL;
  char        msgbuff [1024];
  size_t      len = 0;
  int         done = 0;
  long        route = ROUTE_NONE;
  long        msg = MSG_NONE;


  logProcBegin (LOG_LVL_5, "sockhMainLoop");
  listenSock = sockServer (listenPort, &err);
  si = sockAddCheck (si, listenSock);
  logMsg (LOG_DBG, LOG_LVL_5, "add listen sock %zd", (size_t) listenSock);

  while (done < 100) {
    msgsock = sockCheck (si);
    if (socketInvalid (msgsock)) {
      continue;
    }

    if (msgsock != 0) {
      if (msgsock == listenSock) {
        logMsg (LOG_DBG, LOG_LVL_5, "got connection request");
        Sock_t clsock = sockAccept (listenSock, &err);
        if (! socketInvalid (clsock)) {
          logMsg (LOG_DBG, LOG_LVL_5, "connected");
          si = sockAddCheck (si, clsock);
          logMsg (LOG_DBG, LOG_LVL_5, "add client sock %zd", (size_t) clsock);
        }
      } else {
        char *rval = sockReadBuff (msgsock, &len, msgbuff, sizeof (msgbuff));

        if (rval == NULL) {
          /* either an indicator that the code is mucked up,
           * or that the socket has been closed.
           */
          logMsg (LOG_DBG, LOG_LVL_5, "remove sock %zd", (size_t) msgsock);
          sockRemoveCheck (si, msgsock);
          sockClose (msgsock);
          continue;
        }
        logMsg (LOG_DBG, LOG_LVL_5, "got message: %s", rval);

        // ### FIX handle args
        msgDecode (msgbuff, &route, &msg, NULL);
        logMsg (LOG_DBG, LOG_LVL_5, "got: route: %ld msg:%ld", route, msg);
        switch (msg) {
          case MSG_FORCE_EXIT: {
            logMsg (LOG_DBG, LOG_LVL_5, "force exit");
            exit (1);
            break;
          }
          case MSG_CLOSE_SOCKET: {
            logMsg (LOG_DBG, LOG_LVL_5, "got: close socket");
            sockRemoveCheck (si, msgsock);
            sockClose (msgsock);
            break;
          }
          default: {
            done = msgProc (route, msg, NULL);
            if (done) {
              sockhRequestClose (si);
            }
          }
        } /* switch */
      } /* msg from client */
    } /* have message */

    if (done) {
      /* wait for close messages to come in */
      ++done;
    }

    otherProc ();
    msleep (SOCK_MAINLOOP_TIMEOUT);
  } /* wait for a message */

  sockhCloseClients (si);
  sockFreeCheck (si);
  sockClose (listenSock);
  logProcEnd (LOG_LVL_5, "sockhMainLoop", "");
}

int
sockhSendMessage (Sock_t sock, long route, long msg, char *args)
{
  char        msgbuff [BDJMSG_MAX];

  logProcBegin (LOG_LVL_5, "sockhSendMessage");
  logMsg (LOG_DBG, LOG_LVL_5, "route:%ld msg:%ld", route, msg);
  size_t len = msgEncode (route, msg, args, msgbuff, sizeof (msgbuff));
  int rc = sockWriteBinary (sock, msgbuff, len);
  logProcEnd (LOG_LVL_5, "sockhSendMessage", "");
  return rc;
}

void
sockhRequestClose (sockinfo_t *sockinfo)
{
  if (sockinfo == NULL) {
    return;
  }

  for (size_t i = 0; i < (size_t) sockinfo->count; ++i) {
    Sock_t tsock = sockinfo->socklist[i];
    if (socketInvalid (tsock)) {
      continue;
    }

    //### FIX TODO  send a msg, request close
  }

  return;
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

