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

#include "dirlist.h"
#include "dirop.h"
#include "filemanip.h"
#include "fileop.h"
#include "check_bdj.h"
#include "slist.h"
#include "sysvars.h"

typedef struct {
  int       type;
  int       count;
  char      *name;
  char      *fname;
} dirlist_chk_t;

enum {
  CHK_DIR,
  CHK_FILE,
  CHK_DLINK,
  CHK_LINK,
};

dirlist_chk_t tvalues [] = {
  { CHK_DIR,  1, "tmp/abc", NULL },
  { CHK_FILE, 0, "tmp/abc/abc.txt", "abc.txt" },
  { CHK_DIR,  1, "tmp/abc/def", NULL },
  { CHK_FILE, 0, "tmp/abc/def/def.txt", "def.txt" },
  { CHK_DIR,  2, "tmp/abc/ghi", "chk" },
  { CHK_FILE, 0, "tmp/abc/ghi/ghi.txt", "ghi.txt" },
  { CHK_DLINK, 2, "tmp/abc/jkl", "ghi" },
  { CHK_LINK, 0, "tmp/abc/ghi/jkl.txt", "ghi.txt" },
  { CHK_DIR,  2, "tmp/abc/ÄÑÄÑ", NULL },
  { CHK_FILE, 0, "tmp/abc/ÄÑÄÑ/abc-def.txt", "abc-def.txt" },
  { CHK_FILE, 0, "tmp/abc/ÄÑÄÑ/ÜÄÑÖ.txt", "ÜÄÑÖ.txt" },
  { CHK_DIR,  4, "tmp/abc/夕陽伴我歸", NULL },
  { CHK_FILE, 0, "tmp/abc/夕陽伴我歸/내가제일잘나가.txt", "내가제일잘나가.txt" },
  { CHK_FILE, 0, "tmp/abc/夕陽伴我歸/ははは.txt", "ははは.txt" },
  { CHK_FILE, 0, "tmp/abc/夕陽伴我歸/夕陽伴我歸.txt", "夕陽伴我歸.txt" },
  { CHK_FILE, 0, "tmp/abc/夕陽伴我歸/Ne_Русский_Шторм.txt", "Ne_Русский_Шторм.txt" }
};
enum {
  tvaluesz = sizeof (tvalues) / sizeof (dirlist_chk_t),
};
static int fcount = 0;
static int dcount = 0;

static void
teardown (void)
{
  for (int i = 0; i < tvaluesz; ++i) {
    if (tvalues [i].type == CHK_LINK) {
      fileopDelete (tvalues [i].name);
    }
    if (tvalues [i].type == CHK_DIR) {
      diropDeleteDir (tvalues [i].name);
    }
  }
}

static void
setup (void)
{
  FILE *fh;

  teardown ();

  for (int i = 0; i < tvaluesz; ++i) {
    if (tvalues [i].type == CHK_LINK) {
      if (! isWindows ()) {
        filemanipLinkCopy (tvalues [i].fname, tvalues [i].name);
      }
    }
    if (tvalues [i].type == CHK_DLINK) {
      if (isWindows ()) {
        diropMakeDir (tvalues [i].name);
      } else {
        filemanipLinkCopy (tvalues [i].fname, tvalues [i].name);
      }
      if (isWindows ()) {
        tvalues [i].count = 0;
      }
      ++dcount;
      fcount += tvalues [i].count;
    }
    if (tvalues [i].type == CHK_FILE) {
      fh = fileopOpen (tvalues [i].name, "w");
      fclose (fh);
    }
    if (tvalues [i].type == CHK_DIR) {
      diropMakeDir (tvalues [i].name);
      ++dcount;
      if (isWindows () && tvalues [i].fname != NULL) {
        tvalues [i].count -= 1;
      }
      fcount += tvalues [i].count;
    }
  }

  --dcount; // don't count top level
}

START_TEST(dirlist_basic)
{
  slist_t   *slist;

  slist = NULL;
  for (int i = 0; i < tvaluesz; ++i) {
    if (tvalues [i].type == CHK_DIR ||
        tvalues [i].type == CHK_DLINK) {
      if (slist != NULL) {
        slistFree (slist);
      }
      slist = dirlistBasicDirList (tvalues [i].name, NULL);
      ck_assert_int_eq (slistGetCount (slist), tvalues [i].count);
      slistSort (slist); // for testing
    }
    if (tvalues [i].type == CHK_FILE) {
      int val;

      val = slistGetNum (slist, tvalues [i].fname);
      ck_assert_int_eq (val, 0);
    }
    if (! isWindows() && tvalues [i].type == CHK_LINK) {
      int val;

      val = slistGetNum (slist, tvalues [i].fname);
      ck_assert_int_eq (val, 0);
    }
  }
  if (slist != NULL) {
    slistFree (slist);
  }
}
END_TEST

START_TEST(dirlist_recursive)
{
  slist_t   *slist;

  slist = dirlistRecursiveDirList ("tmp/abc", FILEMANIP_DIRS);
  slistSort (slist);  // for testing
  ck_assert_int_eq (slistGetCount (slist), dcount);
  slistFree (slist);

  slist = dirlistRecursiveDirList ("tmp/abc", FILEMANIP_FILES);
  slistSort (slist);  // for testing
  ck_assert_int_eq (slistGetCount (slist), fcount);
  slistFree (slist);
}
END_TEST

Suite *
dirlist_suite (void)
{
  Suite     *s;
  TCase     *tc;

  s = suite_create ("dirlist");
  tc = tcase_create ("dirlist");
  tcase_add_unchecked_fixture (tc, setup, teardown);
  tcase_add_test (tc, dirlist_basic);
  tcase_add_test (tc, dirlist_recursive);
  suite_add_tcase (s, tc);
  return s;
}

