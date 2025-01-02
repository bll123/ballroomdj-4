/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
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
#include "pathinfo.h"
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
  /* relative paths */
  { "Tango/stuff.mp3", 5, 9, 5, 4 },
  { "xyzzy/Tango/stuff.mp3", 11, 9, 5, 4 },
//   1234567890123456789012345678901234567890
  { "/home/bll/s/bdj4/tmp/E", 20, 1, 1, 0 },  /* bug 2024-3-16 */
  { "/home/bll/s/bdj4/tmp/EE", 20, 2, 2, 0 },
};
enum {
  TCOUNT = (sizeof(tests)/sizeof (ftest_t))
};

START_TEST(pathinfo_chk)
{
  pathinfo_t    *pi;
  char          tbuff [MAXPATHLEN];
  char          tmp [MAXPATHLEN];

  logMsg (LOG_DBG, LOG_IMPORTANT, "--chk-- pathinfo_chk");
  mdebugSubTag ("pathinfo_chk");

  for (int i = 0; i < TCOUNT; ++i) {
    pi = pathInfo (tests[i].path);
    // fprintf (stderr, "%d d: %d %.*s\n", i, (int) pi->dlen, (int) pi->dlen, pi->dirname);
    // fprintf (stderr, "%d f: %d %.*s\n", i, (int) pi->flen, (int) pi->flen, pi->filename);
    // fprintf (stderr, "%d f: %d %.*s\n", i, (int) pi->blen, (int) pi->blen, pi->basename);
    // fprintf (stderr, "%d e: %d %.*s\n", i, (int) pi->elen, (int) pi->elen, pi->extension);
    ck_assert_msg (pi->dlen == tests[i].dlen,
        "i:%d %s dlen: %s: have: %d want: %d",
        i, tests [i].path, "dlen", (int) pi->dlen, (int) tests[i].dlen);
    ck_assert_msg (pi->flen == tests[i].flen,
        "i:%d %s flen: %s: have: %d want: %d",
        i, tests [i].path, "flen", (int) pi->flen, (int) tests[i].flen);
    ck_assert_msg (pi->blen == tests[i].blen,
        "i:%d %s blen: %s: have: %d want: %d",
        i, tests [i].path, "blen", (int) pi->blen, (int) tests[i].blen);
    ck_assert_msg (pi->elen == tests[i].elen,
        "i:%d %s elen: %s: have: %d want: %d",
        i, tests [i].path, "elen", (int) pi->elen, (int) tests[i].elen);

    if (pi->dlen > 0) {
      pathInfoGetDir (pi, tbuff, sizeof (tbuff));
      stpecpy (tmp, tmp + sizeof (tmp), tests [i].path);
      tmp [pi->dlen] = '\0';
      ck_assert_str_eq (tbuff, tmp);
    }

    if (pi->elen > 0) {
      pathInfoGetExt (pi, tbuff, sizeof (tbuff));
      /* pi->extension is null terminated, as it is the last */
      ck_assert_str_eq (tbuff, pi->extension);
    }

    /* the dirname pointer always points to the beginning */
    ck_assert_ptr_eq (pi->dirname, tests [i].path);

    /* check against self */
    ck_assert_int_eq (pathInfoExtCheck (pi, pi->extension), 1);
    ck_assert_int_eq (pathInfoExtCheck (pi, ".junk"), 0);

    pathInfoFree (pi);
  }
}
END_TEST

Suite *
pathinfo_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("pathinfo");
  tc = tcase_create ("pathinfo");
  tcase_set_tags (tc, "libcommon");
  tcase_add_test (tc, pathinfo_chk);
  suite_add_tcase (s, tc);
  return s;
}


#pragma clang diagnostic pop
#pragma GCC diagnostic pop
