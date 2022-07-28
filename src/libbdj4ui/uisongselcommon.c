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

#include "musicdb.h"
#include "song.h"
#include "uisongsel.h"
#include "ui.h"

void
uisongselQueueProcess (uisongsel_t *uisongsel, dbidx_t dbidx, musicqidx_t mqidx)
{
  if (uisongsel->queuecb != NULL) {
    uiutilsCallbackLongIntHandler (uisongsel->queuecb, dbidx, mqidx);
  }
}

void
uisongselPlayProcess (uisongsel_t *uisongsel, dbidx_t dbidx, musicqidx_t mqidx)
{
  if (uisongsel->playcb != NULL) {
    uiutilsCallbackLongIntHandler (uisongsel->playcb, dbidx, mqidx);
  }
}

void
uisongselSetPeerFlag (uisongsel_t *uisongsel, bool val)
{
  uisongsel->ispeercall = val;
}

void
uisongselChangeFavorite (uisongsel_t *uisongsel, dbidx_t dbidx)
{
  song_t    *song;

  song = dbGetByIdx (uisongsel->musicdb, dbidx);
  if (song != NULL) {
    songChangeFavorite (song);
    if (uisongsel->songsavecb != NULL) {
      uiutilsCallbackLongHandler (uisongsel->songsavecb, dbidx);
    }
  }
}

