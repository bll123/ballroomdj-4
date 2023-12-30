#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>

#include "audioadjust.h"
#include "audiofile.h"
#include "audiosrc.h"
#include "audiotag.h"
#include "bdj4.h"
#include "bdjopt.h"
#include "callback.h"
#include "conn.h"
#include "m3u.h"
#include "mdebug.h"
#include "mp3exp.h"
#include "musicdb.h"
#include "nlist.h"
#include "pathinfo.h"
#include "slist.h"
#include "song.h"
#include "tagdef.h"
#include "tmutil.h"

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
mp3ExportInit (char *msgdata, musicdb_t *musicdb,
    const char *dirname, int mqidx)
{
  mp3exp_t  *mp3exp;

  mp3exp = mdmalloc (sizeof (mp3exp_t));
  mp3exp->msgdata = msgdata;
  mp3exp->musicdb = musicdb;
  mp3exp->dirname = mdstrdup (dirname);
  mp3exp->mqidx = mqidx;
  mp3exp->savelist = NULL;
  mp3exp->counter = 0;
  mp3exp->totcount = 0;
  mp3exp->tokstr = NULL;
  mp3exp->fadein = bdjoptGetNumPerQueue (OPT_Q_FADEINTIME, mqidx);
  mp3exp->fadeout = bdjoptGetNumPerQueue (OPT_Q_FADEOUTTIME, mqidx);
  mp3exp->state = BDJ4_STATE_START;

  return mp3exp;
}

void
mp3ExportFree (mp3exp_t *mp3exp)
{
  if (mp3exp != NULL) {
    dataFree (mp3exp->dirname);
    dataFree (mp3exp->msgdata);
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

  if (mp3exp->state == BDJ4_STATE_START) {
    mp3exp->savelist = nlistAlloc ("mp3-export-nm", LIST_UNORDERED, NULL);

    mp3exp->counter = 1;
    p = strtok_r (mp3exp->msgdata, MSG_ARGS_RS_STR, &mp3exp->tokstr);
    if (p == NULL) {
      mp3exp->state = BDJ4_STATE_OFF;
      return true;
    }
    mp3exp->totcount = atoi (p);
    mp3exp->state = BDJ4_STATE_PROCESS;
    return false;
  }

  if (mp3exp->state == BDJ4_STATE_PROCESS) {
    char    outfn [MAXPATHLEN];
    char    ffn [MAXPATHLEN];

    p = strtok_r (NULL, MSG_ARGS_RS_STR, &mp3exp->tokstr);
    if (p != NULL) {
      dbidx_t dbidx;
      long    dur;
      int     gap;
      song_t  *song;
      void    *savedtags = NULL;
      slist_t *taglist;
      int     owrite;
      int     intagtype, outtagtype;
      int     infiletype, outfiletype;

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
      audiosrcFullPath (songGetStr (song, TAG_URI), ffn, sizeof (ffn));
      pi = pathInfo (ffn);

      snprintf (outfn, sizeof (outfn), "%s/%03d-%.*s.mp3",
          mp3exp->dirname, mp3exp->counter, (int) pi->blen, pi->basename);

      nlistSetStr (mp3exp->savelist, dbidx, outfn);

      audiotagDetermineTagType (ffn, &intagtype, &infiletype);
      audiotagDetermineTagType (outfn, &outtagtype, &outfiletype);
      if (intagtype == outtagtype && infiletype == outfiletype) {
        savedtags = audiotagSaveTags (ffn);
      }
      aaAdjust (mp3exp->musicdb, song, ffn, outfn,
          dur, mp3exp->fadein, mp3exp->fadeout, gap);
      if (intagtype == outtagtype && infiletype == outfiletype) {
        audiotagRestoreTags (outfn, savedtags);
        audiotagFreeSavedTags (outfn, savedtags);
        savedtags = NULL;
      }

      /* always write the tags, they may have been updated by aaAdjust() */
      taglist = songTagList (song);
      owrite = bdjoptGetNum (OPT_G_WRITETAGS);
      bdjoptSetNum (OPT_G_WRITETAGS, WRITE_TAGS_ALL);
      audiotagWriteTags (outfn, NULL, taglist, AF_REWRITE_NONE, AT_KEEP_MOD_TIME);
      bdjoptSetNum (OPT_G_WRITETAGS, owrite);
      slistFree (taglist);

      pathInfoFree (pi);

      ++mp3exp->counter;
    }

    if (p == NULL) {
      mp3exp->state = BDJ4_STATE_FINISH;
    }
    return false;
  }

  if (mp3exp->state == BDJ4_STATE_FINISH) {
    char        tname [MAXPATHLEN];
    char        tslname [200];

    pi = pathInfo (mp3exp->dirname);
    snprintf (tname, sizeof (tname), "%s/%.*s.m3u", mp3exp->dirname,
        (int) pi->flen, pi->filename);
    snprintf (tslname, sizeof (tslname), "%.*s",
        (int) pi->flen, pi->filename);
    pathInfoFree (pi);

    m3uExport (mp3exp->musicdb, mp3exp->savelist, tname, tslname);
    mp3exp->state = BDJ4_STATE_OFF;
    return true;
  }

  /* off */
  return true;
}
