/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "bdj4.h"
#include "datafile.h"
#include "dispsel.h"
#include "fileop.h"
#include "mdebug.h"
#include "pathbld.h"
#include "slist.h"
#include "tagdef.h"

typedef struct dispsel {
  char          *name [DISP_SEL_MAX];
  datafile_t    *df [DISP_SEL_MAX];
  slist_t       *dispsel [DISP_SEL_MAX];
} dispsel_t;

typedef struct dispselinfo {
  const char    *fname;
  int           type;
} dispselinfo_t;

static dispselinfo_t dispselmap [DISP_SEL_MAX] = {
  [DISP_SEL_AUDIOID]        = { "ds-audioid", DISP_SEL_LOAD_MANAGE },
  [DISP_SEL_AUDIOID_LIST]   = { "ds-audioid-list", DISP_SEL_LOAD_MANAGE },
  [DISP_SEL_HISTORY]        = { "ds-history", DISP_SEL_LOAD_PLAYER },
  [DISP_SEL_MARQUEE]        = { "ds-marquee", DISP_SEL_LOAD_MARQUEE },
  [DISP_SEL_MM]             = { "ds-mm", DISP_SEL_LOAD_MANAGE },
  [DISP_SEL_MUSICQ]         = { "ds-musicq", DISP_SEL_LOAD_PLAYER },
  [DISP_SEL_CURRSONG]      = { "ds-currsong", DISP_SEL_LOAD_PLAYER | DISP_SEL_LOAD_MANAGE},
  [DISP_SEL_REQUEST]        = { "ds-request", DISP_SEL_LOAD_PLAYER },
  [DISP_SEL_SBS_SONGLIST]   = { "ds-sbssonglist", DISP_SEL_LOAD_MANAGE },
  [DISP_SEL_SBS_SONGSEL]    = { "ds-sbssongsel", DISP_SEL_LOAD_MANAGE },
  [DISP_SEL_SONGEDIT_A]     = { "ds-songedit-a", DISP_SEL_LOAD_MANAGE },
  [DISP_SEL_SONGEDIT_B]     = { "ds-songedit-b", DISP_SEL_LOAD_MANAGE },
  [DISP_SEL_SONGEDIT_C]     = { "ds-songedit-c", DISP_SEL_LOAD_MANAGE },
  [DISP_SEL_SONGLIST]       = { "ds-songlist", DISP_SEL_LOAD_MANAGE },
  [DISP_SEL_SONGSEL]        = { "ds-songsel", DISP_SEL_LOAD_MANAGE },
};

static void dispselCreateList (dispsel_t *dispsel, slist_t *tlist, int selidx);

dispsel_t *
dispselAlloc (int loadtype)
{
  dispsel_t     *dispsel;
  slist_t       *tlist;
  char          fn [MAXPATHLEN];


  dispsel = mdmalloc (sizeof (dispsel_t));

  for (dispselsel_t i = 0; i < DISP_SEL_MAX; ++i) {
    dispsel->dispsel [i] = NULL;
    dispsel->df [i] = NULL;
    dispsel->name [i] = NULL;

    if (loadtype != DISP_SEL_LOAD_ALL &&
        (dispselmap [i].type & loadtype) != loadtype) {
      continue;
    }

    pathbldMakePath (fn, sizeof (fn),
        dispselmap [i].fname, BDJ4_CONFIG_EXT, PATHBLD_MP_DREL_DATA | PATHBLD_MP_USEIDX);
    if (! fileopFileExists (fn)) {
      fprintf (stderr, "%s does not exist\n", fn);
      continue;
    }
    dispsel->name [i] = mdstrdup (fn);

    dispsel->df [i] = datafileAllocParse (dispselmap [i].fname,
        DFTYPE_LIST, fn, NULL, 0, DF_NO_OFFSET, NULL);
    tlist = datafileGetList (dispsel->df [i]);

    dispselCreateList (dispsel, tlist, i);
  }

  return dispsel;
}

void
dispselFree (dispsel_t *dispsel)
{
  if (dispsel != NULL) {
    for (dispselsel_t i = 0; i < DISP_SEL_MAX; ++i) {
      dataFree (dispsel->name [i]);
      datafileFree (dispsel->df [i]);
      slistFree (dispsel->dispsel [i]);
    }
    mdfree (dispsel);
  }
}

slist_t *
dispselGetList (dispsel_t *dispsel, dispselsel_t idx)
{
  if (dispsel == NULL) {
    return NULL;
  }
  if (idx >= DISP_SEL_MAX) {
    return NULL;
  }

  return dispsel->dispsel [idx];
}

void
dispselSave (dispsel_t *dispsel, dispselsel_t idx, slist_t *list)
{
  if (dispsel == NULL) {
    return;
  }
  if (idx >= DISP_SEL_MAX) {
    return;
  }

  datafileSave (dispsel->df [idx], NULL, list, DF_NO_OFFSET,
      datafileDistVersion (dispsel->df [idx]));
  dispselCreateList (dispsel, list, idx);
}

/* internal routines */

static void
dispselCreateList (dispsel_t *dispsel, slist_t *tlist, int selidx)
{
  slist_t       *tsellist;
  slistidx_t    iteridx;
  int           tagkey;
  const char    *keystr;
  char          tbuff [80];

  slistFree (dispsel->dispsel [selidx]);
  dispsel->dispsel [selidx] = NULL;

  snprintf (tbuff, sizeof (tbuff), "dispsel-%s", dispsel->name [selidx]);
  tsellist = slistAlloc (tbuff, LIST_UNORDERED, NULL);
  slistStartIterator (tlist, &iteridx);
  while ((keystr = slistIterateKey (tlist, &iteridx)) != NULL) {
    tagkey = tagdefLookup (keystr);
    if (tagkey >= 0 && tagkey < TAG_KEY_MAX) {
      slistSetNum (tsellist, tagdefs [tagkey].displayname, tagkey);
    }
  }
  dispsel->dispsel [selidx] = tsellist;
}
