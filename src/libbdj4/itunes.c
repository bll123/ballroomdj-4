/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>

#include <glib.h>

#include "audiosrc.h"
#include "bdj4.h"
#include "bdjopt.h"
#include "datafile.h"
#include "fileop.h"
#include "genre.h"
#include "itunes.h"
#include "log.h"
#include "mdebug.h"
#include "nlist.h"
#include "nodiscard.h"
#include "pathbld.h"
#include "rating.h"
#include "slist.h"
#include "songfav.h"
#include "songutil.h"
#include "tagdef.h"
#include "tmutil.h"
#include "xmlparse.h"

static const char * datamainxpath =
      "/plist/dict/dict/key";
static const char * dataxpath =
      "/plist/dict/dict/dict/key|"
      "/plist/dict/dict/dict/integer|"
      "/plist/dict/dict/dict/string|"
      "/plist/dict/dict/dict/date|"
      "/plist/dict/dict/dict/true|"
      "/plist/dict/dict/dict/false";
static const char * plmainxpath =
      "/plist/dict/array/dict/key";
static const char * plxpath =
      "/plist/dict/array/dict/key|"
      "/plist/dict/array/dict/integer|"
      "/plist/dict/array/dict/string|"
      "/plist/dict/array/dict/array/dict/key|"
      "/plist/dict/array/dict/array/dict/integer";
static const char *ITUNES_LOCALHOST = "file://localhost";
static const char *ITUNES_LOCAL = "file://";

typedef struct itunes {
  datafile_t    *starsdf;
  nlist_t       *stars;
  datafile_t    *fieldsdf;
  nlist_t       *fields;
  time_t        lastparse;
  nlist_t       *songbyidx;
  slist_t       *songbyname;
  slist_t       *playlists;
  slist_t       *itunesAvailFields;
  slistidx_t    availiteridx;
  slistidx_t    songiteridx;
  slistidx_t    pliteridx;
} itunes_t;

/* must be sorted in ascii order */
static datafilekey_t starsdfkeys [ITUNES_STARS_MAX] = {
  { "10",   ITUNES_STARS_10,      VALUE_NUM,  ratingConv, DF_NORM },
  { "100",  ITUNES_STARS_100,     VALUE_NUM,  ratingConv, DF_NORM },
  { "20",   ITUNES_STARS_20,      VALUE_NUM,  ratingConv, DF_NORM },
  { "30",   ITUNES_STARS_30,      VALUE_NUM,  ratingConv, DF_NORM },
  { "40",   ITUNES_STARS_40,      VALUE_NUM,  ratingConv, DF_NORM },
  { "50",   ITUNES_STARS_50,      VALUE_NUM,  ratingConv, DF_NORM },
  { "60",   ITUNES_STARS_60,      VALUE_NUM,  ratingConv, DF_NORM },
  { "70",   ITUNES_STARS_70,      VALUE_NUM,  ratingConv, DF_NORM },
  { "80",   ITUNES_STARS_80,      VALUE_NUM,  ratingConv, DF_NORM },
  { "90",   ITUNES_STARS_90,      VALUE_NUM,  ratingConv, DF_NORM },
};

static bool itunesParseData (itunes_t *itunes, xmlparse_t *xmlparse, const char *xpathExpr);
static bool itunesParsePlaylists (itunes_t *itunes, xmlparse_t *xmlparse, const char *xpathExpr);
static bool itunesParseXPath (xmlparse_t *xmlparse, const char *xpathExpr, slist_t *rawdata, const char *mainxpath);

bool
itunesConfigured (void)
{
  const char  *tval;
  int         have = 0;

  tval = bdjoptGetStr (OPT_M_DIR_ITUNES_MEDIA);
  if (fileopIsDirectory (tval)) {
    ++have;
  }
  tval = bdjoptGetStr (OPT_M_ITUNES_XML_FILE);
  if (fileopFileExists (tval)) {
    ++have;
  }
  return have == 2;
}

