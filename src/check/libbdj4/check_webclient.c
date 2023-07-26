/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#pragma clang diagnostic push
#pragma GCC diagnostic push
#pragma clang diagnostic ignored "-Wformat-extra-args"
#pragma GCC diagnostic ignored "-Wformat-extra-args"

#include <check.h>

#include "check_bdj.h"
#include "mdebug.h"
#include "fileop.h"
#include "mdebug.h"
#include "webclient.h"
#include "log.h"
#include "sysvars.h"

static void checkWebclientCB (void *udata, char *resp, size_t len);

typedef struct {
  char    *resp;
  size_t  len;
} checkwc_t;

#define DLFILE "tmp/wc-dl.txt"
#define UPFILE "tmp/wc-up.txt"


START_TEST(webclient_alloc)
{
  webclient_t    *wc;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- webclient_alloc");
  mdebugSubTag ("webclient_alloc");

  wc = webclientAlloc (NULL, NULL);
  webclientClose (wc);
}
END_TEST

START_TEST(webclient_get)
{
  webclient_t   *wc;
  char          tbuff [200];
  checkwc_t     r = { NULL, 0 };

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- webclient_get");
  mdebugSubTag ("webclient_get");

  wc = webclientAlloc (&r, checkWebclientCB);
  snprintf (tbuff, sizeof (tbuff), "%s/%s",
      sysvarsGetStr (SV_HOST_WEB), sysvarsGetStr (SV_WEB_VERSION_FILE));
  webclientGet (wc, tbuff);
  ck_assert_ptr_nonnull (r.resp);
  ck_assert_int_ge (r.len, 5);
  webclientClose (wc);
}
END_TEST

START_TEST(webclient_dl)
{
  webclient_t   *wc;
  char          tbuff [200];
  checkwc_t     r = { NULL, 0 };
  FILE          *fh;
  bool          exists;
  size_t        sz;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- webclient_dl");
  mdebugSubTag ("webclient_dl");

  wc = webclientAlloc (&r, checkWebclientCB);
  snprintf (tbuff, sizeof (tbuff), "%s/%s",
      sysvarsGetStr (SV_HOST_WEB), sysvarsGetStr (SV_WEB_VERSION_FILE));
  webclientGet (wc, tbuff);
  ck_assert_ptr_nonnull (r.resp);
  ck_assert_int_ge (r.len, 5);
  webclientDownload (wc, tbuff, DLFILE);
  exists = fileopFileExists (DLFILE);
  sz = fileopSize (DLFILE);
  ck_assert_int_eq (exists, 1);
  fh = fopen (DLFILE, "r");
  (void) ! fgets (tbuff, sizeof (tbuff), fh);
  ck_assert_str_eq (r.resp, tbuff);
  ck_assert_int_eq (r.len, sz);
  fclose (fh);
  webclientClose (wc);
  unlink (DLFILE);
}
END_TEST


START_TEST(webclient_post)
{
  webclient_t   *wc;
  char          tbuff [200];
  char          query [200];
  checkwc_t     r = { NULL, 0 };

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- webclient_post");
  mdebugSubTag ("webclient_post");

  wc = webclientAlloc (&r, checkWebclientCB);
  snprintf (tbuff, sizeof (tbuff), "%s/%s",
      sysvarsGetStr (SV_HOST_SUPPORTMSG), "bdj4tester.php");
  snprintf (query, sizeof (query), "key=8634556&testdata=abc123");
  webclientPost (wc, tbuff, query);
  ck_assert_ptr_nonnull (r.resp);
  ck_assert_int_ge (r.len, 9);
  ck_assert_str_eq (r.resp, "OK abc123");
  webclientClose (wc);
}
END_TEST

START_TEST(webclient_localip)
{
  char          *ip = NULL;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- webclient_localip");
  mdebugSubTag ("webclient_localip");

  ip = webclientGetLocalIP ();
  ck_assert_ptr_nonnull (ip);
  ck_assert_int_ge (strlen (ip), 9);
  dataFree (ip);
}
END_TEST

