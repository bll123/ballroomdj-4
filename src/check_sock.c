#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <check.h>
#include <sys/types.h>
#include <sys/stat.h>
#if _hdr_unistd
# include <unistd.h>
#endif
#if _sys_select
# include <sys/select.h>
#endif

#include "sock.h"
#include "check_bdj.h"
#include "tmutil.h"

START_TEST(sock_server_create)
{
  int         err;

  Sock_t s = sockServer (32700, &err);
  ck_assert (s >= 0);
  sockClose (s);
}
END_TEST

START_TEST(sock_server_create_check)
{
  sockinfo_t    *si;
  int         err;

  si = NULL;
  Sock_t s = sockServer (32701, &err);
  ck_assert (s >= 0);
  si = sockAddCheck (si, s);
  ck_assert (si->count == 1);
  ck_assert (si->socklist[0] == s);
  sockRemoveCheck (si, s);
  sockFreeCheck (si);
  sockClose (s);
}
END_TEST

START_TEST(sock_server_check)
{
  sockinfo_t    *si;
  int           rc;
  int         err;

  si = NULL;
  Sock_t s = sockServer (32702, &err);
  ck_assert (s >= 0);
  si = sockAddCheck (si, s);
  rc = sockCheck (si);
  ck_assert (rc == 0);
  sockRemoveCheck (si, s);
  sockFreeCheck (si);
  sockClose (s);
}
END_TEST


START_TEST(sock_connect_accept)
{
  int           err;
  pid_t         pid;
  Sock_t        c = -1;
  Sock_t        r = -1;
  Sock_t        l = -1;

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
  sockClose (l);
  check_waitpid_and_exit (pid);
}
END_TEST

