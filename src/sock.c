#include "bdjconfig.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#if _sys_socket
# include <sys/socket.h>
#endif
#if _hdr_arpa_inet
# include <arpa/inet.h>
#endif
#if _hdr_netdb
# include <netdb.h>
#endif
#if _hdr_netinet_in
# include <netinet/in.h>
#endif
#include <signal.h>
#if _hdr_poll
# include <poll.h>
#endif
#if _hdr_fcntl
# include <fcntl.h>
#endif
#if _hdr_unistd
# include <unistd.h>
#endif

#include "sock.h"
#include "tmutil.h"

static ssize_t  sockReadData (int, char *, size_t);
static int      sockWriteData (int, char *, size_t);
static void     sockFlush (int sock);
static int      sockSetNonblocking (int sock);
// static int       sockSetBlocking (int sock);


int
sockServer (short listenPort, int *err)
{
  struct sockaddr_in  saddr;
  int                 rc;
  int                 opt = 1;

  int lsock = socket (AF_INET, SOCK_STREAM, 0);
  if (lsock < 0) {
    *err = errno;
    return lsock;
  }

  rc = setsockopt (lsock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof (opt));
  if (rc != 0) {
    *err = errno;
    close (lsock);
    return -1;
  }
  rc = setsockopt (lsock, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof (opt));
  if (rc != 0) {
    *err = errno;
    close (lsock);
    return -1;
  }

  memset (&saddr, 0, sizeof (struct sockaddr_in));
  saddr.sin_family = AF_INET;
  saddr.sin_port = htons ((uint16_t) listenPort);
  saddr.sin_addr.s_addr = inet_addr ("127.0.0.1");
  rc = bind (lsock, (struct sockaddr *) &saddr, sizeof (struct sockaddr_in));
  while (rc != 0 && (errno == EADDRINUSE)) {
    msleep (100);
    rc = bind (lsock, (struct sockaddr *) &saddr, sizeof (struct sockaddr_in));
  }
  if (rc != 0) {
    *err = errno;
    close (lsock);
    return -1;
  }
  rc = listen (lsock, 10);
  if (rc != 0) {
    *err = errno;
    close (lsock);
    return -1;
  }

  *err = 0;
  return lsock;
}

void
sockClose (int sock)
{
  close (sock);
}

sockinfo_t *
sockAddPoll (sockinfo_t *sockinfo, int sock)
{
  struct pollfd   *p;
  int             idx;

  if (sockinfo == NULL) {
    sockinfo = malloc (sizeof (sockinfo_t));
    assert (sockinfo != NULL);
    sockinfo->pollcount = 0;
    sockinfo->pollfds = NULL;
  }

  idx = sockinfo->pollcount;
  ++sockinfo->pollcount;
  sockinfo->pollfds = realloc (sockinfo->pollfds,
      sizeof (struct pollfd) * (size_t) sockinfo->pollcount);
  p = &(sockinfo->pollfds [idx]);
  p->fd = sock;
  p->events |= POLLIN;

  return sockinfo;
}

void
sockRemovePoll (sockinfo_t *sockinfo, int sock)
{
  if (sockinfo == NULL) {
    return;
  }

  for (size_t i = 0; i < (size_t) sockinfo->pollcount; ++i) {
    if (sockinfo->pollfds [i].fd == sock) {
      sockinfo->pollfds [i].fd = -1;
    }
  }
}

void
sockFreePoll (sockinfo_t *sockinfo)
{
  if (sockinfo != NULL) {
    if (sockinfo->pollfds != NULL) {
      free (sockinfo->pollfds);
      sockinfo->pollcount = 0;
    }
    free (sockinfo);
  }
}

int
sockPoll (sockinfo_t *sockinfo)
{
  int     rc;

  if (sockinfo == NULL) {
    return 0;
  }

  rc = poll (sockinfo->pollfds, (nfds_t) sockinfo->pollcount, 10);
  if (rc < 0) {
    /* ### FIX */
    /* process error */
    return -1;
  }
  if (rc > 0) {
    for (size_t i = 0; i < (size_t) sockinfo->pollcount; ++i) {
      if (sockinfo->pollfds [i].revents & (POLLIN | POLLHUP)) {
        return sockinfo->pollfds [i].fd;
      }
    }
    --rc;
  }
  return 0;
}

