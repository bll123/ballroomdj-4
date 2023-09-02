/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
/*
 * https://docs.microsoft.com/en-us/windows/win32/winsock/windows-sockets-error-codes-2
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <inttypes.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#if _hdr_arpa_inet
# include <arpa/inet.h>
#endif
#if _hdr_netdb
# include <netdb.h>
#endif
#if _hdr_netinet_in
# include <netinet/in.h>
#endif
#if _sys_select
# include <sys/select.h>
#endif
#if _sys_socket
# include <sys/socket.h>
#endif

#if _hdr_winsock2
# include <winsock2.h>
#endif
#if _hdr_ws2tcpip
# include <ws2tcpip.h>
#endif
#if _hdr_windows
# include <windows.h>
#endif

#include "bdjstring.h"
#include "log.h"
#include "mdebug.h"
#include "sock.h"
#include "tmutil.h"

typedef struct {
  Sock_t          sock;
  bool            havedata;
} socklist_t;

typedef struct sockinfo {
  int             count;
  int             active;
  int             havecount;
  Sock_t          max;
  fd_set          readfdsbase;
  fd_set          readfds;
  socklist_t      *socklist;
} sockinfo_t;

static ssize_t  sockReadData (Sock_t, char *, size_t);
static int      sockWriteData (Sock_t, char *, size_t);
static void     sockFlush (Sock_t);
static int      sockSetNonblocking (Sock_t sock);
 // static int       sockSetBlocking (Sock_t sock);
static void     sockInit (void);
static void     sockCleanup (void);
static Sock_t   sockSetOptions (Sock_t sock, int *err);
static void     sockUpdateReadCheck (sockinfo_t *sockinfo);

static int      sockInitialized = 0;
static int      sockCount = 0;

Sock_t
sockServer (uint16_t listenPort, int *err)
{
  struct sockaddr_in  saddr;
  int                 rc;
  int                 count;
  int                 typ;
  Sock_t              lsock;


  if (! sockInitialized) {
    sockInit ();
  }

  typ = SOCK_STREAM;
#if _define_SOCK_CLOEXEC
  typ |= SOCK_CLOEXEC;
#endif
  lsock = socket (AF_INET, typ, 0);
  if (socketInvalid (lsock)) {
    *err = errno;
    logError ("socket:");
#if _lib_WSAGetLastError
    logMsg (LOG_ERR, LOG_SOCKET, "socket: wsa last-error: %d", WSAGetLastError() );
#endif
    return INVALID_SOCKET;
  }

  mdextsock (lsock);
  lsock = sockSetOptions (lsock, err);
  memset (&saddr, 0, sizeof (struct sockaddr_in));
  saddr.sin_family = AF_INET;
  saddr.sin_addr.s_addr = inet_addr ("127.0.0.1");
  saddr.sin_port = htons (listenPort);

  count = 0;
  rc = bind (lsock, (struct sockaddr *) &saddr, sizeof (struct sockaddr_in));
  while (rc != 0 && count < 1000 && (errno == EADDRINUSE)) {
    mssleep (100);
    ++count;
    rc = bind (lsock, (struct sockaddr *) &saddr, sizeof (struct sockaddr_in));
  }
  if (rc != 0) {
    *err = errno;
    logError ("bind:");
#if _lib_WSAGetLastError
    logMsg (LOG_ERR, LOG_SOCKET, "bind: wsa last-error: %d", WSAGetLastError() );
    logMsg (LOG_DBG, LOG_SOCKET, "bind: wsa last-error: %d", WSAGetLastError() );
#endif
    mdextclose (lsock);
    close (lsock);
    return INVALID_SOCKET;
  }
  rc = listen (lsock, 10);
  if (rc != 0) {
    *err = errno;
    logError ("listen:");
#if _lib_WSAGetLastError
    logMsg (LOG_DBG, LOG_SOCKET, "select: wsa last-error: %d", WSAGetLastError() );
#endif
    mdextclose (lsock);
    close (lsock);
    return INVALID_SOCKET;
  }

  ++sockCount;
  *err = 0;
  return lsock;
}

void
sockClose (Sock_t sock)
{
  if (! socketInvalid (sock) && sock > 2) {
    mdextclose (sock);
    close (sock);
    --sockCount;
  }
  if (sockCount <= 0) {
    sockCleanup ();
    sockCount = 0;
  }
}

sockinfo_t *
sockAddCheck (sockinfo_t *sockinfo, Sock_t sock)
{
  int             idx;

  if (sockinfo == NULL) {
    sockinfo = mdmalloc (sizeof (sockinfo_t));
    sockinfo->count = 0;
    sockinfo->active = 0;
    sockinfo->havecount = 0;
    sockinfo->max = 0;
    sockinfo->socklist = NULL;
  }

  if (socketInvalid (sock) || sockinfo->count >= FD_SETSIZE) {
    logMsg (LOG_DBG, LOG_SOCKET, "sockAddCheck: invalid socket");
    return sockinfo;
  }

  idx = sockinfo->count;
  ++sockinfo->count;
  sockinfo->socklist = mdrealloc (sockinfo->socklist,
      (size_t) sockinfo->count * sizeof (socklist_t));
  sockinfo->socklist [idx].sock = sock;
  sockinfo->socklist [idx].havedata = false;

  sockUpdateReadCheck (sockinfo);

  return sockinfo;
}

void
sockIncrActive (sockinfo_t *sockinfo)
{
  if (sockinfo == NULL) {
    return;
  }

  ++sockinfo->active;
  logMsg (LOG_DBG, LOG_SOCKET, "incr active: %d", sockinfo->active);
}

void
sockRemoveCheck (sockinfo_t *sockinfo, Sock_t sock)
{
  if (sockinfo == NULL) {
    return;
  }

  if (socketInvalid (sock)) {
    logMsg (LOG_DBG, LOG_SOCKET, "invalid socket");
    return;
  }


  for (int i = 0; i < sockinfo->count; ++i) {
    if (sockinfo->socklist [i].sock == sock) {
      sockinfo->socklist [i].sock = INVALID_SOCKET;
      sockinfo->socklist [i].havedata = false;
      break;
    }
  }

  sockUpdateReadCheck (sockinfo);
}

void
sockDecrActive (sockinfo_t *sockinfo)
{
  --sockinfo->active;
  logMsg (LOG_DBG, LOG_SOCKET, "decr active: %d", sockinfo->active);
}

void
sockFreeCheck (sockinfo_t *sockinfo)
{
  if (sockinfo != NULL) {
    sockinfo->count = 0;
    dataFree (sockinfo->socklist);
    mdfree (sockinfo);
  }
}

Sock_t
sockCheck (sockinfo_t *sockinfo)
{
  int               rc;
  struct timeval    tv;
  int               ridx = -1;

  if (sockinfo == NULL) {
    return INVALID_SOCKET;
  }

  /* prevent any particular socket from being starved out */
  /* from processing */
  for (int i = 0; sockinfo->havecount && i < sockinfo->count; ++i) {
    if (sockinfo->socklist [i].havedata) {
      sockinfo->socklist [i].havedata = false;
      return sockinfo->socklist [i].sock;
    }
  }

  memcpy (&(sockinfo->readfds), &sockinfo->readfdsbase, sizeof (fd_set));

  tv.tv_sec = 0;
  tv.tv_usec = (suseconds_t)
      (SOCK_READ_TIMEOUT * sockinfo->count * 1000);

  rc = select (sockinfo->max + 1, &(sockinfo->readfds), NULL, NULL, &tv);
  if (rc < 0) {
    if (errno == EINTR || errno == EAGAIN) {
      return 0;
    }
    logError ("select");
#if _lib_WSAGetLastError
    logMsg (LOG_DBG, LOG_SOCKET, "select: wsa last-error:%d", WSAGetLastError());
#endif

    return INVALID_SOCKET;
  }
  if (rc > 0) {
    sockinfo->havecount = 0;
    for (int i = 0; i < sockinfo->count; ++i) {
      Sock_t  tsock;

      sockinfo->socklist [i].havedata = false;
      tsock = sockinfo->socklist [i].sock;
      if (socketInvalid (tsock)) {
        continue;
      }
      if (FD_ISSET (tsock, &(sockinfo->readfds))) {
        sockinfo->socklist [i].havedata = true;
        ++sockinfo->havecount;
        /* want to process any listener socket items first */
        if (ridx == -1) { ridx = i; }
      }
    }
  }

  if (ridx >= 0) {
    sockinfo->socklist [ridx].havedata = false;
    return sockinfo->socklist [ridx].sock;
  }

  return 0;
}

