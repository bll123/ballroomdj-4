#include "config.h"

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
  song->songInfo = llistAlloc (LIST_UNORDERED, free);
  return song;
}

void
songFree (void *tsong)
{
  song_t  *song = (song_t *) tsong;
  if (song != NULL) {
    if (song->songInfo != NULL) {
      llistFree (song->songInfo);
    }
    free (song);
  }
}

char *
songGet (song_t *song, long idx) {
  char *value = llistGetData (song->songInfo, idx);
  return value;
}

long
songGetLong (song_t *song, long idx) {
  long value = llistGetLong (song->songInfo, idx);
  return value;
}

void
songSetAll (song_t *song, char *data[], size_t count)
{
  long          idx;

  for (size_t i = 0; i < count; i += 2) {
    idx = tagdefGetIdx (data [i]);
    if (idx < 0 || idx >= MAX_TAG_KEY) {
      logMsg (LOG_DBG, "Unknown tag key: %s\n", data[i]);
      continue;
    }

    switch (tagdefs [idx].valuetype) {
      case VALUE_DOUBLE:
      {
        llistSetDouble (song->songInfo, idx, atof (data[i+1]));
        break;
      }
      case VALUE_LONG:
      {
        llistSetLong (song->songInfo, idx, atol (data[i+1]));
        break;
      }
      case VALUE_DATA:
      {
        llistSetData (song->songInfo, idx, strdup (data[i+1]));
        break;
      }
    }
  }
  llistSort (song->songInfo);
}

void
songSetLong (song_t *song, long tagidx, long value)
{
  llistSetLong (song->songInfo, tagidx, value);
}
