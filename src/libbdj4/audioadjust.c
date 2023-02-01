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
#include "bdj4.h"
#include "bdjopt.h"
#include "bdjstring.h"
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

static void aaApplySpeed (song_t *song, const char *infn, const char *outfn, int speed, int gap);

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

void
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

  rc = osProcessPipe (targv, OS_PROC_WAIT | OS_PROC_DETACH, resp, sizeof (resp), &retsz);
  if (rc != 0) {
    char  cmd [1000];

    *cmd = '\0';
    for (int i = 0; i < targc - 1; ++i) {
      strlcat (cmd, targv [i], sizeof (cmd));;
      strlcat (cmd, " ", sizeof (cmd));;
    }
    logMsg (LOG_DBG, LOG_IMPORTANT, "aa-trim: cmd: %s", cmd);
    logMsg (LOG_DBG, LOG_IMPORTANT, "aa-trim: rc: %d", rc);
    logMsg (LOG_DBG, LOG_IMPORTANT, "aa-trim: resp:\n%s", resp);
  }
  logMsg (LOG_DBG, LOG_MAIN, "aa: trim: elapsed: %ld\n",
      (long) mstimeend (&etm));

  return;
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
  if (rc != 0) {
    char  cmd [1000];

    *cmd = '\0';
    for (int i = 0; i < targc - 1; ++i) {
      strlcat (cmd, targv [i], sizeof (cmd));;
      strlcat (cmd, " ", sizeof (cmd));;
    }
    logMsg (LOG_DBG, LOG_IMPORTANT, "aa-norm-a: cmd: %s", cmd);
    logMsg (LOG_DBG, LOG_IMPORTANT, "aa-norm-a: rc: %d", rc);
    logMsg (LOG_DBG, LOG_IMPORTANT, "aa-norm-a: resp:\n%s", resp);
    logMsg (LOG_DBG, LOG_IMPORTANT, "aa-norm: failed, pass one failed.");
    return;
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
  logMsg (LOG_DBG, LOG_MAIN, "aa: norm: elapsed pass 1: %ld\n",
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
  if (rc != 0) {
    char  cmd [1000];

    *cmd = '\0';
    for (int i = 0; i < targc - 1; ++i) {
      strlcat (cmd, targv [i], sizeof (cmd));;
      strlcat (cmd, " ", sizeof (cmd));;
    }
    logMsg (LOG_DBG, LOG_IMPORTANT, "aa-norm-b: cmd: %s", cmd);
    logMsg (LOG_DBG, LOG_IMPORTANT, "aa-norm-b: rc: %d", rc);
    logMsg (LOG_DBG, LOG_IMPORTANT, "aa-norm-b: resp:\n%s", resp);
  }
  logMsg (LOG_DBG, LOG_MAIN, "aa: norm: elapsed: %ld\n",
      (long) mstimeend (&etm));
  return;
}

void
aaApplyAdjustments (song_t *song, const char *infn, const char *outfn,
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
  long        songdur;
  int         speed;
  int         fadetype;
  size_t      retsz;
  const char  *afprefix = "";
  const char  *ftstr = "tri";
  mstime_t    etm;

  mstimestart (&etm);
  *aftext = '\0';

  songstart = songGetNum (song, TAG_SONGSTART);
  songdur = songGetNum (song, TAG_DURATION);
  speed = songGetNum (song, TAG_SPEEDADJUSTMENT);
  if (speed < 0) {
    speed = 100;
  }
  fadetype = bdjoptGetNum (OPT_P_FADETYPE);

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

  targv [targc++] = "-q:a";
  targv [targc++] = "0";
  targv [targc++] = "-i";
  targv [targc++] = infn;

  if (dur > 0 && dur < songdur) {
    long    tdur = dur;

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
        afprefix, ftstr, dur - fadeout, fadeout);
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
  targv [targc++] = outfn;
  targv [targc++] = NULL;
  rc = osProcessPipe (targv, OS_PROC_WAIT | OS_PROC_DETACH, resp, sizeof (resp), &retsz);
  if (rc != 0) {
    char  cmd [1000];

    *cmd = '\0';
    for (int i = 0; i < targc - 1; ++i) {
      strlcat (cmd, targv [i], sizeof (cmd));;
      strlcat (cmd, " ", sizeof (cmd));;
    }
    logMsg (LOG_DBG, LOG_IMPORTANT, "aa-apply-adj: cmd: %s", cmd);
    logMsg (LOG_DBG, LOG_IMPORTANT, "aa-apply-adj: rc: %d", rc);
    logMsg (LOG_DBG, LOG_IMPORTANT, "aa-apply-adj: resp:\n%s", resp);
  }
  logMsg (LOG_DBG, LOG_MAIN, "aa: adjust: elapsed: %ld\n",
      (long) mstimeend (&etm));

  /* applying the speed change afterwards is slower, but saves a lot */
  /* of headache. */
  if (speed != 100) {
    char  tmpfn [MAXPATHLEN];

    snprintf (tmpfn, sizeof (tmpfn), "%s.tmp", outfn);
    filemanipMove (outfn, tmpfn);
    aaApplySpeed (song, tmpfn, outfn, speed, gap);
    fileopDelete (tmpfn);
    logMsg (LOG_DBG, LOG_MAIN, "aa: adjust-with-speed: elapsed: %ld\n",
        (long) mstimeend (&etm));
  }
}

static void
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
  if (rc != 0) {
    char  cmd [1000];

    *cmd = '\0';
    for (int i = 0; i < targc - 1; ++i) {
      strlcat (cmd, targv [i], sizeof (cmd));;
      strlcat (cmd, " ", sizeof (cmd));;
    }
    logMsg (LOG_DBG, LOG_IMPORTANT, "aa-apply-spd: cmd: %s", cmd);
    logMsg (LOG_DBG, LOG_IMPORTANT, "aa-apply-spd: rc: %d", rc);
    logMsg (LOG_DBG, LOG_IMPORTANT, "aa-apply-spd: resp:\n%s", resp);
  }
}

