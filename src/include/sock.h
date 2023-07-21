/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_SOCK_H
#define INC_SOCK_H

#include "config.h"

#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>

#if _sys_socket
# include <sys/socket.h>
#endif

/* winsock2.h should come before windows.h */
#if _hdr_winsock2
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wmissing-declarations"
# include <winsock2.h>
# pragma clang diagnostic pop
#endif
#if _hdr_ws2tcpip
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wmissing-declarations"
# include <ws2tcpip.h>
# pragma clang diagnostic pop
#endif
#if _hdr_windows
# include <windows.h>
#endif

#if _typ_SOCKET
 typedef SOCKET Sock_t;
#else
 typedef int Sock_t;
#endif

#if _typ_socklen_t
 typedef socklen_t Socklen_t;
#else
 typedef int Socklen_t;
#endif

#if ! _typ_suseconds_t && ! defined (BDJ_TYPEDEF_USECONDS)
 typedef useconds_t suseconds_t;
 #define BDJ_TYPEDEF_USECONDS 1
#endif

typedef struct sockinfo sockinfo_t;

enum {
  SOCK_CONN_OK,
  SOCK_CONN_IN_PROGRESS,
  SOCK_CONN_FAIL,
  SOCK_CONN_ERROR,
};

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
Sock_t        sockConnect (uint16_t port, int *connerr, Sock_t clsock);
char *        sockRead (Sock_t, size_t *);
char *        sockReadBuff (Sock_t, size_t *, char *data, size_t dlen);
int           sockWriteStr (Sock_t, char *s, size_t slen);
int           sockWriteBinary (Sock_t, char *data, size_t dlen);
bool          socketInvalid (Sock_t sock);
bool          sockWritable (Sock_t sock);
bool          sockWaitClosed (sockinfo_t *sockinfo);

#endif /* INC_SOCK_H */
