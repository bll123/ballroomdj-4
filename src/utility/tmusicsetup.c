/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <unistd.h>

#include "ati.h"
#include "audiosrc.h"
#include "audiofile.h"
#include "audiotag.h"
#include "bdj4arg.h"
#include "bdjstring.h"
#include "dance.h"
#include "datafile.h"
#include "bdj4.h"
#include "bdjopt.h"
#include "bdjvars.h"
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
#include "pathinfo.h"
#include "rating.h"
#include "slist.h"
#include "song.h"
#include "songdb.h"
#include "status.h"
#include "sysvars.h"
#include "tagdef.h"
#include "templateutil.h"
#include "tmutil.h"

enum {
  TM_FSOURCE = TAG_KEY_MAX + 1,
  TM_DEST = TAG_KEY_MAX + 2,
  TM_MAX_DANCE = 30,        // normally 14 or so in the standard template.
};

/* 'source' is added */
/* everything is left as a string for parsing purposes */
static datafilekey_t tmdfkeys [] = {
  { "ADJUSTFLAGS",  TAG_ADJUSTFLAGS,          VALUE_STR, NULL, DF_NORM },
  { "ALBUM",        TAG_ALBUM,                VALUE_STR, NULL, DF_NORM },
  { "ALBUMARTIST",  TAG_ALBUMARTIST,          VALUE_STR, NULL, DF_NORM },
  { "ALBUMARTISTSORT", TAG_SORT_ALBUMARTIST,  VALUE_STR, NULL, DF_NORM },
  { "ALBUMSORT",    TAG_SORT_ALBUM,           VALUE_STR, NULL, DF_NORM },
  { "ARTIST",       TAG_ARTIST,               VALUE_STR, NULL, DF_NORM },
  { "ARTISTSORT",   TAG_SORT_ARTIST,          VALUE_STR, NULL, DF_NORM },
  { "BPM",          TAG_BPM,                  VALUE_STR, NULL, DF_NORM },
  { "COMPOSER",     TAG_COMPOSER,             VALUE_STR, NULL, DF_NORM },
  { "COMPOSERSORT", TAG_SORT_COMPOSER,        VALUE_STR, NULL, DF_NORM },
  { "CONDUCTOR",    TAG_CONDUCTOR,            VALUE_STR, NULL, DF_NORM },
  { "DANCE",        TAG_DANCE,                VALUE_STR, NULL, DF_NORM },
  { "DANCELEVEL",   TAG_DANCELEVEL,           VALUE_STR, NULL, DF_NORM },
  { "DANCERATING",  TAG_DANCERATING,          VALUE_STR, NULL, DF_NORM },
  { "DATE",         TAG_DATE,                 VALUE_STR, NULL, DF_NORM },
  { "DBADDDATE",    TAG_DBADDDATE,            VALUE_STR, NULL, DF_NORM },
  { "DB_LOC_LOCK",  TAG_DB_LOC_LOCK,          VALUE_STR, NULL, DF_NORM },
  { "DEST",         TM_DEST,                  VALUE_STR, NULL, DF_NORM },
  { "DISC",         TAG_DISCNUMBER,           VALUE_STR, NULL, DF_NORM },
  { "DISCTOTAL",    TAG_DISCTOTAL,            VALUE_STR, NULL, DF_NORM },
  { "DURATION",     TAG_DURATION,             VALUE_STR, NULL, DF_NORM },
  { "FAVORITE",     TAG_FAVORITE,             VALUE_STR, NULL, DF_NORM },
  { "FSOURCE",      TM_FSOURCE,               VALUE_STR, NULL, DF_NORM },
  { "GENRE",        TAG_GENRE,                VALUE_STR, NULL, DF_NORM },
  { "GROUPING",     TAG_GROUPING,             VALUE_STR, NULL, DF_NORM },
  { "KEYWORD",      TAG_KEYWORD,              VALUE_STR, NULL, DF_NORM },
  { "MOVEMENTCOUNT",TAG_MOVEMENTCOUNT,        VALUE_STR, NULL, DF_NORM },
  { "MOVEMENTNAME", TAG_MOVEMENTNAME,         VALUE_STR, NULL, DF_NORM },
  { "MOVEMENTNUM",  TAG_MOVEMENTNUM,          VALUE_STR, NULL, DF_NORM },
  { "MQDISPLAY",    TAG_MQDISPLAY,            VALUE_STR, NULL, DF_NORM },
  { "NOPLAYTMLIMIT",TAG_NO_PLAY_TM_LIMIT,     VALUE_STR, NULL, DF_NORM },
  { "NOTES",        TAG_NOTES,                VALUE_STR, NULL, DF_NORM },
  { "RECORDING_ID", TAG_RECORDING_ID,         VALUE_STR, NULL, DF_NORM },
  { "SAMESONG",     TAG_SAMESONG,             VALUE_STR, NULL, DF_NORM },
  { "SONGEND",      TAG_SONGEND,              VALUE_STR, NULL, DF_NORM },
  { "SONGSTART",    TAG_SONGSTART,            VALUE_STR, NULL, DF_NORM },
  { "SPEEDADJUSTMENT", TAG_SPEEDADJUSTMENT,   VALUE_STR, NULL, DF_NORM },
  { "STATUS",       TAG_STATUS,               VALUE_STR, NULL, DF_NORM },
  { "TAGS",         TAG_TAGS,                 VALUE_STR, NULL, DF_NORM },
  { "TITLE",        TAG_TITLE,                VALUE_STR, NULL, DF_NORM },
  { "TITLESORT",    TAG_SORT_TITLE,           VALUE_STR, NULL, DF_NORM },
  { "TRACKNUMBER",  TAG_TRACKNUMBER,          VALUE_STR, NULL, DF_NORM },
  { "TRACKTOTAL",   TAG_TRACKTOTAL,           VALUE_STR, NULL, DF_NORM },
  { "TRACK_ID",     TAG_TRACK_ID,             VALUE_STR, NULL, DF_NORM },
  { "VOLUMEADJUSTPERC", TAG_VOLUMEADJUSTPERC, VALUE_STR, NULL, DF_NORM },
  { "WORK",         TAG_WORK,                 VALUE_STR, NULL, DF_NORM },
  { "WORK_ID",      TAG_WORK_ID,              VALUE_STR, NULL, DF_NORM },
};
enum {
  tmdfcount = sizeof (tmdfkeys) / sizeof (datafilekey_t),
};

