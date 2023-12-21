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

#include "bdjstring.h"
#include "check_bdj.h"
#include "mdebug.h"
#include "dirop.h"
#include "fileop.h"
#include "log.h"
#include "mdebug.h"
#include "osdir.h"

char *osdirtestdata [] = {
  "abc.txt",
  "def.txt",
  "ghi.txt",
  "ÄÑÄÑ_ÜÄÑÖ.txt",
  "夕陽伴我歸_내가제일잘나가.txt",
  "夕陽伴我歸_ははは.txt",
  "夕陽伴我歸_夕陽伴我歸.txt",
  "夕陽伴我歸_Ne_Русский_Шторм.txt",
};
enum {
  tdatasz = sizeof (osdirtestdata) / sizeof (char *),
};

#define OSDIRTESTDIR "tmp/osdir"

START_TEST(osdir_chk)
{
  char        tbuff [MAXPATHLEN];
  int         i;
  int         count = 0;
  bool        found [tdatasz];
  dirhandle_t *dh = NULL;
  char        *fn;

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- osdir_chk");
  mdebugSubTag ("osdir_chk");

  diropMakeDir (OSDIRTESTDIR);

  for (i = 0; i < tdatasz; ++i) {
    FILE    *fh;

    snprintf (tbuff, sizeof (tbuff), "%s/%s", OSDIRTESTDIR, osdirtestdata [i]);
    fh = fileopOpen (tbuff, "w");
    if (fh != NULL) {
      mdextfclose (fh);
      fclose (fh);
    }
    found [i] = false;
  }

  dh = osDirOpen (OSDIRTESTDIR);
  ck_assert_ptr_nonnull (dh);
  while ((fn = osDirIterate (dh)) != NULL) {
    for (i = 0; i < tdatasz; ++i) {
      if (strcmp (fn, osdirtestdata [i]) == 0) {
        found [i] = true;
        break;
      }
    }
    mdfree (fn);
  }
  osDirClose (dh);

  count = 0;
  for (i = 0; i < tdatasz; ++i) {
    if (found [i]) {
      ++count;
    }
  }
  ck_assert_int_eq (count, tdatasz);

  for (i = 0; i < tdatasz; ++i) {
    snprintf (tbuff, sizeof (tbuff), "%s/%s", OSDIRTESTDIR, osdirtestdata [i]);
    fileopDelete (tbuff);
  }
  diropDeleteDir (OSDIRTESTDIR, DIROP_ALL);
}
END_TEST

Suite *
osdir_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("osdir");
  tc = tcase_create ("osdir");
  tcase_set_tags (tc, "libcommon");
  tcase_add_test (tc, osdir_chk);
  suite_add_tcase (s, tc);

  return s;
}

#pragma clang diagnostic pop
#pragma GCC diagnostic pop