Sock_t
sockAccept (Sock_t lsock, int *err)
{
  struct sockaddr_in  saddr;
  Socklen_t           alen;
  Sock_t              nsock;

  alen = sizeof (struct sockaddr_in);
  nsock = accept (lsock, (struct sockaddr *) &saddr, &alen);
  if (socketInvalid (nsock)) {
    *err = errno;
    logError ("accept");
#if _lib_WSAGetLastError
    logMsg (LOG_DBG, LOG_SOCKET, "accept: wsa last-error:%d", WSAGetLastError());
#endif
    return INVALID_SOCKET;
  }

  mdextsock (nsock);
  if (sockSetNonblocking (nsock) < 0) {
    mdextclose (nsock);
    close (nsock);
    return INVALID_SOCKET;
  }

  ++sockCount;
  *err = 0;
  return nsock;
}

Sock_t
sockConnect (uint16_t connPort, int *connerr, Sock_t clsock)
{
  struct sockaddr_in  raddr;
  int                 rc;
  int                 typ;
  int                 err = 0;


  if (! sockInitialized) {
    sockInit ();
  }

  if (clsock == INVALID_SOCKET) {
    typ = SOCK_STREAM;
#if _define_SOCK_CLOEXEC
    typ |= SOCK_CLOEXEC;
#endif
    clsock = socket (AF_INET, typ, 0);

    if (socketInvalid (clsock)) {
      *connerr = SOCK_CONN_FAIL;
      logError ("connect");
#if _lib_WSAGetLastError
      logMsg (LOG_DBG, LOG_SOCKET, "socket: wsa last-error:%d", WSAGetLastError());
#endif
      return INVALID_SOCKET;
    }

    mdextsock (clsock);
    if (sockSetNonblocking (clsock) < 0) {
      *connerr = SOCK_CONN_FAIL;
      mdextclose (clsock);
      close (clsock);
      return INVALID_SOCKET;
    }

    clsock = sockSetOptions (clsock, &err);
    if (err != 0) {
      *connerr = SOCK_CONN_FAIL;
      mdextclose (clsock);
      close (clsock);
      return INVALID_SOCKET;
    }
  }

  memset (&raddr, 0, sizeof (struct sockaddr_in));
  raddr.sin_family = AF_INET;
  raddr.sin_addr.s_addr = inet_addr ("127.0.0.1");
  raddr.sin_port = htons (connPort);

  rc = connect (clsock, (struct sockaddr *) &raddr, sizeof (struct sockaddr_in));

  if (rc == 0) {
    *connerr = SOCK_CONN_OK;
  } else {
    err = errno;
  }

  /* the system may finish the connection on its own, in which case   */
  /* the next call to connect returns EISCONN */
  if (rc < 0 && (errno == EISCONN || errno == EALREADY)) {
    err = 0;
    *connerr = SOCK_CONN_OK;
    rc = 0;
  }
#if _lib_WSAGetLastError
  if (WSAGetLastError() == WSAEISCONN) {
    err = 0;
    *connerr = SOCK_CONN_OK;
    rc = 0;
  }
#endif

  if (rc < 0) {
    *connerr = SOCK_CONN_ERROR;

#if _lib_WSAGetLastError
    /* 10037 */
    if (WSAGetLastError() == WSAEALREADY) {
      *connerr = SOCK_CONN_IN_PROGRESS;
      errno = EINPROGRESS;
      err = EINPROGRESS;
    }
    /* 10035 */
    if (WSAGetLastError() == WSAEWOULDBLOCK) {
      *connerr = SOCK_CONN_IN_PROGRESS;
      errno = EWOULDBLOCK;
      err = EWOULDBLOCK;
    }
#endif
    /* EINPROGRESS == 115 */
    /* EAGAIN == 11 */
    /* EWOULDBLOCK == 11 (linux) */
    /* ECONNREFUSED == 111 */
    /* ECONNABORTED == 103 */
    if (err == EINPROGRESS || err == EAGAIN || err == EINTR || err == EWOULDBLOCK) {
      *connerr = SOCK_CONN_IN_PROGRESS;
      /* leave the socket open */
    } else {
      /* conn refused is a normal response */
      if (err != ECONNREFUSED) {
        logError ("connect");
#if _lib_WSAGetLastError
        logMsg (LOG_DBG, LOG_SOCKET, "connect: wsa last-error:%d", WSAGetLastError());
#endif
      }
      mdextclose (clsock);
      close (clsock);
      clsock = INVALID_SOCKET;
    }
    return clsock;
  }

  logMsg (LOG_DBG, LOG_SOCKET, "Connected to port:%d sock:%ld", connPort, (long) clsock);
  ++sockCount;
  return clsock;
}