static slist_t *updateData (ilist_t *tmusiclist, ilistidx_t key);
static char *createFile (const char *src, const char *dest, bool keepmusic);

static int  gdiscnum [200];
static int  gtracknum [200];
static int  gseqnum [TM_MAX_DANCE];
static const char *tmusicorig = "test-music-orig";
static const char *tmusicdir = "test-music";

int
main (int argc, char *argv [])
{
  int         c = 0;
  int         option_index = 0;
  bool        isbdj4 = false;
  bool        emptydb = false;
  datafile_t  *df = NULL;
  ilist_t     *tmusiclist = NULL;
  ilistidx_t  tmiteridx;
  ilistidx_t  key;
  char        dbfn [MAXPATHLEN];
  char        infn [MAXPATHLEN];
  char        seconddir [MAXPATHLEN];
  char        tbuff [MAXPATHLEN];
  musicdb_t   *db;
  slist_t     *empty = NULL;
  loglevel_t  loglevel = LOG_IMPORTANT | LOG_INFO;
  bool        loglevelset = false;
  bool        keepmusic = false;
  int         supported [AFILE_TYPE_MAX];
  int         tagtype;
  int         filetype;
  bdj4arg_t   *bdj4arg;
  const char  *targ;
  songdb_t    *songdb;
  song_t      *song;

  static struct option bdj_options [] = {
    { "bdj4",         no_argument,        NULL,   'B' },
    { "seconddir",    required_argument,  NULL,   'A' },
    { "emptydb",      no_argument,        NULL,   'E' },
    { "infile",       required_argument,  NULL,   'I' },
    { "keepmusic",    no_argument,        NULL,   'K' },
    { "outfile",      required_argument,  NULL,   'O' },
    { "tmusicsetup",  no_argument,        NULL,   0 },
    { "musicdir",     required_argument,  NULL,   'D' },
    { "verbose",      no_argument,        NULL,   0, },
    /* launcher options */
    { "debug",        required_argument,  NULL,   'd' },
    { "debugself",    no_argument,        NULL,   0 },
    { "nodetach",     no_argument,        NULL,   0, },
    { "origcwd",      required_argument,  NULL,   0 },
    { "scale",        required_argument,  NULL,   0 },
    { "theme",        required_argument,  NULL,   0 },
    { "pli",          required_argument,  NULL,   0, },
    { "wait",         no_argument,        NULL,   0, },
  };

#if BDJ4_MEM_DEBUG
  mdebugInit ("tset");
#endif

  stpecpy (dbfn, dbfn + sizeof (dbfn), "data/musicdb.dat");
  stpecpy (infn, infn + sizeof (infn), "test-templates/test-music.txt");
  *seconddir = '\0';

  bdj4arg = bdj4argInit (argc, argv);

  while ((c = getopt_long_only (argc, bdj4argGetArgv (bdj4arg),
      "B3O:I:Ed:", bdj_options, &option_index)) != -1) {
    switch (c) {
      case 'B': {
        isbdj4 = true;
        break;
      }
      case 'D': {
        tmusicdir = optarg;
        break;
      }
      case 'd': {
        if (optarg != NULL) {
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
        if (optarg != NULL) {
          targ = bdj4argGet (bdj4arg, optind - 1, optarg);
          stpecpy (dbfn, dbfn + sizeof (dbfn), targ);
        }
        break;
      }
      case 'I': {
        if (optarg != NULL) {
          targ = bdj4argGet (bdj4arg, optind - 1, optarg);
          stpecpy (infn, infn + sizeof (infn), targ);
        }
        break;
      }
      case 'K': {
        keepmusic = true;
        break;
      }
      case 'A': {
        if (optarg != NULL) {
          targ = bdj4argGet (bdj4arg, optind - 1, optarg);
          stpecpy (seconddir, seconddir + sizeof (seconddir), targ);
        }
        break;
      }
      default: {
        break;
      }
    }
  }

  if (! isbdj4) {
    fprintf (stderr, "not started with launcher\n");
    bdj4argCleanup (bdj4arg);
    return 1;
  }

  targ = bdj4argGet (bdj4arg, 0, argv [0]);
  sysvarsInit (targ, SYSVARS_FLAG_ALL);
  localeInit ();
  bdjoptInit ();
  bdjvarsInit ();
  tagdefInit ();
  audiotagInit ();

  for (int i = 0; i < TM_MAX_DANCE; ++i) {
    gseqnum [i] = 1;
  }
  for (size_t i = 0; i < sizeof (gtracknum) / sizeof (int); ++i) {
    gdiscnum [i] = 1;
    gtracknum [i] = 1;
  }
  empty = slistAlloc ("tm-empty", LIST_ORDERED, NULL);

  bdjoptSetStr (OPT_M_DIR_MUSIC, tmusicdir);
  /* audio tags are written separately, not via the songdb interface */
  /* the option is set around the write */
  bdjoptSetNum (OPT_G_WRITETAGS, WRITE_TAGS_NONE);
  bdjvarsdfloadInit ();
  audiosrcInit ();

  if (! loglevelset) {
    loglevel = bdjoptGetNum (OPT_G_DEBUGLVL);
  }
  logStartAppend ("tmusicsetup", "tset", loglevel);

  logMsg (LOG_DBG, LOG_IMPORTANT, "ati: %s", bdjoptGetStr (OPT_M_AUDIOTAG_INTFC));
  atiGetSupportedTypes (bdjoptGetStr (OPT_M_AUDIOTAG_INTFC), supported);

  fileopDelete (dbfn);
  db = dbOpen (dbfn);
  dbStartBatch (db);

  songdb = songdbAlloc (db);
  song = songAlloc ();

  df = datafileAllocParse ("test-music", DFTYPE_INDIRECT, infn,
      tmdfkeys, tmdfcount, DF_NO_OFFSET, NULL);
  tmusiclist = datafileGetList (df);

  ilistStartIterator (tmusiclist, &tmiteridx);
  while ((key = ilistIterateKey (tmusiclist, &tmiteridx)) >= 0) {
    const char  *src;
    char        from [MAXPATHLEN];
    const char  *dest;
    char        *fn;
    slist_t     *tagdata = NULL;
    const char  *songfn = NULL;
    int32_t     songdbflags;
    const char  *tval;

    tagdata = updateData (tmusiclist, key);

    src = ilistGetStr (tmusiclist, key, TM_FSOURCE);
    dest = ilistGetStr (tmusiclist, key, TM_DEST);

    /* need full path to determine tag type */
    snprintf (from, sizeof (from), "%s/%s", tmusicorig, src);
    audiotagDetermineTagType (from, &tagtype, &filetype);
    if (supported [filetype] == ATI_NONE) {
      slistFree (tagdata);
      continue;
    }

    fn = createFile (src, dest, keepmusic);

    tval = slistGetStr (tagdata, "DBADDDATE");
    if (tval != NULL) {
      time_t  tmval;
      char    tmp [40];

      tmval = tmutilStringToUTC (tval, "%F");
      snprintf (tmp, sizeof (tmp), "%" PRIu64, (uint64_t) tmval);
      slistSetStr (tagdata, tagdefs [TAG_DBADDDATE].tag, tmp);
    }

    if (! keepmusic &&
        supported [filetype] == ATI_READ_WRITE) {
      bdjoptSetNum (OPT_G_WRITETAGS, WRITE_TAGS_ALL);
      audiotagWriteTags (fn, empty, tagdata, AF_REWRITE_NONE, AT_FLAGS_MOD_TIME_UPDATE);
      bdjoptSetNum (OPT_G_WRITETAGS, WRITE_TAGS_NONE);
    }

    if (emptydb) {
      char        *dur;
      char        *title;

      dur = mdstrdup (slistGetStr (tagdata, tagdefs [TAG_DURATION].tag));
      title = mdstrdup (slistGetStr (tagdata, tagdefs [TAG_TITLE].tag));
      slistFree (tagdata);
      tagdata = slistAlloc ("tm-slist", LIST_ORDERED, NULL);
      slistSetStr (tagdata, tagdefs [TAG_DURATION].tag, dur);
      slistSetStr (tagdata, tagdefs [TAG_TITLE].tag, title);
      mdfree (dur);
      mdfree (title);
    }

    slistSetStr (tagdata, tagdefs [TAG_PREFIX_LEN].tag, "0");

    songfn = fn;
    if (strcmp (tmusicdir, "test-music") == 0) {
      songfn = fn + strlen (tmusicdir) + 1;
    } else {
      char  tmp [40];

      snprintf (tmp, sizeof (tmp), "%d", (int) strlen (tmusicdir) + 1);
      slistSetStr (tagdata, tagdefs [TAG_PREFIX_LEN].tag, tmp);
    }
    slistSetStr (tagdata, tagdefs [TAG_URI].tag, songfn);

    songFromTagList (song, tagdata);
    songSetChanged (song);
    songdbflags = SONGDB_NONE;
    /* if write-tags is on, invalid genres get deleted */
    bdjoptSetNum (OPT_G_WRITETAGS, WRITE_TAGS_NONE);
    songdbWriteDBSong (songdb, song, &songdbflags, MUSICDB_ENTRY_NEW);

    if (*seconddir) {
      char    tmp [40];

      /* if the alternate dir is set, create a duplicate entry */
      /* this is a simple method used to create the secondary dir */
      /* it could be removed (dbtest.sh) */
      snprintf (tbuff, sizeof (tbuff), "%s/%s", seconddir, songfn);
      slistSetStr (tagdata, tagdefs [TAG_URI].tag, tbuff);
      snprintf (tmp, sizeof (tmp), "%d", (int) strlen (seconddir) + 1);
      slistSetStr (tagdata, tagdefs [TAG_PREFIX_LEN].tag, tmp);
      songFromTagList (song, tagdata);
      songSetChanged (song);
      songdbflags = SONGDB_NONE;
      songdbWriteDBSong (songdb, song, &songdbflags, MUSICDB_ENTRY_NEW);
    }
    slistFree (tagdata);
    mdfree (fn);
  }

  datafileFree (df);
  slistFree (empty);

  songFree (song);
  songdbFree (songdb);
  dbEndBatch (db);
  dbClose (db);

  audiosrcCleanup ();
  bdjvarsdfloadCleanup ();
  audiotagCleanup ();
  tagdefCleanup ();
  bdjvarsCleanup ();
  bdjoptCleanup ();
  localeCleanup ();
  logEnd ();
  bdj4argCleanup (bdj4arg);
#if BDJ4_MEM_DEBUG
  mdebugReport ();
  mdebugCleanup ();
#endif
  return 0;
}

static slist_t *
updateData (ilist_t *tmusiclist, ilistidx_t key)
{
  slist_t       *tagdata;
  datafilekey_t *dfkey;
  datafileconv_t  conv;
  ilistidx_t    danceIdx = LIST_VALUE_INVALID;

  conv.str = ilistGetStr (tmusiclist, key, TAG_DANCE);
  conv.invt = VALUE_STR;
  danceConvDance (&conv);
  danceIdx = conv.num;

  tagdata = slistAlloc ("tm-slist", LIST_ORDERED, NULL);

  for (int i = 0; i < tmdfcount; ++i) {
    const char  *val;
    char        nval [200];
    int         tn;
    int         sn;

    dfkey = &tmdfkeys [i];
    val = ilistGetStr (tmusiclist, key, dfkey->itemkey);
    if (val == NULL || *val == '\0') {
      continue;
    }

    tn = 1;
    sn = 1;
    if (danceIdx >= 0 && danceIdx < TM_MAX_DANCE) {
      sn = gseqnum [danceIdx];
    }

    if (dfkey->itemkey == TAG_DISCNUMBER) {
      int   tval;

      tval = atoi (val);
      if (tval > 0 && gdiscnum [sn] != tval) {
        gtracknum [sn] = 1;
      }
      gdiscnum [sn] = tval;
    }

    tn = gtracknum [sn];

    if (dfkey->itemkey == TAG_TRACKNUMBER) {
      snprintf (nval, sizeof (nval), val, tn);
      gtracknum [sn]++;
    } else {
      snprintf (nval, sizeof (nval), val, sn);
    }

    ilistSetStr (tmusiclist, key, dfkey->itemkey, nval);
    if (dfkey->itemkey == TAG_DBADDDATE) {
      slistSetStr (tagdata, dfkey->name, nval);
    } else if (dfkey->itemkey != TM_FSOURCE && dfkey->itemkey != TM_DEST) {
      slistSetStr (tagdata, tagdefs [dfkey->itemkey].tag, nval);
    }
  }

  if (danceIdx >= 0) {
    gseqnum [danceIdx]++;
  }
  return tagdata;
}

static char *
createFile (const char *src, const char *dest, bool keepmusic)
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
  if (keepmusic && ! fileopFileExists (to)) {
    fprintf (stderr, "no dest file: %s\n", to);
  }

  if (! keepmusic) {
    pathInfoFree (pi);
    pi = pathInfo (to);
    pathInfoGetDir (pi, dir, sizeof (dir));
    diropMakeDir (dir);

    filemanipCopy (from, to);
  }

  pathInfoFree (pi);

  return mdstrdup (to);
}
