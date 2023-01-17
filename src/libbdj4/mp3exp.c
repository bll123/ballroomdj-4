#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <assert.h>

#include "audioadjust.h"
#include "bdj4.h"
#include "bdjopt.h"
#include "conn.h"
#include "m3u.h"
#include "mdebug.h"
#include "mp3exp.h"
#include "musicdb.h"
#include "nlist.h"
#include "pathutil.h"
#include "song.h"
#include "songutil.h"
#include "tagdef.h"
#include "callback.h"

enum {
  MP3_EXP_DUR,
  MP3_EXP_GAP,
};

void
mp3ExportQueue (char *msg, musicdb_t *musicdb, const char *dirname,
    int mqidx, callbackFuncIntInt dispcb)
{
  char        tname [200];
  char        tslname [200];
  nlist_t     *savelist = NULL;
  char        *ffn;
  char        outfn [MAXPATHLEN];
  pathinfo_t  *pi;
  char        *p;
  char        *tokstr;
  int         counter;
  int         fadein;
  int         fadeout;

  fadein = bdjoptGetNumPerQueue (OPT_Q_FADEINTIME, mqidx);
  fadeout = bdjoptGetNumPerQueue (OPT_Q_FADEOUTTIME, mqidx);

  pi = pathInfo (dirname);
  snprintf (tname, sizeof (tname), "%s/%.*s.m3u", dirname,
      (int) pi->flen, pi->filename);
  snprintf (tslname, sizeof (tslname), "%.*s",
      (int) pi->flen, pi->filename);
  pathInfoFree (pi);

  savelist = nlistAlloc ("mp3-export-nm", LIST_UNORDERED, NULL);

  counter = 1;
  p = strtok_r (msg, MSG_ARGS_RS_STR, &tokstr);
  while (p != NULL) {
    dbidx_t dbidx;
    long    dur;
    int     gap;
    song_t  *song;

    dbidx = atol (p);
    p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
    if (p == NULL) { continue; }
    dur = atol (p);
    p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
    if (p == NULL) { continue; }
    gap = atol (p);

    song = dbGetByIdx (musicdb, dbidx);
    if (song == NULL) {
      continue;
    }
    ffn = songFullFileName (songGetStr (song, TAG_FILE));
    if (ffn == NULL) {
      continue;
    }
    pi = pathInfo (ffn);

    snprintf (outfn, sizeof (outfn), "%s/%03d-%.*s.mp3",
        dirname, counter, (int) pi->blen, pi->basename);

    nlistSetStr (savelist, dbidx, outfn);
    aaApplyAdjustments (song, ffn, outfn, fadein, fadeout, dur, gap);

    pathInfoFree (pi);
    mdfree (ffn);

    p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
    ++counter;
  }

  m3uExport (musicdb, savelist, tname, tslname);

  nlistFree (savelist);
}
