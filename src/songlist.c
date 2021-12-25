#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "songlist.h"
#include "list.h"
#include "datafile.h"
#include "fileutil.h"
#include "bdjstring.h"

static void songlistInit (void);
static void songlistFreeInternal (songlist_t *);

static list_t     *slkeylookup = NULL;
static int        slInitialized = 0;

songlist_t *
songlistAlloc (char *fname)
{
  songlist_t      *sl;

  songlistInit ();
  sl = malloc (sizeof (songlist_t));
  assert (sl != NULL);
  sl->fname = NULL;
  sl->songs = NULL;
  songlistLoad (sl, fname);
  return sl;
}

void
songlistFree (songlist_t *sl)
{
  if (sl != NULL) {
    songlistFreeInternal (sl);
    free (sl);
  }
}

void
songlistCleanup (void)
{
  if (slInitialized) {
    listFree (slkeylookup);
    slkeylookup = NULL;
  }
}

void
songlistLoad (songlist_t *sl, char *fname)
{
  char          *data;
  char          **strdata;
  parseinfo_t   *pi;
  long          key = -1L;
  long          ikey;
  size_t        dataCount;
  listkey_t     lkey;
  list_t        *itemList = NULL;

  /* may be re-using the song list */
  songlistFreeInternal (sl);
  sl->fname = strdup (fname);

  data = fileReadAll (sl->fname);
  pi = parseInit ();
  dataCount = parseKeyValue (pi, data);
  strdata = parseGetData (pi);
  for (size_t i = 0; i < dataCount; i += 2) {
    if (strcmp (strdata [i], "count") == 0) {
      sl->songs = vlistAlloc (KEY_LONG, LIST_UNORDERED, NULL, NULL, listFree);
      vlistSetSize (sl->songs, (size_t) atol (strdata [i + 1]));
      continue;
    }
    if (strcmp (strdata [i], "KEY") == 0) {
      if (key >= 0) {
        if (key >= 0 && key < SL_KEY_MAX) {
          /* an item was previously allocated */
          lkey.key = key;
          vlistSetData (sl->songs, lkey, itemList);
        }
        key = -1L;
      }
      key = atol (strdata [i + 1]);
      itemList = vlistAlloc (KEY_LONG, LIST_ORDERED, NULL, NULL, listFree);
      continue;
    }
    lkey.name = strdata [i];
    ikey = vlistGetLong (slkeylookup, lkey);
    lkey.key = ikey;
    vlistSetLong (itemList, lkey, atol (strdata [i + 1]));
  }
  if (key >= 0) {
    /* an item was previously allocated */
    lkey.key = key;
    vlistSetData (sl->songs, lkey, itemList);
    key = -1L;
  }
  parseFree (pi);
}

/* internal routines */

static void
songlistInit (void)
{
  if (slInitialized) {
    return;
  }

  listkey_t         lkey;

  slkeylookup = vlistAlloc (KEY_STR, LIST_UNORDERED, stringCompare, NULL, NULL);
  vlistSetSize (slkeylookup, SL_KEY_MAX);
  lkey.name = "FILE";
  vlistSetLong (slkeylookup, lkey, SL_FILE);
  lkey.name = "TITLE";
  vlistSetLong (slkeylookup, lkey, SL_TITLE);
  lkey.name = "DANCE";
  vlistSetLong (slkeylookup, lkey, SL_DANCE);
  vlistSort (slkeylookup);
  slInitialized = 1;
}

static void
songlistFreeInternal (songlist_t *sl)
{
  if (sl != NULL) {
    if (sl->fname != NULL) {
      free (sl->fname);
      sl->fname = NULL;
    }
    if (sl->songs != NULL) {
      vlistFree (sl->songs);
      sl->songs = NULL;
    }
  }
}
