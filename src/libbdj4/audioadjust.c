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
#include "bdjvarsdf.h"
#include "datafile.h"
#include "fileop.h"
#include "log.h"
#include "mdebug.h"
#include "osprocess.h"
#include "pathbld.h"
#include "sysvars.h"

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
aaNormalize (const char *ffn)
{
  aa_t        *aa;
  const char  *targv [10];
  int         targc = 0;
  char        ffargs [300];
  char        resp [2000];
  int         rc;
  size_t      retsz;

  aa = bdjvarsdfGet (BDJVDF_AUDIO_ADJUST);

  snprintf (ffargs, sizeof (ffargs),
      "loudnorm=I=%.3f:LRA=%.3f:tp=%.3f:print_format=json",
      nlistGetDouble (aa->values, AA_LOUDNORM_TARGET_IL),
      nlistGetDouble (aa->values, AA_LOUDNORM_TARGET_LRA),
      nlistGetDouble (aa->values, AA_LOUDNORM_TARGET_TP));

  targv [targc++] = sysvarsGetStr (SV_PATH_FFMPEG);
  targv [targc++] = "-hide_banner";
  targv [targc++] = "-i";
  targv [targc++] = ffn;
  targv [targc++] = "-filter:a";
  targv [targc++] = ffargs;
  targv [targc++] = "-f";
  targv [targc++] = "null";
  targv [targc++] = "-";
  targv [targc++] = NULL;
  rc = osProcessPipe (targv, OS_PROC_WAIT | OS_PROC_DETACH, resp, sizeof (resp), &retsz);
  if (rc != 0) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "aa-norm: rc: %d", rc);
  }
  fprintf (stderr, "resp: %s\n", resp);
}
