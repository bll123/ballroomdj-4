#ifndef INC_SOCK_H
#define INC_SOCK_H

#include "config.h"

#include <stdint.h>
#include <sys/types.h>

typedef struct {
  int             count;
  Sock_t          max;
  fd_set          readfds;
  Sock_t          *socklist;
} sockinfo_t;

#define SOCK_READ_TIMEOUT   2
#define SOCK_WRITE_TIMEOUT  2

#if ! _define_INVALID_SOCKET
# define INVALID_SOCKET -1
#endif

Sock_t        sockServer (uint16_t port, int *err);
void          sockClose (Sock_t);
sockinfo_t *  sockAddCheck (sockinfo_t *, Sock_t);
void          sockRemoveCheck (sockinfo_t *, Sock_t);
void          sockFreeCheck (sockinfo_t *);
Sock_t        sockCheck (sockinfo_t *);
Sock_t        sockAccept (Sock_t, int *);
Sock_t        sockConnect (uint16_t port, int *, size_t);
char *        sockRead (Sock_t, size_t *);
char *        sockReadBuff (Sock_t, size_t *, char *data, size_t dlen);
int           sockWriteStr (Sock_t, char *s, size_t slen);
int           sockWriteBinary (Sock_t, char *data, size_t dlen);
int           socketInvalid (Sock_t sock);

#endif /* INC_SOCK_H */
