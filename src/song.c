#include "bdjconfig.h"

#include <stdlib.h>
#include <memory.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>

#include "song.h"
#include "list.h"

song_t *
songAlloc ()
{
  song_t    *song;

  song = malloc (sizeof (song_t));
  assert (song != NULL);
  song->songInfo = listAlloc (sizeof (songtag_t), LIST_ORDERED);
  return song;
}

void
songFree (song_t *song)
{
  if (song != NULL) {
    if (song->songInfo != NULL) {
      listFree (song->songInfo);
    }
    free (song);
  }
}

