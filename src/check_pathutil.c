#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <check.h>
#include <sys/types.h>
#include <time.h>
#if _hdr_unistd
# include <unistd.h>
#endif

#include "pathutil.h"
#include "check_bdj.h"
#include "portability.h"

typedef struct {
  char    *path;
  size_t  dlen;
  size_t  flen;
  size_t  blen;
  size_t  elen;
} ftest_t;

static ftest_t tests [] = {
  { "/usr/binx", 4, 4, 4, 0 },
  { "/usr", 1, 3, 3, 0 },
  { "/home/bll/stuff.txt", 9, 9, 5, 3 },
  { "/home/bll/stuff/", 9, 5, 5, 0 },
  { "/home/bll/stuff", 9, 5, 5, 0 },
  { "/home/bll/stuff.x", 9, 7, 5, 1 },
  { "/home/bll/x.stuff", 9, 7, 1, 5 },
  { "/home/bll/stuff.x/", 9, 7, 7, 0 },
  { "/", 1, 1, 1, 0 },
};
#define TCOUNT (sizeof(tests)/sizeof (ftest_t))

START_TEST(pathinfo_chk)
{
  pathinfo_t    *pi;

  for (size_t i = 0; i < TCOUNT; ++i) {
    pi = pathInfo (tests[i].path);
    ck_assert_msg (pi->dlen == tests[i].dlen,
        "%s: i:%zd %zd/%zd", "dlem", i, pi->dlen, tests[i].dlen);
    ck_assert_msg (pi->blen == tests[i].blen,
        "%s: i:%zd %zd/%zd", "blen", i, pi->blen, tests[i].blen);
    ck_assert_msg (pi->flen == tests[i].flen,
        "%s: i:%zd %zd/%zd", "flem", i, pi->flen, tests[i].flen);
    ck_assert_msg (pi->elen == tests[i].elen,
        "%s: i:%zd %zd/%zd", "elem", i, pi->elen, tests[i].elen);
    pathInfoFree (pi);
  }
}
END_TEST

START_TEST(path_winpath)
{
  char    *from;
  char    to [MAXPATHLEN];

  from = "/tmp/abc.txt";
  pathToWinPath (from, to, MAXPATHLEN);
  ck_assert_str_eq (to, "\\tmp\\abc.txt");
  ck_assert_int_eq (strlen (from), strlen (to));
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