BDJ_NODISCARD
itunes_t *
itunesAlloc (void)
{
  itunes_t    *itunes;
  char        tbuff [BDJ4_PATH_MAX];
  slist_t     *tlist;
  slistidx_t  iteridx;
  const char  *key;

  itunes = mdmalloc (sizeof (itunes_t));

  pathbldMakePath (tbuff, sizeof (tbuff),
      ITUNES_STARS_FN, BDJ4_CONFIG_EXT, PATHBLD_MP_DREL_DATA);
  itunes->starsdf = datafileAllocParse (ITUNES_STARS_FN,
      DFTYPE_KEY_VAL, tbuff, starsdfkeys, ITUNES_STARS_MAX, DF_NO_OFFSET, NULL);
  itunes->stars = datafileGetList (itunes->starsdf);

  pathbldMakePath (tbuff, sizeof (tbuff),
      ITUNES_FIELDS_FN, BDJ4_CONFIG_EXT, PATHBLD_MP_DREL_DATA);
  itunes->fieldsdf = datafileAllocParse (ITUNES_FIELDS_FN,
      DFTYPE_LIST, tbuff, NULL, 0, DF_NO_OFFSET, NULL);
  tlist = datafileGetList (itunes->fieldsdf);

  itunes->fields = nlistAlloc ("itunes-fields", LIST_ORDERED, NULL);
  nlistSetSize (itunes->fields, slistGetCount (tlist));

  slistStartIterator (tlist, &iteridx);
  while ((key = slistIterateKey (tlist, &iteridx)) != NULL) {
    int     val;

    val = tagdefLookup (key);
    nlistSetNum (itunes->fields, val, 1);
  }

  itunes->itunesAvailFields = slistAlloc ("itunes-fields", LIST_ORDERED, NULL);
  for (int i = 0; i < TAG_KEY_MAX; ++i) {
    if (tagdefs [i].itunesName == NULL) {
      continue;
    }
    slistSetNum (itunes->itunesAvailFields, tagdefs [i].itunesName, i);
  }

  itunes->lastparse = 0;
  itunes->songbyidx = NULL;
  itunes->songbyname = NULL;
  itunes->playlists = NULL;

#if 0
  /* for debugging */
  {
    nlistidx_t    iteridx;
    nlistidx_t    nkey;
    const char    *skey;

    //bdjoptSetStr (OPT_M_DIR_ITUNES_MEDIA, "/home/music/m");
    //bdjoptSetStr (OPT_M_ITUNES_XML_FILE, "/home/bll/s/bdj4/test-files/iTunes Library.xml");
    //bdjoptSetStr (OPT_M_ITUNES_XML_FILE, "/home/bll/s/bdj4/test-files/iTunes-test-music.xml");
    itunesParse (itunes);
    nlistStartIterator (itunes->songbyidx, &iteridx);
    while ((nkey = nlistIterateKey (itunes->songbyidx, &iteridx)) >= 0) {
      int           tagidx;
      nlist_t       *entry;
      nlistidx_t    eiteridx;

      entry = nlistGetList (itunes->songbyidx, nkey);
      fprintf (stderr, "-- %" PRId32 " %s \n", nkey, nlistGetStr (entry, TAG_URI));

      nlistStartIterator (entry, &eiteridx);
      while ((tagidx = nlistIterateKey (entry, &eiteridx)) >= 0) {
        if (tagidx == TAG_URI) {
          continue;
        }
        if (tagdefs [tagidx].valueType == VALUE_NUM) {
          fprintf (stderr, "  %s %" PRId32 "\n", tagdefs [tagidx].tag, nlistGetNum (entry, tagidx));
        } else {
          fprintf (stderr, "  %s %s\n", tagdefs [tagidx].tag, nlistGetStr (entry, tagidx));
        }
      }
    }

    slistStartIterator (itunes->playlists, &iteridx);
    while ((skey = slistIterateKey (itunes->playlists, &iteridx)) != NULL) {
      int           ikey;
      nlist_t       *ids;
      nlistidx_t    iiteridx;

      fprintf (stderr, "-- %s\n", skey);

      ids = slistGetList (itunes->playlists, skey);
      nlistStartIterator (ids, &iiteridx);
      while ((ikey = nlistIterateKey (ids, &iiteridx)) >= 0) {
        fprintf (stderr, " %" PRId32, ikey);
      }
      fprintf (stderr, "\n");
    }
  }
#endif

  return itunes;
}

