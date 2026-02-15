/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
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
#include "pathdisp.h"
#include "pathutil.h"
#include "check_bdj.h"
#include "mdebug.h"
#include "sysvars.h"

START_TEST(path_strippath)
{
  char    to [BDJ4_PATH_MAX];

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- path_strippath");
  mdebugSubTag ("path_strippath");

  stpecpy (to, to + sizeof (to), "/tmp/abc.txt");
  pathStripPath (to, sizeof (to));
  ck_assert_str_eq (to, "/tmp/abc.txt");

  stpecpy (to, to + sizeof (to), "./tmp/abc.txt");
  pathStripPath (to, sizeof (to));
  ck_assert_str_eq (to, "tmp/abc.txt");

  stpecpy (to, to + sizeof (to), "/tmp/../dir/abc.txt");
  pathStripPath (to, sizeof (to));
  ck_assert_str_eq (to, "/dir/abc.txt");

  stpecpy (to, to + sizeof (to), "/tmp/one/../dir/abc.txt");
  pathStripPath (to, sizeof (to));
  ck_assert_str_eq (to, "/tmp/dir/abc.txt");

  stpecpy (to, to + sizeof (to), "/tmp/../abc.txt");
  pathStripPath (to, sizeof (to));
  ck_assert_str_eq (to, "/abc.txt");

  stpecpy (to, to + sizeof (to), "/tmp/./abc.txt");
  pathStripPath (to, sizeof (to));
  ck_assert_str_eq (to, "/tmp/abc.txt");
}
END_TEST

START_TEST(path_realpath)
{
  FILE    *fh;
  char    path [BDJ4_PATH_MAX];
  char    cwd [BDJ4_PATH_MAX];
  char    actual [BDJ4_PATH_MAX];

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- path_realpath");
  mdebugSubTag ("path_realpath");

  osGetCurrentDir (cwd, sizeof (cwd));
  pathNormalizePath (cwd, sizeof (cwd));

  stpecpy (path, path + sizeof (path), "tmp/abc.txt");
  snprintf (actual, sizeof (actual), "%s/%s", cwd, path);
  fh = fileopOpen (path, "w");
  mdextfclose (fh);
  fclose (fh);
  pathRealPath (path, sizeof (path));
  pathNormalizePath (path, sizeof (path));
  ck_assert_str_eq (path, actual);

  stpecpy (path, path + sizeof (path), "tmp/abc-long-name.txt");
  snprintf (actual, sizeof (actual), "%s/%s", cwd, path);
  fh = fileopOpen (path, "w");
  mdextfclose (fh);
  fclose (fh);
  pathRealPath (path, sizeof (path));
  pathNormalizePath (path, sizeof (path));
  if (isWindows ()) {
    ck_assert_str_ne (path, actual);
  } else {
    ck_assert_str_eq (path, actual);
  }
  pathDisplayPath (path, sizeof (path));
  pathLongPath (path, sizeof (path));
  pathNormalizePath (path, sizeof (path));
  ck_assert_str_eq (path, actual);

  stpecpy (path, path + sizeof (path), "tmp/../tmp/abc.txt");
  snprintf (actual, sizeof (actual), "%s/%s", cwd, "tmp/abc.txt");
  pathRealPath (path, sizeof (path));
  pathNormalizePath (path, sizeof (path));
  ck_assert_str_eq (path, actual);

  stpecpy (path, path + sizeof (path), actual);
  pathRealPath (path, sizeof (path));
  pathNormalizePath (path, sizeof (path));
  ck_assert_str_eq (path, actual);

#if _lib_symlink
  (void) ! symlink (path, "tmp/def.txt");
  stpecpy (path, path + sizeof (path), "tmp/def.txt");
  pathRealPath (path, sizeof (path));
  pathNormalizePath (path, sizeof (path));
  ck_assert_str_eq (path, actual);
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
  tcase_add_test (tc, path_strippath);
  tcase_add_test (tc, path_realpath);
  suite_add_tcase (s, tc);
  return s;
}


#pragma clang diagnostic pop
#pragma GCC diagnostic pop
