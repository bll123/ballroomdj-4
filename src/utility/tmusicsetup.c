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
#include "bdjstring.h"
#include "dance.h"
#include "datafile.h"
#include "bdj4.h"
#include "bdjopt.h"
#include "bdjvarsdfload.h"
#include "dirop.h"
#include "filemanip.h"
#include "fileop.h"
#include "genre.h"
#include "ilist.h"
#include "level.h"
#include "localeutil.h"
#include "log.h"
#include "mdebug.h"
#include "musicdb.h"
#include "pathutil.h"
#include "rating.h"
#include "slist.h"
#include "song.h"
#include "songdb.h"
#include "status.h"
#include "sysvars.h"
#include "tagdef.h"
#include "templateutil.h"

enum {
  TM_SOURCE = TAG_KEY_MAX + 1,
  TM_DEST = TAG_KEY_MAX + 2,
  TM_MAX_DANCE = 20,        // normally 14 or so in the standard template.
};

/* 'source' is added */
/* everything is left as a string for parsing purposes */
static datafilekey_t tmdfkeys [] = {
  { "ALBUM",                TAG_ALBUM,                VALUE_STR, NULL, -1 },
  { "ALBUMARTIST",          TAG_ALBUMARTIST,          VALUE_STR, NULL, -1 },
  { "ARTIST",               TAG_ARTIST,               VALUE_STR, NULL, -1 },
  { "BPM",                  TAG_BPM,                  VALUE_STR, NULL, -1 },
  { "DANCE",                TAG_DANCE,                VALUE_STR, NULL, -1 },
  { "DANCELEVEL",           TAG_DANCELEVEL,           VALUE_STR, NULL, -1 },
  { "DANCERATING",          TAG_DANCERATING,          VALUE_STR, NULL, -1 },
  { "DEST",                 TM_DEST,                  VALUE_STR, NULL, -1 },
  { "DISC",                 TAG_DISCNUMBER,           VALUE_STR, NULL, -1 },
  { "DISCTOTAL",            TAG_DISCTOTAL,            VALUE_STR, NULL, -1 },
  { "DURATION",             TAG_DURATION,             VALUE_STR, NULL, -1 },
  { "FAVORITE",             TAG_FAVORITE,             VALUE_STR, NULL, -1 },
  { "GENRE",                TAG_GENRE,                VALUE_STR, NULL, -1 },
  { "KEYWORD",              TAG_KEYWORD,              VALUE_STR, NULL, -1 },
  { "NOTES",                TAG_NOTES,                VALUE_STR, NULL, -1 },
  { "SAMESONG",             TAG_SAMESONG,             VALUE_STR, NULL, -1 },
  { "SOURCE",               TM_SOURCE,                VALUE_STR, NULL, -1 },
  { "STATUS",               TAG_STATUS,               VALUE_STR, NULL, -1 },
  { "TAGS",                 TAG_TAGS,                 VALUE_STR, NULL, -1 },
  { "TITLE",                TAG_TITLE,                VALUE_STR, NULL, -1 },
  { "TRACKNUMBER",          TAG_TRACKNUMBER,          VALUE_STR, NULL, -1 },
  { "TRACKTOTAL",           TAG_TRACKTOTAL,           VALUE_STR, NULL, -1 },
};
enum {
  tmdfcount = sizeof (tmdfkeys) / sizeof (datafilekey_t),
};

static slist_t *updateData (ilist_t *tmlist, ilistidx_t key);
static char *createFile (const char *src, const char *dest);

static int  gtracknum [TM_MAX_DANCE];
static int  gseqnum [TM_MAX_DANCE];
static char *tmusicorig = "test-music-orig";
static char *tmusicdir = "test-music";

