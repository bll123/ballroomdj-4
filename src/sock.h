#ifndef INC_SOCK_H
#define INC_SOCK_H

#include "bdjconfig.h"

#include <sys/types.h>

typedef struct {
  int             count;
  int             max;
  fd_set          readfds;
  Sock_t          *socklist;
} sockinfo_t;

#define SOCK_READ_TIMEOUT   5
#define SOCK_WRITE_TIMEOUT  2

Sock_t        sockServer (short, int *);
void          sockClose (int);
sockinfo_t *  sockAddCheck (sockinfo_t *, int);
void          sockRemoveCheck (sockinfo_t *, int);
void          sockFreeCheck (sockinfo_t *);
Sock_t        sockCheck (sockinfo_t *);
Sock_t        sockAccept (int, int *);
Sock_t        sockConnect (short, int *);
Sock_t        sockConnectWait (short, size_t);
char *        sockRead (int, size_t *);
char *        sockReadBuff (int, size_t *, char *data, size_t);
int           sockWriteStr (int, char *, size_t);
int           sockWriteBinary (int, char *, size_t);

#endif /* INC_SOCK_H */
