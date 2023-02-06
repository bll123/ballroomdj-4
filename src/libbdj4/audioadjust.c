/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <assert.h>

#include "audioadjust.h"
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
#include "pathutil.h"
#include "song.h"
#include "songdb.h"
#include "songutil.h"
#include "sysvars.h"
#include "tagdef.h"
#include "tmutil.h"

enum {
  AA_INPUT_I,
  AA_INPUT_TP,
  AA_INPUT_LRA,
  AA_INPUT_THRESH,
  AA_TARGET_OFFSET,
  AA_NONE,
  AA_NORM_IN_MAX,
};

typedef struct aa {
  datafile_t      *df;
  nlist_t         *values;
} aa_t;

static datafilekey_t aadfkeys [AA_KEY_MAX] = {
  { "LOUDNORM_TARGET_IL",     AA_LOUDNORM_TARGET_IL,  VALUE_DOUBLE, NULL, -1 },
  { "LOUDNORM_TARGET_LRA",    AA_LOUDNORM_TARGET_LRA, VALUE_DOUBLE, NULL, -1 },
  { "LOUDNORM_TARGET_TP",     AA_LOUDNORM_TARGET_TP,  VALUE_DOUBLE, NULL, -1 },
  { "TRIMSILENCE_PERIOD",     AA_TRIMSILENCE_PERIOD,  VALUE_NUM,    NULL, -1 },
  { "TRIMSILENCE_START",      AA_TRIMSILENCE_START,   VALUE_DOUBLE, NULL, -1 },
  { "TRIMSILENCE_THRESHOLD",  AA_TRIMSILENCE_THRESHOLD, VALUE_NUM,  NULL, -1 },
};

