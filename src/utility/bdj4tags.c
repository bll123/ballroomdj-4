/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
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
#include "log.h"
#include "mdebug.h"
#include "slist.h"
#include "sysvars.h"
#include "tagdef.h"

int
main (int argc, char *argv [])
{
  char        *data = NULL;
  slist_t     *wlist = NULL;
  slistidx_t  iteridx;
  char        *key = NULL;
  char        *val = NULL;
  bool        rawdata = false;
  bool        isbdj4 = false;
  int         c = 0;
  int         option_index = 0;
  int         fidx = -1;
  tagdefkey_t tagkey;
  bool        clbdj3tags = false;
  slist_t     *tagdata;
  bool        writetags;
  int         rewrite;
  bool        verbose = true;
  bool        cleantags = false;
  int         rc = AUDIOTAG_WRITE_OK;


  static struct option bdj_options [] = {
    { "bdj4",         no_argument,      NULL,   'B' },
    { "bdj4tags",     no_argument,      NULL,   0 },
    { "rawdata",      no_argument,      NULL,   'r' },
    { "bdj3tags",     no_argument,      NULL,   '3' },
    { "debugself",    no_argument,      NULL,   0 },
    { "verbose",      no_argument,      NULL,   0, },
    { "quiet",        no_argument,      NULL,   'Q', },
    { "cleantags",    no_argument,      NULL,   'L', },
    { "nodetach",     no_argument,      NULL,   0, },
    { "scale",        required_argument,NULL,   0 },
    { "theme",        required_argument,NULL,   0 },
  };

#if BDJ4_MEM_DEBUG
  mdebugInit ("tags");
#endif

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
      case 'Q': {
        verbose = false;
        break;
      }
      case 'L': {
        cleantags = true;
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
  tagdefInit ();
  audiotagInit ();
  logStartAppend ("bdj4tags", "tags", LOG_DBUPDATE | LOG_IMPORTANT | LOG_BASIC | LOG_MAIN);

  if (clbdj3tags) {
    bdjoptSetNum (OPT_G_BDJ3_COMPAT_TAGS, clbdj3tags);
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
  if (verbose && rawdata) {
    fprintf (stdout, "%s\n", data);
  }
  tagdata = audiotagParseData (argv [fidx], data, &rewrite);
  logMsg (LOG_DBG, LOG_BASIC, "rewrite: %08x", rewrite);

  wlist = slistAlloc ("bdj4tags-write", LIST_ORDERED, NULL);

  /* need a copy of the tag list for the write */
  slistStartIterator (tagdata, &iteridx);
  while ((key = slistIterateKey (tagdata, &iteridx)) != NULL) {
    val = slistGetStr (tagdata, key);
    if (cleantags) {
      slistSetStr (wlist, key, NULL);
    } else {
      slistSetStr (wlist, key, val);
    }
  }

  writetags = false;
  if (cleantags) {
    writetags = true;
  } else {
    for (int i = fidx + 1; i < argc; ++i) {
      char    *p;
      char    *tokstr;

      val = mdstrdup (argv [i]);
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
  }

  if (writetags || rewrite) {
    int   value;

    value = bdjoptGetNum (OPT_G_WRITETAGS);
    bdjoptSetNum (OPT_G_WRITETAGS, WRITE_TAGS_ALL);
    rc = audiotagWriteTags (argv [fidx], tagdata, wlist, rewrite, AT_UPDATE_MOD_TIME);
    bdjoptSetNum (OPT_G_WRITETAGS, value);
  }
  slistFree (tagdata);
  dataFree (data);

  /* output the tags after writing the new ones */
  if (verbose && rawdata) {
    data = audiotagReadTags (argv [fidx]);
    fprintf (stdout, "%s\n", data);
    dataFree (data);
  }

  if (verbose) {
    // fprintf (stdout, "-- %s\n", argv [fidx]);
    slistStartIterator (wlist, &iteridx);
    while ((key = slistIterateKey (wlist, &iteridx)) != NULL) {
      val = slistGetStr (wlist, key);
      if (val != NULL) {
        fprintf (stdout, "%-20s %s\n", key, val);
      }
    }
  }
  slistFree (wlist);

  tagdefCleanup ();
  bdjoptCleanup ();
  audiotagCleanup ();
  localeCleanup ();
  logEnd ();
#if BDJ4_MEM_DEBUG
  mdebugReport ();
  mdebugCleanup ();
#endif
  return rc;
}
