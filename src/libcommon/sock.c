/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 *
 * https://docs.microsoft.com/en-us/windows/win32/winsock/windows-sockets-error-codes-2
 * 10054 - WSAECONNRESET
 *
 * select() is the default and FORCE_SELECT can be set to force it.
 * epoll() implemented 2025-8-1
 * Windows IOCP has a different model, research needs to be done
 * to see if can be used.
 *
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <errno.h>
#include <inttypes.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

/* override to use select() */
#define FORCE_SELECT 0

#if __has_include (<arpa/inet.h>)
# include <arpa/inet.h>
#endif
#if __has_include (<netdb.h>)
# include <netdb.h>
#endif
#if __has_include (<netinet/in.h>)
# include <netinet/in.h>
#endif
#if __has_include (<sys/epoll.h>)
# include <sys/epoll.h>
#endif
#if __has_include (<sys/event.h>)
# include <sys/event.h>               /* kqueue */
#endif
#if __has_include (<sys/select.h>) && \
   (FORCE_SELECT || (! _lib_epoll_create1 && ! _lib_kqueue))
# include <sys/select.h>
#endif
#if __has_include (<sys/socket.h>)
# include <sys/socket.h>
#endif

#if __has_include (<winsock2.h>)
# include <winsock2.h>
#endif
#if __has_include (<ws2tcpip.h>)
# include <ws2tcpip.h>
#endif

#include "bdjstring.h"
#include "log.h"
#include "mdebug.h"
#include "sock.h"
#include "tmutil.h"

enum {
  SOCK_READ_TIMEOUT = 2,
  SOCK_WRITE_TIMEOUT = 2,
  SOCK_MAX_EVENTS = 10,
};

typedef struct {
  Sock_t          sock;
  bool            havedata;
  int             idx;
} socklist_t;

typedef struct sockinfo {
  int                 count;
  int                 active;
  int                 havecount;
  Sock_t              max;
#if (_lib_epoll_create1 || _lib_kqueue) && ! FORCE_SELECT
  Sock_t              eventfd;
#endif
#if _lib_epoll_create1 && ! FORCE_SELECT
  struct epoll_event  events [SOCK_MAX_EVENTS];
#endif
#if _lib_kqueue && ! FORCE_SELECT
  struct kevent       events [SOCK_MAX_EVENTS];
#endif
#if FORCE_SELECT || (! _lib_epoll_create1 && ! _lib_kqueue)
  fd_set              readfdsbase;
  fd_set              readfds;
#endif
  socklist_t          **socklist;
} sockinfo_t;

static volatile atomic_flag sockInitialized = ATOMIC_FLAG_INIT;

static ssize_t  sockReadData (Sock_t, char *, size_t);
static int      sockWriteData (Sock_t, const char *, size_t);
static void     sockFlush (Sock_t);
static int      sockSetNonBlocking (Sock_t sock);
static void     sockInit (void);
static void     sockCleanup (void);
static Sock_t   sockSetOptions (Sock_t sock, int *err);
static void     sockUpdateReadCheck (sockinfo_t *sockinfo);
static int      sockGetAddrInfo (uint16_t port, struct addrinfo **result);
static Sock_t sockOpenClientSocket (struct addrinfo *rp, int *connerr);
static int      sockCheckAlready (int rc, int *connerr);
static int      sockCheckInProgress (int rc, int *connerr);

