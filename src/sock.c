#include "bdjconfig.h"

#include <stdio.h>
#include <stdlib.h>
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
#if _hdr_windows
# include <windows.h>
#endif
#if _hdr_winsock2
# include <winsock2.h>
#endif

#include "sock.h"
#include "tmutil.h"
#include "log.h"

static ssize_t  sockReadData (Sock_t, char *, size_t);
static int      sockWriteData (Sock_t, char *, size_t);
static void     sockFlush (Sock_t);
static int      sockCanWrite (Sock_t);
static int      sockSetNonblocking (Sock_t sock);
// static int       sockSetBlocking (Sock_t sock);

Sock_t
sockServer (short listenPort, int *err)
{
  struct sockaddr_in  saddr;
  int                 rc;
  int                 opt = 1;

  Sock_t lsock = socket (AF_INET, SOCK_STREAM, 0);
  if (lsock < 0) {
    *err = errno;
    logError (LOG_ERR, "sockServer: socket:", errno);
    return lsock;
  }

  rc = setsockopt (lsock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof (opt));
  if (rc != 0) {
    *err = errno;
    logError (LOG_ERR, "sockServer: setsockopt:", errno);
    close (lsock);
    return -1;
  }
#if _define_SO_REUSEPORT
  rc = setsockopt (lsock, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof (opt));
  if (rc != 0) {
    logError (LOG_ERR, "sockServer: setsockopt-b:", errno);
    *err = errno;
    close (lsock);
    return -1;
  }
#endif

  memset (&saddr, 0, sizeof (struct sockaddr_in));
  saddr.sin_family = AF_INET;
  saddr.sin_port = htons ((Uint16_t) listenPort);
  saddr.sin_addr.s_addr = inet_addr ("127.0.0.1");

  rc = bind (lsock, (struct sockaddr *) &saddr, sizeof (struct sockaddr_in));
  while (rc != 0 && (errno == EADDRINUSE)) {
    msleep (100);
    rc = bind (lsock, (struct sockaddr *) &saddr, sizeof (struct sockaddr_in));
  }
  if (rc != 0) {
    *err = errno;
    logError (LOG_ERR, "sockServer: bind:", errno);
    close (lsock);
    return -1;
  }
  rc = listen (lsock, 10);
  if (rc != 0) {
    *err = errno;
    logError (LOG_ERR, "sockServer: listen:", errno);
    close (lsock);
    return -1;
  }

  *err = 0;
  return lsock;
}

void
sockClose (Sock_t sock)
{
  close (sock);
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

  if (sock < 0 || sock >= FD_SETSIZE) {
    logVarMsg (LOG_ERR, "sockAddCheck: invalid socket", "%d", sock);
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

  if (sock < 0 || sock >= FD_SETSIZE) {
    logVarMsg (LOG_ERR, "sockRemoveCheck: invalid socket", "%d", sock);
    return;
  }

  for (size_t i = 0; i < (size_t) sockinfo->count; ++i) {
    if (sockinfo->socklist[i] == sock) {
      sockinfo->socklist[i] = -1;
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
    if (tsock < 0 || tsock >= FD_SETSIZE) {
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
    return -1;
  }
  if (rc > 0) {
    for (size_t i = 0; i < (size_t) sockinfo->count; ++i) {
      Sock_t tsock = sockinfo->socklist[i];
      if (tsock < 0) {
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

  Sock_t nsock = accept (lsock, (struct sockaddr *) &saddr, &alen);
  if (nsock < 0) {
    *err = errno;
    logError (LOG_ERR, "sockAccept: accept", errno);
    return -1;
  }

  if (sockSetNonblocking (nsock) < 0) {
    close (nsock);
    return -1;
  }

  *err = 0;
  return nsock;
}

Sock_t
sockConnect (short connPort, int *err)
{
  struct sockaddr_in  raddr;
  int                 rc;
  int                 opt = 1;

  Sock_t clsock = socket (AF_INET, SOCK_STREAM, 0);
  if (clsock < 0) {
    *err = errno;
    logError (LOG_ERR, "sockConnect: socket", errno);
    return clsock;
  }
  if (sockSetNonblocking (clsock) < 0) {
    *err = errno;
    close (clsock);
    return -1;
  }

  rc = setsockopt (clsock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof (opt));
  if (rc != 0) {
    *err = errno;
    logError (LOG_ERR, "sockConnect: setsockopt", errno);
    close (clsock);
    return -1;
  }
#if _define_SO_REUSEPORT
  rc = setsockopt (clsock, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof (opt));
  if (rc != 0) {
    *err = errno;
    logError (LOG_ERR, "sockConnect: setsockopt-b", errno);
    close (clsock);
    return -1;
  }
#endif

  memset (&raddr, 0, sizeof (struct sockaddr_in));
  raddr.sin_family = AF_INET;
  raddr.sin_addr.s_addr = inet_addr ("127.0.0.1");
  raddr.sin_port = htons ((Uint16_t) connPort);
  rc = connect (clsock, (struct sockaddr *) &raddr, sizeof (struct sockaddr_in));
  if (rc != 0) {
    *err = errno;
    if (errno != EINPROGRESS && errno != EAGAIN && errno != EINTR) {
      logError (LOG_ERR, "sockConnect: connect", errno);
    }
  }
  /* darwin needs a boost to get the socket fully connected */
  msleep (1);

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
  while (clsock < 0 &&
     (err == 0 || err == EALREADY || err == EAGAIN ||
     err == EINPROGRESS || err == EINTR)) {
    msleep (30);
    size_t m = mtimeend (&mi);
    if (m > timeout) {
      return -1;
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
  rc = read (sock, data, len);
  if (rc < 0 &&
      errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
    logError (LOG_ERR, "sockReadData: read", errno);
    return -1;
  }
  if (rc > 0) {
    tot += (size_t) rc;
  }
  while (tot < len) {
    rc = read (sock, data + tot, len - tot);
    if (rc < 0 &&
        errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
      logError (LOG_ERR, "sockReadData: read-b", errno);
      return -1;
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
  ssize_t       rc;

  /* ugh.  the write() call blocks on a non-blocking socket.  sigh. */
  /* call select() and check the condition the socket is in to see  */
  /* if it is writable.                                             */
  rc = sockCanWrite (sock);
  if (rc != sock) {
    return -1;
  }
  rc = write (sock, data, len);
  if (rc < 0 &&
      errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
    logError (LOG_ERR, "sockWriteData: write", errno);
    return -1;
  }
  if (rc > 0) {
    tot += (size_t) rc;
  }
  while (tot < len) {
    rc = write (sock, data + tot, len - tot);
    if (rc < 0 &&
        errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
      logError (LOG_ERR, "sockWriteData: write-b", errno);
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
  rc = read (sock, data, len);
  if (rc < 0 &&
      errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
    logError (LOG_ERR, "sockFlush: read", errno);
    return;
  }
  while (rc >= 0) {
    rc = read (sock, data, len);
    if (rc < 0 &&
        errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
      logError (LOG_ERR, "sockFlush: read-b", errno);
      return;
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