int
sockAccept (int lsock, int *err)
{
  struct sockaddr_in  saddr;
  socklen_t           alen;

  int nsock = accept (lsock, (struct sockaddr *) &saddr, &alen);
  if (nsock < 0) {
    *err = errno;
    return -1;
  }

  if (sockSetNonblocking (nsock) < 0) {
    close (nsock);
    return -1;
  }

  *err = 0;
  return nsock;
}

int
sockConnect (short connPort, int *err)
{
  struct sockaddr_in  raddr;
  int                 rc;
  int                 opt = 1;

  int clsock = socket (AF_INET, SOCK_STREAM, 0);
  if (clsock < 0) {
    *err = errno;
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
    close (clsock);
    return -1;
  }
  rc = setsockopt (clsock, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof (opt));
  if (rc != 0) {
    *err = errno;
    close (clsock);
    return -1;
  }

  memset (&raddr, 0, sizeof (struct sockaddr_in));
  raddr.sin_family = AF_INET;
  raddr.sin_addr.s_addr = inet_addr ("127.0.0.1");
  raddr.sin_port = htons ((uint16_t) connPort);
  rc = connect (clsock, (struct sockaddr *) &raddr, sizeof (struct sockaddr_in));
  if (rc != 0) {
    *err = errno;
  }
  /* darwin needs a boost to get the socket fully connected */
  msleep (1);

  *err = 0;
  return clsock;
}

int
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
sockReadBuff (int sock, size_t *rlen, char *data, size_t maxlen)
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
sockRead (int sock, size_t *rlen)
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
sockWriteStr (int sock, char *data, size_t len)
{
  int rc = sockWriteBinary (sock, data, len + 1);
  return rc;
}

int
sockWriteBinary (int sock, char *data, size_t len)
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
sockReadData (int sock, char *data, size_t len)
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
    return -1;
  }
  if (rc > 0) {
    tot += (size_t) rc;
  }
  while (tot < len) {
    rc = read (sock, data + tot, len - tot);
    if (rc < 0 &&
        errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
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
  return 0;
}

static int
sockWriteData (int sock, char *data, size_t len)
{
  size_t        tot = 0;
  ssize_t       rc;

  rc = write (sock, data, len);
  if (rc < 0 &&
      errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
    return -1;
  }
  if (rc > 0) {
    tot += (size_t) rc;
  }
  while (tot < len) {
    rc = write (sock, data + tot, len - tot);
    if (rc < 0 &&
        errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
      return -1;
    }
    if (rc > 0) {
      tot += (size_t) rc;
    }
  }
  return 0;
}

static void
sockFlush (int sock)
{
  char      data [1024];
  size_t    len = 1024;
  int       count;
  ssize_t   rc;

  count = 0;
  rc = read (sock, data, len);
  if (rc < 0 &&
      errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
    return;
  }
  while (rc >= 0) {
    rc = read (sock, data, len);
    if (rc < 0 &&
        errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
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
sockSetNonblocking (int sock)
{
  int         flags;
  int         rc;

  flags = fcntl (sock, F_GETFL, 0);
  if (flags < 0) {
    return -1;
  }
  flags |= O_NONBLOCK;
  rc = fcntl (sock, F_SETFL, flags);
  if (rc != 0) {
    return -1;
  }

  return 0;
}

#if 0

static int
sockSetBlocking (int sock)
{
  int         flags;
  int         rc;

  flags = fcntl (sock, F_GETFL, 0);
  if (flags < 0) {
    return -1;
  }
  flags &= ~O_NONBLOCK;
  rc = fcntl (sock, F_SETFL, flags);
  if (rc != 0) {
    return -1;
  }

  return 0;
}

#endif
