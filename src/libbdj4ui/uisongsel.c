#include "config.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>
#include <math.h>

#include "bdj4.h"
#include "bdj4intl.h"
#include "bdj4ui.h"
#include "bdjopt.h"
#include "datafile.h"
#include "log.h"
#include "uimusicq.h"
#include "uisongsel.h"
#include "ui.h"

uisongsel_t *
uisongselInit (const char *tag, conn_t *conn, musicdb_t *musicdb,
    dispsel_t *dispsel, nlist_t *options,
    uisongfilter_t *uisf, dispselsel_t dispselType)
{
  uisongsel_t   *uisongsel;

  logProcBegin (LOG_PROC, "uisongselInit");

  uisongsel = malloc (sizeof (uisongsel_t));
  assert (uisongsel != NULL);

  uisongsel->tag = tag;
  uisongsel->windowp = NULL;
  uisongsel->uisongfilter = uisf;
  uisongsel->songfilter = uisfGetSongFilter (uisf);

  uiutilsUICallbackInit (&uisongsel->sfapplycb,
      uisongselApplySongFilter, uisongsel);
  uisfSetApplyCallback (uisf, &uisongsel->sfapplycb);

  uiutilsUICallbackLongInit (&uisongsel->sfdanceselcb,
      uisongselDanceSelectCallback, uisongsel);
  uisfSetDanceSelectCallback (uisf, &uisongsel->sfdanceselcb);

  uisongsel->conn = conn;
  uisongsel->dispsel = dispsel;
  uisongsel->musicdb = musicdb;
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
  uisongsel->songlistdbidxlist = NULL;
  uisongsel->samesonglist = slistAlloc ("samesong-list", LIST_ORDERED, free);

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
uisongselFree (uisongsel_t *uisongsel)
{
  logProcBegin (LOG_PROC, "uisongselFree");

  if (uisongsel != NULL) {
    if (uisongsel->songlistdbidxlist != NULL) {
      nlistFree (uisongsel->songlistdbidxlist);
    }
    if (uisongsel->samesonglist != NULL) {
      slistFree (uisongsel->samesonglist);
    }
    uidanceFree (uisongsel->uidance);
    uisongselUIFree (uisongsel);
    free (uisongsel);
  }

  logProcEnd (LOG_PROC, "uisongselFree", "");
}

void
uisongselMainLoop (uisongsel_t *uisongsel)
{
  return;
}

int
uisongselProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
    bdjmsgmsg_t msg, char *args, void *udata)
{
  uisongsel_t   *uisongsel = udata;
  bool          dbgdisp = false;

  /* if a message is added that has arguments, then the args var */
  /* needs to be strdup'd (see uimusicq.c) */

  switch (route) {
    case ROUTE_NONE:
    case ROUTE_MANAGEUI:
    case ROUTE_PLAYERUI: {
      switch (msg) {
        case MSG_DB_ENTRY_UPDATE:
        case MSG_DATABASE_UPDATE: {
          /* re-filter the display */
          uisongselApplySongFilter (uisongsel);
          dbgdisp = true;
          break;
        }
        default: {
          break;
        }
      }
      break;
    }
    default: {
      break;
    }
  }

  if (dbgdisp) {
    logMsg (LOG_DBG, LOG_MSGS, "uisongsel %s (%d): rcvd: from:%d/%s route:%d/%s msg:%d/%s args:%s",
        uisongsel->tag, uisongsel->dispselType,
        routefrom, msgRouteDebugText (routefrom),
        route, msgRouteDebugText (route), msg, msgDebugText (msg), args);
  }

  return 0;
}

void
uisongselSetSelectionCallback (uisongsel_t *uisongsel, UICallback *uicbdbidx)
{
  if (uisongsel == NULL) {
    return;
  }
  uisongsel->newselcb = uicbdbidx;
}

void
uisongselSetQueueCallback (uisongsel_t *uisongsel, UICallback *uicb)
{
  if (uisongsel == NULL) {
    return;
  }
  uisongsel->queuecb = uicb;
}

void
uisongselSetPlayCallback (uisongsel_t *uisongsel, UICallback *uicb)
{
  if (uisongsel == NULL) {
    return;
  }
  uisongsel->playcb = uicb;
}

void
uisongselSetEditCallback (uisongsel_t *uisongsel, UICallback *uicb)
{
  if (uisongsel == NULL) {
    return;
  }

  uisongsel->editcb = uicb;
}

void
uisongselSetSongSaveCallback (uisongsel_t *uisongsel, UICallback *uicb)
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
  nlistidx_t  iteridx;
  mp_musicqupditem_t   *musicqupditem;

  if (uisongsel->songlistdbidxlist != NULL) {
    nlistFree (uisongsel->songlistdbidxlist);
    uisongsel->songlistdbidxlist = NULL;
  }

  uisongsel->songlistdbidxlist = nlistAlloc ("songlist-dbidx",
      LIST_ORDERED, NULL);

  nlistStartIterator (musicqupdate->dispList, &iteridx);
  while ((musicqupditem = nlistIterateValueData (musicqupdate->dispList, &iteridx)) != NULL) {
    nlistSetNum (uisongsel->songlistdbidxlist, musicqupditem->dbidx, 0);
  }

  uisongselPopulateData (uisongsel);
}
