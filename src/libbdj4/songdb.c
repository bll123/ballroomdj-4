#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <assert.h>

#include "audiotag.h"
#include "slist.h"
#include "song.h"
#include "songutil.h"

void
songWriteAudioTags (song_t *song)
{
  char  *ffn;
  char  *data;

  ffn = songFullFileName (songGetStr (song, TAG_FILE));
  data = audiotagReadTags (ffn);
  if (data != NULL) {
    slist_t *tagdata;
    slist_t *newtaglist;

    tagdata = audiotagParseData (ffn, data);
    free (data);
    newtaglist = songTagList (song);
    audiotagWriteTags (ffn, tagdata, newtaglist);
    slistFree (tagdata);
    slistFree (newtaglist);
  }
  free (ffn);
}
