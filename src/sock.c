/*
 * https://docs.microsoft.com/en-us/windows/win32/winsock/windows-sockets-error-codes-2https://docs.microsoft.com/en-us/windows/win32/winsock/windows-sockets-error-codes-2
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>

#if _hdr_arpa_inet
# include <arpa/inet.h>
#endif
#if _hdr_fcntl
# include <fcntl.h>
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
#if _hdr_unistd
# include <unistd.h>
#endif

/* winsock2.h should come before windows.h */
#if _hdr_winsock2
# include <winsock2.h>
#endif
#if _hdr_windows
# include <windows.h>
#endif

#include "sock.h"
#include "tmutil.h"
#include "log.h"

#define SOCK_FMT "%d"
#if _siz_Sock_t == 8 && _siz_long == 8
# undef SOCK_FMT
# define SOCK_FMT "%ld"
#endif
#if _siz_Sock_t == 8 && _siz_long == 4
# undef SOCK_FMT
# define SOCK_FMT "%lld"
#endif

static ssize_t  sockReadData (Sock_t, char *, size_t);
static int      sockWriteData (Sock_t, char *, size_t);
static void     sockFlush (Sock_t);
static Sock_t   sockCanWrite (Sock_t);
static int      sockSetNonblocking (Sock_t sock);
// static int       sockSetBlocking (Sock_t sock);
static void     sockInit (void);
static void     sockCleanup (void);

static int      sockInitialized = 0;
static int      sockCount = 0;

