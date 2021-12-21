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

int           sockServer (short, int *);
void          sockClose (int);
sockinfo_t *  sockAddCheck (sockinfo_t *, int);
void          sockRemoveCheck (sockinfo_t *, int);
void          sockFreeCheck (sockinfo_t *);
int           sockCheck (sockinfo_t *);
int           sockAccept (int, int *);
int           sockConnect (short, int *);
int           sockConnectWait (short, size_t);
char *        sockRead (int, size_t *);
char *        sockReadBuff (int, size_t *, char *data, size_t);
int           sockWriteStr (int, char *, size_t);
int           sockWriteBinary (int, char *, size_t);

#endif /* INC_SOCK_H */