NODISCARD
Sock_t
sockServer (uint16_t listenPort, int *err)
{
  int                 rc;
  int                 retrycount;
  Sock_t              lsock = INVALID_SOCKET;
  struct addrinfo     *result = NULL;
  struct addrinfo     *rp;

  sockInit ();

  if (sockGetAddrInfo (listenPort, &result) != 0) {
    return INVALID_SOCKET;
  }

  rc = -1;
  *err = SOCK_CONN_ERROR;

  for (rp = result; rp != NULL; rp = rp->ai_next) {
    lsock = socket (rp->ai_family, rp->ai_socktype, rp->ai_protocol);

    if (socketInvalid (lsock)) {
      logMsg (LOG_DBG, LOG_SOCKET, "bind-socket: errno: %d/%s", errno, strerror (errno));
#if _lib_WSAGetLastError
      logMsg (LOG_DBG, LOG_SOCKET, "bind-socket: wsa last-error: %d", WSAGetLastError() );
#endif
      continue;
    }
    mdextsock (lsock);
    lsock = sockSetOptions (lsock, err);
    if (socketInvalid (lsock)) {
      mdextclose (lsock);
      close (lsock);
      continue;
    }

    retrycount = 0;
    rc = bind (lsock, rp->ai_addr, rp->ai_addrlen);
    if (rc < 0 && retrycount < 1000 && (errno == EADDRINUSE)) {
      mssleep (100);
      ++retrycount;
      rc = bind (lsock, rp->ai_addr, rp->ai_addrlen);
    }

    if (rc != 0) {
      *err = errno;
      logError ("bind:");
#if _lib_WSAGetLastError
      logMsg (LOG_ERR, LOG_SOCKET, "bind: wsa last-error: %d", WSAGetLastError() );
#endif
      mdextclose (lsock);
      close (lsock);
      continue;
    }

    rc = listen (lsock, 10);
    if (rc != 0) {
      *err = errno;
      logError ("listen:");
#if _lib_WSAGetLastError
      logMsg (LOG_ERR, LOG_SOCKET, "listen: wsa last-error: %d", WSAGetLastError() );
#endif
      mdextclose (lsock);
      close (lsock);
      continue;
    }

    /* valid socket, use the first found where the bind works */
    break;
  }

  mdextfree (result);
  freeaddrinfo (result);

  *err = SOCK_CONN_OK;
  return lsock;
}

void
sockClose (Sock_t sock)
{
  if (! socketInvalid (sock) && sock > 2) {
    mdextclose (sock);
    close (sock);
  }
}

NODISCARD
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
#if _lib_epoll_create1 && ! FORCE_SELECT
    sockinfo->eventfd = epoll_create1 (0);
    mdextsock (sockinfo->eventfd);
    if (socketInvalid (sockinfo->eventfd)) {
      logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: sockAddCheck: epoll_create1 fail %d", errno);
    }
    logMsg (LOG_DBG, LOG_SOCKET, "using epoll");
#endif
#if _lib_kqueue && ! FORCE_SELECT
    sockinfo->eventfd = kqueue ();
    if (socketInvalid (sockinfo->eventfd)) {
      logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: sockAddCheck: kqueue fail %d", errno);
    }
    logMsg (LOG_DBG, LOG_SOCKET, "using kqueue");
#endif
#if FORCE_SELECT || (! _lib_epoll_create1 && ! _lib_kqueue)
    logMsg (LOG_DBG, LOG_SOCKET, "using select");
#endif
  }

  if (socketInvalid (sock)) {
    logMsg (LOG_DBG, LOG_SOCKET, "sockAddCheck: invalid socket");
    return sockinfo;
  }

  idx = sockinfo->count;
  ++sockinfo->count;
  /* kqueue uses the allocated pointer in a callback, so the */
  /* socklist pointer must stay at the same location */
  sockinfo->socklist = mdrealloc (sockinfo->socklist,
      (size_t) sockinfo->count * sizeof (socklist_t *));
  sockinfo->socklist [idx] = mdmalloc (sizeof (socklist_t));
  sockinfo->socklist [idx]->idx = idx;
  sockinfo->socklist [idx]->sock = sock;
  sockinfo->socklist [idx]->havedata = false;

#if _lib_epoll_create1 && ! FORCE_SELECT
  {
    struct epoll_event  ev;

    ev.events = EPOLLIN;
    ev.data.fd = sock;
    if (epoll_ctl (sockinfo->eventfd, EPOLL_CTL_ADD, sock, &ev) < 0) {
      logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: sockAddCheck: epoll_ctl add fail %d", errno);
      return sockinfo;
    }
  }
#endif

