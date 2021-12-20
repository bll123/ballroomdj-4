#include "bdjconfig.h"

#include <stdio.h>
#include <stdlib.h>
#include <check.h>
#include <sys/types.h>
#include <sys/stat.h>
#if _hdr_unistd
# include <unistd.h>
#endif
#if _hdr_poll
# include <poll.h>
#endif

#include "sock.h"
#include "check_bdj.h"
#include "utility.h"

START_TEST(sock_server_create)
{
  int         err;

  int s = sockServer (32700, &err);
  ck_assert (s >= 0);
  sockClose (s);
}
END_TEST

START_TEST(sock_server_create_poll)
{
  sockinfo_t    *si;
  int         err;

  si = NULL;
  int s = sockServer (32701, &err);
  ck_assert (s >= 0);
  si = sockAddPoll (si, s);
  ck_assert (si->pollcount == 1);
  ck_assert (si->pollfds[0].fd == s);
  ck_assert (si->pollfds[0].events != 0);
  sockFreePoll (si);
  sockClose (s);
}
END_TEST

START_TEST(sock_server_poll)
{
  sockinfo_t    *si;
  int           rc;
  int         err;

  si = NULL;
  int s = sockServer (32702, &err);
  ck_assert (s >= 0);
  si = sockAddPoll (si, s);
  rc = sockPoll (si);
  ck_assert (rc == 0);
  sockFreePoll (si);
  sockClose (s);
}
END_TEST


START_TEST(sock_connect_accept)
{
  int           err;
  pid_t         pid;
  int           c;
  int           r;
  int           l;

  pid = check_fork ();

  if (pid == 0) {
    c = sockConnectWait (32703, 1000);
    ck_assert (c >= 0);
  } else {
    l = sockServer (32703, &err);
    ck_assert (l >= 0);
    r = sockAccept (l, &err);
    ck_assert (r >= 0);
    ck_assert (l != r);
  }
  msleep (200);
  if (pid == 0) {
    sockClose (c);
    exit (0);
  } else {
    sockClose (r);
  }
  check_waitpid_and_exit (pid);
  sockClose (l);
}
END_TEST

START_TEST(sock_poll_connect_accept)
{
  sockinfo_t    *si;
  int           rc;
  int           err;
  pid_t         pid;
  int           c;
  int           l;
  int           r;

  si = NULL;
  pid = check_fork ();

  if (pid == 0) {
    c = sockConnectWait (32704, 1000);
    ck_assert (c >= 0);
    ck_assert (l != c);
  } else {
    l = sockServer (32704, &err);
    ck_assert (l >= 0);
    si = sockAddPoll (si, l);
    rc = sockPoll (si);
    while (rc == 0) {
      msleep (20);
      rc = sockPoll (si);
    }
    ck_assert (rc == l);
    r = sockAccept (l, &err);
    ck_assert (r >= 0);
    ck_assert (l != r);
  }
  msleep (200);
  if (pid == 0) {
    sockClose (c);
    exit (0);
  } else {
    sockClose (r);
  }
  check_waitpid_and_exit (pid);
  sockClose (l);
}
END_TEST

START_TEST(sock_write)
{
  sockinfo_t    *si;
  int           rc;
  int           err;
  pid_t         pid;
  int           c;
  int           l;
  int           r;
  char          *data = { "aaaabbbbccccddddeeeeffff" };
  char          datab [4096];

  si = NULL;
  pid = check_fork ();
  memset (datab, 'a', 4096);

  if (pid == 0) {
    c = sockConnectWait (32705, 1000);
    ck_assert (c >= 0);
    ck_assert (l != c);
    rc = sockWriteStr (c, data, strlen (data));
    ck_assert (rc == 0);
    rc = sockWriteBinary (c, datab, 4096);
    ck_assert (rc == 0);
  } else {
    l = sockServer (32705, &err);
    ck_assert (l >= 0);
    si = sockAddPoll (si, l);
    rc = sockPoll (si);
    while (rc == 0) {
      msleep (20);
      rc = sockPoll (si);
    }
    ck_assert (rc == l);
    r = sockAccept (l, &err);
    ck_assert (r >= 0);
    ck_assert (l != r);
  }
  msleep (200);
  if (pid == 0) {
    sockClose (c);
    exit (0);
  } else {
    sockClose (r);
  }
  check_waitpid_and_exit (pid);
  sockClose (l);
}
END_TEST

