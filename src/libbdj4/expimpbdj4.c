#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>

#include "bdj4.h"
#include "expimpbdj4.h"
#include "log.h"
#include "mdebug.h"

typedef struct eibdj4 {
  musicdb_t   *musicdb;
  const char  *dirname;
  const char  *plName;
  const char  *newName;
  int         eiflag;
  int         counter;
  int         totcount;
  int         state;
} eibdj4_t;

static bool eibdj4ProcessExport (eibdj4_t *eibdj4);
static bool eibdj4ProcessImport (eibdj4_t *eibdj4);

eibdj4_t *
eibdj4Init (musicdb_t *musicdb, const char *dirname, int eiflag)
{
  eibdj4_t  *eibdj4;

  eibdj4 = mdmalloc (sizeof (eibdj4_t));
  eibdj4->musicdb = musicdb;
  eibdj4->dirname = dirname;
  eibdj4->plName = NULL;
  eibdj4->newName = NULL;
  eibdj4->eiflag = eiflag;
  eibdj4->counter = 0;
  eibdj4->totcount = 0;
  eibdj4->state = BDJ4_STATE_OFF;

  return eibdj4;
}

void
eibdj4Free (eibdj4_t *eibdj4)
{
  if (eibdj4 != NULL) {
    mdfree (eibdj4);
  }
}

void
eibdj4GetCount (eibdj4_t *eibdj4, int *count, int *tot)
{
  if (eibdj4 == NULL) {
    *count = 0;
    *tot = 0;
    return;
  }

  *count = eibdj4->counter;
  *tot = eibdj4->totcount;
}

bool
eibdj4Process (eibdj4_t *eibdj4)
{
  bool    rc = true;

  if (eibdj4 == NULL) {
    return rc;
  }

  if (eibdj4->eiflag == EIBDJ4_EXPORT) {
    rc = eibdj4ProcessExport (eibdj4);
  }
  if (eibdj4->eiflag == EIBDJ4_IMPORT) {
    rc = eibdj4ProcessImport (eibdj4);
  }

  return rc;
}

/* internal routines */

static bool
eibdj4ProcessExport (eibdj4_t *eibdj4)
{
  if (eibdj4 == NULL || eibdj4->state == BDJ4_STATE_OFF) {
    return true;
  }
  return true;
}

static bool
eibdj4ProcessImport (eibdj4_t *eibdj4)
{
  if (eibdj4 == NULL || eibdj4->state == BDJ4_STATE_OFF) {
    return true;
  }
  return true;
}