#if _lib_kqueue && ! FORCE_SELECT
  {
    struct kevent   ev;

    EV_SET (&ev, sock, EVFILT_READ, EV_ADD | EV_ENABLE,
        0, 0, sockinfo->socklist [idx]);
    if (kevent (sockinfo->eventfd, &ev, 1, NULL, 0, NULL) < 0) {
      logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: sockAddCheck: kevent add fail %d", errno);
    }
  }
#endif

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
  int     idx;

  if (sockinfo == NULL) {
    return;
  }

  if (socketInvalid (sock)) {
    logMsg (LOG_DBG, LOG_SOCKET, "invalid socket");
    return;
  }

  for (int i = 0; i < sockinfo->count; ++i) {
    if (sockinfo->socklist [i]->sock == sock) {
      sockinfo->socklist [i]->sock = INVALID_SOCKET;
      sockinfo->socklist [i]->havedata = false;
      idx = i;
      break;
    }
  }

#if _lib_epoll_create1 && ! FORCE_SELECT
  {
    struct epoll_event  ev;

    ev.events = EPOLLIN;
    ev.data.fd = sock;
    if (epoll_ctl (sockinfo->eventfd, EPOLL_CTL_DEL, sock, &ev) < 0) {
      if (errno != EBADF) {
        logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: sockAddCheck: epoll_ctl del fail %d", errno);
      }
    }
  }
#endif

#if _lib_kqueue && ! FORCE_SELECT
  {
    struct kevent    ev;

    EV_SET (&ev, sock, EVFILT_READ, EV_DELETE, 0, 0, 0);
    if (kevent (sockinfo->eventfd, &ev, 1, NULL, 0, NULL) < 0) {
      if (errno != 2) {
        logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: sockAddCheck: kevent del fail %d", errno);
      }
    }
  }
#endif

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
#if (_lib_epoll_create1 || _lib_kqueue) && ! FORCE_SELECT
    mdextclose (sockinfo->eventfd);
    close (sockinfo->eventfd);
#endif
    logMsg (LOG_DBG, LOG_INFO, "max socket count: %d", sockinfo->count);
    if (sockinfo->socklist != NULL) {
      for (int i = 0; i < sockinfo->count; ++i) {
        mdfree (sockinfo->socklist [i]);
      }
      mdfree (sockinfo->socklist);
    }
    mdfree (sockinfo);
  }
}

Sock_t
sockCheck (sockinfo_t *sockinfo)
{
  int               rc;

  if (sockinfo == NULL) {
    return INVALID_SOCKET;
  }

  /* process all sockets that had data */
  /* this prevents any particular socket from being starved out */
  /* when earlier sockets are busy */
  for (int i = 0; sockinfo->havecount && i < sockinfo->count; ++i) {
    if (sockinfo->socklist [i]->havedata) {
      sockinfo->socklist [i]->havedata = false;
      return sockinfo->socklist [i]->sock;
    }
  }

#if _lib_epoll_create1 && ! FORCE_SELECT
  rc = epoll_wait (sockinfo->eventfd, sockinfo->events,
      SOCK_MAX_EVENTS, SOCK_READ_TIMEOUT * sockinfo->count);
#endif
#if _lib_kqueue && ! FORCE_SELECT
  {
    struct timespec  ts;

    ts.tv_sec = 0;
    ts.tv_nsec = (long) SOCK_READ_TIMEOUT *
        (long) sockinfo->count * 1000;

    rc = kevent (sockinfo->eventfd, NULL, 0, sockinfo->events,
        SOCK_MAX_EVENTS, &ts);
  }
#endif
#if FORCE_SELECT || (! _lib_epoll_create1 && ! _lib_kqueue)
  {
    struct timeval    tv;

    memcpy (&(sockinfo->readfds), &sockinfo->readfdsbase, sizeof (fd_set));

    tv.tv_sec = 0;
    tv.tv_usec = (suseconds_t) SOCK_READ_TIMEOUT *
        (suseconds_t) sockinfo->count * 1000;

    rc = select (sockinfo->max + 1, &(sockinfo->readfds), NULL, NULL, &tv);
  }
#endif
  if (rc < 0) {
    if (errno == EINTR || errno == EAGAIN) {
      return 0;
    }
    logError ("select/epoll/kevent");
#if _lib_WSAGetLastError
    logMsg (LOG_DBG, LOG_SOCKET, "select: wsa last-error:%d", WSAGetLastError());
#endif

    return INVALID_SOCKET;
  }
  if (rc > 0) {
    sockinfo->havecount = 0;
    for (int i = 0; i < sockinfo->count; ++i) {
      sockinfo->socklist [i]->havedata = false;
    }

#if _lib_kqueue && ! FORCE_SELECT
    for (int j = 0; j < rc; ++j) {
      socklist_t  *tsl;

      tsl = sockinfo->events [j].udata;
      if (socketInvalid (tsl->sock)) {
        continue;
      }
      tsl->havedata = true;
      ++sockinfo->havecount;
    }
#endif
#if ! _lib_kqueue
    for (int i = 0; i < sockinfo->count; ++i) {
      Sock_t  tsock;

      tsock = sockinfo->socklist [i]->sock;
      if (socketInvalid (tsock)) {
        continue;
      }
# if _lib_epoll_create1 && ! FORCE_SELECT
      for (int j = 0; j < rc; ++j) {
        if (tsock == sockinfo->events [j].data.fd) {
          sockinfo->socklist [i]->havedata = true;
          ++sockinfo->havecount;
        }
      }
# endif
# if FORCE_SELECT || (! _lib_epoll_create1 && ! _lib_kqueue)
      if (FD_ISSET (tsock, &(sockinfo->readfds))) {
        sockinfo->socklist [i]->havedata = true;
        ++sockinfo->havecount;
      }
# endif
    }
#endif
  }

  return 0;
}