static long aaApplySpeed (song_t *song, const char *infn, const char *outfn, int speed, int gap);
static long aaParseDuration (const char *resp);

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
      aadfkeys, AA_KEY_MAX);
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
aaApplyAdjustments (musicdb_t *musicdb, dbidx_t dbidx, long aaflags)
{
  song_t      *song;
  long        dur;
  long        newdur = -1;
  pathinfo_t  *pi;
  const char  *songfn;
  char        *infn;
  char        origfn [MAXPATHLEN];
  char        fullfn [MAXPATHLEN];
  char        outfn [MAXPATHLEN];
  bool        changed = false;

  song = dbGetByIdx (musicdb, dbidx);
  if (song == NULL) {
    return changed;
  }

  dur = songGetNum (song, TAG_DURATION);

  songfn = songGetStr (song, TAG_FILE);
  infn = songFullFileName (songfn);
  strlcpy (fullfn, infn, sizeof (fullfn));
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
      char    *data;

      filemanipMove (origfn, fullfn);
      data = audiotagReadTags (fullfn);
      if (data != NULL) {
        slist_t *tagdata;
        int     rewrite;
        char    tbuff [3096];

        tagdata = audiotagParseData (fullfn, data, &rewrite);
        slistSetStr (tagdata, tagdefs [TAG_ADJUSTFLAGS].tag, NULL);
        /* the data in the database must be replaced with the original data */
        dbWrite (musicdb, songfn, tagdata, songGetNum (song, TAG_RRN));
        /* and the song's data must be replaced with the original data */
        dbCreateSongEntryFromTags (tbuff, sizeof (tbuff), tagdata,
            songfn, songGetNum (song, TAG_RRN));
        songParse (song, tbuff, dbidx);
        slistFree (tagdata);
      }
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
  dataFree (infn);

  /* start with the input as the original filename */
  infn = origfn;

  if (aaflags != SONG_ADJUST_NONE) {
    /* the adjust flags must be reset, as the user may have selected */
    /* different settings */
    songSetNum (song, TAG_ADJUSTFLAGS, SONG_ADJUST_NONE);
  }

  /* trim silence is done first */
  if ((aaflags & SONG_ADJUST_TRIM) == SONG_ADJUST_TRIM) {
    newdur = aaTrimSilence (infn, outfn);
    if (fileopFileExists (outfn)) {
      if (newdur > 1 && newdur != dur) {
        long    adjflags;

        filemanipMove (outfn, fullfn);
        infn = fullfn;
        songSetNum (song, TAG_DURATION, newdur);
        adjflags = songGetNum (song, TAG_ADJUSTFLAGS);
        adjflags |= SONG_ADJUST_TRIM;
        songSetNum (song, TAG_ADJUSTFLAGS, adjflags);
        changed = true;
      } else {
        fileopDelete (outfn);
      }
    }
  }
  /* any adjustments to the song are made */
  if ((aaflags & SONG_ADJUST_ADJUST) == SONG_ADJUST_ADJUST) {
    newdur = aaAdjust (song, infn, outfn, 0, 0, 0, 0);
    if (fileopFileExists (outfn)) {
      if (newdur > 0) {
        long    adjflags;

        filemanipMove (outfn, fullfn);
        infn = fullfn;
        songSetNum (song, TAG_DURATION, newdur);
        songSetNum (song, TAG_SPEEDADJUSTMENT, 0);
        songSetNum (song, TAG_SONGSTART, 0);
        songSetNum (song, TAG_SONGEND, 0);
        adjflags = songGetNum (song, TAG_ADJUSTFLAGS);
        adjflags |= SONG_ADJUST_ADJUST;
        songSetNum (song, TAG_ADJUSTFLAGS, adjflags);
        changed = true;
      } else {
        fileopDelete (outfn);
      }
    }
  }
  /* after the adjustments are done, then normalize */
  if ((aaflags & SONG_ADJUST_NORM) == SONG_ADJUST_NORM) {
    aaNormalize (infn, outfn);
    if (fileopFileExists (outfn)) {
      long    adjflags;

      filemanipMove (outfn, fullfn);
      infn = fullfn;
      songSetNum (song, TAG_VOLUMEADJUSTPERC, 0.0);
      adjflags = songGetNum (song, TAG_ADJUSTFLAGS);
      adjflags |= SONG_ADJUST_NORM;
      songSetNum (song, TAG_ADJUSTFLAGS, adjflags);
      changed = true;
    }
  }

  if (changed) {
    songWriteDB (musicdb, dbidx);
  }

  return changed;
}

long
aaTrimSilence (const char *infn, const char *outfn)
{
  aa_t        *aa;
  const char  *targv [40];
  int         targc = 0;
  char        ffargs [300];
  char        resp [2000];
  int         rc;
  size_t      retsz;
  mstime_t    etm;
  long        newdur = -1;

fprintf (stderr, "trim: %s\n", infn);
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

  rc = osProcessPipe (targv, OS_PROC_WAIT | OS_PROC_DETACH, resp, sizeof (resp), &retsz);
  if (rc != 0 || logCheck (LOG_DBG, LOG_AUDIO_ADJUST)) {
    char  cmd [1000];

    *cmd = '\0';
    for (int i = 0; i < targc - 1; ++i) {
      strlcat (cmd, targv [i], sizeof (cmd));;
      strlcat (cmd, " ", sizeof (cmd));;
    }
    logMsg (LOG_DBG, LOG_IMPORTANT, "aa-trim: cmd: %s", cmd);
    logMsg (LOG_DBG, LOG_IMPORTANT, "aa-trim: rc: %d", rc);
    logMsg (LOG_DBG, LOG_IMPORTANT, "aa-trim: resp: %s", resp);
  }
  logMsg (LOG_DBG, LOG_MAIN, "aa: trim: elapsed: %ld",
      (long) mstimeend (&etm));

  if (rc == 0) {
    newdur = aaParseDuration (resp);
  }

  return newdur;
}

