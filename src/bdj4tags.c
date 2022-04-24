#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <getopt.h>
#include <unistd.h>

#include "bdj4.h"
#include "bdjopt.h"
#include "audiotag.h"
#include "tagdef.h"
#include "localeutil.h"
#include "slist.h"
#include "sysvars.h"

int
main (int argc, char *argv [])
{
  char    *data;
  slist_t *list;
  slistidx_t  iteridx;
  char    *key;
  char    *val;
  bool    rawdata = false;
  bool    isbdj4 = false;
  int     c = 0;
  int     option_index = 0;

  static struct option bdj_options [] = {
    { "bdj4",         no_argument,      NULL,   'B' },
    { "bdj4tags",     no_argument,      NULL,   0 },
    { "rawdata",      no_argument,      NULL,   'r' },
    { "debugself",    no_argument,      NULL,   0 },
    { "theme",        no_argument,      NULL,   0 },
    { "msys",         no_argument,      NULL,   0 },
  };

  while ((c = getopt_long_only (argc, argv, "BCp:d:mnNRs", bdj_options, &option_index)) != -1) {
    switch (c) {
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

  sysvarsInit (argv [0]);
  localeInit ();
  bdjoptInit ();

  for (int i = 1; i < argc; ++i) {
    if (strncmp (argv [i], "--", 2) == 0) {
      continue;
    }
    fprintf (stdout, "-- %s\n", argv [i]);
    data = audiotagReadTags (argv [i]);
    if (rawdata) {
      fprintf (stdout, "%s\n", data);
    }
    list = audiotagParseData (argv [i], data);
    slistStartIterator (list, &iteridx);
    while ((key = slistIterateKey (list, &iteridx)) != NULL) {
      val = slistGetStr (list, key);
      fprintf (stdout, "%-20s %s\n", key, val);
    }
  }

  return 0;
}