void
itunesFree (itunes_t *itunes)
{
  if (itunes == NULL) {
    return;
  }

  datafileFree (itunes->starsdf);
  datafileFree (itunes->fieldsdf);
  nlistFree (itunes->fields);
  nlistFree (itunes->songbyidx);
  slistFree (itunes->songbyname);
  slistFree (itunes->playlists);
  slistFree (itunes->itunesAvailFields);
  dataFree (itunes);
}

int
itunesGetStars (itunes_t *itunes, int idx)
{
  return nlistGetNum (itunes->stars, idx);
}

void
itunesSetStars (itunes_t *itunes, int idx, int value)
{
  nlistSetNum (itunes->stars, idx, value);
}

void
itunesSaveStars (itunes_t *itunes)
{
  datafileSave (itunes->starsdf, NULL, itunes->stars, DF_NO_OFFSET,
      datafileDistVersion (itunes->starsdf));
}

int
itunesGetField (itunes_t *itunes, int idx)
{
  return nlistGetNum (itunes->fields, idx);
}

void
itunesSetField (itunes_t *itunes, int idx, int value)
{
  nlistSetNum (itunes->fields, idx, value);
}

void
itunesSaveFields (itunes_t *itunes)
{
  int         key;
  int         tval;
  nlistidx_t  iteridx;
  slist_t     *newlist;

  newlist = slistAlloc ("itunes-fields", LIST_ORDERED, NULL);
  nlistStartIterator (itunes->fields, &iteridx);
  while ((key = nlistIterateKey (itunes->fields, &iteridx)) >= 0) {
    tval = nlistGetNum (itunes->fields, key);
    if (tval > 0) {
      slistSetNum (newlist, tagdefs [key].tag, 0);
    }
  }

  datafileSave (itunes->fieldsdf, NULL, newlist, DF_NO_OFFSET,
      datafileDistVersion (itunes->fieldsdf));
  slistFree (newlist);
}

void
itunesStartIterateAvailFields (itunes_t *itunes)
{
  if (itunes == NULL) {
    return;
  }
  slistStartIterator (itunes->itunesAvailFields, &itunes->availiteridx);
}

const char *
itunesIterateAvailFields (itunes_t *itunes, int *val)
{
  const char  *skey;

  *val = -1;
  if (itunes == NULL) {
    return NULL;
  }
  skey = slistIterateKey (itunes->itunesAvailFields, &itunes->availiteridx);
  *val = slistGetNum (itunes->itunesAvailFields, skey);
  return skey;
}

bool
itunesParse (itunes_t *itunes)
{
  xmlparse_t          *xmlparse;
  const char          *fn;
  time_t              xmlts;

  if (! itunesConfigured ()) {
    logMsg (LOG_DBG, LOG_INFO, "itunesParse: itunes not configured");
    return false;
  }

  fn = bdjoptGetStr (OPT_M_ITUNES_XML_FILE);
  xmlts = fileopModTime (fn);
  if (xmlts <= itunes->lastparse) {
    return true;
  }

  xmlparse = xmlParseInitFile (fn, XMLPARSE_NONS);

  if (! itunesParseData (itunes, xmlparse, dataxpath)) {
    return false;
  }
  if (! itunesParsePlaylists (itunes, xmlparse, plxpath)) {
    return false;
  }

  xmlParseFree (xmlparse);
  itunes->lastparse = xmlts;
  return true;
}

nlist_t *
itunesGetSongData (itunes_t *itunes, nlistidx_t idx)
{
  nlist_t   *entry;

  if (itunes == NULL || itunes->songbyidx == NULL) {
    return NULL;
  }

  entry = nlistGetList (itunes->songbyidx, idx);
  return entry;
}

nlist_t *
itunesGetSongDataByName (itunes_t *itunes, const char *skey)
{
  nlistidx_t  idx;

  if (itunes == NULL || itunes->songbyname == NULL) {
    return NULL;
  }

  idx = slistGetNum (itunes->songbyname, skey);
  return itunesGetSongData (itunes, idx);
}