void
aaNormalize (const char *infn, const char *outfn)
{
  aa_t        *aa;
  const char  *targv [40];
  int         targc = 0;
  char        ffargs [300];
  char        resp [2000];
  int         rc;
  size_t      retsz;
  char        *p;
  char        *tokstr;
  int         incount = 0;
  int         inidx = AA_NONE;
  double      indata [AA_NORM_IN_MAX];
  bool        found = false;
  mstime_t    etm;

fprintf (stderr, "norm: %s\n", infn);
  mstimestart (&etm);
  aa = bdjvarsdfGet (BDJVDF_AUDIO_ADJUST);

  snprintf (ffargs, sizeof (ffargs),
      "loudnorm=I=%.3f:LRA=%.3f:tp=%.3f:print_format=json",
      nlistGetDouble (aa->values, AA_LOUDNORM_TARGET_IL),
      nlistGetDouble (aa->values, AA_LOUDNORM_TARGET_LRA),
      nlistGetDouble (aa->values, AA_LOUDNORM_TARGET_TP));

  targv [targc++] = sysvarsGetStr (SV_PATH_FFMPEG);
  targv [targc++] = "-hide_banner";
  targv [targc++] = "-y";
  targv [targc++] = "-vn";
  targv [targc++] = "-dn";
  targv [targc++] = "-sn";
  targv [targc++] = "-i";
  targv [targc++] = infn;
  targv [targc++] = "-af";
  targv [targc++] = ffargs;
  targv [targc++] = "-f";
  targv [targc++] = "null";
  targv [targc++] = "-";
  targv [targc++] = NULL;

  rc = osProcessPipe (targv, OS_PROC_WAIT | OS_PROC_DETACH, resp, sizeof (resp), &retsz);
  if (rc != 0 || logCheck (LOG_DBG, LOG_AUDIO_ADJUST)) {
    char  cmd [1000];

    *cmd = '\0';
    for (int i = 0; i < targc - 1; ++i) {
      strlcat (cmd, targv [i], sizeof (cmd));;
      strlcat (cmd, " ", sizeof (cmd));;
    }
    logMsg (LOG_DBG, LOG_IMPORTANT, "aa-norm-a: cmd: %s", cmd);
    logMsg (LOG_DBG, LOG_IMPORTANT, "aa-norm-a: rc: %d", rc);
    logMsg (LOG_DBG, LOG_IMPORTANT, "aa-norm-a: resp: %s", resp);
    if (rc != 0) {
      logMsg (LOG_DBG, LOG_IMPORTANT, "aa-norm: failed, pass one failed.");
      return;
    }
  }

/*
 * {
 *         "input_i" : "-21.65",
 *         "input_tp" : "-4.83",
 *         "input_lra" : "6.80",
 *         "input_thresh" : "-31.82",
 *         "output_i" : "-22.02",
 *         "output_tp" : "-5.83",
 *         "output_lra" : "5.30",
 *         "output_thresh" : "-32.15",
 *         "normalization_type" : "dynamic",
 *         "target_offset" : "-0.98"
 * }
 */

  incount = 0;
  found = false;

  p = strstr (resp, "{");
  p = strtok_r (p, "\": ,", &tokstr);
  while (p != NULL) {
    if (found) {
      indata [inidx] = atof (p);
      found = false;
      inidx = AA_NONE;
      ++incount;
    }

    if (strcmp (p, "input_i") == 0) {
      found = true;
      inidx = AA_INPUT_I;
    }
    if (strcmp (p, "input_tp") == 0) {
      found = true;
      inidx = AA_INPUT_TP;
    }
    if (strcmp (p, "input_lra") == 0) {
      found = true;
      inidx = AA_INPUT_LRA;
    }
    if (strcmp (p, "input_thresh") == 0) {
      found = true;
      inidx = AA_INPUT_THRESH;
    }
    if (strcmp (p, "target_offset") == 0) {
      found = true;
      inidx = AA_TARGET_OFFSET;
    }
    p = strtok_r (NULL, "\": ,", &tokstr);
  }

  if (incount != 5) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "aa-norm: failed, could not find input data");
    return;
  }
  logMsg (LOG_DBG, LOG_MAIN, "aa: norm: elapsed pass 1: %ld",
      (long) mstimeend (&etm));

  /* now do the normalization */

  snprintf (ffargs, sizeof (ffargs),
      "loudnorm=print_format=summary:linear=true:"
      "I=%.3f:LRA=%.3f:tp=%.3f:"
      "measured_I=%.3f:measured_LRA=%.3f:measured_tp=%.3f"
      ":measured_thresh=%.3f:offset=%.3f",
      nlistGetDouble (aa->values, AA_LOUDNORM_TARGET_IL),
      nlistGetDouble (aa->values, AA_LOUDNORM_TARGET_LRA),
      nlistGetDouble (aa->values, AA_LOUDNORM_TARGET_TP),
      indata [AA_INPUT_I], indata [AA_INPUT_LRA], indata [AA_INPUT_TP],
      indata [AA_INPUT_THRESH], indata [AA_TARGET_OFFSET]);

  targc = 0;
  targv [targc++] = sysvarsGetStr (SV_PATH_FFMPEG);
  targv [targc++] = "-hide_banner";
  targv [targc++] = "-y";
  targv [targc++] = "-vn";
  targv [targc++] = "-dn";
  targv [targc++] = "-sn";
  targv [targc++] = "-i";
  targv [targc++] = infn;
  targv [targc++] = "-af";
  targv [targc++] = ffargs;
  targv [targc++] = "-q:a";
  targv [targc++] = "0";
  targv [targc++] = outfn;
  targv [targc++] = NULL;

  rc = osProcessPipe (targv, OS_PROC_WAIT | OS_PROC_DETACH, resp, sizeof (resp), &retsz);
  if (rc != 0 || logCheck (LOG_DBG, LOG_AUDIO_ADJUST)) {
    char  cmd [1000];

    *cmd = '\0';
    for (int i = 0; i < targc - 1; ++i) {
      strlcat (cmd, targv [i], sizeof (cmd));;
      strlcat (cmd, " ", sizeof (cmd));;
    }
    logMsg (LOG_DBG, LOG_IMPORTANT, "aa-norm-b: cmd: %s", cmd);
    logMsg (LOG_DBG, LOG_IMPORTANT, "aa-norm-b: rc: %d", rc);
    logMsg (LOG_DBG, LOG_IMPORTANT, "aa-norm-b: resp: %s", resp);
  }
  logMsg (LOG_DBG, LOG_MAIN, "aa: norm: elapsed: %ld",
      (long) mstimeend (&etm));
  return;
}

