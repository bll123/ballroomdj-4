#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "bdj4.h"
#include "datafile.h"
#include "dispsel.h"
#include "fileop.h"
#include "pathbld.h"
#include "slist.h"
#include "tagdef.h"

typedef struct {
  dispselsel_t  seltype;
  char          *fname;
} dispselmap_t;

dispselmap_t dispselmap [DISP_SEL_MAX] = {
  { DISP_SEL_MM,          "ds-mm" },
  { DISP_SEL_MUSICQ,      "ds-musicq" },
  { DISP_SEL_REQUEST,     "ds-request" },
  { DISP_SEL_SONGEDIT_A,  "ds-songedit-a" },
  { DISP_SEL_SONGEDIT_B,  "ds-songedit-b" },
  { DISP_SEL_SONGLIST,    "ds-songlist" },
//  { DISP_SEL_SONGSEL,     "ds-songsel" },
};


dispsel_t *
dispselAlloc (void)
{
  dispsel_t     *dispsel;
  slist_t       *tlist;
  slist_t       *tsellist;
  datafile_t    *df;
  char          fn [MAXPATHLEN];
  slistidx_t    iteridx;
  tagdefkey_t   tagkey;
  char          *keystr;


  dispsel = malloc (sizeof (dispsel_t));
  assert (dispsel != NULL);

  for (dispselsel_t i = 0; i < DISP_SEL_MAX; ++i) {
    dispsel->dispsel [i] = NULL;

    pathbldMakePath (fn, sizeof (fn), "profiles",
        dispselmap [i].fname, ".txt", PATHBLD_MP_USEIDX);
    if (! fileopFileExists (fn)) {
      fprintf (stderr, "%s does not exist\n", fn);
      return NULL;
    }

    df = datafileAllocParse ("dispsel", DFTYPE_LIST, fn,
        NULL, 0, DATAFILE_NO_LOOKUP);
    tlist = datafileGetList (df);

    tsellist = slistAlloc ("dispsel-disp", LIST_UNORDERED, NULL);
    slistStartIterator (tlist, &iteridx);
    while ((keystr = slistIterateKey (tlist, &iteridx)) != NULL) {
      tagkey = tagdefLookup (keystr);
      if (tagkey >= 0 && tagkey < TAG_KEY_MAX) {
        slistSetNum (tsellist, tagdefs [tagkey].displayname, tagkey);
      }
    }
    datafileFree (df);
    dispsel->dispsel [i] = tsellist;
  }

  return dispsel;
}

void
dispselFree (dispsel_t *dispsel)
{
  if (dispsel != NULL) {
    for (dispselsel_t i = 0; i < DISP_SEL_MAX; ++i) {
      if (dispsel->dispsel [i] != NULL) {
        slistFree (dispsel->dispsel [i]);
      }
    }
    free (dispsel);
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
