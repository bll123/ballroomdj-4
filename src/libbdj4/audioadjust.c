/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
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

static datafilekey_t aadfkeys [AA_KEY_MAX] = {
  { "NORMVOL_MAX",            AA_NORMVOL_MAX,         VALUE_DOUBLE, NULL, DF_NORM },
  { "NORMVOL_TARGET",         AA_NORMVOL_TARGET,      VALUE_DOUBLE, NULL, DF_NORM },
  { "TRIMSILENCE_PERIOD",     AA_TRIMSILENCE_PERIOD,  VALUE_NUM,    NULL, DF_NORM },
  { "TRIMSILENCE_START",      AA_TRIMSILENCE_START,   VALUE_DOUBLE, NULL, DF_NORM },
  { "TRIMSILENCE_THRESHOLD",  AA_TRIMSILENCE_THRESHOLD, VALUE_NUM,  NULL, DF_NORM },
};

static void aaTrimSilence (musicdb_t *musicdb, dbidx_t dbidx, const char *infn, const char *outfn);
static void aaApplySpeed (song_t *song, const char *infn, const char *outfn, int speed, int gap);
static void aaRestoreTags (musicdb_t *musicdb, song_t *song, dbidx_t dbidx, const char *infn, const char *songfn);
static void aaSetDuration (musicdb_t *musicdb, song_t *song, const char *ffn);
static int  aaProcess (const char *tag, const char *targv [], int targc, char *resp);

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

  strlcpy (songfn, songGetStr (song, TAG_URI), sizeof (songfn));
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
    audiotagWriteTags (fullfn, taglist, taglist, AF_FORCE_WRITE_BDJ, AT_KEEP_MOD_TIME);
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

  /* trim silence is done first */
  if ((aaflags & SONG_ADJUST_TRIM) == SONG_ADJUST_TRIM) {
    aaTrimSilence (musicdb, dbidx, infn, outfn);
    if (fileopFileExists (outfn)) {
      long    adjflags;

      filemanipMove (outfn, fullfn);
      infn = fullfn;
      adjflags = songGetNum (song, TAG_ADJUSTFLAGS);
      adjflags |= SONG_ADJUST_TRIM;
      songSetNum (song, TAG_ADJUSTFLAGS, adjflags);
      changed = true;
    } else {
      fileopDelete (outfn);
    }
  }

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
    audiotagFreeSavedTags (fullfn, savedtags);
    savedtags = NULL;

    songdb = songdbAlloc (musicdb);
    songdbWriteDB (songdb, dbidx);
    songdbFree (songdb);
  }
  dataFree (savedtags);

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
    snprintf (sstmp, sizeof (sstmp), "%d" PRId32 "ms", songstart);
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
    strlcat (aftext, tmp, sizeof (aftext));
    afprefix = ", ";
  }

  if (fadeout > 0) {
    snprintf (tmp, sizeof (tmp),
        "%safade=t=out:curve=%s:st=%" PRId32 "ms:d=%dms",
        afprefix, ftstr, calcdur - fadeout, fadeout);
    strlcat (aftext, tmp, sizeof (aftext));
    afprefix = ", ";
  }

  /* if the speed is not 100, apply the gap along with the speed step */
  /* otherwise, do it here. */
  if (speed == 100 && gap > 0) {
    snprintf (tmp, sizeof (tmp), "%sapad=pad_dur=%dms", afprefix, gap);
    strlcat (aftext, tmp, sizeof (aftext));
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

static void
aaTrimSilence (musicdb_t *musicdb, dbidx_t dbidx,
    const char *infn, const char *outfn)
{
  aa_t        *aa;
  const char  *targv [40];
  int         targc = 0;
  char        ffargs [300];
  int         rc;
  mstime_t    etm;
  song_t      *song = NULL;
  char        *resp;

  song = dbGetByIdx (musicdb, dbidx);
  if (song == NULL) {
    return;
  }

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
      "silenceremove=start_periods=%d:start_silence=%.2f:start_threshold=%ddB:detection=peak,aformat=dblp,areverse,silenceremove=start_periods=%d:start_silence=%.2f:start_threshold=%ddB:detection=peak,aformat=dblp,areverse",
      (int) nlistGetNum (aa->values, AA_TRIMSILENCE_PERIOD),
      nlistGetDouble (aa->values, AA_TRIMSILENCE_START),
      (int) nlistGetNum (aa->values, AA_TRIMSILENCE_THRESHOLD),
      (int) nlistGetNum (aa->values, AA_TRIMSILENCE_PERIOD),
      nlistGetDouble (aa->values, AA_TRIMSILENCE_START),
      (int) nlistGetNum (aa->values, AA_TRIMSILENCE_THRESHOLD));
  targv [targc++] = "-af";
  targv [targc++] = ffargs;

  targv [targc++] = "-q:a";
  targv [targc++] = "0";
  targv [targc++] = outfn;
  targv [targc++] = NULL;

  resp = mdmalloc (AA_RESP_BUFF_SZ);
  rc = aaProcess ("aa-trim", targv, targc, resp);
  logMsg (LOG_DBG, LOG_INFO, "aa-trim: elapsed: %ld",
      (long) mstimeend (&etm));

  if (rc == 0) {
    aaSetDuration (musicdb, song, outfn);
  }

  dataFree (resp);
  return;
}

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
    strlcat (aftext, tmp, sizeof (aftext));
    afprefix = ", ";
  }

  if (gap > 0) {
    snprintf (tmp, sizeof (tmp), "%sapad=pad_dur=%dms", afprefix, gap);
    strlcat (aftext, tmp, sizeof (aftext));
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
  int         songdbflags;
  int32_t     rrn;
  song_t      *tsong;

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
  songSetNum (song, TAG_VOLUMEADJUSTPERC, songGetNum (tsong, TAG_VOLUMEADJUSTPERC));
  songSetNum (song, TAG_BPM, songGetNum (tsong, TAG_BPM));

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
  long        dur;

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

    *cmd = '\0';
    for (int i = 0; i < targc - 1; ++i) {
      strlcat (cmd, targv [i], sizeof (cmd));;
      strlcat (cmd, " ", sizeof (cmd));;
    }
    logMsg (LOG_DBG, LOG_IMPORTANT, "%s: cmd: %s", tag, cmd);
    logMsg (LOG_DBG, LOG_IMPORTANT, "%s: rc: %d", tag, rc);
    logMsg (LOG_DBG, LOG_IMPORTANT, "%s: resp: %s", tag, resp);
  }

  return rc;
}
