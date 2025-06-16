/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
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

#if _hdr_winsock2
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wmissing-declarations"
# include <winsock2.h>
# pragma clang diagnostic pop
#endif

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
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
  SOCK_LOCAL,
  SOCK_ANY,
};

#if ! _define_INVALID_SOCKET
# define INVALID_SOCKET -1
#endif

Sock_t        sockServer (uint16_t port, int *err, int which);
void          sockClose (Sock_t);
sockinfo_t *  sockAddCheck (sockinfo_t *, Sock_t);
void          sockIncrActive (sockinfo_t *);
void          sockRemoveCheck (sockinfo_t *, Sock_t);
void          sockDecrActive (sockinfo_t *);
void          sockFreeCheck (sockinfo_t *);
Sock_t        sockCheck (sockinfo_t *);
Sock_t        sockAccept (Sock_t, int *);
Sock_t        sockConnect (uint16_t port, int *connerr, Sock_t clsock);
char *        sockReadBuff (Sock_t, size_t *, char *data, size_t dlen);
int           sockWriteBinary (Sock_t, const char *data, size_t dlen, const char *args, size_t alen);
bool          socketInvalid (Sock_t sock);
bool          sockWaitClosed (sockinfo_t *sockinfo);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_SOCK_H */
