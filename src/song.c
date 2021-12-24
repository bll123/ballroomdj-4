#include "bdjconfig.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "song.h"
#include "list.h"
#include "bdjstring.h"
#include "tagdef.h"
#include "log.h"

song_t *
songAlloc (void)
{
  song_t    *song;

  song = malloc (sizeof (song_t));
  assert (song != NULL);
  song->songInfo = vlistAlloc (KEY_LONG, LIST_UNORDERED,
      istringCompare, free, free);
  return song;
}

void
songFree (void *tsong)
{
  song_t  *song = (song_t *) tsong;
  if (song != NULL) {
    if (song->songInfo != NULL) {
      vlistFree (song->songInfo);
    }
    free (song);
  }
}

char *
songGet (song_t *song, long idx) {
  listkey_t     key;
  key.key = idx;
  char *value = vlistGetData (song->songInfo, key);
  return value;
}

void
songSetAll (song_t *song, char *data[], size_t count)
{
  long          idx;
  listkey_t     lkey;

  for (size_t i = 0; i < count; i += 2) {
    idx = tagdefGetIdx (data[i]);
    if (idx < 0) {
      logVarMsg (LOG_ERR, "songSetAll: Unknown song key: %s\n", data[i]);
      continue;
    }

    lkey.key = idx;
    switch (tagdefs[idx].valuetype) {
      case VALUE_DOUBLE:
      {
        vlistSetDouble (song->songInfo, lkey, atof (data[i+1]));
        break;
      }
      case VALUE_LONG:
      {
        vlistSetLong (song->songInfo, lkey, atol (data[i+1]));
        break;
      }
      case VALUE_DATA:
      {
        vlistSetData (song->songInfo, lkey, strdup (data[i+1]));
        break;
      }
    }
  }
  vlistSort (song->songInfo);
}

void
songSetLong (song_t *song, long tagidx, long value)
{
  listkey_t       lkey;

  lkey.key = tagidx;
  vlistSetLong (song->songInfo, lkey, value);
}
