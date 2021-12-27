#include "config.h"

#include <stdio.h>
#include <check.h>
#include <sys/types.h>
#include <time.h>

#include "tmutil.h"
#include "check_bdj.h"


START_TEST(msleep_chk)
{
  time_t      tm_s;
  time_t      tm_e;

  tm_s = time (NULL);
  msleep (2000);
  tm_e = time (NULL);
  ck_assert (tm_e - tm_s >= 2);
}
END_TEST

START_TEST(milli_start_end)
{
  time_t      tm_s;
  time_t      tm_e;
  mtime_t     mi;

  tm_s = time (NULL);
  mtimestart (&mi);
  msleep (2000);
  size_t m = mtimeend (&mi);
  tm_e = time (NULL);
  long d = (((long) tm_e - (long) tm_s) * 1000 - (long) m);
  if (d < 0) {
    d = - d;
  }
  ck_assert (d < 100);
}
END_TEST

START_TEST(dstamp_chk)
{
  char        buff [80];

  dstamp (buff, sizeof (buff));
  ck_assert (strlen (buff) == 23);
}
END_TEST

Suite *
tmutil_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("Time Utils Suite");
  tc = tcase_create ("Time Utils");
  tcase_add_test (tc, msleep_chk);
  tcase_add_test (tc, milli_start_end);
  tcase_add_test (tc, dstamp_chk);
  suite_add_tcase (s, tc);
  return s;
}

