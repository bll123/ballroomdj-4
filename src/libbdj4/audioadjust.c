/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
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
#include "bdjstring.h"
#include "bdjvars.h"
#include "bdjvarsdf.h"
#include "datafile.h"
#include "fileop.h"
#include "filemanip.h"
#include "log.h"
#include "mdebug.h"
#include "musicdb.h"
#include "nlist.h"
#include "nodiscard.h"
#include "osprocess.h"
#include "pathbld.h"
#include "pathinfo.h"
#include "song.h"
#include "songdb.h"
#include "songutil.h"
#include "sysvars.h"
#include "tagdef.h"
#include "tmutil.h"

enum {
  AA_NONE,
  AA_NORM_IN_MAX,
};

enum {
  /* ffmpeg output progress lines */
  /* also, if there are errors in the audio file, the output can be long */
  AA_RESP_BUFF_SZ = 300000,
};

typedef struct aa {
  datafile_t      *df;
  nlist_t         *values;
} aa_t;

enum {
  AA_TRIMSILENCE_NOISE,
  AA_TRIMSILENCE_DURATION,
  AA_KEY_MAX,
};

static datafilekey_t aadfkeys [AA_KEY_MAX] = {
  { "TRIMSILENCE_DURATION",   AA_TRIMSILENCE_DURATION,  VALUE_DOUBLE, NULL, DF_NORM },
  { "TRIMSILENCE_NOISE",      AA_TRIMSILENCE_NOISE,     VALUE_NUM,    NULL, DF_NORM },
};

static const char * const SILENCE_START_STR = { "silence_start:" };
static const char * const SILENCE_DUR_STR = { "silence_duration:" };
#define SILENCE_START_LEN (strlen (SILENCE_START_STR))
#define SILENCE_DUR_LEN (strlen (SILENCE_DUR_STR))


static void aaApplySpeed (song_t *song, const char *infn, const char *outfn, int speed, int gap);
static void aaRestoreTags (musicdb_t *musicdb, song_t *song, dbidx_t dbidx, const char *infn, const char *songfn);
static void aaSetDuration (musicdb_t *musicdb, song_t *song, const char *ffn);
static int  aaProcess (const char *tag, const char *targv [], int targc, char *resp);

NODISCARD
aa_t *
aaAlloc (void)
{
  aa_t  *aa = NULL;
  char        fname [MAXPATHLEN];

  pathbldMakePath (fname, sizeof (fname), AUDIOADJ_FN,
      BDJ4_CONFIG_EXT, PATHBLD_MP_DREL_DATA);
  if (! fileopFileExists (fname)) {
    logMsg (LOG_ERR, LOG_IMPORTANT, "ERR: audioadjust: missing %s", fname);
    return NULL;
  }
  aa = mdmalloc (sizeof (aa_t));

  aa->df = datafileAllocParse ("audioadjust", DFTYPE_KEY_VAL, fname,
      aadfkeys, AA_KEY_MAX, DF_NO_OFFSET, NULL);
  aa->values = datafileGetList (aa->df);

  return aa;
}

void
aaFree (aa_t *aa)
{
  if (aa != NULL) {
    datafileFree (aa->df);
    mdfree (aa);
  }
}

