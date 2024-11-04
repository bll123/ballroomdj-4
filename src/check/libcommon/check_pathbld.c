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

#include "bdj4.h"
#include "bdjstring.h"
#include "check_bdj.h"
#include "mdebug.h"
#include "log.h"
#include "osdirutil.h"
#include "pathbld.h"
#include "pathutil.h"
#include "sysvars.h"

typedef struct {
  char    *base;
  char    *extension;
  int     flags;
  bool    usecwd;
  char    *finalpath;
  char    *finalfn;
} ftest_t;

static ftest_t tests [] = {
  { "test", "", PATHBLD_MP_NONE, false, ".", "test" },
  { "", "", PATHBLD_MP_DREL_DATA, false, "data", "" },
  { "", "", PATHBLD_MP_DREL_HTTP, false, "http", "" },
  { "", "", PATHBLD_MP_DREL_TMP, false, "tmp", "" },
  { "", "", PATHBLD_MP_DREL_IMG, false, "img", "" },
  { "", "", PATHBLD_MP_DIR_MAIN, true, "", "" },
  { "", "", PATHBLD_MP_DIR_EXEC, true, "bin", "" },
  { "", "", PATHBLD_MP_DIR_IMG, true, "img", "" },
  { "", "", PATHBLD_MP_DIR_LOCALE, true, "locale", "" },
  { "", "", PATHBLD_MP_DIR_TEMPLATE, true, "templates", "" },
  { "", "", PATHBLD_MP_DIR_INST, true, "install", "" },
  { "", "", PATHBLD_MP_DIR_DATATOP, true, "", "" },
  { "test", "", PATHBLD_MP_DREL_DATA, false, "data", "test" },
  { "test", ".txt", PATHBLD_MP_DREL_DATA, false, "data", "test.txt" },
  { "test", "", PATHBLD_MP_DREL_DATA | PATHBLD_MP_USEIDX, false, "data", "test" },
  { "test", "", PATHBLD_MP_DREL_DATA | PATHBLD_MP_HOSTNAME, false, "data", "test" },
  { "test", "", PATHBLD_MP_DREL_DATA | PATHBLD_MP_HOSTNAME | PATHBLD_MP_USEIDX, false, "data", "test" },
  { "test", ".txt", PATHBLD_MP_DREL_TMP | PATHBLD_MP_USEIDX, false, "tmp", "l00-00-test.txt" },
  { "test", ".txt", PATHBLD_MP_DREL_IMG | PATHBLD_MP_USEIDX, false, "img", "test.txt" },
};
enum {
  TCOUNT = (sizeof(tests)/sizeof (ftest_t))
};

START_TEST(pathbld_chk)
{
  char    tbuff [MAXPATHLEN];
  char    f [MAXPATHLEN];
  char    cwd [MAXPATHLEN];
  char    *p = f;
  char    *end = f + sizeof (f);

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- pathbld_chk");
  mdebugSubTag ("pathbld_chk");

  osGetCurrentDir (cwd, sizeof (cwd));
  pathNormalizePath (cwd, sizeof (cwd));

  for (size_t i = 0; i < TCOUNT; ++i) {
    pathbldMakePath (tbuff, sizeof (tbuff),
        tests [i].base, tests [i].extension, tests [i].flags);
    *f = '\0';
    p = f;
    if (tests [i].flags & PATHBLD_IS_ABSOLUTE) {
      p = stpecpy (p, end, cwd);
      if (*tests [i].finalpath) {
        p = stpecpy (p, end, "/");
      }
    }
    if (*tests [i].finalpath) {
      p = stpecpy (p, end, tests [i].finalpath);
    }
    if ((tests [i].flags & PATHBLD_MP_HOSTNAME) == PATHBLD_MP_HOSTNAME) {
      p = stpecpy (p, end, "/");
      p = stpecpy (p, end, sysvarsGetStr (SV_HOSTNAME));
    }
    if ((tests [i].flags & PATHBLD_MP_USEIDX) == PATHBLD_MP_USEIDX &&
        (tests [i].flags & PATHBLD_MP_DREL_TMP) != PATHBLD_MP_DREL_TMP) {
      p = stpecpy (p, end, "/");
      p = stpecpy (p, end, "profile00");
    }
    if (*tests [i].finalfn) {
      p = stpecpy (p, end, "/");
      p = stpecpy (p, end, tests [i].finalfn);
    }
    ck_assert_str_eq (tbuff, f);
  }
}
END_TEST

Suite *
pathbld_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("pathbld");
  tc = tcase_create ("pathbld");
  tcase_set_tags (tc, "libcommon");
  tcase_add_test (tc, pathbld_chk);
  suite_add_tcase (s, tc);
  return s;
}


#pragma clang diagnostic pop
#pragma GCC diagnostic pop