slist_t *
itunesGetPlaylistData (itunes_t *itunes, const char *skey)
{
  slist_t   *ids;

  if (itunes == NULL || itunes->playlists == NULL) {
    return NULL;
  }

  ids = slistGetList (itunes->playlists, skey);
  return ids;
}

void
itunesStartIteratePlaylists (itunes_t *itunes)
{
  if (itunes == NULL || itunes->playlists == NULL) {
    return;
  }

  nlistStartIterator (itunes->playlists, &itunes->pliteridx);
}

const char *
itunesIteratePlaylists (itunes_t *itunes)
{
  const char    *skey;

  if (itunes == NULL || itunes->playlists == NULL) {
    return NULL;
  }

  skey = slistIterateKey (itunes->playlists, &itunes->pliteridx);
  return skey;
}

/* internal routines */

static bool
itunesParseData (itunes_t *itunes, xmlparse_t *xmlparse,
    const char *xpathExpr)
{
  slist_t     *rawdata;
  slistidx_t  iteridx;
  const char  *key;
  const char  *val;
  nlistidx_t  lastval = -1;
  nlist_t     *entry = NULL;
  bool        ratingset = false;
  bool        skip = false;

  rawdata = slistAlloc ("itunes-data-raw", LIST_UNORDERED, NULL);
  if (! itunesParseXPath (xmlparse, xpathExpr, rawdata, datamainxpath)) {
    slistFree (rawdata);
    return false;
  }

  slistFree (itunes->songbyidx);
  slistFree (itunes->songbyname);
  itunes->songbyidx = nlistAlloc ("itunes-songs-by-idx", LIST_ORDERED, NULL);
  itunes->songbyname = slistAlloc ("itunes-song-by-name", LIST_ORDERED, NULL);

  slistStartIterator (rawdata, &iteridx);
  while ((key = slistIterateKey (rawdata, &iteridx)) != NULL) {
    val = slistGetStr (rawdata, key);
    if (val == NULL) {
      continue;
    }

    if (strcmp (key, "") == 0) {
    } else if (strcmp (key, "Track ID") == 0) {
      if (skip) {
        nlistFree (entry);
      }
      if (! skip && lastval >= 0) {
        const char  *tval;

        tval = nlistGetStr (entry, TAG_URI);
        if (tval != NULL) {
          nlistSetList (itunes->songbyidx, lastval, entry);
          slistSetNum (itunes->songbyname, tval, lastval);
        } else {
          nlistFree (entry);
        }
      }
      entry = nlistAlloc ("itunes-entry", LIST_ORDERED, NULL);
      lastval = atol (val);
      ratingset = false;
      skip = false;
      continue;
    }
    if (skip) {
      continue;
    }

    if (strcmp (key, "Movie") == 0 ||
        strcmp (key, "Has Video") == 0) {
      skip = true;
      logMsg (LOG_DBG, LOG_ITUNES, "song: skip-video");
      continue;
    }

    if (strcmp (key, "Rating Computed") == 0) {
      if (atoi (val) == 1) {
        /* set the dance rating to unrated */
        nlistSetNum (entry, TAG_DANCERATING, 0);
        logMsg (LOG_DBG, LOG_ITUNES, "song: unrated");
        ratingset = true;
      }
    } else if (strcmp (key, "Loved") == 0 ||
        strcmp (key, "Disliked") == 0) {
      if (atoi (val) == 1) {
        datafileconv_t  conv;

        conv.invt = VALUE_STR;
        if (strcmp (key, "Loved") == 0) {
          conv.str = "pinkheart";
        }
        if (strcmp (key, "Disliked") == 0) {
          conv.str = "brokenheart";
        }
        songFavoriteConv (&conv);
        nlistSetNum (entry, TAG_FAVORITE, conv.num);
        logMsg (LOG_DBG, LOG_ITUNES, "song: %s %" PRId64 "",
            tagdefs [TAG_FAVORITE].tag, conv.num);
      }
    } else {
      int   tagidx;

      /* if the key is in the list and has an associated tag */
      tagidx = slistGetNum (itunes->itunesAvailFields, key);
      if (tagidx < 0) {
        continue;
      }

      if (tagidx == TAG_URI) {
        bool    ok = false;
        int     offset = 0;

        if (strncmp (val, ITUNES_LOCALHOST, strlen (ITUNES_LOCALHOST)) == 0) {
          offset = strlen (ITUNES_LOCALHOST);
          ok = true;
        } else if (strncmp (val, ITUNES_LOCAL, strlen (ITUNES_LOCAL)) == 0) {
          offset = strlen (ITUNES_LOCAL);
          ok = true;
        } else {
          skip = true;
        }

        if (ok) {
          char        *nstr;

          val += offset;
          /* not sure that the string is decomposed */
          nstr = g_uri_unescape_string (val, NULL);
          mdextalloc (nstr);

          val = audiosrcRelativePath (nstr, 0);
          nlistSetStr (entry, tagidx, val);
          logMsg (LOG_DBG, LOG_ITUNES, "song: %s %s", tagdefs [tagidx].tag, val);
          mdfree (nstr);    // allocated by glib
        }
      } else if (tagidx == TAG_DBADDDATE) {
        time_t    tmval;

        /* 2023-01-03T18:34:58Z */
        tmval = tmutilStringToUTC (val, "%FT%TZ");
        nlistSetNum (entry, tagidx, tmval);
        logMsg (LOG_DBG, LOG_ITUNES, "song: %s %" PRIu64, tagdefs [tagidx].tag, (uint64_t) tmval);
      } else if (tagidx == TAG_DANCERATING) {
        int   ratingidx;
        int   tval;

        if (ratingset) {
          continue;
        }

        tval = atoi (val);
        tval = tval / 10 - 1;
        ratingidx = nlistGetNum (itunes->stars, tval);
        nlistSetNum (entry, tagidx, ratingidx);
        logMsg (LOG_DBG, LOG_ITUNES, "song: %s %" PRId32, tagdefs [tagidx].tag, ratingidx);
      } else if (tagidx == TAG_GENRE) {
        datafileconv_t  conv;

        conv.invt = VALUE_STR;
        conv.str = val;
        genreConv (&conv);
        nlistSetNum (entry, tagidx, conv.num);
        logMsg (LOG_DBG, LOG_ITUNES, "song: %s %" PRId64, tagdefs [tagidx].tag, conv.num);
      } else {
        /* start time and stop time are already in the correct format (ms) */
        if (tagdefs [tagidx].valueType == VALUE_NUM) {
          nlistSetNum (entry, tagidx, atoll (val));
        }
        if (tagdefs [tagidx].valueType == VALUE_STR) {
          nlistSetStr (entry, tagidx, val);
        }
        logMsg (LOG_DBG, LOG_ITUNES, "song: %s %s", tagdefs [tagidx].tag, val);
      }
    }
  }

  if (skip) {
    nlistFree (entry);
  }

  if (! skip) {
    const char  *tval;

    tval = nlistGetStr (entry, TAG_URI);
    if (tval != NULL) {
      nlistSetList (itunes->songbyidx, lastval, entry);
      slistSetNum (itunes->songbyname, tval, lastval);
    } else {
      nlistFree (entry);
    }

    /* the itunes xml file does not appear to save the show-movement flag */
    /* use our own setting to determine whether to use work/movement */
    songutilTitleFromWorkMovement (entry);
  }

  slistFree (rawdata);
  return true;
}