bool
aaApplyAdjustments (musicdb_t *musicdb, dbidx_t dbidx, int aaflags)
{
  song_t      *song;
  pathinfo_t  *pi;
  char        songfn [MAXPATHLEN];
  char        *infn;
  char        origfn [MAXPATHLEN];
  char        fullfn [MAXPATHLEN];
  char        outfn [MAXPATHLEN];
  bool        changed = false;
  void        *savedtags = NULL;

  song = dbGetByIdx (musicdb, dbidx);
  if (song == NULL) {
    return changed;
  }

  stpecpy (songfn, songfn + sizeof (songfn), songGetStr (song, TAG_URI));
  if (audiosrcGetType (songfn) != AUDIOSRC_TYPE_FILE) {
    return changed;
  }

  audiosrcFullPath (songfn, fullfn, sizeof (fullfn), NULL, 0);
  infn = fullfn;
  snprintf (origfn, sizeof (origfn), "%s%s",
      infn, bdjvarsGetStr (BDJV_ORIGINAL_EXT));

  /* check for a non-localized .original file, and if there, rename it */
  if (strcmp (BDJ4_GENERIC_ORIG_EXT, bdjvarsGetStr (BDJV_ORIGINAL_EXT)) != 0 &&
      ! fileopFileExists (origfn)) {
    /* use outfn as a temporary area */
    snprintf (outfn, sizeof (outfn), "%s%s", infn, BDJ4_GENERIC_ORIG_EXT);
    if (fileopFileExists (outfn)) {
      filemanipMove (outfn, origfn);
    }
  }

  if (aaflags == SONG_ADJUST_RESTORE) {
    if (fileopFileExists (origfn)) {
      filemanipMove (origfn, fullfn);
      aaRestoreTags (musicdb, song, dbidx, fullfn, songfn);
      changed = true;
    }
    return changed;
  }

  if (aaflags != SONG_ADJUST_NONE &&
      ! fileopFileExists (origfn)) {
    int     value;
    slist_t *taglist;

    value = bdjoptGetNum (OPT_G_WRITETAGS);
    if (value == WRITE_TAGS_NONE) {
      bdjoptSetNum (OPT_G_WRITETAGS, WRITE_TAGS_BDJ_ONLY);
    }
    taglist = songTagList (song);
    audiotagWriteTags (fullfn, taglist, taglist, AF_FORCE_WRITE_BDJ, AT_FLAGS_MOD_TIME_KEEP);
    bdjoptSetNum (OPT_G_WRITETAGS, value);
    slistFree (taglist);
    filemanipCopy (fullfn, origfn);
  }

  pi = pathInfo (fullfn);
  snprintf (outfn, sizeof (outfn), "%.*s/n-%.*s",
      (int) pi->dlen, pi->dirname,
      (int) pi->flen, pi->filename);
  pathInfoFree (pi);

  /* ffmpeg (et.al.) does not handle all tags properly. */
  /* save all the original tags */
  savedtags = audiotagSaveTags (fullfn);

  /* start with the input as the original filename */
  infn = origfn;

  /* the adjust flags must be reset, as the user may have selected */
  /* different settings */
  songSetNum (song, TAG_ADJUSTFLAGS, SONG_ADJUST_NONE);

  /* any adjustments to the song are made */
  if ((aaflags & SONG_ADJUST_ADJUST) == SONG_ADJUST_ADJUST) {
    aaAdjust (musicdb, song, infn, outfn, 0, 0, 0, 0);
    if (fileopFileExists (outfn)) {
      long    adjflags;
      int     obpm, nbpm;
      int     ospeed;

      filemanipMove (outfn, fullfn);
      infn = fullfn;
      ospeed = songGetNum (song, TAG_SPEEDADJUSTMENT);
      songSetNum (song, TAG_SPEEDADJUSTMENT, 0);
      songSetNum (song, TAG_SONGSTART, 0);
      songSetNum (song, TAG_SONGEND, 0);
      obpm = songGetNum (song, TAG_BPM);
      nbpm = 0;
      if (obpm > 0 && ospeed > 0 && ospeed != 100) {
        nbpm = songutilAdjustBPM (obpm, ospeed);
        songSetNum (song, TAG_BPM, nbpm);
      }
      adjflags = songGetNum (song, TAG_ADJUSTFLAGS);
      adjflags |= SONG_ADJUST_ADJUST;
      songSetNum (song, TAG_ADJUSTFLAGS, adjflags);
      changed = true;
    } else {
      fileopDelete (outfn);
    }
  }

  if (changed) {
    songdb_t    *songdb;

    /* ffmpeg (et.al.) does not handle all tags properly. */
    /* restore all the original tags */
    audiotagRestoreTags (fullfn, savedtags);

    songdb = songdbAlloc (musicdb);
    songdbWriteDB (songdb, dbidx, false);
    songdbFree (songdb);
  }

  audiotagFreeSavedTags (fullfn, savedtags);
  savedtags = NULL;

  return changed;
}

