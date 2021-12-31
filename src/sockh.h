#ifndef INC_SOCKH_H
#define INC_SOCKH_H

#include <stdint.h>

#include "sock.h"

typedef void (*sockProcessMsg_t)(long, long, char *);
typedef void (*sockOtherProcessing_t)(void);

#define SOCK_MAINLOOP_TIMEOUT   5

void    sockhMainLoop (uint16_t listenPort, sockProcessMsg_t msgProc,
            sockOtherProcessing_t);
void    sockhRequestClose (sockinfo_t *sockinfo);
void    sockhCloseClients (sockinfo_t *sockinfo);

#endif /* INC_SOCKH_H */