long
aaAdjust (song_t *song, const char *infn, const char *outfn,
    long dur, int fadein, int fadeout, int gap)
{
  const char  *targv [40];
  int         targc = 0;
  char        aftext [500];
  char        resp [2000];
  char        sstmp [60];
  char        durtmp [60];
  char        tmp [60];
  int         rc;
  long        songstart;
  long        songend;
  long        songdur;
  long        calcdur;
  long        newdur = -1;
  int         speed;
  int         fadetype;
  size_t      retsz;
  const char  *afprefix = "";
  const char  *ftstr = "tri";
  mstime_t    etm;

fprintf (stderr, "adjust: %s\n", infn);
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

  if (songstart == 0 && songend == 0 && speed == 100 && dur == 0 &&
      fadein == 0 && fadeout == 0 && gap == 0) {
    /* no adjustments need to be made */
fprintf (stderr, "  no adjustments needed\n");
    return -1;
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
    snprintf (sstmp, sizeof (sstmp), "%ldms", songstart);
    targv [targc++] = "-ss";
    targv [targc++] = sstmp;
  }

  targv [targc++] = "-i";
  targv [targc++] = infn;

  if (calcdur > 0 && calcdur < songdur) {
    long    tdur = calcdur;

    if (speed == 100 && gap > 0) {
      /* duration passed to ffmpeg must include gap time */
      /* this is only done here if the speed is not changed */
      tdur += gap;
    }
    snprintf (durtmp, sizeof (durtmp), "%ldms", tdur);
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
        "%safade=t=out:curve=%s:st=%ldms:d=%dms",
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

  rc = osProcessPipe (targv, OS_PROC_WAIT | OS_PROC_DETACH, resp, sizeof (resp), &retsz);
  if (rc != 0 || logCheck (LOG_DBG, LOG_AUDIO_ADJUST)) {
    char  cmd [1000];

    *cmd = '\0';
    for (int i = 0; i < targc - 1; ++i) {
      strlcat (cmd, targv [i], sizeof (cmd));;
      strlcat (cmd, " ", sizeof (cmd));;
    }
    logMsg (LOG_DBG, LOG_IMPORTANT, "aa-adjust: cmd: %s", cmd);
    logMsg (LOG_DBG, LOG_IMPORTANT, "aa-adjust: rc: %d", rc);
    logMsg (LOG_DBG, LOG_IMPORTANT, "aa-adjust: resp: %s", resp);
  }
  logMsg (LOG_DBG, LOG_MAIN, "aa: adjust: elapsed: %ld",
      (long) mstimeend (&etm));

  if (rc == 0) {
    newdur = aaParseDuration (resp);
  }

  /* applying the speed change afterwards is slower, but saves a lot */
  /* of headache. */
  if (speed != 100) {
    char  tmpfn [MAXPATHLEN];

    snprintf (tmpfn, sizeof (tmpfn), "%s.tmp", outfn);
    filemanipMove (outfn, tmpfn);
    newdur = aaApplySpeed (song, tmpfn, outfn, speed, gap);
    fileopDelete (tmpfn);
    logMsg (LOG_DBG, LOG_MAIN, "aa: adjust-with-speed: elapsed: %ld",
        (long) mstimeend (&etm));
  }

  return newdur;
}

static long
aaApplySpeed (song_t *song, const char *infn, const char *outfn,
    int speed, int gap)
{
  const char  *targv [30];
  int         targc = 0;
  char        aftext [500];
  char        resp [2000];
  char        tmp [60];
  int         rc;
  size_t      retsz;
  const char  *afprefix = "";
  long        newdur = -1;

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

  rc = osProcessPipe (targv, OS_PROC_WAIT | OS_PROC_DETACH, resp, sizeof (resp), &retsz);
  if (rc != 0 || logCheck (LOG_DBG, LOG_AUDIO_ADJUST)) {
    char  cmd [1000];

    *cmd = '\0';
    for (int i = 0; i < targc - 1; ++i) {
      strlcat (cmd, targv [i], sizeof (cmd));;
      strlcat (cmd, " ", sizeof (cmd));;
    }
    logMsg (LOG_DBG, LOG_IMPORTANT, "aa-apply-spd: cmd: %s", cmd);
    logMsg (LOG_DBG, LOG_IMPORTANT, "aa-apply-spd: rc: %d", rc);
    logMsg (LOG_DBG, LOG_IMPORTANT, "aa-apply-spd: resp: %s", resp);
  }

  if (rc == 0) {
    newdur = aaParseDuration (resp);
  }

  return newdur;
}

static long
aaParseDuration (const char *resp)
{
  char    *p = NULL;
  long    newdur = -1;
  int     h, m, s, ss;

  /* size=     932kB time=00:00:30.85 bitrate= 247.4kbits/s speed=65.2x */
  p = strstr (resp, "time=");
  if (p != NULL) {
    if (sscanf (p, "time=%d:%d:%d.%d", &h, &m, &s, &ss) == 4) {
      newdur = 0;
      newdur += h;
      newdur *= 60;
      newdur += m;
      newdur *= 60;
      newdur += s;
      newdur *= 1000;
      newdur += ss * 10;
    }
  }

  return newdur;
}
