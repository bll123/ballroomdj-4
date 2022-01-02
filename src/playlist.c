#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "playlist.h"
#include "datafile.h"
#include "fileop.h"
#include "pathutil.h"
#include "portability.h"

static void     playlistFreeList (void *tpllist);
static void     plConvResume (char *, datafileret_t *ret);
static void     plConvWait (char *, datafileret_t *ret);
static void     plConvStopType (char *, datafileret_t *ret);

  /* must be sorted in ascii order */
static datafilekey_t playlistdfkeys[] = {
  { "AllowedKeywords",  PLAYLIST_ALLOWED_KEYWORDS,
      VALUE_DATA, parseConvTextList },
  { "DanceRating",      PLAYLIST_RATING,
      VALUE_DATA, NULL },
  { "Gap",              PLAYLIST_GAP,
      VALUE_DATA, NULL },
  { "HighDanceLevel",   PLAYLIST_LEVEL_LOW,
      VALUE_DATA, NULL },
  { "LowDanceLevel",    PLAYLIST_LEVEL_HIGH,
      VALUE_DATA, NULL },
  { "ManualList",       PLAYLIST_MANUAL_LIST_NAME,
      VALUE_DATA, NULL },
  { "MaxPlayTime",      PLAYLIST_MAX_PLAY_TIME,
      VALUE_DATA, NULL },
  { "MqMessage",        PLAYLIST_MQ_MESSAGE,
      VALUE_DATA, NULL },
  { "PauseEachSong",    PLAYLIST_PAUSE_EACH_SONG,
      VALUE_LONG, parseConvBoolean },
  { "PlayAnnounce",     PLAYLIST_ANNOUNCE,
      VALUE_LONG, parseConvBoolean },
  { "RequiredKeywords", PLAYLIST_REQ_KEYWORDS,
      VALUE_DATA, parseConvTextList },
  { "Resume",           PLAYLIST_RESUME,
      VALUE_DATA, plConvResume },
  { "Sequence",         PLAYLIST_SEQ_NAME,
      VALUE_DATA, NULL },
  { "StopAfter",        PLAYLIST_STOP_AFTER,
      VALUE_DATA, plConvWait },
  { "StopAfterWait",    PLAYLIST_STOP_AFTER_WAIT,
      VALUE_DATA, plConvWait },
  { "StopTime",         PLAYLIST_STOP_TIME,
      VALUE_DATA, NULL },
  { "StopType",         PLAYLIST_STOP_TYPE,
      VALUE_DATA, plConvStopType },
  { "StopWait",         PLAYLIST_STOP_WAIT,
      VALUE_DATA, plConvWait },
  { "UseStatus",        PLAYLIST_USE_STATUS,
      VALUE_DATA, parseConvBoolean },
  { "UseUnrated",       PLAYLIST_USE_UNRATED,
      VALUE_DATA, parseConvBoolean },
};
#define PLAYLIST_DFKEY_COUNT (sizeof (playlistdfkeys) / sizeof (datafilekey_t))

datafile_t *
playlistAlloc (char *fname)
{
  datafile_t    *pldf;
  datafile_t    *pldancedf;
  pathinfo_t    *pi;
  char          tfn [MAXPATHLEN];
  list_t        *pllist;

  pi = pathInfo (fname);
  snprintf (tfn, sizeof (tfn), "data/%s.pl", pi->basename);
  if (! fileopExists (tfn)) {
    return NULL;
  }
  pldf = datafileAlloc ("playlist", playlistdfkeys, PLAYLIST_DFKEY_COUNT,
      fname, DFTYPE_KEY_VAL);
  snprintf (tfn, sizeof (tfn), "data/%s.pldance", pi->basename);
  if (! fileopExists (tfn)) {
    datafileFree (pldf);
    return NULL;
  }
    /* ### FIX TODO: want to redo this to use a dynamic list of dance keys */
    /* the dance loader must be written &etc. */
  pldancedf = datafileAlloc ("playlist-dances", NULL, 0, fname, DFTYPE_KEY_VAL);
  pathInfoFree (pi);

  pllist = datafileGetData (pldf);
  llistSetFreeHook (pllist, playlistFreeList);
  llistSetData (pllist, PLAYLIST_DANCEDF, pldancedf);
  llistSetData (pllist, PLAYLIST_DANCES, datafileGetData (pldancedf));

  // rebuild allowed keywords
  // rebuild required keywords
  // convert high level
  // convert low level

  // handle dances; this may require a re-parse?
  // have datafile save the parse???

  return pldf;
}

void
playlistFree (datafile_t *df)
{
  datafileFree (df);
}

/* internal routines */

static void
playlistFreeList (void *tpllist)
{
  list_t      *pllist = (list_t *) tpllist;
  datafileFree (llistGetData (pllist, PLAYLIST_DANCEDF));
  llistSetData (pllist, PLAYLIST_DANCEDF, NULL);
    /* this is no longer a valid pointer */
  llistSetData (pllist, PLAYLIST_DANCES, NULL);
}

static void
plConvResume (char *data, datafileret_t *ret)
{
  ret->valuetype = VALUE_LONG;
  ret->u.l = RESUME_FROM_LAST;
  if (strcmp (data, "From Start") == 0) {
    ret->u.l = RESUME_FROM_START;
  }
}

static void
plConvWait (char *data, datafileret_t *ret)
{
  ret->valuetype = VALUE_LONG;
  ret->u.l = WAIT_CONTINUE;
  if (strcmp (data, "") == 0) {
    ret->u.l = WAIT_PAUSE;
  }
}

static void
plConvStopType (char *data, datafileret_t *ret)
{
  ret->valuetype = VALUE_LONG;
  ret->u.l = STOP_AT;
  if (strcmp (data, "In") == 0) {
    ret->u.l = STOP_IN;
  }
}

