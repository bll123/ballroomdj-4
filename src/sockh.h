#ifndef INC_SOCKH_H
#define INC_SOCKH_H

#include <stdint.h>

#include "sock.h"
#include "bdjmsg.h"

typedef int (*sockProcessMsg_t)(bdjmsgroute_t routefrom, bdjmsgroute_t routeto,
    bdjmsgmsg_t msg, char *args, void *userdata);
typedef int (*sockOtherProcessing_t)(void *);

#define SOCK_MAINLOOP_TIMEOUT   5

void    sockhMainLoop (uint16_t listenPort, sockProcessMsg_t msgProc,
            sockOtherProcessing_t, void *userData);
int     sockhSendMessage (Sock_t sock, bdjmsgroute_t routefrom,
            bdjmsgroute_t route, bdjmsgmsg_t msg, char *args);
void    sockhRequestClose (sockinfo_t *sockinfo);
void    sockhCloseClients (sockinfo_t *sockinfo);

#endif /* INC_SOCKH_H */