static bool
itunesParsePlaylists (itunes_t *itunes, xmlparse_t *xmlparse,
    const char *xpathExpr)
{
  slist_t     *rawdata = NULL;
  slistidx_t  iteridx;
  const char  *key;
  const char  *val;
  char        keepname [1000];
  nlist_t     *ids = NULL;
  bool        skip = false;
  bool        ismaster = false;

  rawdata = slistAlloc ("itunes-pl-raw", LIST_UNORDERED, NULL);
  if (! itunesParseXPath (xmlparse, xpathExpr, rawdata, plmainxpath)) {
    slistFree (rawdata);
    return false;
  }

  *keepname = '\0';
  slistFree (itunes->playlists);
  itunes->playlists = slistAlloc ("itunes-playlists", LIST_ORDERED, NULL);

  slistStartIterator (rawdata, &iteridx);
  while ((key = slistIterateKey (rawdata, &iteridx)) != NULL) {
    val = slistGetStr (rawdata, key);

    if (strcmp (key, "Master") == 0) {
      ismaster = true;
      continue;
    }
    if (strcmp (key, "Playlist ID") == 0) {
      if (! skip && ids != NULL && nlistGetCount (ids) == 0) {
        slistFree (ids);
        ids = NULL;
      }
      if (skip == false && *keepname && ids != NULL) {
        slistSetList (itunes->playlists, keepname, ids);
        /* it is possible for a playlist to not have an item list */
        ids = NULL;
      }
      skip = false;
      if (ismaster) {
        skip = true;
        ismaster = false;
      }
      continue;
    }
    if (strcmp (key, "Playlist Items") == 0) {
      ids = NULL;
      if (! skip) {
        ids = nlistAlloc ("itunes-pl-ids", LIST_UNORDERED, NULL);
        skip = false;
      }
      continue;
    }
    if (skip) {
      continue;
    }

    if (strcmp (key, "Distinguished Kind") == 0 ||
        strcmp (key, "Smart Info") == 0) {
      logMsg (LOG_DBG, LOG_ITUNES, "pl: skip");
      skip = true;
    } else if (strcmp (key, "Name") == 0) {
      stpecpy (keepname, keepname + sizeof (keepname), val);
      logMsg (LOG_DBG, LOG_ITUNES, "pl-name: %s", keepname);
    } else if (strcmp (key, "Track ID") == 0) {
      slist_t   *entry;
      int32_t   tval;

      tval = atoll (val);
      entry = nlistGetList (itunes->songbyidx, tval);
      if (entry != NULL) {
        nlistSetNum (ids, tval, 1);
        logMsg (LOG_DBG, LOG_ITUNES, "pl: %s %" PRId32, keepname, tval);
      }
    }
  }

  if (! skip && ids != NULL && nlistGetCount (ids) == 0) {
    slistFree (ids);
    ids = NULL;
  }
  if (skip == false && *keepname && ids != NULL) {
    slistSetList (itunes->playlists, keepname, ids);
  }

  slistFree (rawdata);
  return true;
}

