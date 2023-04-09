/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
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

  logProcBegin (LOG_PROC, "uisongselInit");

  uisongsel = mdmalloc (sizeof (uisongsel_t));

  uisongsel->tag = tag;
  uisongsel->windowp = NULL;
  uisongsel->uisongfilter = uisf;
  uisongsel->songfilter = uisfGetSongFilter (uisf);
  uisongsel->sfapplycb = NULL;
  uisongsel->sfdanceselcb = NULL;

  uisongsel->sfapplycb = callbackInit (
      uisongselApplySongFilter, uisongsel, NULL);
  uisfSetApplyCallback (uisf, uisongsel->sfapplycb);

  uisongsel->sfdanceselcb = callbackInitLong (
      uisongselDanceSelectCallback, uisongsel);
  uisfSetDanceSelectCallback (uisf, uisongsel->sfdanceselcb);

  uisongsel->conn = conn;
  uisongsel->dispsel = dispsel;
  uisongsel->musicdb = musicdb;
  uisongsel->samesong = samesong;
  uisongsel->dispselType = dispselType;
  uisongsel->options = options;
  uisongsel->idxStart = 0;
  uisongsel->danceIdx = -1;
  uisongsel->dfilterCount = (double) dbCount (musicdb);
  uisongsel->uidance = NULL;
  uisongsel->peercount = 0;
  uisongsel->ispeercall = false;
  for (int i = 0; i < UISONGSEL_PEER_MAX; ++i) {
    uisongsel->peers [i] = NULL;
  }
  uisongsel->newselcb = NULL;
  uisongsel->queuecb = NULL;
  uisongsel->playcb = NULL;
  uisongsel->editcb = NULL;
  uisongsel->songsavecb = NULL;
  uisongsel->songlistdbidxlist = NULL;

  uisongselUIInit (uisongsel);

  logProcEnd (LOG_PROC, "uisongselInit", "");
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
  logProcBegin (LOG_PROC, "uisongselFree");

  if (uisongsel != NULL) {
    nlistFree (uisongsel->songlistdbidxlist);
    uidanceFree (uisongsel->uidance);
    uisongselUIFree (uisongsel);
    callbackFree (uisongsel->sfapplycb);
    callbackFree (uisongsel->sfdanceselcb);
    mdfree (uisongsel);
  }

  logProcEnd (LOG_PROC, "uisongselFree", "");
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
    mp_musicqupdate_t *musicqupdate, int updflag)
{
  nlistidx_t  iteridx;
  mp_musicqupditem_t   *musicqupditem;

  if (updflag == UISONGSEL_MARK_REPLACE ||
      uisongsel->songlistdbidxlist == NULL) {
    nlistFree (uisongsel->songlistdbidxlist);
    uisongsel->songlistdbidxlist = NULL;
    uisongsel->songlistdbidxlist = nlistAlloc ("songlist-dbidx",
        LIST_UNORDERED, NULL);
    nlistSetSize (uisongsel->songlistdbidxlist, nlistGetCount (musicqupdate->dispList));
    updflag = UISONGSEL_MARK_REPLACE;
  }

  nlistSetNum (uisongsel->songlistdbidxlist, musicqupdate->currdbidx, 0);

  nlistStartIterator (musicqupdate->dispList, &iteridx);
  while ((musicqupditem = nlistIterateValueData (musicqupdate->dispList, &iteridx)) != NULL) {
    nlistSetNum (uisongsel->songlistdbidxlist, musicqupditem->dbidx, 0);
  }

  if (updflag == UISONGSEL_MARK_REPLACE) {
    nlistSort (uisongsel->songlistdbidxlist);
  }

  uisongselPopulateData (uisongsel);
}