Sock_t
sockServer (short listenPort, int *err)
{
  struct sockaddr_in  saddr;
  int                 rc;
  int                 opt = 1;

  if (! sockInitialized) {
    sockInit ();
  }

  Sock_t lsock = socket (AF_INET, SOCK_STREAM, 0);
  if (socketInvalid (lsock)) {
    *err = errno;
    logError (LOG_ERR, "sockServer: socket:", errno);
#if _lib_WSAGetLastError
    logVarMsg (LOG_ERR, "sockCheck: select: wsa err", "%d", WSAGetLastError() );
#endif
    return INVALID_SOCKET;
  }

  rc = setsockopt (lsock, SOL_SOCKET, SO_REUSEADDR, (const char *) &opt, sizeof (opt));
  if (rc != 0) {
    *err = errno;
    logError (LOG_ERR, "sockServer: setsockopt:", errno);
#if _lib_WSAGetLastError
    logVarMsg (LOG_ERR, "sockCheck: select: wsa err", "%d", WSAGetLastError() );
#endif
    close (lsock);
    return INVALID_SOCKET;
  }
#if _define_SO_REUSEPORT
  rc = setsockopt (lsock, SOL_SOCKET, SO_REUSEPORT, (const char *) &opt, sizeof (opt));
  if (rc != 0) {
    logError (LOG_ERR, "sockServer: setsockopt-b:", errno);
    *err = errno;
    close (lsock);
    return INVALID_SOCKET;
  }
#endif

  memset (&saddr, 0, sizeof (struct sockaddr_in));
  saddr.sin_family = AF_INET;
  saddr.sin_addr.s_addr = inet_addr ("127.0.0.1");
  saddr.sin_port = htons ((uint16_t) listenPort);

  rc = bind (lsock, (struct sockaddr *) &saddr, sizeof (struct sockaddr_in));
  while (rc != 0 && (errno == EADDRINUSE)) {
    msleep (100);
    rc = bind (lsock, (struct sockaddr *) &saddr, sizeof (struct sockaddr_in));
  }
  if (rc != 0) {
    *err = errno;
    logError (LOG_ERR, "sockServer: bind:", errno);
#if _lib_WSAGetLastError
    logVarMsg (LOG_ERR, "sockCheck: select: wsa err", "%d", WSAGetLastError() );
#endif
    close (lsock);
    return INVALID_SOCKET;
  }
  rc = listen (lsock, 10);
  if (rc != 0) {
    *err = errno;
    logError (LOG_ERR, "sockServer: listen:", errno);
#if _lib_WSAGetLastError
    logVarMsg (LOG_ERR, "sockCheck: select: wsa err", "%d", WSAGetLastError() );
#endif
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
  if (!socketInvalid (sock) && sock > 2) {
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
    sockinfo = malloc (sizeof (sockinfo_t));
    assert (sockinfo != NULL);
    sockinfo->count = 0;
    sockinfo->max = 0;
    sockinfo->socklist = NULL;
  }

  if (socketInvalid (sock) || sockinfo->count >= FD_SETSIZE) {
    logVarMsg (LOG_ERR, "sockAddCheck: invalid socket", SOCK_FMT, sock);
    return sockinfo;
  }

  idx = sockinfo->count;
  ++sockinfo->count;
  sockinfo->socklist = realloc (sockinfo->socklist,
      (size_t) sockinfo->count * sizeof (Sock_t));
  assert (sockinfo->socklist != NULL);
  sockinfo->socklist[idx] = sock;
  if (sock > sockinfo->max) {
    sockinfo->max = sock;
  }

  return sockinfo;
}

void
sockRemoveCheck (sockinfo_t *sockinfo, Sock_t sock)
{
  if (sockinfo == NULL) {
    return;
  }

  if (socketInvalid (sock)) {
    logVarMsg (LOG_ERR, "sockRemoveCheck: invalid socket", SOCK_FMT, sock);
    return;
  }

  for (size_t i = 0; i < (size_t) sockinfo->count; ++i) {
    if (sockinfo->socklist[i] == sock) {
      sockinfo->socklist[i] = INVALID_SOCKET;
      break;
    }
  }
}

void
sockFreeCheck (sockinfo_t *sockinfo)
{
  if (sockinfo != NULL) {
    sockinfo->count = 0;
    if (sockinfo->socklist != NULL) {
      free (sockinfo->socklist);
    }
    free (sockinfo);
  }
}

Sock_t
sockCheck (sockinfo_t *sockinfo)
{
  int               rc;
  struct timeval    tv;

  if (sockinfo == NULL) {
    return 0;
  }

  FD_ZERO (&(sockinfo->readfds));
  for (size_t i = 0; i < (size_t) sockinfo->count; ++i) {
    Sock_t tsock = sockinfo->socklist[i];
    if (socketInvalid (tsock)) {
      continue;
    }
    FD_SET (tsock, &(sockinfo->readfds));
  }

  tv.tv_sec = 0;
  tv.tv_usec = (suseconds_t)
      (SOCK_READ_TIMEOUT * sockinfo->count * 1000);

  rc = select (sockinfo->max + 1, &(sockinfo->readfds), NULL, NULL, &tv);
  if (rc < 0) {
    if (errno == EINTR) {
      return 0;
    }
    logError (LOG_ERR, "sockCheck: select", errno);
#if _lib_WSAGetLastError
    logVarMsg (LOG_ERR, "sockCheck: select: wsa err", "%d", WSAGetLastError() );
#endif
    return -1;
  }
  if (rc > 0) {
    for (size_t i = 0; i < (size_t) sockinfo->count; ++i) {
      Sock_t tsock = sockinfo->socklist [i];
      if (socketInvalid (tsock)) {
        continue;
      }
      if (FD_ISSET (tsock, &(sockinfo->readfds))) {
        return tsock;
      }
    }
  }
  return 0;
}

Sock_t
sockAccept (Sock_t lsock, int *err)
{
  struct sockaddr_in  saddr;
  Socklen_t           alen;

  alen = sizeof (struct sockaddr_in);
  Sock_t nsock = accept (lsock, (struct sockaddr *) &saddr, &alen);
  if (socketInvalid (nsock)) {
    *err = errno;
    logError (LOG_ERR, "sockAccept: accept", errno);
#if _lib_WSAGetLastError
    logVarMsg (LOG_ERR, "sockAccept: accept: wsa err", "%d", WSAGetLastError() );
#endif
    return INVALID_SOCKET;
  }

  if (sockSetNonblocking (nsock) < 0) {
    close (nsock);
    return INVALID_SOCKET;
  }

  ++sockCount;
  *err = 0;
  return nsock;
}

Sock_t
sockConnect (short connPort, int *err)
{
  struct sockaddr_in  raddr;
  int                 rc;
  int                 opt = 1;

  if (! sockInitialized) {
    sockInit ();
  }

  Sock_t clsock = socket (AF_INET, SOCK_STREAM, 0);
  if (socketInvalid (clsock)) {
    *err = errno;
    logError (LOG_ERR, "sockConnect: connect", errno);
#if _lib_WSAGetLastError
    logVarMsg (LOG_ERR, "sockConnect: connect: wsa err", "%d", WSAGetLastError() );
#endif
    return INVALID_SOCKET;
  }
  if (sockSetNonblocking (clsock) < 0) {
    *err = errno;
    close (clsock);
    return INVALID_SOCKET;
  }

  rc = setsockopt (clsock, SOL_SOCKET, SO_REUSEADDR, (const char *) &opt, sizeof (opt));
  if (rc != 0) {
    *err = errno;
    logError (LOG_ERR, "sockConnect: setsockopt", errno);
    close (clsock);
    return INVALID_SOCKET;
  }
#if _define_SO_REUSEPORT
  rc = setsockopt (clsock, SOL_SOCKET, SO_REUSEPORT, (const char *) &opt, sizeof (opt));
  if (rc != 0) {
    *err = errno;
    logError (LOG_ERR, "sockConnect: setsockopt-b", errno);
    close (clsock);
    return INVALID_SOCKET;
  }
#endif

  memset (&raddr, 0, sizeof (struct sockaddr_in));
  raddr.sin_family = AF_INET;
  raddr.sin_addr.s_addr = inet_addr ("127.0.0.1");
  raddr.sin_port = htons ((uint16_t) connPort);
  rc = connect (clsock, (struct sockaddr *) &raddr, sizeof (struct sockaddr_in));
  if (rc != 0) {
    *err = errno;
#if _lib_WSAGetLastError
    if (WSAGetLastError() == WSAEWOULDBLOCK) {
      errno = EWOULDBLOCK;
    }
#endif
    *err = errno;
    if (errno != EINPROGRESS && errno != EAGAIN && errno != EINTR && errno != EWOULDBLOCK) {
      logError (LOG_ERR, "sockConnect: connect", errno);
#if _lib_WSAGetLastError
      logVarMsg (LOG_ERR, "sockConnect: connect: wsa err", "%d", WSAGetLastError() );
#endif
      close (clsock);
      return INVALID_SOCKET;
    }
  }
  /* darwin needs a boost to get the socket fully connected */
  msleep (1);

  ++sockCount;
  *err = 0;
  return clsock;
}

Sock_t
sockConnectWait (short connPort, size_t timeout)
{
  int               clsock;
  int               err;
  int               count;
  mtime_t           mi;

  mtimestart (&mi);
  count = 0;
  clsock = sockConnect (connPort, &err);
  ++count;
  while (socketInvalid (clsock) &&
     (err == 0 || err == EALREADY || err == EAGAIN ||
     err == EINPROGRESS || err == EINTR)) {
    msleep (30);
    size_t m = mtimeend (&mi);
    if (m > timeout) {
      return INVALID_SOCKET;
    }
    clsock = sockConnect (connPort, &err);
    ++count;
  }

  return clsock;
}

char *
sockReadBuff (Sock_t sock, size_t *rlen, char *data, size_t maxlen)
{
  size_t      len;
  ssize_t     rc;

  len = 0;
  *rlen = 0;
  rc = sockReadData (sock, (char *) &len, sizeof (size_t));
  if (rc < 0) {
    return NULL;
  }
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
  size_t      len;
  ssize_t     rc;
  char        *data;

  *rlen = 0;
  rc = sockReadData (sock, (char *) &len, sizeof (size_t));
  if (rc < 0) {
    return NULL;
  }
  data = malloc (len);
  assert (data != NULL);
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
  int rc = sockWriteBinary (sock, data, len + 1);
  return rc;
}

int
sockWriteBinary (Sock_t sock, char *data, size_t len)
{
  ssize_t     rc;

  rc = sockWriteData (sock, (char *) &len, sizeof (size_t));
  if (rc < 0) {
    return -1;
  }
  rc = sockWriteData (sock, data, len);
  if (rc < 0) {
    return -1;
  }
  return 0;
}

/* boolean */
inline int
socketInvalid (Sock_t sock)
{
#if _define_INVALID_SOCKET
  return (sock == INVALID_SOCKET);
#else
  return (sock < 0);
#endif
}

/* internal routines */

static ssize_t
sockReadData (Sock_t sock, char *data, size_t len)
{
  size_t        tot = 0;
  ssize_t       rc;
  mtime_t       mi;
  size_t        timeout;

  timeout = len * SOCK_READ_TIMEOUT;
  mtimestart (&mi);
  rc = recv (sock, data, len, 0);
  if (rc < 0) {
#if _lib_WSAGetLastError
    if (WSAGetLastError() == WSAEWOULDBLOCK) {
      errno = EWOULDBLOCK;
    }
#endif
    if (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
      logError (LOG_ERR, "sockReadData: recv", errno);
#if _lib_WSAGetLastError
      logVarMsg (LOG_ERR, "sockReadData: recv: wsa err", "%d", WSAGetLastError() );
#endif
      return -1;
    }
  }
  if (rc > 0) {
    tot += (size_t) rc;
  }
  while (tot < len) {
    rc = recv (sock, data + tot, len - tot, 0);
    if (rc < 0) {
#if _lib_WSAGetLastError
      if (WSAGetLastError() == WSAEWOULDBLOCK) {
        errno = EWOULDBLOCK;
      }
#endif
      if (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
        logError (LOG_ERR, "sockReadData: recv-b", errno);
#if _lib_WSAGetLastError
        logVarMsg (LOG_ERR, "sockReadData: recv-b: wsa err", "%d", WSAGetLastError() );
#endif
        return -1;
      }
    }
    if (rc > 0) {
      tot += (size_t) rc;
    }
    if (tot == 0) {
      msleep (5);
    }
    size_t m = mtimeend (&mi);
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
  Sock_t        sval;
  ssize_t       rc;

  /* ugh.  the write() call blocks on a non-blocking socket.  sigh. */
  /* call select() and check the condition the socket is in to see  */
  /* if it is writable.                                             */
  sval = sockCanWrite (sock);
  if (sval != sock) {
    return -1;
  }
  rc = send (sock, data, len, 0);
  if (rc < 0 &&
      errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
    logError (LOG_ERR, "sockWriteData: send", errno);
#if _lib_WSAGetLastError
    logVarMsg (LOG_ERR, "sockWriteData: send: wsa err", "%d", WSAGetLastError() );
#endif
    return -1;
  }
  if (rc > 0) {
    tot += (size_t) rc;
  }
  while (tot < len) {
    rc = send (sock, data + tot, len - tot, 0);
    if (rc < 0 &&
        errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
      logError (LOG_ERR, "sockWriteData: send-b", errno);
#if _lib_WSAGetLastError
      logVarMsg (LOG_ERR, "sockWriteData: send-b: wsa err", "%d", WSAGetLastError() );
#endif
      return -1;
    }
    if (rc > 0) {
      tot += (size_t) rc;
    }
  }
  return 0;
}

static void
sockFlush (Sock_t sock)
{
  char      data [1024];
  size_t    len = 1024;
  int       count;
  ssize_t   rc;

  count = 0;
  rc = recv (sock, data, len, 0);
  if (rc < 0) {
#if _lib_WSAGetLastError
    if (WSAGetLastError() == WSAEWOULDBLOCK) {
      errno = EWOULDBLOCK;
    }
#endif
    if (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
      logError (LOG_ERR, "sockFlush: recv", errno);
#if _lib_WSAGetLastError
      logVarMsg (LOG_ERR, "sockFlush: recv: wsa err", "%d", WSAGetLastError() );
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
        logError (LOG_ERR, "sockFlush: recv-b", errno);
#if _lib_WSAGetLastError
        logVarMsg (LOG_ERR, "sockFlush: recv-b wsa err", "%d", WSAGetLastError() );
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

static Sock_t
sockCanWrite (Sock_t sock)
{
  fd_set            readfds;
  fd_set            writefds;
  struct timeval    tv;

  FD_ZERO (&readfds);
  FD_ZERO (&writefds);
  FD_SET (sock, &readfds);
  FD_SET (sock, &writefds);

  tv.tv_sec = 0;
  tv.tv_usec = (suseconds_t) (SOCK_WRITE_TIMEOUT * 1 * 1000);

  int rc = select (sock + 1, &readfds, &writefds, NULL, &tv);
  if (rc < 0) {
    if (errno == EINTR) {
      return sockCanWrite (sock);
    }
    logError (LOG_ERR, "sockCanWrite: select", errno);
#if _lib_WSAGetLastError
    logVarMsg (LOG_ERR, "sockCanWrite: select: wsa err", "%d", WSAGetLastError() );
#endif
    return -1;
  }
  if (FD_ISSET (sock, &writefds) && ! FD_ISSET(sock, &readfds)) {
    return sock;
  }
  return 0;
}


static int
sockSetNonblocking (Sock_t sock)
{
#if _lib_fcntl
  int         flags;
  int         rc;

  flags = fcntl (sock, F_GETFL, 0);
  if (flags < 0) {
    logError (LOG_ERR, "sockSetNonblocking: fcntl-get", errno);
    return -1;
  }
  flags |= O_NONBLOCK;
  rc = fcntl (sock, F_SETFL, flags);
  if (rc != 0) {
    logError (LOG_ERR, "sockSetNonblocking: fcntl-set", errno);
    return -1;
  }
#endif
#if _lib_ioctlsocket
  unsigned long flag = 1;
  if (ioctlsocket (sock, FIONBIO, &flag) < 0) {
    logError (LOG_ERR, "sockSetNonblocking: ioctlsocket", errno);
#if _lib_WSAGetLastError
    logVarMsg (LOG_ERR, "sockSetNonblocking: ioctlsocket: wsa err", "%d", WSAGetLastError() );
#endif
  }
#endif
  return 0;
}

#if 0

static int
sockSetBlocking (Sock_t sock)
{
#if _lib_fcntl
  int         flags;
  int         rc;

  flags = fcntl (sock, F_GETFL, 0);
  if (flags < 0) {
    logError (LOG_ERR, "sockSetBlocking: fcntl-get", errno);
    return -1;
  }
  flags &= ~O_NONBLOCK;
  rc = fcntl (sock, F_SETFL, flags);
  if (rc != 0) {
    logError (LOG_ERR, "sockSetBlocking: fcntl-set", errno);
    return -1;
  }
#endif
#if _lib_ioctlsocket
  unsigned long flag = 1;
  if (ioctlsocket (sock, FIOBIO, &flag) < 0) {
    logError (LOG_ERR, "sockSetNonblocking: ioctlsocket", errno);
  }
#endif
  return 0;
}

#endif


static void
sockInit (void)
{
#if _lib_WSAStartup
  WSADATA wsa;
  int rc = WSAStartup (MAKEWORD (2, 2), &wsa);
  if (rc < 0) {
    logError (LOG_ERR, "sockInit: WSAStartup:", errno);
#if _lib_WSAGetLastError
    logVarMsg (LOG_ERR, "sockInit: WSAStartup: wsa err", "%d", WSAGetLastError() );
#endif
  }
  assert (rc == 0);
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