static bool
itunesParseXPath (xmlparse_t *xmlparse, const char *xpathExpr,
    slist_t *rawdata, const char *mainxpath)
{
  char          lastkey [50];
  bool          valset = true;
  ilist_t       *tlist;
  ilistidx_t    iteridx;
  ilistidx_t    key;

  tlist = xmlParseGetList (xmlparse, xpathExpr, NULL);

  /* itunes just dumps everything into a dict structure. */
  /* poor use of xml */
  ilistStartIterator (tlist, &iteridx);
  while ((key = ilistIterateKey (tlist, &iteridx)) >= 0) {
    const char *val;
    const char *nm;

    val = ilistGetStr (tlist, key, XMLPARSE_VAL);
    nm = ilistGetStr (tlist, key, XMLPARSE_NM);
    logMsg (LOG_DBG, LOG_ITUNES, "xml: raw: %s %s", nm, val);
    // fprintf (stderr, "i: %s %s\n", nm, val);
    if (strcmp (nm, "key") == 0) {
      if (! valset) {
        slistSetStr (rawdata, lastkey, "0");
        // fprintf (stderr, "  0: %s 0\n", lastkey);
      }
      stpecpy (lastkey, lastkey + sizeof (lastkey), val);
      valset = false;
    }
    if (strcmp (nm, "integer") == 0 ||
        strcmp (nm, "string") == 0 ||
        strcmp (nm, "date") == 0) {
      slistSetStr (rawdata, lastkey, (const char *) val);
      // fprintf (stderr, "  s: %s\n", val);
      valset = true;
    }
    if (strcmp (nm, "true") == 0) {
      slistSetStr (rawdata, lastkey, (const char *) "1");
      // fprintf (stderr, "  b: %s\n", "1");
      valset = true;
    }
  }

  ilistFree (tlist);
  return true;
}
