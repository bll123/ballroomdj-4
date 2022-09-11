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
#include "dance.h"
#include "datafile.h"
#include "bdj4.h"
#include "bdjopt.h"
#include "bdjvarsdfload.h"
#include "filemanip.h"
#include "fileop.h"
#include "genre.h"
#include "ilist.h"
#include "level.h"
#include "localeutil.h"
#include "log.h"
#include "musicdb.h"
#include "pathutil.h"
#include "rating.h"
#include "slist.h"
#include "song.h"
#include "songdb.h"
#include "songfav.h"
#include "status.h"
#include "sysvars.h"
#include "tagdef.h"
#include "templateutil.h"

enum {
  TM_SOURCE = TAG_KEY_MAX + 1,
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
static const char *createFile (const char *src, const char *dest);

static int  gtracknum [TM_MAX_DANCE];
static int  gseqnum [TM_MAX_DANCE];
static char *tmusicdir = "test-music/";

int
main (int argc, char *argv [])
{
  int         c = 0;
  int         option_index = 0;
  bool        bdj3tags = false;
  bool        clbdj3tags = false;
  bool        isbdj4 = false;
  char        tbuff [MAXPATHLEN];
  datafile_t  *df = NULL;
  ilist_t     *tmlist;
  ilistidx_t  tmiteridx;
  ilistidx_t  key;
  char        dbfn [MAXPATHLEN];
  musicdb_t   *db;
  slist_t     *empty;

  static struct option bdj_options [] = {
    { "bdj3tags",     no_argument,      NULL,   '3' },
    { "bdj4",         no_argument,      NULL,   'B' },
    { "tmusicsetup",  no_argument,      NULL,   0 },
    { "debugself",    no_argument,      NULL,   0 },
    { "msys",         no_argument,      NULL,   0 },
    { "nodetach",     no_argument,      NULL,   0, },
    { "theme",        no_argument,      NULL,   0 },
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
      default: {
        break;
      }
    }
  }

  if (! isbdj4) {
    fprintf (stderr, "not started with launcher\n");
    exit (1);
  }

  templateFileCopy ("dancetypes.txt", "dancetypes.txt");
  templateFileCopy ("dances.txt", "dances.txt");
  templateFileCopy ("genres.txt", "genres.txt");
  templateFileCopy ("levels.txt", "levels.txt");
  templateFileCopy ("ratings.txt", "ratings.txt");
  filemanipCopy ("test-templates/status.txt", "data/status.txt");

  sysvarsInit (argv [0]);
  localeInit ();
  bdjoptInit ();
  audiotagInit ();

  bdj3tags = bdjoptGetNum (OPT_G_BDJ3_COMPAT_TAGS);
  if (clbdj3tags) {
    bdjoptSetNum (OPT_G_BDJ3_COMPAT_TAGS, clbdj3tags);
    bdj3tags = clbdj3tags;
  }

  for (int i = 0; i < TM_MAX_DANCE; ++i) {
    gseqnum [i] = 1;
    gtracknum [i] = 1;
  }
  empty = slistAlloc ("tm-empty", LIST_ORDERED, free);

  bdjoptSetStr (OPT_M_DIR_MUSIC, tmusicdir);
  bdjoptSetNum (OPT_G_WRITETAGS, WRITE_TAGS_ALL);
  bdjvarsdfloadInit ();

  logStart ("tmusicsetup", "tm", LOG_ALL);

  snprintf (dbfn, sizeof (dbfn), "test-templates/musicdb.dat");
  /* create an entirely new database */
  fileopDelete (dbfn);
  db = dbOpen (dbfn);

  snprintf (tbuff, sizeof (tbuff), "test-templates/test-music.txt");
  df = datafileAllocParse ("test-music", DFTYPE_INDIRECT, tbuff,
      tmdfkeys, tmdfcount);
  tmlist = datafileGetList (df);

  ilistStartIterator (tmlist, &tmiteridx);
  while ((key = ilistIterateKey (tmlist, &tmiteridx)) >= 0) {
    const char  *src;
    const char  *dest;
    const char  *fn;
    slist_t     *tagdata;

    tagdata = updateData (tmlist, key);

    src = ilistGetStr (tmlist, key, TM_SOURCE);
    dest = ilistGetStr (tmlist, key, TAG_TITLE);
    fn = createFile (src, dest);
    audiotagWriteTags (fn, empty, tagdata, 0, AT_UPDATE_MOD_TIME);
    dbWrite (db, fn + strlen (tmusicdir) + 1, tagdata, MUSICDB_ENTRY_NEW);
  }

  dbClose (db);
  audiotagCleanup ();
  bdjvarsdfloadCleanup ();
  bdjoptCleanup ();
  localeCleanup ();
  return 0;
}

static slist_t *
updateData (ilist_t *tmlist, ilistidx_t key)
{
  slist_t       *tagdata;
  datafilekey_t *dfkey;
  datafileconv_t  conv;
  ilistidx_t    danceIdx;

  conv.allocated = false;
  conv.str = ilistGetStr (tmlist, key, TAG_DANCE);
  conv.valuetype = VALUE_STR;
  danceConvDance (&conv);
  danceIdx = conv.num;

  tagdata = slistAlloc ("tm-slist", LIST_ORDERED, free);

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
    if (dfkey->itemkey != TM_SOURCE) {
      slistSetStr (tagdata, tagdefs [dfkey->itemkey].tag, nval);
    }
  }

  gtracknum [danceIdx]++;
  gseqnum [danceIdx]++;
  return tagdata;
}

static const char *
createFile (const char *src, const char *dest)
{
  char        from [MAXPATHLEN];
  char        to [MAXPATHLEN];
  pathinfo_t  *pi;

  snprintf (from, sizeof (from), "%s/%s", tmusicdir, src);
  pi = pathInfo (src);
  snprintf (to, sizeof (to), "%s/%s%*s", tmusicdir, dest,
      (int) pi->elen, pi->extension);
  filemanipCopy (from, to);
  pathInfoFree (pi);
  return strdup (to);
}
