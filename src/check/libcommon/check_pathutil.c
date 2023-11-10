/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#pragma clang diagnostic push
#pragma GCC diagnostic push
#pragma clang diagnostic ignored "-Wformat-extra-args"
#pragma GCC diagnostic ignored "-Wformat-extra-args"

#include <check.h>

#include "bdj4.h"
#include "bdjstring.h"
#include "log.h"
#include "osdirutil.h"
#include "pathutil.h"
#include "check_bdj.h"
#include "mdebug.h"

typedef struct {
  char    *path;
  size_t  dlen;
  size_t  flen;
  size_t  blen;
  size_t  elen;
} ftest_t;

static ftest_t tests [] = {
  { "profile", 0, 7, 7, 0 },
  { "01 Bolero.mp3", 0, 13, 9, 4 },
  { "/usr/binx", 4, 4, 4, 0 },
  { "/usr", 1, 3, 3, 0 },
  { "/home/bll/stuff.txt", 9, 9, 5, 4 },
  { "/home/bll/stuff/", 9, 5, 5, 0 },
  { "/home/bll/stuff", 9, 5, 5, 0 },
  { "/home/bll/stuff.x", 9, 7, 5, 2 },
  { "/home/bll/x.stuff", 9, 7, 1, 6 },
  { "/home/bll/stuff.x/", 9, 7, 7, 0 },   // trailing slash, is dir
//   1234567890123456789012345678901234567890
  { "/home/bll/s/bdj4/src", 16, 3, 3, 0 },
  { "bdjconfig.txt.g", 0, 15, 13, 2 },
  { "dances.txt.nl", 0, 13, 10, 3 },
//   1234567890123456789012345678901234567890
  { "/usr/share/themes/Adwaita-dark/gtk-3.0", 30, 7, 5, 2 },
//   123456789012345678901234567 1234567890123456789012
  { "/home/bll/s/bdj4/test-music/001-argentinetango.mp3", 27, 22, 18, 4 },
  { "/", 1, 1, 1, 0 },
};
enum {
  TCOUNT = (sizeof(tests)/sizeof (ftest_t))
};

START_TEST(pathinfo_chk)
{
  pathinfo_t    *pi;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- pathinfo_chk");
  mdebugSubTag ("pathinfo_chk");

  for (int64_t i = 0; i < TCOUNT; ++i) {
    pi = pathInfo (tests[i].path);
    if (pi->dlen > 0) {
      ck_assert_msg (pi->dirname != NULL,
          "dirname: %s: i:%" PRId64 " have: %" PRId64 " want: %" PRId64,
          "dlen", i, (int64_t) pi->dlen, (int64_t) tests[i].dlen);
    }
    if (pi->flen > 0) {
      ck_assert_msg (pi->filename != NULL,
          "filename: %s: i:%" PRId64 " have: %" PRId64 " want: %" PRId64,
          "flen", i, (int64_t) pi->flen, (int64_t) tests[i].flen);
    }
    if (pi->blen > 0) {
      ck_assert_msg (pi->basename != NULL,
          "basename: %s: i:%" PRId64 " have: %" PRId64 " want: %" PRId64,
          "blen", i, (int64_t) pi->blen, (int64_t) tests[i].blen);
    }
    if (pi->elen > 0) {
      ck_assert_msg (pi->extension != NULL,
          "extension: %s: i:%" PRId64 " have: %" PRId64 " want: %" PRId64,
          "elen", i, (int64_t) pi->elen, (int64_t) tests[i].elen);
    }
    ck_assert_msg (pi->dlen == tests[i].dlen,
        "dlen: %s: i:%" PRId64 " have: %" PRId64 " want: %" PRId64,
        "dlen", i, (int64_t) pi->dlen, (int64_t) tests[i].dlen);
    ck_assert_msg (pi->flen == tests[i].flen,
        "flen: %s: i:%" PRId64 " have: %" PRId64 " want: %" PRId64,
        "flen", i, (int64_t) pi->flen, (int64_t) tests[i].flen);
    ck_assert_msg (pi->blen == tests[i].blen,
        "blen: %s: i:%" PRId64 " have: %" PRId64 " want: %" PRId64,
        "blen", i, (int64_t) pi->blen, (int64_t) tests[i].blen);
    ck_assert_msg (pi->elen == tests[i].elen,
        "elen: %s: i:%" PRId64 " have: %" PRId64 " want: %" PRId64,
        "elen", i, (int64_t) pi->elen, (int64_t) tests[i].elen);

    /* the dirname pointer always points to the beginning */
    ck_assert_ptr_eq (pi->dirname, tests [i].path);

    /* check against self */
    ck_assert_int_eq (pathInfoExtCheck (pi, pi->extension), 1);
    ck_assert_int_eq (pathInfoExtCheck (pi, ".junk"), 0);

    pathInfoFree (pi);
  }
}
END_TEST