char *
sockReadBuff (Sock_t sock, size_t *rlen, char *data, size_t maxlen)
{
  uint32_t    len;
  ssize_t     rc;

  len = 0;
  *rlen = 0;
  rc = sockReadData (sock, (char *) &len, sizeof (len));
  if (rc < 0) {
    return NULL;
  }

  len = ntohl (len);
  if (len > maxlen) {
    sockFlush (sock);
    return NULL;
  }
  rc = sockReadData (sock, data, len);
  if (rc < 0) {
    return NULL;
  }

  *rlen = len;
  return data;
}

/* allocates the data buffer.               */
/* the buffer must be freed by the caller.  */
char *
sockRead (Sock_t sock, size_t *rlen)
{
  uint32_t     len;
  ssize_t     rc;
  char        *data;

  *rlen = 0;
  rc = sockReadData (sock, (char *) &len, sizeof (len));
  if (rc < 0) {
    return NULL;
  }
  len = ntohl (len);
  data = mdmalloc (len);
  rc = sockReadData (sock, data, len);
  if (rc < 0) {
    return NULL;
  }
  *rlen = len;
  return data;
}

/* sockWriteStr() writes the null byte also */
int
sockWriteStr (Sock_t sock, char *data, size_t len)
{
  int   rc;

  if (socketInvalid (sock)) {
    return 0;
  }
  rc = sockWriteBinary (sock, data, len + 1);
  return rc;
}