NODISCARD
Sock_t
sockAccept (Sock_t lsock, int *err)
{
  Sock_t              nsock;

  *err = SOCK_CONN_ERROR;
  nsock = accept (lsock, NULL, NULL);

  if (socketInvalid (nsock)) {
    *err = errno;
    logError ("accept");
#if _lib_WSAGetLastError
    logMsg (LOG_DBG, LOG_SOCKET, "accept: wsa last-error:%d", WSAGetLastError());
#endif
    return INVALID_SOCKET;
  }

  mdextsock (nsock);
  if (sockSetNonBlocking (nsock) < 0) {
    mdextclose (nsock);
    close (nsock);
    return INVALID_SOCKET;
  }

  *err = SOCK_CONN_OK;
  return nsock;
}

/* note that in many cases, especially on windows, multiple calls to */
/* connect are necessary to make the connection */
NODISCARD
Sock_t
sockConnect (uint16_t connPort, int *connerr, Sock_t clsock)
{
  int                 rc;
  int                 err = 0;
  struct addrinfo     *result;
  struct addrinfo     *rp;

  sockInit ();

  *connerr = SOCK_CONN_ERROR;
  if (sockGetAddrInfo (connPort, &result) != 0) {
    return INVALID_SOCKET;
  }

  rc = -1;

  for (rp = result; rp != NULL; rp = rp->ai_next) {
    if (clsock == INVALID_SOCKET) {
      clsock = sockOpenClientSocket (rp, connerr);
      if (socketInvalid (clsock)) {
        /* try the next addr-info entry */
        continue;
      }
    }

    rc = connect (clsock, rp->ai_addr, rp->ai_addrlen);

    if (rc == 0) {
      *connerr = SOCK_CONN_OK;
    } else {
      err = errno;
    }

    if (rc < 0) {
      rc = sockCheckAlready (rc, connerr);
    }
    if (rc < 0) {
      err = sockCheckInProgress (rc, connerr);
      if (*connerr == SOCK_CONN_IN_PROGRESS) {
        rc = 0;
      }
    }

    if (rc < 0) {
      *connerr = SOCK_CONN_ERROR;

      /* ECONNREFUSED == 111 */
      /* ECONNABORTED == 103 */

      /* conn refused is a normal response, do not log */
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

    break;
  }

  mdextfree (result);
  freeaddrinfo (result);

  if (rc == 0 && *connerr == SOCK_CONN_OK) {
    logMsg (LOG_DBG, LOG_SOCKET, "connected to port:%d sock:%ld", connPort, (long) clsock);
  }
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
    logMsg (LOG_DBG, LOG_SOCKET, "sockReadBuff-b: len %d > maxlen %ld", len, (long) maxlen);
    return NULL;
  }
  rc = sockReadData (sock, data, len);
  if (rc < 0) {
    return NULL;
  }

  *rlen = len;
  return data;
}