START_TEST(sock_check_connect_accept)
{
  sockinfo_t    *si;
  int           rc;
  int           err;
  pid_t         pid;
  Sock_t        c = -1;
  Sock_t        l = -1;
  Sock_t        r = -1;

  si = NULL;
  pid = check_fork ();

  if (pid == 0) {
    c = sockConnectWait (32704, 1000);
    ck_assert (c >= 0);
  } else {
    l = sockServer (32704, &err);
    ck_assert (l >= 0);
    si = sockAddCheck (si, l);
    rc = sockCheck (si);
    while (rc == 0) {
      msleep (20);
      rc = sockCheck (si);
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
  sockRemoveCheck (si, l);
  sockFreeCheck (si);
  sockClose (l);
  check_waitpid_and_exit (pid);
}
END_TEST

START_TEST(sock_write)
{
  sockinfo_t    *si;
  int           rc;
  int           err;
  pid_t         pid;
  Sock_t        c = -1;
  Sock_t        l = -1;
  Sock_t        r = -1;
  char          *data = { "aaaabbbbccccddddeeeeffff" };
  char          datab [4096];

  si = NULL;
  pid = check_fork ();
  memset (datab, 'a', 4096);

  if (pid == 0) {
    c = sockConnectWait (32705, 1000);
    ck_assert (c >= 0);
    rc = sockWriteStr (c, data, strlen (data));
    ck_assert (rc == 0);
    rc = sockWriteBinary (c, datab, 4096);
    ck_assert (rc == 0);
  } else {
    l = sockServer (32705, &err);
    ck_assert (l >= 0);
    si = sockAddCheck (si, l);
    rc = sockCheck (si);
    while (rc == 0) {
      msleep (20);
      rc = sockCheck (si);
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
  sockRemoveCheck (si, l);
  sockFreeCheck (si);
  sockClose (l);
  check_waitpid_and_exit (pid);
}
END_TEST

START_TEST(sock_write_read)
{
  sockinfo_t    *si;
  int           rc;
  int           err;
  pid_t         pid;
  Sock_t        c = -1;
  Sock_t        l = -1;
  Sock_t        r = -1;
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
    rc = sockWriteStr (c, data, strlen (data));
    ck_assert (rc == 0);
    rc = sockWriteBinary (c, datab, 4096);
    ck_assert (rc == 0);
  } else {
    l = sockServer (32706, &err);
    ck_assert (l >= 0);
    si = sockAddCheck (si, l);
    rc = sockCheck (si);
    while (rc == 0) {
      msleep (20);
      rc = sockCheck (si);
    }
    ck_assert (rc == l);
    r = sockAccept (l, &err);
    ck_assert (r >= 0);
    ck_assert (l != r);

    ndata = sockRead (r, &len);
    ck_assert (ndata != NULL);
    if (ndata != NULL) {
      ck_assert (strlen (ndata) + 1 == len);
      ck_assert (strlen (ndata) == strlen (data));
      ck_assert (strcmp (ndata, data) == 0);
      free (ndata);
    }

    ndata = sockRead (r, &len);
    ck_assert (ndata != NULL);
    ck_assert (len == 4096);
    if (ndata != NULL) {
      ck_assert (memcmp (ndata, datab, 4096) == 0);
      free (ndata);
    }
  }
  msleep (200);
  if (pid == 0) {
    sockClose (c);
    exit (0);
  } else {
    sockClose (r);
  }
  sockRemoveCheck (si, l);
  sockFreeCheck (si);
  sockClose (l);
  check_waitpid_and_exit (pid);
}
END_TEST

START_TEST(sock_write_read_buff)
{
  sockinfo_t    *si;
  int           rc;
  int           err;
  pid_t         pid;
  Sock_t        c = -1;
  Sock_t        l = -1;
  Sock_t        r = -1;
  char          *data = { "aaaabbbbccccddddeeeeffff" };
  char          buff [80];
  char          *ndata;
  size_t        len;

  si = NULL;
  pid = check_fork ();

  if (pid == 0) {
    c = sockConnectWait (32707, 1000);
    ck_assert (c >= 0);
    rc = sockWriteStr (c, data, strlen (data));
    ck_assert (rc == 0);
  } else {
    l = sockServer (32707, &err);
    ck_assert (l >= 0);
    si = sockAddCheck (si, l);
    rc = sockCheck (si);
    while (rc == 0) {
      msleep (20);
      rc = sockCheck (si);
    }
    ck_assert (rc == l);
    r = sockAccept (l, &err);
    ck_assert (r >= 0);
    ck_assert (l != r);

    ndata = sockReadBuff (r, &len, buff, sizeof(buff));
    ck_assert (ndata != NULL);
    if (ndata != NULL) {
      ck_assert (strlen (ndata) + 1 == len);
      ck_assert (strlen (ndata) == strlen (data));
      ck_assert (strcmp (ndata, data) == 0);
    }
  }
  msleep (200);
  if (pid == 0) {
    sockClose (c);
    exit (0);
  } else {
    sockClose (r);
  }
  sockRemoveCheck (si, l);
  sockFreeCheck (si);
  sockClose (l);
  check_waitpid_and_exit (pid);
}
END_TEST

START_TEST(sock_write_read_buff_fail)
{
  sockinfo_t    *si;
  int           rc;
  int           err;
  pid_t         pid;
  Sock_t        c = -1;
  Sock_t        l = -1;
  Sock_t        r = -1;
  char          *data = { "aaaabbbbccccddddeeeeffff" };
  char          buff [10];
  char          *ndata;
  size_t        len;

  si = NULL;
  pid = check_fork ();

  if (pid == 0) {
    c = sockConnectWait (32708, 1000);
    ck_assert (c >= 0);
    rc = sockWriteStr (c, data, strlen (data));
    ck_assert (rc == 0);
  } else {
    l = sockServer (32708, &err);
    ck_assert (l >= 0);
    si = sockAddCheck (si, l);
    rc = sockCheck (si);
    while (rc == 0) {
      msleep (20);
      rc = sockCheck (si);
    }
    ck_assert (rc == l);
    r = sockAccept (l, &err);
    ck_assert (r >= 0);
    ck_assert (l != r);

    ndata = sockReadBuff (r, &len, buff, sizeof(buff));
    ck_assert (ndata == NULL);
    ck_assert (len == 0);
  }
  msleep (200);
  if (pid == 0) {
    sockClose (c);
    exit (0);
  } else {
    sockClose (r);
  }
  sockRemoveCheck (si, l);
  sockFreeCheck (si);
  sockClose (l);
  check_waitpid_and_exit (pid);
}
END_TEST

START_TEST(sock_write_check_read)
{
  sockinfo_t    *si;
  int           rc;
  int           err;
  pid_t         pid;
  Sock_t        c = -1;
  Sock_t        l = -1;
  Sock_t        r = -1;
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
    rc = sockWriteStr (c, data, strlen (data));
    ck_assert (rc == 0);
    rc = sockWriteBinary (c, datab, 4096);
    ck_assert (rc == 0);
  } else {
    l = sockServer (32709, &err);
    ck_assert (l >= 0);
    si = sockAddCheck (si, l);
    rc = sockCheck (si);
    while (rc == 0) {
      msleep (20);
      rc = sockCheck (si);
    }
    ck_assert (rc == l);
    r = sockAccept (l, &err);
    ck_assert (r >= 0);
    ck_assert (l != r);
    si = sockAddCheck (si, r);

    rc = sockCheck (si);
    while (rc == 0) {
      msleep (20);
      rc = sockCheck (si);
    }
    ck_assert (rc == r);
    ndata = sockRead (r, &len);
    ck_assert (ndata != NULL);
    if (ndata != NULL) {
      ck_assert (strlen (ndata) + 1 == len);
      ck_assert (strlen (ndata) == strlen (data));
      ck_assert (strcmp (ndata, data) == 0);
      free (ndata);
    }

    ndata = sockRead (r, &len);
    ck_assert (ndata != NULL);
    if (ndata != NULL) {
      ck_assert (len == 4096);
      ck_assert (memcmp (ndata, datab, 4096) == 0);
      free (ndata);
    }
  }
  msleep (200);
  if (pid == 0) {
    sockClose (c);
    exit (0);
  } else {
    sockClose (r);
  }
  sockRemoveCheck (si, l);
  sockRemoveCheck (si, r);
  sockFreeCheck (si);
  sockClose (l);
  check_waitpid_and_exit (pid);
}
END_TEST

START_TEST(sock_close)
{
  sockinfo_t    *si;
  int           rc;
  int           err;
  pid_t         pid;
  Sock_t        c = -1;
  Sock_t        l = -1;
  Sock_t        r = -1;
  char          *ndata;
  size_t        len;

  si = NULL;
  pid = check_fork ();

  if (pid == 0) {
    c = sockConnectWait (32710, 1000);
    ck_assert (c >= 0);
    /* close the socket early */
    sockClose (c);
  } else {
    l = sockServer (32710, &err);
    ck_assert (l >= 0);

    si = sockAddCheck (si, l);
    rc = sockCheck (si);
    while (rc == 0) {
      msleep (20);
      rc = sockCheck (si);
    }
    ck_assert (rc == l);
    r = sockAccept (l, &err);
    ck_assert (r >= 0);
    ck_assert (l != r);
    si = sockAddCheck (si, r);

    msleep (20); /* a delay to allow client to close */

    rc = sockCheck (si);
    while (rc == 0) {
      msleep (20);
      rc = sockCheck (si);
    }
    ck_assert (rc == r);
    ndata = sockRead (r, &len);
    ck_assert (ndata == NULL);
    if (ndata != NULL) {
      free (ndata);
    }
  }
  msleep (200);
  if (pid == 0) {
    sockClose (c);
    exit (0);
  } else {
    sockClose (r);
  }
  sockRemoveCheck (si, l);
  sockRemoveCheck (si, r);
  sockFreeCheck (si);
  sockClose (l);
  check_waitpid_and_exit (pid);
}
END_TEST

START_TEST(sock_write_close)
{
  sockinfo_t    *si;
  int           rc;
  int           err;
  pid_t         pid;
  Sock_t        c = -1;
  Sock_t        l = -1;
  Sock_t        r = -1;
  char          *data = { "aaaabbbbccccddddeeeeffff" };
  char          datab [4096];
  char          *ndata;
  size_t        len;

  si = NULL;
  pid = check_fork ();
  memset (datab, 'a', 4096);

  if (pid == 0) {
    c = sockConnectWait (32710, 1000);
    ck_assert (c >= 0);
    rc = sockWriteStr (c, data, strlen (data));
    ck_assert (rc == 0);
    rc = sockWriteBinary (c, datab, 4096);
    ck_assert (rc == 0);
    /* close the socket early */
    sockClose (c);
  } else {
    l = sockServer (32710, &err);
    ck_assert (l >= 0);

    si = sockAddCheck (si, l);
    rc = sockCheck (si);
    while (rc == 0) {
      msleep (20);
      rc = sockCheck (si);
    }
    ck_assert (rc == l);
    r = sockAccept (l, &err);
    ck_assert (r >= 0);
    ck_assert (l != r);
    si = sockAddCheck (si, r);

    msleep (20); /* a delay to allow client to close */

    rc = sockCheck (si);
    while (rc == 0) {
      msleep (20);
      rc = sockCheck (si);
    }
    ck_assert (rc == r);
    ndata = sockRead (r, &len);
    ck_assert (ndata != NULL);
    if (ndata != NULL) {
      free (ndata);
    }

    rc = sockCheck (si);
    while (rc == 0) {
      msleep (20);
      rc = sockCheck (si);
    }
    ck_assert (rc == r);
    ndata = sockRead (r, &len);
    ck_assert (ndata != NULL);
    if (ndata != NULL) {
      free (ndata);
    }
  }
  msleep (200);
  if (pid == 0) {
    sockClose (c);
    exit (0);
  } else {
    sockClose (r);
  }
  sockRemoveCheck (si, l);
  sockRemoveCheck (si, r);
  sockFreeCheck (si);
  sockClose (l);
  check_waitpid_and_exit (pid);
}
END_TEST

START_TEST(sock_server_close)
{
  sockinfo_t    *si;
  int           rc;
  int           err;
  pid_t         pid;
  Sock_t        c = -1;
  Sock_t        l = -1;
  Sock_t        r = -1;
  char          *data = { "aaaabbbbccccddddeeeeffff" };
  char          datab [4096];

  si = NULL;
  pid = check_fork ();
  memset (datab, 'a', 4096);

  if (pid == 0) {
    c = sockConnectWait (32710, 1000);
    ck_assert (c >= 0);
    rc = sockWriteStr (c, data, strlen (data));
    ck_assert (rc < 0);
    rc = sockWriteBinary (c, datab, 4096);
    ck_assert (rc < 0);
  } else {
    l = sockServer (32710, &err);
    ck_assert (l >= 0);

    si = sockAddCheck (si, l);
    rc = sockCheck (si);
    while (rc == 0) {
      msleep (20);
      rc = sockCheck (si);
    }
    ck_assert (rc == l);
    r = sockAccept (l, &err);
    ck_assert (r >= 0);
    ck_assert (l != r);
    sockClose (r);
  }
  if (pid == 0) {
    sockClose (c);
    exit (0);
  } else {
    for (int i = 0; i < 60; ++i) {
      msleep (100);
    }
    sockClose (r);
  }
  sockRemoveCheck (si, l);
  sockFreeCheck (si);
  sockClose (l);
  check_waitpid_and_exit (pid);
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
  tcase_add_test (tc, sock_server_create_check);
  tcase_add_test (tc, sock_server_check);
  tcase_add_test (tc, sock_connect_accept);
  tcase_add_test (tc, sock_check_connect_accept);
  tcase_add_test (tc, sock_write);
  tcase_add_test (tc, sock_write_read);
  tcase_add_test (tc, sock_write_read_buff);
  tcase_add_test (tc, sock_write_read_buff_fail);
  tcase_add_test (tc, sock_write_check_read);
  tcase_add_test (tc, sock_close);
  tcase_add_test (tc, sock_write_close);
  tcase_add_test (tc, sock_server_close);
  tcase_set_timeout (tc, 7.0);
  suite_add_tcase (s, tc);
  return s;
}