int
sockWriteBinary (Sock_t sock, char *data, size_t dlen)
{
  uint32_t    len;
  ssize_t     rc;

  if (socketInvalid (sock)) {
    return -1;
  }

  len = (uint32_t) dlen;
  len = htonl (len);
  rc = sockWriteData (sock, (char *) &len, sizeof (len));
  if (rc < 0) {
    return -1;
  }
  rc = sockWriteData (sock, data, dlen);
  if (rc < 0) {
    return -1;
  }
  return 0;
}

bool
socketInvalid (Sock_t sock)
{
#if _define_INVALID_SOCKET
  return (sock == INVALID_SOCKET);
#else
  return (sock < 0);
#endif
}

bool
sockWaitClosed (sockinfo_t *sockinfo)
{
  bool    rc;

  rc = false;
  if (sockinfo != NULL) {
    logMsg (LOG_ERR, LOG_SOCKET, "socket: wait-closed: active: %d", sockinfo->active);
  }
  /* do not count the listener socket as open */
  if (sockinfo == NULL || sockinfo->active <= 0) {
    rc = true;
  }
  return rc;
}

/* internal routines */

static ssize_t
sockReadData (Sock_t sock, char *data, size_t len)
{
  size_t        tot = 0;
  ssize_t       rc;
  mstime_t      mi;
  ssize_t       timeout;

  if (socketInvalid (sock)) {
    return -1;
  }

  timeout = (ssize_t) (len * SOCK_READ_TIMEOUT);
  mstimestart (&mi);
  rc = recv (sock, data, len, 0);
  if (rc < 0) {
#if _lib_WSAGetLastError
    if (WSAGetLastError() == WSAEWOULDBLOCK) {
      errno = EWOULDBLOCK;
    }
#endif
    if (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
      logError ("recv");
#if _lib_WSAGetLastError
      logMsg (LOG_DBG, LOG_SOCKET, "recv: wsa last-error:%d", WSAGetLastError());
#endif
      return -1;
    }
  }
  if (rc > 0) {
    tot += (size_t) rc;
  }
  while (tot < len) {
    ssize_t        m;

    rc = recv (sock, data + tot, len - tot, 0);
    if (rc < 0) {
#if _lib_WSAGetLastError
      if (WSAGetLastError() == WSAEWOULDBLOCK) {
        errno = EWOULDBLOCK;
      }
#endif
      if (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
        logError ("recv-b");
#if _lib_WSAGetLastError
        logMsg (LOG_DBG, LOG_SOCKET, "recv: wsa last-error:%d", WSAGetLastError());
#endif
        return -1;
      }
    }
    if (rc > 0) {
      tot += (size_t) rc;
    }
    if (tot == 0) {
      mssleep (5);
    }
    m = mstimeend (&mi);
    if (m > timeout) {
      break;
    }
  }
  if (tot < len) {
    sockFlush (sock);
    return -1;
  }
  return 0;
}

