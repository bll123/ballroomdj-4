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

#include "log.h"
#include "musicdb.h"
#include "song.h"
#include "uisongsel.h"
#include "ui.h"

void
uisongselQueueProcess (uisongsel_t *uisongsel, dbidx_t dbidx)
{
  if (uisongsel->queuecb != NULL) {
    callbackHandlerI (uisongsel->queuecb, dbidx);
  }
}

void
uisongselPlayProcess (uisongsel_t *uisongsel, dbidx_t dbidx, musicqidx_t mqidx)
{
  if (uisongsel->playcb != NULL) {
    callbackHandlerII (uisongsel->playcb, dbidx, mqidx);
  }
}

void
uisongselSetPeerFlag (uisongsel_t *uisongsel, bool val)
{
  uisongsel->ispeercall = val;
}

