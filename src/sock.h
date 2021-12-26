#ifndef INC_SOCK_H
#define INC_SOCK_H

#include "config.h"

#include <sys/types.h>

typedef struct {
  int             count;
  Sock_t          max;
  fd_set          readfds;
  Sock_t          *socklist;
} sockinfo_t;

#define SOCK_READ_TIMEOUT   5
#define SOCK_WRITE_TIMEOUT  2

Sock_t        sockServer (short, int *);
void          sockClose (Sock_t);
sockinfo_t *  sockAddCheck (sockinfo_t *, Sock_t);
void          sockRemoveCheck (sockinfo_t *, Sock_t);
void          sockFreeCheck (sockinfo_t *);
Sock_t        sockCheck (sockinfo_t *);
Sock_t        sockAccept (Sock_t, int *);
Sock_t        sockConnect (short, int *);
Sock_t        sockConnectWait (short, size_t);
char *        sockRead (Sock_t, size_t *);
char *        sockReadBuff (Sock_t, size_t *, char *data, size_t);
int           sockWriteStr (Sock_t, char *, size_t);
int           sockWriteBinary (Sock_t, char *, size_t);

#endif /* INC_SOCK_H */
