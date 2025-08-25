/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include <stdint.h>

#include "sock.h"
#include "bdjmsg.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

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
int   sockhSendMessage (Sock_t sock, bdjmsgroute_t routefrom, bdjmsgroute_t route, bdjmsgmsg_t msg, const char *args);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