START_TEST(webclient_upload_plain)
{
  webclient_t   *wc;
  char          tbuff [200];
  const char    *query [10];
  int           qc = 0;
  checkwc_t     r = { NULL, 0 };
  FILE          *fh;
  bool          exists = false;
  size_t        sz;
  const char    *tdata = "def456";

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- webclient_upload_plain");
  mdebugSubTag ("webclient_upload_plain");

  fh = fopen (UPFILE, "w");
  if (fh != NULL) {
    fputs (tdata, fh);
    fclose (fh);
  }

  wc = webclientAlloc (&r, checkWebclientCB);
  snprintf (tbuff, sizeof (tbuff), "%s/%s",
      sysvarsGetStr (SV_HOST_SUPPORTMSG), "bdj4tester.php");
  query [qc++] = "key";
  query [qc++] = "8634556";
  query [qc++] = "origfn";
  query [qc++] = UPFILE;
  query [qc++] = NULL;
  webclientUploadFile (wc, tbuff, query, UPFILE);

  ck_assert_ptr_nonnull (r.resp);
  ck_assert_int_ge (r.len, 2);
  ck_assert_str_eq (r.resp, "OK");


  snprintf (tbuff, sizeof (tbuff), "%s/tmptest/%s",
      sysvarsGetStr (SV_HOST_SUPPORTMSG), UPFILE);
  webclientDownload (wc, tbuff, DLFILE);
  exists = fileopFileExists (DLFILE);
  sz = fileopSize (DLFILE);
  ck_assert_int_eq (exists, 1);
  fh = fopen (DLFILE, "r");
  (void) ! fgets (tbuff, sizeof (tbuff), fh);
  ck_assert_str_eq (tbuff, tdata);
  ck_assert_int_eq (sz, 6);
  fclose (fh);

  webclientClose (wc);
}
END_TEST

START_TEST(webclient_upload_gzip)
{
  webclient_t   *wc;
  char          tbuff [200];
  const char    *query [10];
  int           qc = 0;
  checkwc_t     r = { NULL, 0 };
  FILE          *fh;
  bool          exists = false;
  size_t        sz;
  const char    *tdata = "ghi789";

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- webclient_upload_gzip");
  mdebugSubTag ("webclient_upload_gzip");

  fh = fopen (UPFILE, "w");
  if (fh != NULL) {
    fputs (tdata, fh);
    fclose (fh);
  }

  snprintf (tbuff, sizeof (tbuff), "%s.gz.b64", UPFILE);
  webclientCompressFile (UPFILE, tbuff);

  wc = webclientAlloc (&r, checkWebclientCB);
  snprintf (tbuff, sizeof (tbuff), "%s/%s",
      sysvarsGetStr (SV_HOST_SUPPORTMSG), "bdj4tester.php");
  query [qc++] = "key";
  query [qc++] = "8634556";
  query [qc++] = "origfn";
  query [qc++] = UPFILE;
  query [qc++] = NULL;
  webclientUploadFile (wc, tbuff, query, UPFILE);

  ck_assert_ptr_nonnull (r.resp);
  ck_assert_int_ge (r.len, 2);
  ck_assert_str_eq (r.resp, "OK");

  snprintf (tbuff, sizeof (tbuff), "%s/tmptest/%s",
      sysvarsGetStr (SV_HOST_SUPPORTMSG), UPFILE);
  webclientDownload (wc, tbuff, DLFILE);
  exists = fileopFileExists (DLFILE);
  sz = fileopSize (DLFILE);
  ck_assert_int_eq (exists, 1);
  fh = fopen (DLFILE, "r");
  (void) ! fgets (tbuff, sizeof (tbuff), fh);
  ck_assert_str_eq (tbuff, tdata);
  ck_assert_int_eq (sz, 6);
  fclose (fh);

  webclientClose (wc);
  unlink (DLFILE);
  unlink (UPFILE);
  snprintf (tbuff, sizeof (tbuff), "%s.gz.b64", UPFILE);
  unlink (tbuff);
}
END_TEST


Suite *
webclient_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("webclient");
  tc = tcase_create ("webclient");
  tcase_set_tags (tc, "libbdj4");
  tcase_add_test (tc, webclient_alloc);
  tcase_add_test (tc, webclient_get);
  tcase_add_test (tc, webclient_dl);
  tcase_add_test (tc, webclient_post);
  tcase_add_test (tc, webclient_localip);
  tcase_add_test (tc, webclient_upload_plain);
  tcase_add_test (tc, webclient_upload_gzip);
  suite_add_tcase (s, tc);

  return s;
}

static void
checkWebclientCB (void *udata, char *resp, size_t len)
{
  checkwc_t *r = udata;
  r->resp = resp;
  r->len = len;
}

#pragma clang diagnostic pop
#pragma GCC diagnostic pop