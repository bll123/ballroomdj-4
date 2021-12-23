#include "bdjconfig.h"

#include <stdio.h>
#include <check.h>
#include <sys/types.h>
#include <time.h>

#include "fileutil.h"
#include "check_bdj.h"

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

START_TEST(fileinfo_chk)
{
  fileinfo_t    *fi;

  for (size_t i = 0; i < TCOUNT; ++i) {
    fi = fileInfo (tests[i].path);
    ck_assert_msg (fi->dlen == tests[i].dlen,
        "dlen: i:%ld %d/%d", i, fi->dlen, tests[i].dlen);
    ck_assert_msg (fi->blen == tests[i].blen,
        "blen: i:%ld %d/%d", i, fi->blen, tests[i].blen);
    ck_assert_msg (fi->flen == tests[i].flen,
        "flen: i:%ld %d/%d", i, fi->flen, tests[i].flen);
    ck_assert_msg (fi->elen == tests[i].elen,
        "elen: i:%ld %d/%d", i, fi->elen, tests[i].elen);
    fileInfoFree (fi);
  }
}
END_TEST

Suite *
fileutil_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("File Utils Suite");
  tc = tcase_create ("File Utils");
  tcase_add_test (tc, fileinfo_chk);
  suite_add_tcase (s, tc);
  return s;
}

