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
#include "localeutil.h"
#include "slist.h"
#include "sysvars.h"
#include "tagdef.h"

int
main (int argc, char *argv [])
{
  char        *data;
  slist_t     *list;
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

  static struct option bdj_options [] = {
    { "bdj4",         no_argument,      NULL,   'B' },
    { "bdj4tags",     no_argument,      NULL,   0 },
    { "rawdata",      no_argument,      NULL,   'r' },
    { "bdj3tags",     no_argument,      NULL,   '3' },
    { "debugself",    no_argument,      NULL,   0 },
    { "nodetach",     no_argument,      NULL,   0, },
    { "theme",        no_argument,      NULL,   0 },
    { "msys",         no_argument,      NULL,   0 },
  };

  while ((c = getopt_long_only (argc, argv, "BCp:d:mnNRs", bdj_options, &option_index)) != -1) {
    switch (c) {
      case '3': {
        bdj3tags = true;
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
  /* always override the bdj3 compat flag */
  bdjoptSetNum (OPT_G_BDJ3_COMPAT_TAGS, bdj3tags);

  for (int i = 1; i < argc; ++i) {
    if (strncmp (argv [i], "--", 2) == 0) {
      continue;
    }
    fidx = i;
    break;
  }

  wlist = slistAlloc ("write-tags", LIST_ORDERED, free);
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
      }
    }
  }
  if (slistGetCount (wlist) > 0) {
    audiotagWriteTags (argv [fidx], wlist);
  }
  slistFree (wlist);

  /* output the tags after writing the new ones */
  data = audiotagReadTags (argv [fidx]);
//  fprintf (stdout, "-- %s\n", argv [fidx]);
  if (rawdata) {
    fprintf (stdout, "%s\n", data);
  }
  list = audiotagParseData (argv [fidx], data);
  slistStartIterator (list, &iteridx);
  while ((key = slistIterateKey (list, &iteridx)) != NULL) {
    val = slistGetStr (list, key);
    fprintf (stdout, "%-20s %s\n", key, val);
  }
  slistFree (list);

  audiotagCleanup ();
  return 0;
}
