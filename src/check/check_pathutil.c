#include "config.h"

#pragma clang diagnostic ignored "-Wformat-extra-args"
#pragma gcc diagnostic ignored "-Wformat-extra-args"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <check.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "bdj4.h"
#include "bdjstring.h"
#include "pathutil.h"
#include "check_bdj.h"

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
  { "/home/bll/stuff.x/", 9, 7, 7, 0 },
  { "bdjconfig.txt.g", 0, 15, 13, 2 },
  { "dances.txt.nl", 0, 13, 10, 3 },
//   1234567890123456789012345678901234567890
  { "/usr/share/themes/Adwaita-dark/gtk-3.0", 30, 7, 5, 2 },
  { "/", 1, 1, 1, 0 },
};
enum {
  TCOUNT = (sizeof(tests)/sizeof (ftest_t))
};

START_TEST(pathinfo_chk)
{
  pathinfo_t    *pi;

  for (size_t i = 0; i < TCOUNT; ++i) {
    pi = pathInfo (tests[i].path);
    if (pi->dlen > 0) {
      ck_assert_msg (pi->dirname != NULL,
          "dirname: %s: i:%zd have: %zd want: %zd", "dlen", i, pi->dlen, tests[i].dlen);
    }
    if (pi->flen > 0) {
      ck_assert_msg (pi->filename != NULL,
          "filename: %s: i:%zd have: %zd want: %zd", "flen", i, pi->flen, tests[i].flen);
    }
    if (pi->blen > 0) {
      ck_assert_msg (pi->basename != NULL,
          "basename: %s: i:%zd have: %zd want: %zd", "blen", i, pi->blen, tests[i].blen);
    }
    if (pi->elen > 0) {
      ck_assert_msg (pi->extension != NULL,
          "extension: %s: i:%zd have: %zd want: %zd", "elen", i, pi->elen, tests[i].elen);
    }
    ck_assert_msg (pi->dlen == tests[i].dlen,
        "dlen: %s: i:%zd have: %zd want: %zd", "dlen", i, pi->dlen, tests[i].dlen);
    ck_assert_msg (pi->flen == tests[i].flen,
        "flen: %s: i:%zd have: %zd want: %zd", "flen", i, pi->flen, tests[i].flen);
    ck_assert_msg (pi->blen == tests[i].blen,
        "blen: %s: i:%zd have: %zd want: %zd", "blen", i, pi->blen, tests[i].blen);
    ck_assert_msg (pi->elen == tests[i].elen,
        "elen: %s: i:%zd have: %zd want: %zd", "elen", i, pi->elen, tests[i].elen);
    pathInfoFree (pi);
  }
}
END_TEST

START_TEST(path_winpath)
{
  char    to [MAXPATHLEN];

  strlcpy (to, "/tmp/abc.txt", sizeof (to));
  pathWinPath (to, sizeof (to));
  ck_assert_str_eq (to, "\\tmp\\abc.txt");
}
END_TEST

Suite *
pathutil_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("Path Utils Suite");
  tc = tcase_create ("Path Utils");
  tcase_add_test (tc, pathinfo_chk);
  tcase_add_test (tc, path_winpath);
  suite_add_tcase (s, tc);
  return s;
}