void
aaAdjust (musicdb_t *musicdb, song_t *song,
    const char *infn, const char *outfn,
    long dur, int fadein, int fadeout, int gap)
{
  const char  *targv [40];
  int         targc = 0;
  char        aftext [500];
  char        sstmp [60];
  char        durtmp [60];
  char        tmp [60];
  int         rc;
  int32_t     songstart;
  int32_t     songend;
  int32_t     songdur;
  int32_t     calcdur;
  int         speed;
  int         fadetype;
  const char  *afprefix = "";
  const char  *ftstr = "tri";
  mstime_t    etm;
  char        *resp;
  char        *afp = aftext;
  char        *afend = aftext + sizeof (aftext);

  mstimestart (&etm);
  *aftext = '\0';

  songstart = songGetNum (song, TAG_SONGSTART);
  songend = songGetNum (song, TAG_SONGEND);
  songdur = songGetNum (song, TAG_DURATION);
  speed = songGetNum (song, TAG_SPEEDADJUSTMENT);
  if (speed < 0) {
    speed = 100;
  }
  fadetype = bdjoptGetNum (OPT_P_FADETYPE);

  if (songstart <= 0 && songend <= 0 && speed == 100 && dur == 0 &&
      fadein == 0 && fadeout == 0 && gap == 0) {
    /* no adjustments need to be made */
    return;
  }

  calcdur = dur;
  if (dur == 0) {
    calcdur = songdur;
    if (songend > 0 && songend < songdur) {
      calcdur = songend;
    }
    if (songstart > 0) {
      calcdur -= songstart;
    }
  }

  /* translate to ffmpeg names */
  switch (fadetype) {
    case FADETYPE_EXPONENTIAL_SINE: { ftstr = "esin"; break; }
    case FADETYPE_HALF_SINE: { ftstr = "hsin"; break; }
    case FADETYPE_INVERTED_PARABOLA: { ftstr = "ipar"; break; }
    case FADETYPE_QUADRATIC: { ftstr = "qua"; break; }
    case FADETYPE_QUARTER_SINE: { ftstr = "qsin"; break; }
    case FADETYPE_TRIANGLE: { ftstr = "tri"; break; }
  }

  targv [targc++] = sysvarsGetStr (SV_PATH_FFMPEG);
  targv [targc++] = "-hide_banner";
  targv [targc++] = "-y";
  targv [targc++] = "-vn";
  targv [targc++] = "-dn";
  targv [targc++] = "-sn";

  if (songstart > 0) {
    /* seek start before input file is accurate and fast */
    snprintf (sstmp, sizeof (sstmp), "%" PRId32 "ms", songstart);
    targv [targc++] = "-ss";
    targv [targc++] = sstmp;
  }

  targv [targc++] = "-i";
  targv [targc++] = infn;

  if (calcdur > 0 && calcdur < songdur) {
    int32_t   tdur = calcdur;

    if (speed == 100 && gap > 0) {
      /* duration passed to ffmpeg must include gap time */
      /* this is only done here if the speed is not changed */
      tdur += gap;
    }
    snprintf (durtmp, sizeof (durtmp), "%" PRId32 "ms", tdur);
    targv [targc++] = "-t";
    targv [targc++] = durtmp;
  }

  if (fadein > 0) {
    snprintf (tmp, sizeof (tmp),
        "%safade=t=in:curve=tri:d=%dms",
        afprefix, fadein);
    afp = stpecpy (afp, afend, tmp);
    afprefix = ", ";
  }

  if (fadeout > 0) {
    snprintf (tmp, sizeof (tmp),
        "%safade=t=out:curve=%s:st=%" PRId32 "ms:d=%dms",
        afprefix, ftstr, calcdur - fadeout, fadeout);
    afp = stpecpy (afp, afend, tmp);
    afprefix = ", ";
  }

  /* if the speed is not 100, apply the gap along with the speed step */
  /* otherwise, do it here. */
  if (speed == 100 && gap > 0) {
    snprintf (tmp, sizeof (tmp), "%sapad=pad_dur=%dms", afprefix, gap);
    afp = stpecpy (afp, afend, tmp);
    afprefix = ", ";
  }

  if (*aftext) {
    targv [targc++] = "-af";
    targv [targc++] = aftext;
  }
  targv [targc++] = "-q:a";
  targv [targc++] = "0";
  targv [targc++] = outfn;
  targv [targc++] = NULL;

  resp = mdmalloc (AA_RESP_BUFF_SZ);
  rc = aaProcess ("aa-adjust", targv, targc, resp);
  logMsg (LOG_DBG, LOG_INFO, "aa: adjust: elapsed: %" PRIu64,
      (uint64_t) mstimeend (&etm));

  /* applying the speed change afterwards is slower, but saves a lot */
  /* of headache. */
  if (speed != 100) {
    char  tmpfn [MAXPATHLEN];

    snprintf (tmpfn, sizeof (tmpfn), "%s.tmp", outfn);
    filemanipMove (outfn, tmpfn);
    aaApplySpeed (song, tmpfn, outfn, speed, gap);
    fileopDelete (tmpfn);
    logMsg (LOG_DBG, LOG_INFO, "aa: adjust-with-speed: elapsed: %ld",
        (long) mstimeend (&etm));
  }

  if (rc == 0) {
    aaSetDuration (musicdb, song, outfn);
  }

  dataFree (resp);
  return;
}