int
sockWriteBinary (Sock_t sock, const char *data, size_t dlen,
    const char *args, size_t alen)
{
  uint32_t    sendlen;
  ssize_t     rc;

  if (socketInvalid (sock)) {
    return -1;
  }

  sendlen = (uint32_t) dlen;
  sendlen += alen;
  sendlen = htonl (sendlen);
  rc = sockWriteData (sock, (char *) &sendlen, sizeof (sendlen));
  if (rc < 0) {
    return -1;
  }
  rc = sockWriteData (sock, data, dlen);
  if (rc < 0) {
    return -1;
  }
  if (alen != 0) {
    rc = sockWriteData (sock, args, alen);
    if (rc < 0) {
      return -1;
    }
  }
  return 0;
}

NODISCARD
bool
socketInvalid (Sock_t sock)
{
#if _define_INVALID_SOCKET
  return (sock == INVALID_SOCKET);
#else
  return ((long) sock < 0);
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
    logMsg (LOG_DBG, LOG_SOCKET, "sockReadData-a invalid socket %ld", (long) sock);
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
sockWriteData (Sock_t sock, const char *data, size_t len)
{
  size_t        tot = 0;
  ssize_t       rc;

  if (socketInvalid (sock)) {
    return -1;
  }

  logProcBegin ();
  logMsg (LOG_DBG, LOG_SOCKET, "want to send: %" PRIu64 " bytes", (uint64_t) len);

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
    logProcEnd ("send-a-fail");
    return -1;
  }

  logMsg (LOG_DBG, LOG_SOCKET, "sent: %" PRIu64 " bytes", (uint64_t) rc);
  if (rc > 0) {
    tot += (size_t) rc;
    logMsg (LOG_DBG, LOG_SOCKET, "tot: %" PRIu64 " bytes", (uint64_t) tot);
  }

  while (tot < len) {
    rc = send (sock, data + tot, len - tot, 0);
    if (rc < 0 &&
        errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
      logError ("send-b");
#if _lib_WSAGetLastError
      logMsg (LOG_DBG, LOG_SOCKET, "send-b: wsa last-error: %d", WSAGetLastError());
#endif
      logProcEnd ("send-b-fail");
      return -1;
    }
    logMsg (LOG_DBG, LOG_SOCKET, "sent: %" PRIu64 " bytes", (uint64_t) rc);
    if (rc > 0) {
      tot += (size_t) rc;
      logMsg (LOG_DBG, LOG_SOCKET, "tot: %" PRIu64 " bytes", (uint64_t) tot);
    }
  }

  logProcEnd ("");
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

static int
sockSetNonBlocking (Sock_t sock)
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
# if _lib_WSAGetLastError
    logMsg (LOG_DBG, LOG_SOCKET, "ioctlsocket: wsa last-error:%d", WSAGetLastError());
# endif
  }
#endif
  return 0;
}

static void
sockInit (void)
{
  if (atomic_flag_test_and_set (&sockInitialized)) {
    return;
  }

#if _lib_WSAStartup
  {
    WSADATA wsa;
    int rc;

    rc = WSAStartup (MAKEWORD (2, 2), &wsa);
    if (rc < 0) {
      logError ("WSAStartup:");
# if _lib_WSAGetLastError
      logMsg (LOG_DBG, LOG_SOCKET, "wsastartup: wsa last-error:%d", WSAGetLastError());
# endif
    }
  }
#endif
  atexit (sockCleanup);
}

