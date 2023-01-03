/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <assert.h>

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include "bdj4.h"
#include "itunes.h"
#include "bdjopt.h"
#include "datafile.h"
#include "fileop.h"
#include "log.h"
#include "nlist.h"
#include "pathbld.h"
#include "rating.h"
#include "songfav.h"
#include "slist.h"
#include "tagdef.h"

static const xmlChar * datamainxpath = (const xmlChar *)
      "/plist/dict/dict/key";
static const xmlChar * dataxpath = (const xmlChar *)
      "/plist/dict/dict/dict/key|"
      "/plist/dict/dict/dict/integer|"
      "/plist/dict/dict/dict/string|"
      "/plist/dict/dict/dict/date|"
      "/plist/dict/dict/dict/true|"
      "/plist/dict/dict/dict/false";
static const xmlChar * plmainxpath = (const xmlChar *)
      "/plist/dict/array/dict/key";
static const xmlChar * plxpath = (const xmlChar *)
      "/plist/dict/array/dict/key|"
      "/plist/dict/array/dict/integer|"
      "/plist/dict/array/dict/string|"
      "/plist/dict/array/dict/array/dict/key|"
      "/plist/dict/array/dict/array/dict/integer";
static const char *ITUNES_LOCALHOST = "file://localhost/";
static const char *ITUNES_LOCAL = "file://";
static const char *ITUNES_HTTP = "http://";
static const char *ITUNES_HTTP_CORRUPT = "ttp://";

typedef struct itunes {
  datafile_t    *starsdf;
  nlist_t       *stars;
  datafile_t    *fieldsdf;
  nlist_t       *fields;
  nlist_t       *songs;
  slist_t       *playlists;
  slist_t       *itunesFields;
} itunes_t;

/* must be sorted in ascii order */
static datafilekey_t starsdfkeys [ITUNES_STARS_MAX] = {
  { "10",   ITUNES_STARS_10,      VALUE_NUM,  ratingConv, -1 },
  { "100",  ITUNES_STARS_100,     VALUE_NUM,  ratingConv, -1 },
  { "20",   ITUNES_STARS_20,      VALUE_NUM,  ratingConv, -1 },
  { "30",   ITUNES_STARS_30,      VALUE_NUM,  ratingConv, -1 },
  { "40",   ITUNES_STARS_40,      VALUE_NUM,  ratingConv, -1 },
  { "50",   ITUNES_STARS_50,      VALUE_NUM,  ratingConv, -1 },
  { "60",   ITUNES_STARS_60,      VALUE_NUM,  ratingConv, -1 },
  { "70",   ITUNES_STARS_70,      VALUE_NUM,  ratingConv, -1 },
  { "80",   ITUNES_STARS_80,      VALUE_NUM,  ratingConv, -1 },
  { "90",   ITUNES_STARS_90,      VALUE_NUM,  ratingConv, -1 },
};

static bool itunesParseData (itunes_t *itunes, xmlXPathContextPtr xpathCtx, const xmlChar *xpathExpr);
static bool itunesParsePlaylists (itunes_t *itunes, xmlXPathContextPtr xpathCtx, const xmlChar *xpathExpr);
static bool itunesParseXPath (xmlXPathContextPtr xpathCtx, const xmlChar *xpathExpr, slist_t *rawdata, const xmlChar *mainxpath);

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