int
aaSilenceDetect (const char *infn, double *sstart, double *send)
{
  aa_t        *aa;
  const char  *targv [40];
  int         targc = 0;
  char        ffargs [300];
  int         rc = 1;
  mstime_t    etm;
  char        *resp;
  char        *p = NULL;
  char        *lp = NULL;
  bool        beg = false;
  double      startloc = 0.0;


  if (infn == NULL) {
    return -1;
  }

  *sstart = 0.0;
  *send = 0.0;
  mstimestart (&etm);

  aa = bdjvarsdfGet (BDJVDF_AUDIO_ADJUST);

  targv [targc++] = sysvarsGetStr (SV_PATH_FFMPEG);
  targv [targc++] = "-hide_banner";
  targv [targc++] = "-y";
  targv [targc++] = "-vn";
  targv [targc++] = "-dn";
  targv [targc++] = "-sn";

  targv [targc++] = "-i";
  targv [targc++] = infn;

  snprintf (ffargs, sizeof (ffargs),
      "silencedetect=noise=%ddB:duration=%.2f",
      (int) nlistGetNum (aa->values, AA_TRIMSILENCE_NOISE),
      nlistGetDouble (aa->values, AA_TRIMSILENCE_DURATION));
  targv [targc++] = "-af";
  targv [targc++] = ffargs;

  targv [targc++] = "-f";
  targv [targc++] = "null";
  targv [targc++] = "-";
  targv [targc++] = NULL;

  resp = mdmalloc (AA_RESP_BUFF_SZ);
  rc = aaProcess ("aa-silence-detect", targv, targc, resp);

  if (rc != 0) {
    return rc;
  }

  /* [silencedetect @ 0x56141ab93d00] silence_start: 0 */
  /* [silencedetect @ 0x56141ab93d00] silence_end: 3 | silence_duration: 3 */
  /*   there may be more lines present with silence_start/silence_end pairs */
  /* [silencedetect @ 0x56141ab93d00] silence_start: 33.6343 */
  /* size=N/A time=00:00:36.13 bitrate=N/A speed= 675x */
  /* video:0kB audio:6225kB subtitle:0kB other streams:0kB global headers:0kB muxing overhead: unknown */
  /* [silencedetect @ 0x55ed21549d00] silence_end: 36.1343 | silence_duration: 2.5 */

  /* [silencedetect @ 0x5600a35c8000] silence_start: 0 */
  /* [silencedetect @ 0x5600a35c8000] silence_end: 3 | silence_duration: 3 */
  /* [silencedetect @ 0x5600a35c8000] silence_start: 16.9165 */
  /* [silencedetect @ 0x5600a35c8000] silence_end: 18.9165 | silence_duration: 2 */
  /* size=N/A time=00:00:35.59 bitrate=N/A speed= 658x */
  /* video:0kB audio:6132kB subtitle:0kB other streams:0kB global headers:0kB muxing overhead: unknown */

  p = strstr (resp, SILENCE_START_STR);
  while (p != NULL) {
    /* there is silence, unknown location */

    lp = p;
    p += SILENCE_START_LEN;
    p += 1;
    startloc = atof (p);
    if (startloc == 0.0) {
      beg = true;
    }

    if (beg) {
      /* silence was found at the beginning */

      beg = false;
      p = strstr (p + 1, SILENCE_DUR_STR);
      if (p != NULL) {
        p += SILENCE_DUR_LEN;
        p += 1;
        *sstart = atof (p);
      }
    }

    if (p != NULL) {
      p = strstr (p + 1, SILENCE_START_STR);
    }
  }

  if (lp != NULL) {
    char    *tp = NULL;

    /* a silence detect block at the end of the song will have the */
    /* time= field, then the silence_duration string in that order */
    p = strstr (lp, "time=");
    tp = strstr (lp, SILENCE_DUR_STR);
    if (p != NULL && tp != NULL && p < tp) {
      *send = startloc;
    }
  }

  logMsg (LOG_DBG, LOG_INFO, "aa-silence-detect: elapsed: %ld",
      (long) mstimeend (&etm));
  logMsg (LOG_DBG, LOG_INFO, "aa-silence-detect: start: %.2f end: %.2f",
      *sstart, *send);

  dataFree (resp);
  return rc;
}

/* internal routines */