START_TEST(sock_write_read)
{
  sockinfo_t    *si;
  int           rc;
  int           err;
  pid_t         pid;
  int           c;
  int           l;
  int           r;
  char          *data = { "aaaabbbbccccddddeeeeffff" };
  char          datab [4096];
  char          *ndata;
  size_t        len;

  si = NULL;
  pid = check_fork ();
  memset (datab, 'a', 4096);

  if (pid == 0) {
    c = sockConnectWait (32706, 1000);
    ck_assert (c >= 0);
    ck_assert (l != c);
    rc = sockWriteStr (c, data, strlen (data));
    ck_assert (rc == 0);
    rc = sockWriteBinary (c, datab, 4096);
    ck_assert (rc == 0);
  } else {
    l = sockServer (32706, &err);
    ck_assert (l >= 0);
    si = sockAddPoll (si, l);
    rc = sockPoll (si);
    while (rc == 0) {
      msleep (20);
      rc = sockPoll (si);
    }
    ck_assert (rc == l);
    r = sockAccept (l, &err);
    ck_assert (r >= 0);
    ck_assert (l != r);

    ndata = sockRead (r, &len);
    ck_assert (strlen (ndata) + 1 == len);
    ck_assert (strlen (ndata) == strlen (data));
    ck_assert (strcmp (ndata, data) == 0);
    free (ndata);

    ndata = sockRead (r, &len);
    ck_assert (len == 4096);
    ck_assert (memcmp (ndata, datab, 4096) == 0);
    free (ndata);
  }
  msleep (200);
  if (pid == 0) {
    sockClose (c);
    exit (0);
  } else {
    sockClose (r);
  }
  check_waitpid_and_exit (pid);
  sockClose (l);
}
END_TEST

START_TEST(sock_write_read_buff)
{
  sockinfo_t    *si;
  int           rc;
  int           err;
  pid_t         pid;
  int           c;
  int           l;
  int           r;
  char          *data = { "aaaabbbbccccddddeeeeffff" };
  char          buff [80];
  char          *ndata;
  size_t        len;

  si = NULL;
  pid = check_fork ();

  if (pid == 0) {
    c = sockConnectWait (32707, 1000);
    ck_assert (c >= 0);
    ck_assert (l != c);
    rc = sockWriteStr (c, data, strlen (data));
    ck_assert (rc == 0);
  } else {
    l = sockServer (32707, &err);
    ck_assert (l >= 0);
    si = sockAddPoll (si, l);
    rc = sockPoll (si);
    while (rc == 0) {
      msleep (20);
      rc = sockPoll (si);
    }
    ck_assert (rc == l);
    r = sockAccept (l, &err);
    ck_assert (r >= 0);
    ck_assert (l != r);

    ndata = sockReadBuff (r, &len, buff, sizeof(buff));
    ck_assert (strlen (ndata) + 1 == len);
    ck_assert (strlen (ndata) == strlen (data));
    ck_assert (strcmp (ndata, data) == 0);
  }
  msleep (200);
  if (pid == 0) {
    sockClose (c);
    exit (0);
  } else {
    sockClose (r);
  }
  check_waitpid_and_exit (pid);
  sockClose (l);
}
END_TEST

