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
#include "datautil.h"
#include "rating.h"
#include "level.h"
#include "dance.h"

static void     playlistFreeList (void *tpllist);
static void     plConvResume (char *, datafileret_t *ret);
static void     plConvWait (char *, datafileret_t *ret);
static void     plConvStopType (char *, datafileret_t *ret);
static void     plConvType (char *, datafileret_t *ret);

  /* must be sorted in ascii order */
static datafilekey_t playlistdfkeys[] = {
  { "ALLOWEDKEYWORDS",  PLAYLIST_ALLOWED_KEYWORDS,
      VALUE_DATA, parseConvTextList },
  { "DANCERATING",      PLAYLIST_RATING,
      VALUE_DATA, ratingConv },
  { "GAP",              PLAYLIST_GAP,
      VALUE_DATA, NULL },
  { "HIGHDANCELEVEL",   PLAYLIST_LEVEL_LOW,
      VALUE_DATA, levelConv },
  { "LOWDANCELEVEL",    PLAYLIST_LEVEL_HIGH,
      VALUE_DATA, levelConv },
  { "MANUALLIST",       PLAYLIST_MANUAL_LIST_NAME,
      VALUE_DATA, NULL },
  { "MAXPLAYTIME",      PLAYLIST_MAX_PLAY_TIME,
      VALUE_DATA, NULL },
  { "MQMESSAGE",        PLAYLIST_MQ_MESSAGE,
      VALUE_DATA, NULL },
  { "PAUSEEACHSONG",    PLAYLIST_PAUSE_EACH_SONG,
      VALUE_LONG, parseConvBoolean },
  { "PLAYANNOUNCE",     PLAYLIST_ANNOUNCE,
      VALUE_LONG, parseConvBoolean },
  { "REQUIREDKEYWORDS", PLAYLIST_REQ_KEYWORDS,
      VALUE_DATA, parseConvTextList },
  { "RESUME",           PLAYLIST_RESUME,
      VALUE_DATA, plConvResume },
  { "SEQUENCE",         PLAYLIST_SEQ_NAME,
      VALUE_DATA, NULL },
  { "STOPAFTER",        PLAYLIST_STOP_AFTER,
      VALUE_DATA, plConvWait },
  { "STOPAFTERWAIT",    PLAYLIST_STOP_AFTER_WAIT,
      VALUE_DATA, plConvWait },
  { "STOPTIME",         PLAYLIST_STOP_TIME,
      VALUE_DATA, NULL },
  { "STOPTYPE",         PLAYLIST_STOP_TYPE,
      VALUE_DATA, plConvStopType },
  { "STOPWAIT",         PLAYLIST_STOP_WAIT,
      VALUE_DATA, plConvWait },
  { "TYPE",             PLAYLIST_TYPE,
      VALUE_DATA, plConvType },
  { "USESTATUS",        PLAYLIST_USE_STATUS,
      VALUE_DATA, parseConvBoolean },
  { "USEUNRATED",       PLAYLIST_USE_UNRATED,
      VALUE_DATA, parseConvBoolean },
};
#define PLAYLIST_DFKEY_COUNT (sizeof (playlistdfkeys) / sizeof (datafilekey_t))

  /* must be sorted in ascii order */
static datafilekey_t playlistdancedfkeys[] = {
  { "DANCE",          PLDANCE_DANCE,  VALUE_DATA, danceConvDance },
  { "SELECTED",       PLDANCE_DANCE,  VALUE_LONG, parseConvBoolean },
  { "COUNT",          PLDANCE_DANCE,  VALUE_LONG, NULL },
  { "MAXPLAYTIME",    PLDANCE_DANCE,  VALUE_DATA, NULL },
  { "LOWBPM",         PLDANCE_DANCE,  VALUE_LONG, NULL },
  { "HIGHBPM",        PLDANCE_DANCE,  VALUE_LONG, NULL },
};
#define PLAYLIST_DANCE_DFKEY_COUNT (sizeof (playlistdancedfkeys) / sizeof (datafilekey_t))

datafile_t *
playlistAlloc (char *fname)
{
  datafile_t    *pldf;
  datafile_t    *pldancedf;
  pathinfo_t    *pi;
  char          tfn [MAXPATHLEN];
  list_t        *pllist;

  pi = pathInfo (fname);
  datautilMakePath (tfn, sizeof (tfn), "", pi->basename, ".pl", DATAUTIL_MP_NONE);
  if (! fileopExists (tfn)) {
    return NULL;
  }

  pldf = datafileAllocParse ("playlist", DFTYPE_KEY_VAL, fname,
      playlistdfkeys, PLAYLIST_DFKEY_COUNT, DATAFILE_NO_LOOKUP);
  datautilMakePath (tfn, sizeof (tfn), "", pi->basename, ".pldance", DATAUTIL_MP_NONE);
  if (! fileopExists (tfn)) {
    datafileFree (pldf);
    return NULL;
  }

  pldancedf = datafileAllocParse ("playlist-dances", DFTYPE_KEY_LONG, fname,
      playlistdancedfkeys, PLAYLIST_DANCE_DFKEY_COUNT, DATAFILE_NO_LOOKUP);
  pathInfoFree (pi);

  pllist = datafileGetList (pldf);
  llistSetFreeHook (pllist, playlistFreeList);
  llistSetData (pllist, PLAYLIST_DANCEDF, pldancedf);
  llistSetData (pllist, PLAYLIST_DANCES, datafileGetList (pldancedf));

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

static void
plConvType (char *data, datafileret_t *ret)
{
  ret->valuetype = VALUE_LONG;
  ret->u.l = PLTYPE_MANUAL;
  if (strcmp (data, "Automatic") == 0) {
    ret->u.l = PLTYPE_AUTO;
  }
  if (strcmp (data, "Sequence") == 0) {
    ret->u.l = PLTYPE_SEQ;
  }
}

