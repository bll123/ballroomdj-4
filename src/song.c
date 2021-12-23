#include "bdjconfig.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "song.h"
#include "list.h"
#include "bdjstring.h"

song_t *
songAlloc (void)
{
  song_t    *song;

  song = malloc (sizeof (song_t));
  assert (song != NULL);
  song->songInfo = vlistAlloc (LIST_UNORDERED, istringCompare,
      free, free);
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
songGet (song_t *song, char *key) {
  char *value = vlistGetData (song->songInfo, key);
  return value;
}

void
songSetAll (song_t *song, char *data[], size_t count)
{
  for (size_t i = 0; i < count; i += 2) {
    vlistSetData (song->songInfo, strdup (data[i]), strdup (data[i+1]));
  }
  vlistSort (song->songInfo);
}

void
songSetNumeric (song_t *song, char *tag, long value)
{
  char    temp [30];

  snprintf (temp, sizeof(temp), "%ld", value);
  vlistSetData (song->songInfo, strdup (tag), strdup (temp));
}