static int
sockWriteData (Sock_t sock, char *data, size_t len)
{
  size_t        tot = 0;
  // Sock_t        sval;
  ssize_t       rc;

  if (socketInvalid (sock)) {
    return -1;
  }

  logProcBegin (LOG_PROC, "sockWriteData");
  logMsg (LOG_DBG, LOG_SOCKET, "want to send: %"PRIu64" bytes", (uint64_t) len);

#if 0
  /* ugh.  the write() call blocks on a non-blocking socket.  sigh. */
  /* call select() and check the condition the socket is in to see  */
  /* if it is writable.                                             */
  sval = sockCanWrite (sock);
  if (sval != sock) {
    logMsg (LOG_DBG, LOG_SOCKET, "socket not writable %" PRId64, (int64_t) sock);
    logProcEnd (LOG_PROC, "sockWriteData", "not-writable");
    return -1;
  }
#endif

  rc = send (sock, data, len, 0);
#if _lib_WSAGetLastError
  {
    int   terr;

    terr = WSAGetLastError();
    logMsg (LOG_DBG, LOG_SOCKET, "send-a: wsa last-error: %d", terr);
    if (terr == WSAEWOULDBLOCK) {
      errno = EWOULDBLOCK;
    }
    if (terr == WSAEINTR) {
      errno = EINTR;
    }
  }
#endif
  if (rc < 0 &&
      errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
    logError ("send-a");
#if _lib_WSAGetLastError
    logMsg (LOG_DBG, LOG_SOCKET, "send-a: wsa last-error: %d", WSAGetLastError());
#endif
    logProcEnd (LOG_PROC, "sockWriteData", "send-a-fail");
    return -1;
  }

  logMsg (LOG_DBG, LOG_SOCKET, "sent: %"PRIu64" bytes", (uint64_t) rc);
  if (rc > 0) {
    tot += (size_t) rc;
    logMsg (LOG_DBG, LOG_SOCKET, "tot: %"PRIu64" bytes", (uint64_t) tot);
  }

  while (tot < len) {
    rc = send (sock, data + tot, len - tot, 0);
    if (rc < 0 &&
        errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
      logError ("send-b");
#if _lib_WSAGetLastError
      logMsg (LOG_DBG, LOG_SOCKET, "send-b: wsa last-error: %d", WSAGetLastError());
#endif
      logProcEnd (LOG_PROC, "sockWriteData", "send-b-fail");
      return -1;
    }
    logMsg (LOG_DBG, LOG_SOCKET, "sent: %"PRIu64" bytes", (uint64_t) rc);
    if (rc > 0) {
      tot += (size_t) rc;
      logMsg (LOG_DBG, LOG_SOCKET, "tot: %"PRIu64" bytes", (uint64_t) tot);
    }
  }

  logProcEnd (LOG_PROC, "sockWriteData", "");
  return 0;
}

static void
sockFlush (Sock_t sock)
{
  char      data [1024];
  size_t    len = 1024;
  int       count;
  ssize_t   rc;

  if (socketInvalid (sock)) {
    return;
  }

  count = 0;
  rc = recv (sock, data, len, 0);
  if (rc < 0) {
#if _lib_WSAGetLastError
    if (WSAGetLastError() == WSAEWOULDBLOCK) {
      errno = EWOULDBLOCK;
    }
#endif
    if (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
      logError ("recv");
#if _lib_WSAGetLastError
      logMsg (LOG_DBG, LOG_SOCKET, "recv: wsa last-error:%d", WSAGetLastError());
#endif
      return;
    }
  }
  while (rc >= 0) {
    rc = recv (sock, data, len, 0);
    if (rc < 0) {
#if _lib_WSAGetLastError
      if (WSAGetLastError() == WSAEWOULDBLOCK) {
        errno = EWOULDBLOCK;
      }
#endif
      if (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
        logError ("recv-b");
#if _lib_WSAGetLastError
        logMsg (LOG_DBG, LOG_SOCKET, "recv: wsa last-error:%d", WSAGetLastError());
#endif
        return;
      }
    }
    if (rc == 0) {
      ++count;
    } else {
      count = 0;
    }
    if (count > 3) {
      return;
    }
  }
}

