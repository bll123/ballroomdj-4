/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include <stdbool.h>
#include <sys/types.h>

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

#include "conn.h"
#include "dispsel.h"
#include "grouping.h"
#include "msgparse.h"
#include "musicdb.h"
#include "musicq.h"
#include "ilist.h"
#include "nlist.h"
#include "samesong.h"
#include "songfilter.h"
#include "uidance.h"
#include "uisongfilter.h"
#include "uiwcont.h"

typedef struct uisongsel uisongsel_t;
typedef struct ss_internal ss_internal_t;

enum {
  UISONGSEL_MQ_NOTSET = -1,
};

typedef struct uisongsel {
  const char        *tag;
  conn_t            *conn;
  ilistidx_t        danceIdx;
  nlist_t           *options;
  dispsel_t         *dispsel;
  musicdb_t         *musicdb;
  samesong_t        *samesong;
  dispselsel_t      dispselType;
  int32_t           numrows;
  uiwcont_t         *windowp;
  callback_t        *queuecb;
  callback_t        *playcb;
  callback_t        *editcb;
  callback_t        *songsavecb;
  dbidx_t           lastdbidx;
  nlist_t           *musicqdbidxlist [MUSICQ_MAX];
  nlist_t           *songlistdbidxlist;
  grouping_t        *grouping;
  /* filter data */
  uisongfilter_t    *uisongfilter;
  callback_t        *sfapplycb;
  callback_t        *sfdanceselcb;
  songfilter_t      *songfilter;
  /* song selection tab */
  uidance_t         *uidance;
  /* internal data */
  ss_internal_t     *ssInternalData;
  /* song editor */
  callback_t        *newselcb;
} uisongsel_t;

/* uisongsel.c */
uisongsel_t * uisongselInit (const char *tag, conn_t *conn, musicdb_t *musicdb, grouping_t *grouping, dispsel_t *dispsel, samesong_t *samesong, nlist_t *opts, uisongfilter_t *uisf, dispselsel_t dispselType);
void  uisongselInitializeSongFilter (uisongsel_t *uisongsel, songfilter_t *songfilter);
void  uisongselSetDatabase (uisongsel_t *uisongsel, musicdb_t *musicdb);
void  uisongselSetSamesong (uisongsel_t *uisongsel, samesong_t *samesong);
void  uisongselFree (uisongsel_t *uisongsel);
void  uisongselMainLoop (uisongsel_t *uisongsel);
void  uisongselSetSelectionCallback (uisongsel_t *uisongsel, callback_t *uicb);
void  uisongselSetQueueCallback (uisongsel_t *uisongsel, callback_t *uicb);
void  uisongselSetPlayCallback (uisongsel_t *uisongsel, callback_t *uicb);
void  uisongselSetSongSaveCallback (uisongsel_t *uisongsel, callback_t *uicb);
void  uisongselSetEditCallback (uisongsel_t *uisongsel, callback_t *uicb);
void  uisongselProcessMusicQueueData (uisongsel_t *uisongsel, mp_musicqupdate_t *musicqupdate);

/* uisongselui.c */
void  uisongselUIInit (uisongsel_t *uisongsel);
void  uisongselUIFree (uisongsel_t *uisongsel);
uiwcont_t   * uisongselBuildUI (uisongsel_t *uisongsel, uiwcont_t *parentwin);
void  uisongselPopulateData (uisongsel_t *uisongsel);
bool  uisongselSelectCallback (void *udata);
bool  uisongselNextSelection (void *udata);
bool  uisongselPreviousSelection (void *udata);
bool  uisongselFirstSelection (void *udata);
nlistidx_t uisongselGetSelectLocation (uisongsel_t *uisongsel);
bool  uisongselApplySongFilter (void *udata);
void  uisongselDanceSelectHandler (uisongsel_t *uisongsel, ilistidx_t idx);
bool  uisongselDanceSelectCallback (void *udata, int32_t danceIdx);
bool uisongselPlayCallback (void *udata);
void  uisongselSetPlayButtonState (uisongsel_t *uisongsel, int active);
nlist_t *uisongselGetSelectedList (uisongsel_t *uisongsel);
void uisongselSetRequestLabel (uisongsel_t *uisongsel, const char *txt);
void uisongselSetSelection (uisongsel_t *uisongsel, int32_t idx);
void uisongselCopySelectList (uisongsel_t *uisongsel, uisongsel_t *peer);

/* uisongselcommon.c */
void  uisongselQueueProcess (uisongsel_t *uisongsel, dbidx_t dbidx);
void  uisongselPlayProcess (uisongsel_t *uisongsel, dbidx_t dbidx, musicqidx_t mqidx);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif
