/*
 * Copyright 2024-2026 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>

#include "bdj4.h"
#include "datafile.h"
#include "mdebug.h"
#include "nodiscard.h"
#include "pathbld.h"
#include "quickedit.h"

/* these are the user configurable selections */
datafilekey_t quickeditdisplaydfkeys [QUICKEDIT_DISP_MAX] = {
  { "DANCELEVEL",   QUICKEDIT_DISP_DANCELEVEL,    VALUE_NUM, convBoolean, DF_NORM },
  { "DANCERATING",  QUICKEDIT_DISP_DANCERATING,   VALUE_NUM, convBoolean, DF_NORM },
  { "FAVORITE",     QUICKEDIT_DISP_FAVORITE,      VALUE_NUM, convBoolean, DF_NORM },
  { "SPEED",        QUICKEDIT_DISP_SPEED,         VALUE_NUM, convBoolean, DF_NORM },
  { "VOLUME",       QUICKEDIT_DISP_VOLUME,        VALUE_NUM, convBoolean, DF_NORM },
};

typedef struct quickedit {
  datafile_t    *df;
  nlist_t       *dispsel;
} quickedit_t;

NODISCARD
quickedit_t *
quickeditAlloc (void)
{
  quickedit_t     *qe;
  char            tbuff [BDJ4_PATH_MAX];

  qe = mdmalloc (sizeof (*qe));
  qe->df = NULL;
  qe->dispsel = NULL;

  pathbldMakePath (tbuff, sizeof (tbuff),
      DS_QUICKEDIT_FN, BDJ4_CONFIG_EXT, PATHBLD_MP_DREL_DATA | PATHBLD_MP_USEIDX);
  qe->df = datafileAllocParse (DS_QUICKEDIT_FN,
      DFTYPE_KEY_VAL, tbuff, quickeditdisplaydfkeys, QUICKEDIT_DISP_MAX, DF_NO_OFFSET, NULL);
  qe->dispsel = datafileGetList (qe->df);

  return qe;
}

void
quickeditFree (quickedit_t *qe)
{
  if (qe == NULL) {
    return;
  }

  datafileFree (qe->df);
  mdfree (qe);
}

nlist_t *
quickeditGetList (quickedit_t *qe)
{
  if (qe == NULL) {
    return NULL;
  }

  return qe->dispsel;
}

void
quickeditSave (quickedit_t *qe, nlist_t *dispsel)
{
  if (qe == NULL || qe->df == NULL || dispsel == NULL) {
    return;
  }

  datafileSave (qe->df, NULL, dispsel, DF_NO_OFFSET, 1);
}