int
main (int argc, char *argv [])
{
  int         c = 0;
  int         option_index = 0;
  bool        clbdj3tags = false;
  bool        isbdj4 = false;
  bool        emptydb = false;
  datafile_t  *df = NULL;
  ilist_t     *tmlist = NULL;
  ilistidx_t  tmiteridx;
  ilistidx_t  key;
  char        dbfn [MAXPATHLEN];
  char        infn [MAXPATHLEN];
  musicdb_t   *db;
  slist_t     *empty = NULL;
  loglevel_t  loglevel = LOG_IMPORTANT | LOG_MAIN | LOG_LIST;
  bool        loglevelset = false;

  static struct option bdj_options [] = {
    { "bdj3tags",     no_argument,        NULL,   '3' },
    { "bdj4",         no_argument,        NULL,   'B' },
    { "debugself",    no_argument,        NULL,   0 },
    { "debug",        no_argument,        NULL,   'd' },
    { "emptydb",      no_argument,        NULL,   'E' },
    { "infile",       required_argument,  NULL,   'I' },
    { "nodetach",     no_argument,        NULL,   0, },
    { "outfile",      required_argument,  NULL,   'O' },
    { "scale",        required_argument,  NULL,   0 },
    { "theme",        required_argument,  NULL,   0 },
    { "tmusicsetup",  no_argument,        NULL,   0 },
  };

#if BDJ4_MEM_DEBUG
  mdebugInit ("tset");
#endif

  strlcpy (dbfn, "test-templates/musicdb.dat", sizeof (dbfn));
  strlcpy (infn, "test-templates/test-music.txt", sizeof (infn));

  while ((c = getopt_long_only (argc, argv, "B3O:I:Ed:", bdj_options, &option_index)) != -1) {
    switch (c) {
      case '3': {
        clbdj3tags = true;
        break;
      }
      case 'B': {
        isbdj4 = true;
        break;
      }
      case 'd': {
        if (optarg) {
          loglevel = atol (optarg);
          loglevelset = true;
        }
        break;
      }
      case 'E': {
        emptydb = true;
        break;
      }
      case 'O': {
        strlcpy (dbfn, optarg, sizeof (dbfn));
        break;
      }
      case 'I': {
        strlcpy (infn, optarg, sizeof (infn));
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

  if (clbdj3tags) {
    bdjoptSetNum (OPT_G_BDJ3_COMPAT_TAGS, clbdj3tags);
  }

  for (int i = 0; i < TM_MAX_DANCE; ++i) {
    gseqnum [i] = 1;
    gtracknum [i] = 1;
  }
  empty = slistAlloc ("tm-empty", LIST_ORDERED, NULL);

  bdjoptSetStr (OPT_M_DIR_MUSIC, tmusicdir);
  bdjoptSetNum (OPT_G_WRITETAGS, WRITE_TAGS_ALL);
  bdjvarsdfloadInit ();

  if (! loglevelset) {
    loglevel = bdjoptGetNum (OPT_G_DEBUGLVL);
  }
  logStartAppend ("tmusicsetup", "tset", loglevel);

  /* create an entirely new database */
  fileopDelete (dbfn);
  db = dbOpen (dbfn);

  df = datafileAllocParse ("test-music", DFTYPE_INDIRECT, infn,
      tmdfkeys, tmdfcount);
  tmlist = datafileGetList (df);

  ilistStartIterator (tmlist, &tmiteridx);
  while ((key = ilistIterateKey (tmlist, &tmiteridx)) >= 0) {
    const char  *src;
    const char  *dest;
    char        *fn;
    slist_t     *tagdata = NULL;

    tagdata = updateData (tmlist, key);

    src = ilistGetStr (tmlist, key, TM_SOURCE);
    dest = ilistGetStr (tmlist, key, TM_DEST);
    fn = createFile (src, dest);
    audiotagWriteTags (fn, empty, tagdata, AF_REWRITE_NONE, AT_UPDATE_MOD_TIME);
    if (emptydb) {
      slistFree (tagdata);
      tagdata = slistAlloc ("tm-slist", LIST_ORDERED, NULL);
    }
    dbWrite (db, fn + strlen (tmusicdir) + 1, tagdata, MUSICDB_ENTRY_NEW);
    slistFree (tagdata);
    mdfree (fn);
  }

  datafileFree (df);
  slistFree (empty);

  dbClose (db);

  bdjvarsdfloadCleanup ();
  audiotagCleanup ();
  tagdefCleanup ();
  bdjoptCleanup ();
  localeCleanup ();
  logEnd ();
#if BDJ4_MEM_DEBUG
  mdebugReport ();
  mdebugCleanup ();
#endif
  return 0;
}

static slist_t *
updateData (ilist_t *tmlist, ilistidx_t key)
{
  slist_t       *tagdata;
  datafilekey_t *dfkey;
  datafileconv_t  conv;
  ilistidx_t    danceIdx = LIST_VALUE_INVALID;

  conv.allocated = false;
  conv.str = ilistGetStr (tmlist, key, TAG_DANCE);
  conv.valuetype = VALUE_STR;
  danceConvDance (&conv);
  danceIdx = conv.num;

  tagdata = slistAlloc ("tm-slist", LIST_ORDERED, NULL);

  for (int i = 0; i < tmdfcount; ++i) {
    const char  *val;
    char        nval [200];
    int         tn;
    int         sn;

    dfkey = &tmdfkeys [i];
    val = ilistGetStr (tmlist, key, dfkey->itemkey);
    if (val == NULL || *val == '\0') {
      continue;
    }

    tn = 1;
    sn = 1;
    if (danceIdx >= 0 && danceIdx < TM_MAX_DANCE) {
      tn = gtracknum [danceIdx];
      sn = gseqnum [danceIdx];
    }

    if (dfkey->itemkey == TAG_TRACKNUMBER) {
      snprintf (nval, sizeof (nval), val, tn);
    } else {
      snprintf (nval, sizeof (nval), val, sn);
    }

    ilistSetStr (tmlist, key, dfkey->itemkey, nval);
    if (dfkey->itemkey != TM_SOURCE && dfkey->itemkey != TM_DEST) {
      slistSetStr (tagdata, tagdefs [dfkey->itemkey].tag, nval);
    }
  }

  if (danceIdx >= 0) {
    gtracknum [danceIdx]++;
    gseqnum [danceIdx]++;
  }
  return tagdata;
}

static char *
createFile (const char *src, const char *dest)
{
  char        dir [MAXPATHLEN];
  char        from [MAXPATHLEN];
  char        to [MAXPATHLEN];
  pathinfo_t  *pi;

  snprintf (from, sizeof (from), "%s/%s", tmusicorig, src);
  if (! fileopFileExists (from)) {
    fprintf (stderr, "no source file: %s\n", src);
  }
  pi = pathInfo (src);
  snprintf (to, sizeof (to), "%s/%s%.*s", tmusicdir, dest,
      (int) pi->elen, pi->extension);
  pathInfoFree (pi);
  pi = pathInfo (to);
  snprintf (dir, sizeof (dir), "%.*s", (int) pi->dlen, pi->dirname);
  diropMakeDir (dir);

  filemanipCopy (from, to);
  pathInfoFree (pi);
  return mdstrdup (to);
}