itunes_t *
itunesAlloc (void)
{
  itunes_t    *itunes;
  char        tbuff [MAXPATHLEN];
  slist_t     *tlist;
  slistidx_t  iteridx;
  char        *key;

  itunes = malloc (sizeof (itunes_t));

  pathbldMakePath (tbuff, sizeof (tbuff),
      ITUNES_STARS_FN, BDJ4_CONFIG_EXT, PATHBLD_MP_DREL_DATA);
  itunes->starsdf = datafileAllocParse (ITUNES_STARS_FN,
      DFTYPE_KEY_VAL, tbuff, starsdfkeys, ITUNES_STARS_MAX);
  itunes->stars = datafileGetList (itunes->starsdf);

  pathbldMakePath (tbuff, sizeof (tbuff),
      ITUNES_FIELDS_FN, BDJ4_CONFIG_EXT, PATHBLD_MP_DREL_DATA);
  itunes->fieldsdf = datafileAllocParse (ITUNES_FIELDS_FN,
      DFTYPE_LIST, tbuff, NULL, 0);
  tlist = datafileGetList (itunes->fieldsdf);

  itunes->fields = nlistAlloc ("itunes-fields", LIST_ORDERED, NULL);
  nlistSetSize (itunes->fields, slistGetCount (tlist));

  slistStartIterator (tlist, &iteridx);
  while ((key = slistIterateKey (tlist, &iteridx)) != NULL) {
    int     val;

    val = tagdefLookup (key);
    nlistSetNum (itunes->fields, val, 1);
  }

  itunes->itunesFields = slistAlloc ("itunes-fields", LIST_ORDERED, NULL);
  for (int i = 0; i < TAG_KEY_MAX; ++i) {
    if (tagdefs [i].itunesName == NULL) {
      continue;
    }
    slistSetNum (itunes->itunesFields, tagdefs [i].itunesName, i);
  }

  itunes->songs = NULL;
  itunes->playlists = NULL;

  /* for debugging */
#if 1
  {
    nlistidx_t    iteridx;
    long          nkey;
    char          *skey;

    bdjoptSetStr (OPT_M_DIR_ITUNES_MEDIA, "/home/music/m");
//    bdjoptSetStr (OPT_M_ITUNES_XML_FILE, "/home/bll/vbox_shared/iTunes Music Library.xml");
    bdjoptSetStr (OPT_M_ITUNES_XML_FILE, "/home/bll/s/bdj-test-files/iTunes Library.xml");
    itunesParse (itunes);
    nlistStartIterator (itunes->songs, &iteridx);
    while ((nkey = nlistIterateKey (itunes->songs, &iteridx)) >= 0) {
      int           ekey;
      nlist_t       *entry;
      nlistidx_t    eiteridx;

      entry = nlistGetList (itunes->songs, nkey);
      fprintf (stderr, "-- %ld %s \n", nkey, nlistGetStr (entry, TAG_FILE));

      nlistStartIterator (entry, &eiteridx);
      while ((ekey = nlistIterateKey (entry, &eiteridx)) >= 0) {
        if (ekey == TAG_FILE) {
          continue;
        }
        if (ekey == TAG_FAVORITE || ekey == TAG_DANCERATING) {
          fprintf (stderr, "  %s %ld\n", tagdefs [ekey].tag, nlistGetNum (entry, ekey));
        } else {
          fprintf (stderr, "  %s %s\n", tagdefs [ekey].tag, nlistGetStr (entry, ekey));
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
        fprintf (stderr, " %d", ikey);
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
  nlistFree (itunes->songs);
  slistFree (itunes->playlists);
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
  char        tbuff [MAXPATHLEN];

  pathbldMakePath (tbuff, sizeof (tbuff),
      ITUNES_STARS_FN, BDJ4_CONFIG_EXT, PATHBLD_MP_DREL_DATA);
  datafileSaveKeyVal ("itunes-stars", tbuff, starsdfkeys,
      ITUNES_STARS_MAX, itunes->stars, 0);
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
  char        tbuff [MAXPATHLEN];
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

  pathbldMakePath (tbuff, sizeof (tbuff),
      ITUNES_FIELDS_FN, BDJ4_CONFIG_EXT, PATHBLD_MP_DREL_DATA);
  datafileSaveList ("itunes-fields", tbuff, newlist);
}

bool
itunesParse (itunes_t *itunes)
{
  xmlDocPtr           doc;
  xmlXPathContextPtr  xpathCtx;
  const char          *fn;

  if (! itunesConfigured ()) {
    logMsg (LOG_DBG, LOG_MAIN, "itunesParse: itunes not configured");
    return false;
  }

  xmlInitParser ();

  fn = bdjoptGetStr (OPT_M_ITUNES_XML_FILE);
  doc = xmlParseFile (fn);
  if (doc == NULL)  {
    logMsg (LOG_DBG, LOG_MAIN, "itunesParse: unable to parse %s", fn);
    xmlCleanupParser ();
    return false;
  }

  xpathCtx = xmlXPathNewContext (doc);
  if (xpathCtx == NULL)  {
    logMsg (LOG_DBG, LOG_MAIN, "itunesParse: unable to create xpath context");
    xmlFreeDoc (doc);
    xmlCleanupParser ();
    return false;
  }

  if (! itunesParseData (itunes, xpathCtx, dataxpath)) {
    xmlXPathFreeContext (xpathCtx);
    xmlFreeDoc (doc);
    xmlCleanupParser ();
    return false;
  }
  if (! itunesParsePlaylists (itunes, xpathCtx, plxpath)) {
    xmlXPathFreeContext (xpathCtx);
    xmlFreeDoc (doc);
    xmlCleanupParser ();
    return false;
  }

  xmlXPathFreeContext (xpathCtx);
  xmlFreeDoc (doc);
  xmlCleanupParser ();
  // xmlMemoryDump ();
  return true;
}

/* internal routines */

static bool
itunesParseData (itunes_t *itunes, xmlXPathContextPtr xpathCtx,
    const xmlChar *xpathExpr)
{
  slist_t     *rawdata;
  slistidx_t  iteridx;
  char        *key;
  char        *val;
  long        lastval = -1;
  nlist_t     *entry = NULL;
  bool        ratingset = false;
  bool        skip = false;

  rawdata = slistAlloc ("itunes-data-raw", LIST_UNORDERED, NULL);
  if (! itunesParseXPath (xpathCtx, xpathExpr, rawdata, datamainxpath)) {
    slistFree (rawdata);
    return false;
  }

  slistFree (itunes->songs);
  itunes->songs = nlistAlloc ("itunes-songs", LIST_ORDERED, NULL);

  slistStartIterator (rawdata, &iteridx);
  while ((key = slistIterateKey (rawdata, &iteridx)) != NULL) {
    val = slistGetStr (rawdata, key);
    if (val == NULL) {
      continue;
    }

    if (strcmp (key, "Track ID") == 0) {
      if (skip) {
        nlistFree (entry);
      }
      if (! skip && lastval >= 0) {
        char    *tval;

        tval = nlistGetStr (entry, TAG_FILE);
        if (tval != NULL) {
          nlistSetList (itunes->songs, lastval, entry);
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
      continue;
    }

    if (strcmp (key, "Rating Computed") == 0) {
      if (atoi (val) == 1) {
        /* set the dance rating to unrated */
        nlistSetNum (entry, TAG_DANCERATING, 0);
        ratingset = true;
      }
    } else if (strcmp (key, "Loved") == 0 ||
        strcmp (key, "Disliked") == 0) {
      if (atoi (val) == 1) {
        datafileconv_t  conv;

        conv.allocated = false;
        conv.valuetype = VALUE_STR;
        if (strcmp (key, "Loved") == 0) {
          conv.str = "pinkheart";
        }
        if (strcmp (key, "Disliked") == 0) {
          conv.str = "brokenheart";
        }
        songFavoriteConv (&conv);
        nlistSetNum (entry, TAG_FAVORITE, conv.num);
      }
    } else {
      int   tagidx;

      /* if the key is in the list and has an associated tag */
      tagidx = slistGetNum (itunes->itunesFields, key);
      if (tagidx < 0) {
        continue;
      }
      if (tagidx == TAG_FILE) {
        if (strncmp (val, ITUNES_HTTP, strlen (ITUNES_HTTP)) == 0 ||
            strncmp (val, ITUNES_HTTP_CORRUPT, strlen (ITUNES_HTTP_CORRUPT)) == 0) {
          skip = true;
          nlistSetStr (entry, tagidx, val);
        }
        if (strncmp (val, ITUNES_LOCALHOST, strlen (ITUNES_LOCALHOST)) == 0) {
          nlistSetStr (entry, tagidx, val + strlen (ITUNES_LOCALHOST));
        }
        if (strncmp (val, ITUNES_LOCAL, strlen (ITUNES_LOCAL)) == 0) {
          nlistSetStr (entry, tagidx, val + strlen (ITUNES_LOCAL));
        }
      } else if (tagidx == TAG_DBADDDATE) {
        char    t [200];
        char    *p;

        strlcpy (t, val, sizeof (t));
        p = strchr (t, 'T');
        if (p != NULL) {
          *p = '\0';
        }
        nlistSetStr (entry, tagidx, t);
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
      } else {
        nlistSetStr (entry, tagidx, val);
      }
    }
  }
  if (skip) {
    nlistFree (entry);
  }

  slistFree (rawdata);
  return true;
}

static bool
itunesParsePlaylists (itunes_t *itunes, xmlXPathContextPtr xpathCtx,
    const xmlChar *xpathExpr)
{
  slist_t     *rawdata = NULL;
  slistidx_t  iteridx;
  char        *key;
  char        *val;
  char        keepname [1000];
  nlist_t     *ids = NULL;
  bool        skip = false;
  bool        ismaster = false;

  rawdata = slistAlloc ("itunes-pl-raw", LIST_UNORDERED, NULL);
  if (! itunesParseXPath (xpathCtx, xpathExpr, rawdata, plmainxpath)) {
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
fprintf (stderr, "%s free\n", keepname);
        ids = NULL;
      }
      if (skip == false && *keepname && ids != NULL) {
fprintf (stderr, "%s keep\n", keepname);
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
fprintf (stderr, "%s alloc\n", keepname);
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
      skip = true;
    } else if (strcmp (key, "Name") == 0) {
      strlcpy (keepname, val, sizeof (keepname));
    } else if (strcmp (key, "Track ID") == 0) {
      slist_t   *entry;
      long      tval;

      tval = atol (val);
      entry = nlistGetList (itunes->songs, tval);
      if (entry != NULL) {
        nlistSetNum (ids, tval, 1);
      }
    }
  }

  if (skip == false && *keepname && ids != NULL) {
    slistSetList (itunes->playlists, keepname, ids);
  }

  slistFree (rawdata);
  return true;
}

static bool
itunesParseXPath (xmlXPathContextPtr xpathCtx, const xmlChar *xpathExpr,
    slist_t *rawdata, const xmlChar *mainxpath)
{
  xmlXPathObjectPtr   xpathObj;
  xmlNodeSetPtr       nodes;
  xmlNodePtr          cur;
  int                 size;
  int                 i;
  xmlChar             *val = NULL;
  char                lastkey [50];
  bool                valset = true;

  xpathObj = xmlXPathEvalExpression (xpathExpr, xpathCtx);
  if (xpathObj == NULL)  {
    logMsg (LOG_DBG, LOG_IMPORTANT, "itunesParse: bad xpath expression");
    return false;
  }

  nodes = xpathObj->nodesetval;
  size = (nodes) ? nodes->nodeNr : 0;

  /* itunes just dumps everything into a dict structure. */
  /* poor use of xml */
  for (i = 0; i < size; ++i)  {
    if (nodes->nodeTab [i]->type == XML_ELEMENT_NODE)  {
      cur = nodes->nodeTab [i];
      val = xmlNodeGetContent (cur);
      logMsg (LOG_DBG, LOG_MAIN, "xml: %s %s", cur->name, val);
      // fprintf (stderr, "i: %s %s\n", cur->name, val);
      if (strcmp ((const char *) cur->name, "key") == 0) {
        if (! valset) {
          slistSetStr (rawdata, lastkey, "0");
          // fprintf (stderr, "  0: %s 0\n", lastkey);
        }
        strlcpy (lastkey, (const char *) val, sizeof (lastkey));
        valset = false;
      }
      if (strcmp ((const char *) cur->name, "integer") == 0 ||
          strcmp ((const char *) cur->name, "string") == 0 ||
          strcmp ((const char *) cur->name, "date") == 0) {
        slistSetStr (rawdata, lastkey, (const char *) val);
        // fprintf (stderr, "  s: %s\n", val);
        valset = true;
      }
      if (strcmp ((const char *) cur->name, "true") == 0) {
        slistSetStr (rawdata, lastkey, (const char *) "1");
        // fprintf (stderr, "  b: %s\n", "1");
        valset = true;
      }
      dataFree (val);
    }
  }

  xmlXPathFreeObject (xpathObj);
  return true;
}
