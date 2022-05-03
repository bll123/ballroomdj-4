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

#define SOCKH_MAINLOOP_TIMEOUT   5
#define SOCKH_EXIT_WAIT_COUNT    20

void          sockhMainLoop (uint16_t listenPort, sockhProcessMsg_t msgFunc,
                  sockhProcessFunc_t processFunc, void *userData);
sockserver_t  * sockhStartServer (uint16_t listenPort);
void          sockhCloseServer (sockserver_t *sockserver);
int           sockhProcessMain (sockserver_t *sockserver,
                  sockhProcessMsg_t msgProc, void *userData);
void          sockhCheck (uint16_t listenPort, sockhProcessMsg_t msgProc,
                  sockhProcessFunc_t, void *userData);
int           sockhSendMessage (Sock_t sock, bdjmsgroute_t routefrom,
                  bdjmsgroute_t route, bdjmsgmsg_t msg, char *args);
void          sockhRequestClose (sockinfo_t *sockinfo);
void          sockhCloseClients (sockinfo_t *sockinfo);

#endif /* INC_SOCKH_H */
