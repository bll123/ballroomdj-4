#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <getopt.h>
#include <unistd.h>

#include "audiotag.h"
#include "bdj4.h"
#include "bdjopt.h"
#include "fileop.h"
#include "localeutil.h"
#include "slist.h"
#include "sysvars.h"
#include "tagdef.h"

int
main (int argc, char *argv [])
{
  char        *data;
  slist_t     *wlist;
  slistidx_t  iteridx;
  char        *key;
  char        *val;
  bool        rawdata = false;
  bool        isbdj4 = false;
  int         c = 0;
  int         option_index = 0;
  int         fidx = -1;
  tagdefkey_t tagkey;
  bool        bdj3tags = false;
  bool        clbdj3tags = false;
  slist_t     *tagdata;
  bool        writetags;
  int         rewrite;
  bool        echo = true;


  static struct option bdj_options [] = {
    { "bdj4",         no_argument,      NULL,   'B' },
    { "bdj4tags",     no_argument,      NULL,   0 },
    { "rawdata",      no_argument,      NULL,   'r' },
    { "bdj3tags",     no_argument,      NULL,   '3' },
    { "debugself",    no_argument,      NULL,   0 },
    { "noecho",       no_argument,      NULL,   'e', },
    { "nodetach",     no_argument,      NULL,   0, },
    { "theme",        no_argument,      NULL,   0 },
    { "msys",         no_argument,      NULL,   0 },
  };

  while ((c = getopt_long_only (argc, argv, "BCp:d:mnNRs", bdj_options, &option_index)) != -1) {
    switch (c) {
      case '3': {
        clbdj3tags = true;
        break;
      }
      case 'B': {
        isbdj4 = true;
        break;
      }
      case 'r': {
        rawdata = true;
        break;
      }
      case 'e': {
        echo = false;
        break;
      }
      default: {
        break;
      }
    }
  }

  if (! isbdj4) {
    fprintf (stderr, "not started with launcher\n");
    exit (1);
  }

  sysvarsInit (argv [0]);
  localeInit ();
  bdjoptInit ();
  audiotagInit ();

  bdj3tags = bdjoptGetNum (OPT_G_BDJ3_COMPAT_TAGS);
  if (clbdj3tags) {
    bdjoptSetNum (OPT_G_BDJ3_COMPAT_TAGS, clbdj3tags);
    bdj3tags = clbdj3tags;
  }

  for (int i = 1; i < argc; ++i) {
    if (strncmp (argv [i], "--", 2) == 0) {
      continue;
    }
    fidx = i;
    break;
  }

  if (! fileopFileExists (argv [fidx])) {
    fprintf (stderr, "no file %s\n", argv [fidx]);
    bdjoptCleanup ();
    audiotagCleanup ();
    exit (1);
  }

  data = audiotagReadTags (argv [fidx]);
  if (echo && rawdata) {
    fprintf (stdout, "%s\n", data);
  }
  tagdata = audiotagParseData (argv [fidx], data, &rewrite);

  wlist = slistAlloc ("bdj4tags-write", LIST_ORDERED, free);
  /* need a copy of the tag list for the write */
  slistStartIterator (tagdata, &iteridx);
  while ((key = slistIterateKey (tagdata, &iteridx)) != NULL) {
    val = slistGetStr (tagdata, key);
    slistSetStr (wlist, key, val);
  }

  writetags = false;
  for (int i = fidx + 1; i < argc; ++i) {
    char    *p;
    char    *tokstr;

    val = strdup (argv [i]);
    p = strtok_r (val, "=", &tokstr);
    if (p != NULL) {
      p = strtok_r (NULL, "=", &tokstr);
      tagkey = tagdefLookup (val);
      if (tagkey >= 0) {
        slistSetStr (wlist, val, p);
        writetags = true;
      }
    }
  }

  if (writetags || rewrite) {
    int   value;

    value = bdjoptGetNum (OPT_G_WRITETAGS);
    bdjoptSetNum (OPT_G_WRITETAGS, WRITE_TAGS_ALL);
    audiotagWriteTags (argv [fidx], tagdata, wlist, rewrite);
    bdjoptSetNum (OPT_G_WRITETAGS, value);
  }
  slistFree (tagdata);

  /* output the tags after writing the new ones */
  if (echo && rawdata) {
    data = audiotagReadTags (argv [fidx]);
    fprintf (stdout, "%s\n", data);
  }

  if (echo) {
//    fprintf (stdout, "-- %s\n", argv [fidx]);
    slistStartIterator (wlist, &iteridx);
    while ((key = slistIterateKey (wlist, &iteridx)) != NULL) {
      val = slistGetStr (wlist, key);
      if (val != NULL) {
        fprintf (stdout, "%-20s %s\n", key, val);
      }
    }
  }
  slistFree (wlist);

  bdjoptCleanup ();
  audiotagCleanup ();
  localeCleanup ();
  return 0;
}
