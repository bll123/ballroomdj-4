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

static bool uisongselDanceSelectCallback (void *udata, long danceIdx);

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
//  uisongsel->dancesel = uiDropDownInit ();
  uisongsel->peercount = 0;
  uisongsel->ispeercall = false;
  for (int i = 0; i < UISONGSEL_PEER_MAX; ++i) {
    uisongsel->peers [i] = NULL;
  }
  uisongsel->newselcb = NULL;

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
    uidanceFree (uisongsel->uidance);
//    uiDropDownFree (uisongsel->dancesel);
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

/* handles the dance drop-down */
/* when a dance is selected, the song filter must be updated */
/* call danceselectcallback to set all the peer drop-downs */
/* will apply the filter */
void
uisongselDanceSelectHandler (uisongsel_t *uisongsel, ssize_t danceIdx)
{
  logProcBegin (LOG_PROC, "uisongselFilterDanceProcess");

  uisfSetDanceIdx (uisongsel->uisongfilter, danceIdx);
  uisongselDanceSelectCallback (uisongsel, danceIdx);
  uisongselApplySongFilter (uisongsel);

  logProcEnd (LOG_PROC, "uisongselDanceSelectHandler", "");
}

void
uisongselQueueProcess (uisongsel_t *uisongsel, dbidx_t dbidx, musicqidx_t mqidx)
{
  if (uisongsel->queuecb != NULL) {
    uiutilsCallbackLongIntHandler (uisongsel->queuecb, dbidx, mqidx);
  }
}

void
uisongselChangeFavorite (uisongsel_t *uisongsel, dbidx_t dbidx)
{
  song_t    *song;

  song = dbGetByIdx (uisongsel->musicdb, (ssize_t) dbidx);
  songChangeFavorite (song);
// ### TODO song data must be saved to the database.
  logMsg (LOG_DBG, LOG_SONGSEL, "%s favorite changed", uisongsel->tag);
  uisongselPopulateData (uisongsel);
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
uisongselSetPeerFlag (uisongsel_t *uisongsel, bool val)
{
  uisongsel->ispeercall = val;
}

void
uisongselSetQueueCallback (uisongsel_t *uisongsel, UICallback *uicb)
{
  if (uisongsel == NULL) {
    return;
  }
  uisongsel->queuecb = uicb;
}

bool
uisongselApplySongFilter (void *udata)
{
  uisongsel_t *uisongsel = udata;

  uisongsel->dfilterCount = (double) songfilterProcess (
      uisongsel->songfilter, uisongsel->musicdb);
  uisongsel->idxStart = 0;

  /* the call to cleardata() will remove any selections */
  /* afterwards, make sure something is selected */
  uisongselClearData (uisongsel);
  uisongselPopulateData (uisongsel);

  /* the song filter has been processed, the peers need to be populated */

  for (int i = 0; i < uisongsel->peercount; ++i) {
    if (uisongsel->peers [i] == NULL) {
      continue;
    }
    uisongselClearData (uisongsel->peers [i]);
    uisongselPopulateData (uisongsel->peers [i]);
  }

  /* set the selection after the populate is done */

  uisongselSetDefaultSelection (uisongsel);

//  for (int i = 0; i < uisongsel->peercount; ++i) {
//    if (uisongsel->peers [i] == NULL) {
//      continue;
//    }
//    uisongselSetDefaultSelection (uisongsel->peers [i]);
//  }

  return UICB_CONT;
}

void
uisongselSetEditCallback (uisongsel_t *uisongsel, UICallback *uicb)
{
  if (uisongsel == NULL) {
    return;
  }

  uisongsel->editcb = uicb;
}

/* internal routines */


/* callback for the song filter when the dance selection is changed */
/* also used by DanceSelectHandler to set the peers dance drop-down */
/* does not apply the filter */
static bool
uisongselDanceSelectCallback (void *udata, long danceIdx)
{
  uisongsel_t *uisongsel = udata;

  uidanceSetValue (uisongsel->uidance, danceIdx);
//  uiDropDownSelectionSetNum (uisongsel->dancesel, danceIdx);

  if (uisongsel->ispeercall) {
    return UICB_CONT;
  }

  for (int i = 0; i < uisongsel->peercount; ++i) {
    if (uisongsel->peers [i] == NULL) {
      continue;
    }
    uisongselSetPeerFlag (uisongsel->peers [i], true);
    uisongselDanceSelectCallback (uisongsel->peers [i], danceIdx);
    uisongselSetPeerFlag (uisongsel->peers [i], false);
  }
  return UICB_CONT;
}