START_TEST(path_normpath)
{
  char    to [MAXPATHLEN];

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- path_normpath");
  mdebugSubTag ("path_normpath");

  strlcpy (to, "/tmp/abc.txt", sizeof (to));
  pathNormalizePath (to, sizeof (to));
  ck_assert_str_eq (to, "/tmp/abc.txt");

  strlcpy (to, "\\tmp\\abc.txt", sizeof (to));
  pathNormalizePath (to, sizeof (to));
  ck_assert_str_eq (to, "/tmp/abc.txt");

  strlcpy (to, "C:/tmp/abc.txt", sizeof (to));
  pathNormalizePath (to, sizeof (to));
  ck_assert_str_eq (to, "C:/tmp/abc.txt");

  strlcpy (to, "C:\\tmp\\abc.txt", sizeof (to));
  pathNormalizePath (to, sizeof (to));
  ck_assert_str_eq (to, "C:/tmp/abc.txt");
}
END_TEST

START_TEST(path_strippath)
{
  char    to [MAXPATHLEN];

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- path_normpath");
  mdebugSubTag ("path_normpath");

  strlcpy (to, "/tmp/abc.txt", sizeof (to));
  pathStripPath (to, sizeof (to));
  ck_assert_str_eq (to, "/tmp/abc.txt");

  strlcpy (to, "./tmp/abc.txt", sizeof (to));
  pathStripPath (to, sizeof (to));
  ck_assert_str_eq (to, "tmp/abc.txt");

  strlcpy (to, "/tmp/../dir/abc.txt", sizeof (to));
  pathStripPath (to, sizeof (to));
  ck_assert_str_eq (to, "/dir/abc.txt");

  strlcpy (to, "/tmp/one/../dir/abc.txt", sizeof (to));
  pathStripPath (to, sizeof (to));
  ck_assert_str_eq (to, "/tmp/dir/abc.txt");

  strlcpy (to, "/tmp/../abc.txt", sizeof (to));
  pathStripPath (to, sizeof (to));
  ck_assert_str_eq (to, "/abc.txt");

  strlcpy (to, "/tmp/./abc.txt", sizeof (to));
  pathStripPath (to, sizeof (to));
  ck_assert_str_eq (to, "/tmp/abc.txt");
}
END_TEST

START_TEST(path_realpath)
{
  FILE    *fh;
  char    to [MAXPATHLEN];
  char    from [MAXPATHLEN];
  char    cwd [MAXPATHLEN];
  char    actual [MAXPATHLEN];

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- path_realpath");
  mdebugSubTag ("path_realpath");

  osGetCurrentDir (cwd, sizeof (cwd));
  pathNormalizePath (cwd, sizeof (cwd));

  strlcpy (from, "tmp/abc.txt", sizeof (from));
  fh = fopen (from, "w");
  fclose (fh);
  snprintf (actual, sizeof (actual), "%s/%s", cwd, from);

  pathRealPath (to, from, sizeof (to));
  pathNormalizePath (to, sizeof (to));
  ck_assert_str_eq (to, actual);

  strlcpy (from, "tmp/../tmp/abc.txt", sizeof (from));
  pathRealPath (to, from, sizeof (to));
  pathNormalizePath (to, sizeof (to));
  ck_assert_str_eq (to, actual);

  strlcpy (from, actual, sizeof (from));
  pathRealPath (to, from, sizeof (to));
  pathNormalizePath (to, sizeof (to));
  ck_assert_str_eq (to, actual);

#if _lib_symlink
  (void) ! symlink (from, "tmp/def.txt");
  pathRealPath (to, "tmp/def.txt", sizeof (to));
  pathNormalizePath (to, sizeof (to));
  ck_assert_str_eq (to, actual);
#endif
  unlink ("tmp/def.txt");
  unlink ("tmp/abc.txt");
}
END_TEST

Suite *
pathutil_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("pathutil");
  tc = tcase_create ("pathutil");
  tcase_set_tags (tc, "libcommon");
  tcase_add_test (tc, pathinfo_chk);
  tcase_add_test (tc, path_normpath);
  tcase_add_test (tc, path_strippath);
  tcase_add_test (tc, path_realpath);
  suite_add_tcase (s, tc);
  return s;
}


#pragma clang diagnostic pop
#pragma GCC diagnostic pop