START_TEST(sock_write_read_buff_fail)
{
  sockinfo_t    *si;
  int           rc;
  int           err;
  pid_t         pid;
  int           c;
  int           l;
  int           r;
  char          *data = { "aaaabbbbccccddddeeeeffff" };
  char          buff [10];
  char          *ndata;
  size_t        len;
  char          buff2 [80];

  si = NULL;
  pid = check_fork ();

  if (pid == 0) {
    c = sockConnectWait (32708, 1000);
    ck_assert (c >= 0);
    ck_assert (l != c);
    rc = sockWriteStr (c, data, strlen (data));
    ck_assert (rc == 0);
  } else {
    l = sockServer (32708, &err);
    ck_assert (l >= 0);
    si = sockAddPoll (si, l);
    rc = sockPoll (si);
    while (rc == 0) {
      msleep (20);
      rc = sockPoll (si);
    }
    ck_assert (rc == l);
    r = sockAccept (l, &err);
    ck_assert (r >= 0);
    ck_assert (l != r);

    ndata = sockReadBuff (r, &len, buff, sizeof(buff));
    ck_assert (ndata == NULL);
    ck_assert (len == 0);

    ndata = sockReadBuff (r, &len, buff2, sizeof(buff2));
    ck_assert (ndata == buff2);
    ck_assert (len == 0);
  }
  msleep (200);
  if (pid == 0) {
    sockClose (c);
    exit (0);
  } else {
    sockClose (r);
  }
  check_waitpid_and_exit (pid);
  sockClose (l);
}
END_TEST

START_TEST(sock_write_poll_read)
{
  sockinfo_t    *si;
  int           rc;
  int           err;
  pid_t         pid;
  int           c;
  int           l;
  int           r;
  char          *data = { "aaaabbbbccccddddeeeeffff" };
  char          datab [4096];
  char          *ndata;
  size_t        len;

  si = NULL;
  pid = check_fork ();
  memset (datab, 'a', 4096);

  if (pid == 0) {
    c = sockConnectWait (32709, 1000);
    ck_assert (c >= 0);
    ck_assert (l != c);
    rc = sockWriteStr (c, data, strlen (data));
    ck_assert (rc == 0);
    rc = sockWriteBinary (c, datab, 4096);
    ck_assert (rc == 0);
  } else {
    l = sockServer (32709, &err);
    ck_assert (l >= 0);
    si = sockAddPoll (si, l);
    rc = sockPoll (si);
    while (rc == 0) {
      msleep (20);
      rc = sockPoll (si);
    }
    ck_assert (rc == l);
    r = sockAccept (l, &err);
    ck_assert (r >= 0);
    ck_assert (l != r);
    si = sockAddPoll (si, r);

    rc = sockPoll (si);
    while (rc == 0) {
      msleep (20);
      rc = sockPoll (si);
    }
    ck_assert (rc == r);
    ndata = sockRead (r, &len);
    ck_assert (strlen (ndata) + 1 == len);
    ck_assert (strlen (ndata) == strlen (data));
    ck_assert (strcmp (ndata, data) == 0);
    free (ndata);

    ndata = sockRead (r, &len);
    ck_assert (len == 4096);
    ck_assert (memcmp (ndata, datab, 4096) == 0);
    free (ndata);
  }
  msleep (200);
  if (pid == 0) {
    sockClose (c);
    exit (0);
  } else {
    sockClose (r);
  }
  check_waitpid_and_exit (pid);
  sockClose (l);
}
END_TEST


Suite *
sock_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("Sock Suite");
  tc = tcase_create ("Sock");
  tcase_add_test (tc, sock_server_create);
  tcase_add_test (tc, sock_server_create_poll);
  tcase_add_test (tc, sock_server_poll);
  tcase_add_test (tc, sock_connect_accept);
  tcase_add_test (tc, sock_poll_connect_accept);
  tcase_add_test (tc, sock_write);
  tcase_add_test (tc, sock_write_read);
  tcase_add_test (tc, sock_write_read_buff);
  tcase_add_test (tc, sock_write_read_buff_fail);
  tcase_add_test (tc, sock_write_poll_read);
  /* if the server sockets already exist,
   * the server socket creation may take a long time
   */
  tcase_set_timeout (tc, 30.0);
  suite_add_tcase (s, tc);
  return s;
}

