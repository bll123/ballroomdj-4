#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <assert.h>

#include "audiotag.h"
#include "ilist.h"
#include "musicdb.h"
#include "playlist.h"
#include "slist.h"
#include "song.h"
#include "songdb.h"
#include "songlist.h"
#include "songutil.h"
#include "tagdef.h"

static void songWriteAudioTags (song_t *song);
static void songUpdateAllSonglists (song_t *song);

void
songWriteDB (musicdb_t *musicdb, dbidx_t dbidx)
{
  song_t    *song;

  song = dbGetByIdx (musicdb, dbidx);
  if (song != NULL) {
    if (! songIsChanged (song)) {
      return;
    }
    dbWriteSong (musicdb, song);
    songWriteAudioTags (song);
    if (songHasSonglistChange (song)) {
      /* need to update all songlists if file/title/dance changed. */
      songUpdateAllSonglists (song);
    }
    songClearChanged (song);
  }
}

/* internal routines */

static void
songWriteAudioTags (song_t *song)
{
  char  *ffn;
  char  *data;

  ffn = songFullFileName (songGetStr (song, TAG_FILE));
  data = audiotagReadTags (ffn);
  if (data != NULL) {
    slist_t *tagdata;
    slist_t *newtaglist;
    int     rewrite;

    tagdata = audiotagParseData (ffn, data, &rewrite);
    free (data);
    newtaglist = songTagList (song);
    audiotagWriteTags (ffn, tagdata, newtaglist, 0);
    slistFree (tagdata);
    slistFree (newtaglist);
  }
  free (ffn);
}

static void
songUpdateAllSonglists (song_t *song)
{
  slist_t     *filelist;
  slistidx_t  fiteridx;
  const char  *songfn;
  const char  *fn;
  songlist_t  *songlist;
  ilistidx_t  sliter;

  // ### TODO for re-organization, the old filename will be needed.
  songfn = songGetStr (song, TAG_FILE);
  filelist = playlistGetPlaylistList (PL_LIST_SONGLIST);
  slistStartIterator (filelist, &fiteridx);
  while ((fn = slistIterateKey (filelist, &fiteridx)) != NULL) {
    ilistidx_t  key;
    const char  *slfn;
    bool        chg;

    chg = false;
    songlist = songlistLoad (fn);
    songlistStartIterator (songlist, &sliter);
    while ((key = songlistIterate (songlist, &sliter)) >= 0) {
      slfn = songlistGetStr (songlist, key, SONGLIST_FILE);
      if (strcmp (slfn, songfn) == 0) {
        songlistSetStr (songlist, key, SONGLIST_TITLE,
            songGetStr (song, TAG_TITLE));
        songlistSetNum (songlist, key, SONGLIST_DANCE,
            songGetNum (song, TAG_DANCE));
        chg = true;
      }
    }
    if (chg) {
      songlistSave (songlist, SONGLIST_PRESERVE_TIMESTAMP);
    }
    songlistFree (songlist);
  }
}
