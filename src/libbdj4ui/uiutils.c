#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <math.h>

#include "bdjvarsdf.h"
#include "dance.h"
#include "datafile.h"
#include "log.h"
#include "ui.h"
#include "uiutils.h"

void
uiutilsCreateDanceList (uidropdown_t *dropdown, char *selectLabel)
{
  dance_t           *dances;
  slist_t           *danceList;

  logProcBegin (LOG_PROC, "uiutilsCreateDanceList");

  dances = bdjvarsdfGet (BDJVDF_DANCES);
  danceList = danceGetDanceList (dances);

  uiDropDownSetNumList (dropdown, danceList, selectLabel);
  logProcEnd (LOG_PROC, "uiutilsCreateDanceList", "");
}

uiutilsnbtabid_t *
uiutilsNotebookIDInit (void)
{
  uiutilsnbtabid_t *nbtabid;

  nbtabid = malloc (sizeof (uiutilsnbtabid_t));
  nbtabid->tabcount = 0;
  nbtabid->tabids = NULL;
  return nbtabid;
}

void
uiutilsNotebookIDFree (uiutilsnbtabid_t *nbtabid)
{
  if (nbtabid != NULL) {
    if (nbtabid->tabids != NULL) {
      free (nbtabid->tabids);
    }
    free (nbtabid);
  }
}

void
uiutilsNotebookIDAdd (uiutilsnbtabid_t *nbtabid, int id)
{
  nbtabid->tabids = realloc (nbtabid->tabids,
      sizeof (int) * (nbtabid->tabcount + 1));
  nbtabid->tabids [nbtabid->tabcount] = id;
  ++nbtabid->tabcount;
}

int
uiutilsNotebookIDGet (uiutilsnbtabid_t *nbtabid, int idx)
{
  if (nbtabid == NULL) {
    return 0;
  }
  if (idx >= nbtabid->tabcount) {
    return 0;
  }
  return nbtabid->tabids [idx];
}

int
uiutilsNotebookIDGetPage (uiutilsnbtabid_t *nbtabid, int id)
{
  if (nbtabid == NULL) {
    return 0;
  }

  /* just brute forced.  the list is very small. */
  /* used by manageui */
  for (int i = 0; i < nbtabid->tabcount; ++i) {
    if (id == nbtabid->tabids [i]) {
      return i;
    }
  }

  return 0;
}

void
uiutilsNotebookIDStartIterator (uiutilsnbtabid_t *nbtabid, int *iteridx)
{
  *iteridx = -1;
}

int
uiutilsNotebookIDIterate (uiutilsnbtabid_t *nbtabid, int *iteridx)
{
  ++(*iteridx);
  if (*iteridx >= nbtabid->tabcount) {
    return -1;
  }
  return nbtabid->tabids [*iteridx];
}



inline void
uiutilsUIWidgetCopy (UIWidget *target, UIWidget *source)
{
  if (target == NULL) {
    return;
  }
  if (source == NULL) {
    return;
  }
  memcpy (target, source, sizeof (UIWidget));
}

void
uiutilsUICallbackInit (UICallback *uicb, UICallbackFunc cb, void *udata)
{
  if (uicb == NULL) {
    return;
  }

  uicb->cb = cb;
  uicb->udata = udata;
}

void
uiutilsUICallbackDoubleInit (UICallback *uicb, UIDoubleCallbackFunc cb, void *udata)
{
  if (uicb == NULL) {
    return;
  }

  uicb->doublecb = cb;
  uicb->udata = udata;
}

void
uiutilsUICallbackIntIntInit (UICallback *uicb, UIIntIntCallbackFunc cb, void *udata)
{
  if (uicb == NULL) {
    return;
  }

  uicb->intintcb = cb;
  uicb->udata = udata;
}

void
uiutilsUICallbackLongInit (UICallback *uicb, UILongCallbackFunc cb, void *udata)
{
  if (uicb == NULL) {
    return;
  }

  uicb->longcb = cb;
  uicb->udata = udata;
}

void
uiutilsUICallbackLongIntInit (UICallback *uicb, UILongIntCallbackFunc cb, void *udata)
{
  if (uicb == NULL) {
    return;
  }

  uicb->longintcb = cb;
  uicb->udata = udata;
}

void
uiutilsUICallbackStrInit (UICallback *uicb, UIStrCallbackFunc cb, void *udata)
{
  if (uicb == NULL) {
    return;
  }

  uicb->strcb = cb;
  uicb->udata = udata;
}