static void
aaApplySpeed (song_t *song, const char *infn, const char *outfn,
    int speed, int gap)
{
  const char  *targv [30];
  int         targc = 0;
  char        aftext [500];
  char        *resp;
  char        tmp [60];
  const char  *afprefix = "";
  char        *afp = aftext;
  char        *afend = aftext + sizeof (aftext);

  *aftext = '\0';

  targv [targc++] = sysvarsGetStr (SV_PATH_FFMPEG);
  targv [targc++] = "-hide_banner";
  targv [targc++] = "-y";
  targv [targc++] = "-vn";
  targv [targc++] = "-dn";
  targv [targc++] = "-sn";

  targv [targc++] = "-i";
  targv [targc++] = infn;

  if (speed > 0 && speed != 100) {
    snprintf (tmp, sizeof (tmp), "%srubberband=tempo=%.2f",
        afprefix, (double) speed / 100.0);
    afp = stpecpy (afp, afend, tmp);
    afprefix = ", ";
  }

  if (gap > 0) {
    snprintf (tmp, sizeof (tmp), "%sapad=pad_dur=%dms", afprefix, gap);
    afp = stpecpy (afp, afend, tmp);
    afprefix = ", ";
  }

  if (*aftext) {
    targv [targc++] = "-af";
    targv [targc++] = aftext;
  }
  targv [targc++] = "-q:a";
  targv [targc++] = "0";
  targv [targc++] = outfn;
  targv [targc++] = NULL;

  resp = mdmalloc (AA_RESP_BUFF_SZ);
  aaProcess ("aa-adjust-spd", targv, targc, resp);

  dataFree (resp);
}

static void
aaRestoreTags (musicdb_t *musicdb, song_t *song, dbidx_t dbidx,
    const char *infn, const char *songfn)
{
  slist_t     *tagdata;
  int         rewrite;
  songdb_t    *songdb;
  int32_t     songdbflags;
  int32_t     rrn;
  song_t      *tsong;
  int32_t     dur;

  rrn = songGetNum (song, TAG_RRN);
  tagdata = audiotagParseData (infn, &rewrite);
  slistSetStr (tagdata, tagdefs [TAG_ADJUSTFLAGS].tag, NULL);
  tsong = songAlloc ();
  songFromTagList (tsong, tagdata);

  /* keep the current tags */
  /* only restore song-start, song-end, speed-adjustment, volume-adjustment */
  /* also bpm, as it changes due to speed */
  songSetStr (song, TAG_ADJUSTFLAGS, NULL);
  songSetNum (song, TAG_SONGSTART, songGetNum (tsong, TAG_SONGSTART));
  songSetNum (song, TAG_SONGEND, songGetNum (tsong, TAG_SONGEND));
  songSetNum (song, TAG_SPEEDADJUSTMENT, songGetNum (tsong, TAG_SPEEDADJUSTMENT));
  songSetNum (song, TAG_BPM, songGetNum (tsong, TAG_BPM));
  dur = atol (slistGetStr (tagdata, tagdefs [TAG_DURATION].tag));
  songSetNum (song, TAG_DURATION, dur);

  songdb = songdbAlloc (musicdb);
  songdbflags = SONGDB_NONE;
  songdbWriteDBSong (songdb, song, &songdbflags, rrn);
  songdbFree (songdb);

  slistFree (tagdata);
  songFree (tsong);
}


static void
aaSetDuration (musicdb_t *musicdb, song_t *song, const char *ffn)
{
  slist_t     *tagdata;
  int         rewrite;
  int32_t     dur;

  if (! fileopFileExists (ffn)) {
    return;
  }

  tagdata = audiotagParseData (ffn, &rewrite);
  dur = atol (slistGetStr (tagdata, tagdefs [TAG_DURATION].tag));
  songSetNum (song, TAG_DURATION, dur);
  slistFree (tagdata);
}

static int
aaProcess (const char *tag, const char *targv [], int targc, char *resp)
{
  int     rc;

  rc = osProcessPipe (targv, OS_PROC_WAIT | OS_PROC_DETACH, resp, AA_RESP_BUFF_SZ, NULL);
  if (rc != 0 || logCheck (LOG_DBG, LOG_AUDIO_ADJUST)) {
    char  cmd [1000];
    char  *p = cmd;
    char  *end = cmd + sizeof (cmd);

    *cmd = '\0';
    for (int i = 0; i < targc - 1; ++i) {
      p = stpecpy (p, end, targv [i]);
      p = stpecpy (p, end, " ");
    }
    logMsg (LOG_DBG, LOG_IMPORTANT, "%s: cmd: %s", tag, cmd);
    logMsg (LOG_DBG, LOG_IMPORTANT, "%s: rc: %d", tag, rc);
    logMsg (LOG_DBG, LOG_IMPORTANT, "%s: resp: %s", tag, resp);
  }

  return rc;
}