#if 0
static Sock_t
sockCanWrite (Sock_t sock)
{
  fd_set            readfds;
  fd_set            writefds;
  struct timeval    tv;
  int               rc;

  if (socketInvalid (sock)) {
    return INVALID_SOCKET;
  }

  FD_ZERO (&readfds);
  FD_ZERO (&writefds);
  FD_SET (sock, &readfds);
  FD_SET (sock, &writefds);

  tv.tv_sec = 0;
  tv.tv_usec = (suseconds_t) (SOCK_WRITE_TIMEOUT * 1 * 1000);

  rc = select (sock + 1, &readfds, &writefds, NULL, &tv);
  if (rc < 0) {
    if (errno == EINTR) {
      return sockCanWrite (sock);
    }
    logError ("select");
#if _lib_WSAGetLastError
    logMsg (LOG_DBG, LOG_SOCKET, "select: wsa last-error:%d", WSAGetLastError());
#endif
    return INVALID_SOCKET;
  }
  if (FD_ISSET (sock, &writefds) && ! FD_ISSET(sock, &readfds)) {
    return sock;
  }
  return 0;
}
#endif


static int
sockSetNonblocking (Sock_t sock)
{
#if _lib_fcntl
  int         flags;
  int         rc;

  if (socketInvalid (sock)) {
    return -1;
  }

  flags = fcntl (sock, F_GETFL, 0);
  if (flags < 0) {
    logError ("fcntl-get");
    return -1;
  }
  flags |= O_NONBLOCK | O_CLOEXEC;
  rc = fcntl (sock, F_SETFL, flags);
  if (rc != 0) {
    logError ("fcntl-set");
    return -1;
  }
#endif
#if _lib_ioctlsocket
  unsigned long flag = 1;
  if (ioctlsocket (sock, FIONBIO, &flag) < 0) {
    logError ("ioctlsocket");
#if _lib_WSAGetLastError
    logMsg (LOG_DBG, LOG_SOCKET, "ioctlsocket: wsa last-error:%d", WSAGetLastError());
#endif
  }
#endif
  return 0;
}

static void
sockInit (void)
{
#if _lib_WSAStartup
  WSADATA wsa;
  int rc = WSAStartup (MAKEWORD (2, 2), &wsa);
  if (rc < 0) {
    logError ("WSAStartup:");
#if _lib_WSAGetLastError
    logMsg (LOG_DBG, LOG_SOCKET, "wsastartup: wsa last-error:%d", WSAGetLastError());
#endif
  }
#endif
  sockInitialized = 1;
}

static void
sockCleanup (void)
{
#if _lib_WSACleanup
  WSACleanup ();
#endif
  sockInitialized = 0;
}

static Sock_t
sockSetOptions (Sock_t sock, int *err)
{
  int                 opt = 1;
  int                 rc;

  if (socketInvalid (sock)) {
    return INVALID_SOCKET;
  }

  rc = setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, (const void *) &opt, sizeof (opt));
  if (rc != 0) {
    *err = errno;
    logError ("setsockopt-addr:");
#if _lib_WSAGetLastError
    logMsg (LOG_ERR, LOG_SOCKET, "setsockopt: wsa last-error: %d", WSAGetLastError() );
#endif
    mdextclose (sock);
    close (sock);
    return INVALID_SOCKET;
  }
#if _define_SO_REUSEPORT
  rc = setsockopt (sock, SOL_SOCKET, SO_REUSEPORT, (const char *) &opt, sizeof (opt));
  if (rc != 0) {
    logError ("setsockopt-port:");
    *err = errno;
    mdextclose (sock);
    close (sock);
    return INVALID_SOCKET;
  }
#endif
  return sock;
}

static void
sockUpdateReadCheck (sockinfo_t *sockinfo)
{
  FD_ZERO (&(sockinfo->readfdsbase));
  sockinfo->max = 0;
  for (int i = 0; i < sockinfo->count; ++i) {
    Sock_t tsock = sockinfo->socklist [i].sock;
    if (socketInvalid (tsock)) {
      continue;
    }
    FD_SET (tsock, &(sockinfo->readfdsbase));
    if (tsock > sockinfo->max) {
      sockinfo->max = tsock;
    }
  }
}

