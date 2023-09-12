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
#include "bdjopt.h"
#include "log.h"
#include "mdebug.h"
#include "ui.h"
#include "callback.h"
#include "uiaudioid.h"

uiaudioid_t *
uiaudioidInit (conn_t *conn, musicdb_t *musicdb,
    dispsel_t *dispsel, nlist_t *options)
{
  uiaudioid_t  *uiaudioid;

  logProcBegin (LOG_PROC, "uiaudioidInit");

  uiaudioid = mdmalloc (sizeof (uiaudioid_t));

  uiaudioid->conn = conn;
  uiaudioid->dispsel = dispsel;
  uiaudioid->musicdb = musicdb;
  uiaudioid->options = options;
  uiaudioid->savecb = NULL;
  uiaudioid->statusMsg = NULL;

  uiaudioidUIInit (uiaudioid);

  logProcEnd (LOG_PROC, "uiaudioidInit", "");
  return uiaudioid;
}

void
uiaudioidFree (uiaudioid_t *uiaudioid)
{
  logProcBegin (LOG_PROC, "uiaudioidFree");

  if (uiaudioid != NULL) {
    uiaudioidUIFree (uiaudioid);
    mdfree (uiaudioid);
  }

  logProcEnd (LOG_PROC, "uiaudioidFree", "");
}

void
uiaudioidMainLoop (uiaudioid_t *uiaudioid)
{
  uiaudioidUIMainLoop (uiaudioid);
  return;
}

void
uiaudioidNewSelection (uiaudioid_t *uiaudioid, dbidx_t dbidx)
{
  song_t      *song;

  song = dbGetByIdx (uiaudioid->musicdb, dbidx);
  uiaudioidLoadData (uiaudioid, song, dbidx);
}

void
uiaudioidSetSaveCallback (uiaudioid_t *uiaudioid, callback_t *uicb)
{
  if (uiaudioid == NULL) {
    return;
  }

  uiaudioid->savecb = uicb;
}