static void
sockCleanup (void)
{
#if _lib_WSACleanup
  WSACleanup ();
#endif
  atomic_flag_clear (&sockInitialized);
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
#if FORCE_SELECT || (! _lib_epoll_create1 && ! _lib_kqueue)
  FD_ZERO (&(sockinfo->readfdsbase));
  sockinfo->max = 0;
  for (int i = 0; i < sockinfo->count; ++i) {
    Sock_t tsock = sockinfo->socklist [i]->sock;
    if (socketInvalid (tsock)) {
      continue;
    }
    FD_SET (tsock, &(sockinfo->readfdsbase));
    if (tsock > sockinfo->max) {
      sockinfo->max = tsock;
    }
  }
#endif
}

static int
sockGetAddrInfo (uint16_t port, struct addrinfo **result)
{
  struct addrinfo   hints;
  char              portstr [20];
  int               rc;

  memset (&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_NUMERICSERV; // AI_CANONNAME
  /* NULL + AI_PASSIVE = listen on any network */
  /* NULL + ! AI_PASSIVE = listen on local network, connect on any */
  // hints.ai_flags &= ~ AI_PASSIVE;
#if ! defined (_WIN32)
  /* do not use AI_ADDRCONFIG on windows */
  hints.ai_flags |= AI_ADDRCONFIG;
#endif
  hints.ai_protocol = IPPROTO_TCP;
  hints.ai_canonname = NULL;
  hints.ai_addr = NULL;
  hints.ai_next = NULL;

  snprintf (portstr, sizeof (portstr), "%" PRIu16, port);
  rc = getaddrinfo (NULL, portstr, &hints, result);
  if (rc != 0) {
    logError ("getaddrinfo:");
    logMsg (LOG_ERR, LOG_SOCKET, "getaddrinfo: %s", gai_strerror (rc));
  } else {
    mdextalloc (*result);
  }

  return rc;
}

static Sock_t
sockOpenClientSocket (struct addrinfo *rp, int *connerr)
{
  Sock_t    clsock;
  int       err;

  clsock = socket (rp->ai_family, rp->ai_socktype, rp->ai_protocol);
  if (socketInvalid (clsock)) {
    logError ("connect:socket");
#if _lib_WSAGetLastError
    logMsg (LOG_DBG, LOG_SOCKET, "conn-socket: wsa last-error:%d", WSAGetLastError());
#endif
    *connerr = SOCK_CONN_FAIL;
    return INVALID_SOCKET;
  }

  mdextsock (clsock);

  if (sockSetNonBlocking (clsock) < 0) {
    *connerr = SOCK_CONN_FAIL;
    mdextclose (clsock);
    close (clsock);
    return INVALID_SOCKET;
  }

  clsock = sockSetOptions (clsock, &err);
  if (socketInvalid (clsock)) {
    *connerr = SOCK_CONN_FAIL;
    mdextclose (clsock);
    close (clsock);
    return INVALID_SOCKET;
  }

  return clsock;
}

static int
sockCheckAlready (int rc, int *connerr)
{
  /* the system may finish the connection on its own, in which case   */
  /* the next call to connect returns EISCONN */
  if (rc < 0 && (errno == EISCONN || errno == EALREADY)) {
    *connerr = SOCK_CONN_OK;
    rc = 0;
  }
#if _lib_WSAGetLastError
  if (WSAGetLastError() == WSAEISCONN) {
    *connerr = SOCK_CONN_OK;
    rc = 0;
  }
#endif

  return rc;
}

static int
sockCheckInProgress (int rc, int *connerr)
{
  int   err;

  err = errno;

#if _lib_WSAGetLastError
  /* 10037 */
  if (WSAGetLastError() == WSAEALREADY) {
    err = EINPROGRESS;
  }
  /* 10035 */
  if (WSAGetLastError() == WSAEWOULDBLOCK) {
    err = EWOULDBLOCK;
  }
  /* 10036 */
  if (WSAGetLastError() == WSAEINPROGRESS) {
    err = EINPROGRESS;
  }
#endif
  /* EINPROGRESS == 115 */
  /* EAGAIN == 11 */
  /* EWOULDBLOCK == 11 (linux) */
  if (err == EINPROGRESS || err == EAGAIN || err == EINTR || err == EWOULDBLOCK) {
    *connerr = SOCK_CONN_IN_PROGRESS;
  }

  return err;
}
