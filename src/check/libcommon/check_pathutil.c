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
#include "fileop.h"
#include "log.h"
#include "osdirutil.h"
#include "pathutil.h"
#include "check_bdj.h"
#include "mdebug.h"

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
  fh = fileopOpen (from, "w");
  mdextfclose (fh);
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
  tcase_add_test (tc, path_normpath);
  tcase_add_test (tc, path_strippath);
  tcase_add_test (tc, path_realpath);
  suite_add_tcase (s, tc);
  return s;
}


#pragma clang diagnostic pop
#pragma GCC diagnostic pop
