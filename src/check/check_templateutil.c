#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#pragma clang diagnostic push
#pragma gcc diagnostic push
#pragma clang diagnostic ignored "-Wformat-extra-args"
#pragma gcc diagnostic ignored "-Wformat-extra-args"

#include <check.h>

#include "bdj4.h"
#include "check_bdj.h"
#include "filedata.h"
#include "fileop.h"
#include "log.h"
#include "pathbld.h"
#include "sysvars.h"
#include "templateutil.h"
#include "tmutil.h"

START_TEST(templateutil_file_copy)
{
  size_t  sza;
  size_t  szb;
  char    from [MAXPATHLEN];
  char    to [MAXPATHLEN];

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- templateutil_file_copy");

  pathbldMakePath (from, sizeof (from), "dances",
      BDJ4_CONFIG_EXT, PATHBLD_MP_TEMPLATEDIR);
  pathbldMakePath (to, sizeof (to), "dances",
      BDJ4_CONFIG_EXT, PATHBLD_MP_DATA);

  fileopDelete (to);
  ck_assert_int_eq (fileopFileExists (to), 0);
  templateFileCopy ("dances.txt", "dances.txt");
  ck_assert_int_eq (fileopFileExists (to), 1);
  sza = fileopSize (from);
  szb = fileopSize (to);
  ck_assert_int_eq (sza, szb);
}
END_TEST

START_TEST(templateutil_image_copy)
{
  char    to [MAXPATHLEN];
  char    *data;
  char    *p;
  size_t  rlen;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- templateutil_image_copy");

  pathbldMakePath (to, sizeof (to), "button_play",
      BDJ4_IMG_SVG_EXT, PATHBLD_MP_IMGDIR | PATHBLD_MP_USEIDX);

  fileopDelete (to);
  ck_assert_int_eq (fileopFileExists (to), 0);

  templateImageCopy (NULL);
  ck_assert_int_eq (fileopFileExists (to), 1);
  data = filedataReadAll (to, &rlen);
  p = strstr (data, "#0000ff");
  ck_assert_ptr_null (p);
  p = strstr (data, "#ffa600");
  ck_assert_ptr_nonnull (p);
  free (data);

  templateImageCopy ("#0000ff");
  ck_assert_int_eq (fileopFileExists (to), 1);
  data = filedataReadAll (to, &rlen);
  p = strstr (data, "#0000ff");
  ck_assert_ptr_nonnull (p);
  p = strstr (data, "#ffa600");
  ck_assert_ptr_null (p);
  free (data);

  /* reset the images */
  templateImageCopy (NULL);
}
END_TEST

START_TEST(templateutil_http_copy)
{
  char    to [MAXPATHLEN];

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- templateutil_http_copy");

  pathbldMakePath (to, sizeof (to), "mobilemq",
      ".html", PATHBLD_MP_HTTPDIR);

  fileopDelete (to);
  ck_assert_int_eq (fileopFileExists (to), 0);
  templateHttpCopy ("mobilemq.html", "mobilemq.html");
  ck_assert_int_eq (fileopFileExists (to), 1);
}
END_TEST

START_TEST(templateutil_dispset_copy)
{
  char    to [MAXPATHLEN];

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- templateutil_dispset_copy");

  pathbldMakePath (to, sizeof (to), "ds-musicq",
      BDJ4_CONFIG_EXT, PATHBLD_MP_DATA | PATHBLD_MP_USEIDX);

  fileopDelete (to);
  ck_assert_int_eq (fileopFileExists (to), 0);
  templateDisplaySettingsCopy ();
  if (isWindows ()) {
    mssleep (200);
  }
  ck_assert_int_eq (fileopFileExists (to), 1);
}
END_TEST


Suite *
templateutil_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("templateutil");
  tc = tcase_create ("templateutil");
  tcase_set_tags (tc, "libbdj4");
  tcase_add_test (tc, templateutil_file_copy);
  tcase_add_test (tc, templateutil_image_copy);
  tcase_add_test (tc, templateutil_http_copy);
  tcase_add_test (tc, templateutil_dispset_copy);
  suite_add_tcase (s, tc);
  return s;
}

