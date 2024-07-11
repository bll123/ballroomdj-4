/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <math.h>

#include "bdj4.h"
#include "bdj4intl.h"
#include "bdj4ui.h"
#include "bdjopt.h"
#include "datafile.h"
#include "log.h"
#include "mdebug.h"
#include "samesong.h"
#include "uimusicq.h"
#include "uisongsel.h"
#include "ui.h"
#include "callback.h"

uisongsel_t *
uisongselInit (const char *tag, conn_t *conn, musicdb_t *musicdb,
    dispsel_t *dispsel, samesong_t *samesong, nlist_t *options,
    uisongfilter_t *uisf, dispselsel_t dispselType)
{
  uisongsel_t   *uisongsel;

  logProcBegin ();

  uisongsel = mdmalloc (sizeof (uisongsel_t));

  uisongsel->tag = tag;
  uisongsel->windowp = NULL;
  uisongsel->uisongfilter = uisf;
  uisongsel->songfilter = uisfGetSongFilter (uisf);
  uisongsel->sfapplycb = NULL;
  uisongsel->sfdanceselcb = NULL;
  for (int i = 0; i < UISONGSEL_PEER_MAX; ++i) {
    uisongsel->peers [i] = NULL;
  }
  uisongsel->conn = conn;
  uisongsel->dispsel = dispsel;
  uisongsel->musicdb = musicdb;
  uisongsel->samesong = samesong;
  uisongsel->dispselType = dispselType;
  uisongsel->options = options;
  uisongsel->idxStart = 0;
  uisongsel->danceIdx = -1;
  uisongsel->uidance = NULL;
  uisongsel->peercount = 0;
  uisongsel->ispeercall = false;
  uisongsel->newselcb = NULL;
  uisongsel->queuecb = NULL;
  uisongsel->playcb = NULL;
  uisongsel->editcb = NULL;
  uisongsel->songsavecb = NULL;
  for (int i = 0; i < MUSICQ_MAX; ++i) {
    uisongsel->musicqdbidxlist [i] = NULL;
  }
  uisongsel->songlistdbidxlist = NULL;

  uisongsel->numrows = dbCount (musicdb);

  uisongsel->sfapplycb = callbackInit (
      uisongselApplySongFilter, uisongsel, NULL);
  uisfSetApplyCallback (uisf, uisongsel->sfapplycb);

  uisongsel->sfdanceselcb = callbackInitI (
      uisongselDanceSelectCallback, uisongsel);
  uisfSetDanceSelectCallback (uisf, uisongsel->sfdanceselcb);

  uisongselUIInit (uisongsel);

  logProcEnd ("");
  return uisongsel;
}

void
uisongselSetPeer (uisongsel_t *uisongsel, uisongsel_t *peer)
{
  if (uisongsel->peercount >= UISONGSEL_PEER_MAX) {
    return;
  }
  uisongsel->peers [uisongsel->peercount] = peer;
  ++uisongsel->peercount;
}

void
uisongselSetDatabase (uisongsel_t *uisongsel, musicdb_t *musicdb)
{
  uisongsel->musicdb = musicdb;
}

void
uisongselSetSamesong (uisongsel_t *uisongsel, samesong_t *samesong)
{
  uisongsel->samesong = samesong;
}

void
uisongselFree (uisongsel_t *uisongsel)
{
  logProcBegin ();

  if (uisongsel != NULL) {
    for (int i = 0; i < MUSICQ_MAX; ++i) {
      nlistFree (uisongsel->musicqdbidxlist [i]);
    }
    nlistFree (uisongsel->songlistdbidxlist);
    uidanceFree (uisongsel->uidance);
    uisongselUIFree (uisongsel);
    callbackFree (uisongsel->sfapplycb);
    callbackFree (uisongsel->sfdanceselcb);
    mdfree (uisongsel);
  }

  logProcEnd ("");
}

void
uisongselMainLoop (uisongsel_t *uisongsel)
{
  return;
}

void
uisongselSetSelectionCallback (uisongsel_t *uisongsel, callback_t *uicbdbidx)
{
  if (uisongsel == NULL) {
    return;
  }
  uisongsel->newselcb = uicbdbidx;
}

void
uisongselSetQueueCallback (uisongsel_t *uisongsel, callback_t *uicb)
{
  if (uisongsel == NULL) {
    return;
  }
  uisongsel->queuecb = uicb;
}

void
uisongselSetPlayCallback (uisongsel_t *uisongsel, callback_t *uicb)
{
  if (uisongsel == NULL) {
    return;
  }
  uisongsel->playcb = uicb;
}

void
uisongselSetEditCallback (uisongsel_t *uisongsel, callback_t *uicb)
{
  if (uisongsel == NULL) {
    return;
  }

  uisongsel->editcb = uicb;
}

void
uisongselSetSongSaveCallback (uisongsel_t *uisongsel, callback_t *uicb)
{
  if (uisongsel == NULL) {
    return;
  }
  uisongsel->songsavecb = uicb;
}

void
uisongselProcessMusicQueueData (uisongsel_t *uisongsel,
    mp_musicqupdate_t *musicqupdate)
{
  nlistidx_t          iteridx;
  mp_musicqupditem_t  *musicqupditem;
  int                 mqidx;
  int                 count;

  mqidx = musicqupdate->mqidx;

  nlistFree (uisongsel->musicqdbidxlist [mqidx]);
  /* no need to order the individual music-q dbidx lists */
  uisongsel->musicqdbidxlist [mqidx] = nlistAlloc ("musicq-dbidx",
      LIST_UNORDERED, NULL);
  nlistSetSize (uisongsel->musicqdbidxlist [mqidx], nlistGetCount (musicqupdate->dispList));

  if (musicqupdate->currdbidx >= 0) {
    nlistSetNum (uisongsel->musicqdbidxlist [mqidx], musicqupdate->currdbidx, 0);
  }

  nlistStartIterator (musicqupdate->dispList, &iteridx);
  while ((musicqupditem = nlistIterateValueData (musicqupdate->dispList, &iteridx)) != NULL) {
    nlistSetNum (uisongsel->musicqdbidxlist [mqidx], musicqupditem->dbidx, 0);
  }

  /* now rebuild the complete list using each music-queue-dbidx-list */
  nlistFree (uisongsel->songlistdbidxlist);
  /* there may be duplicate db indexes, so start as ordered */
  uisongsel->songlistdbidxlist = nlistAlloc ("songlist-dbidx",
      LIST_ORDERED, NULL);
  count = 0;
  for (int i = 0; i < MUSICQ_MAX; ++i) {
    if (uisongsel->musicqdbidxlist [i] == NULL) {
      continue;
    }
    count += nlistGetCount (uisongsel->musicqdbidxlist [i]);
  }
  nlistSetSize (uisongsel->songlistdbidxlist, count);

  for (int i = 0; i < MUSICQ_MAX; ++i) {
    dbidx_t   dbidx;

    if (uisongsel->musicqdbidxlist [i] == NULL) {
      continue;
    }
    nlistStartIterator (uisongsel->musicqdbidxlist [i], &iteridx);
    while ((dbidx = nlistIterateKey (uisongsel->musicqdbidxlist [i], &iteridx)) >=0) {
      nlistSetNum (uisongsel->songlistdbidxlist, dbidx, 0);
    }
  }

  uisongselPopulateData (uisongsel);
}

