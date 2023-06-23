/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_SOCKH_H
#define INC_SOCKH_H

#include <stdint.h>

#include "sock.h"
#include "bdjmsg.h"

typedef int (*sockhProcessMsg_t)(bdjmsgroute_t routefrom, bdjmsgroute_t routeto,
    bdjmsgmsg_t msg, char *args, void *userdata);
typedef int (*sockhProcessFunc_t)(void *);

typedef struct {
  sockinfo_t      *si;
  Sock_t          listenSock;
} sockserver_t;

enum {
  SOCKH_CONTINUE = false,
  SOCKH_STOP = true,
  SOCKH_MAINLOOP_TIMEOUT = 5,
};

void  sockhMainLoop (uint16_t listenPort, sockhProcessMsg_t msgFunc, sockhProcessFunc_t processFunc, void *userData);
sockserver_t  * sockhStartServer (uint16_t listenPort);
void  sockhCloseServer (sockserver_t *sockserver);
void  sockhCheck (uint16_t listenPort, sockhProcessMsg_t msgProc, sockhProcessFunc_t, void *userData);
int   sockhSendMessage (Sock_t sock, bdjmsgroute_t routefrom, bdjmsgroute_t route, bdjmsgmsg_t msg, const char *args);
void  sockhRequestClose (sockinfo_t *sockinfo);
void  sockhCloseClients (sockinfo_t *sockinfo);

#endif /* INC_SOCKH_H */
