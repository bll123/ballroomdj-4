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
#include "callback.h"
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
#include "tmutil.h"

enum {
  MP3EXP_STATE_OFF,
  MP3EXP_STATE_INIT,
  MP3EXP_STATE_PROCESS,
  MP3EXP_STATE_FINISH,
};

typedef struct mp3exp {
  char      *msgdata;
  musicdb_t *musicdb;
  char      *dirname;
  int       mqidx;
  nlist_t   *savelist;
  int       counter;
  int       totcount;
  char      *tokstr;
  int       state;
  int       fadein;
  int       fadeout;
} mp3exp_t;

mp3exp_t *
mp3ExportInit (char *msgdata, musicdb_t *musicdb, char *dirname, int mqidx)
{
  mp3exp_t  *mp3exp;

  mp3exp = mdmalloc (sizeof (mp3exp_t));
  mp3exp->msgdata = msgdata;
  mp3exp->musicdb = musicdb;
  mp3exp->dirname = dirname;
  mp3exp->mqidx = mqidx;
  mp3exp->savelist = NULL;
  mp3exp->counter = 0;
  mp3exp->totcount = 0;
  mp3exp->tokstr = NULL;
  mp3exp->fadein = bdjoptGetNumPerQueue (OPT_Q_FADEINTIME, mqidx);
  mp3exp->fadeout = bdjoptGetNumPerQueue (OPT_Q_FADEOUTTIME, mqidx);
  mp3exp->state = MP3EXP_STATE_INIT;

  return mp3exp;
}

void
mp3ExportFree (mp3exp_t *mp3exp)
{
  if (mp3exp != NULL) {
    dataFree (mp3exp->msgdata);
    dataFree (mp3exp->dirname);
    nlistFree (mp3exp->savelist);
    mdfree (mp3exp);
  }
}

void
mp3ExportGetCount (mp3exp_t *mp3exp, int *count, int *tot)
{
  *count = mp3exp->counter;
  *tot = mp3exp->totcount;
}

bool
mp3ExportQueue (mp3exp_t *mp3exp)
{
  pathinfo_t  *pi = NULL;
  char        *p = NULL;

  if (mp3exp->state == MP3EXP_STATE_INIT) {
    mp3exp->savelist = nlistAlloc ("mp3-export-nm", LIST_UNORDERED, NULL);

    mp3exp->counter = 1;
    p = strtok_r (mp3exp->msgdata, MSG_ARGS_RS_STR, &mp3exp->tokstr);
    if (p == NULL) {
      mp3exp->state = MP3EXP_STATE_OFF;
      return true;
    }
    mp3exp->totcount = atoi (p);
    mp3exp->state = MP3EXP_STATE_PROCESS;
    return false;
  }

  if (mp3exp->state == MP3EXP_STATE_PROCESS) {
    char    outfn [MAXPATHLEN];
    char    *ffn;

    p = strtok_r (NULL, MSG_ARGS_RS_STR, &mp3exp->tokstr);
    if (p != NULL) {
      dbidx_t dbidx;
      long    dur;
      int     gap;
      song_t  *song;

      dbidx = atol (p);
      p = strtok_r (NULL, MSG_ARGS_RS_STR, &mp3exp->tokstr);
      if (p == NULL) {
        return false;
      }
      dur = atol (p);
      p = strtok_r (NULL, MSG_ARGS_RS_STR, &mp3exp->tokstr);
      if (p == NULL) {
        return false;
      }
      gap = atol (p);

      song = dbGetByIdx (mp3exp->musicdb, dbidx);
      if (song == NULL) {
        return false;
      }
      ffn = songutilFullFileName (songGetStr (song, TAG_FILE));
      if (ffn == NULL) {
        return false;
      }
      pi = pathInfo (ffn);

      snprintf (outfn, sizeof (outfn), "%s/%03d-%.*s.mp3",
          mp3exp->dirname, mp3exp->counter, (int) pi->blen, pi->basename);

      nlistSetStr (mp3exp->savelist, dbidx, outfn);
      aaAdjust (mp3exp->musicdb, song, ffn, outfn,
          dur, mp3exp->fadein, mp3exp->fadeout, gap);

      pathInfoFree (pi);
      mdfree (ffn);

      ++mp3exp->counter;
    }

    if (p == NULL) {
      mp3exp->state = MP3EXP_STATE_FINISH;
    }
    return false;
  }

  if (mp3exp->state == MP3EXP_STATE_FINISH) {
    char        tname [MAXPATHLEN];
    char        tslname [200];

    pi = pathInfo (mp3exp->dirname);
    snprintf (tname, sizeof (tname), "%s/%.*s.m3u", mp3exp->dirname,
        (int) pi->flen, pi->filename);
    snprintf (tslname, sizeof (tslname), "%.*s",
        (int) pi->flen, pi->filename);
    pathInfoFree (pi);

    m3uExport (mp3exp->musicdb, mp3exp->savelist, tname, tslname);
    return true;
  }

  /* off */
  return true;
}
