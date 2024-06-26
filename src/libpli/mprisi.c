/*
 * Copyright 2023-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>

#if __linux__

#include <glib.h>

#include "audiosrc.h"     // for audio-source type
#include "bdj4.h"
#include "bdjstring.h"
#include "dbusi.h"
#include "mdebug.h"
#include "mprisi.h"
#include "pli.h"

static const char * const MPRIS_PFX = "MPRIS ";

enum {
  MPRIS_MAX_PLAYERS = 10,
};

enum {
  MPRIS_IDENT = 0x6d7072697300aabb,
};

typedef struct {
  char      *bus;
  char      *name;
} mpris_player_t;

typedef struct mpris {
  int64_t         ident;
  dbus_t          *dbus;
  const char      *mpbus;
  char            trackid [DBUS_MAX_TRACKID];
  int64_t         dur;
  plistate_t      state;
  bool            canseek : 1;
  bool            hasspeed : 1;
} mpris_t;

typedef struct {
  mpris_player_t  playerInfo [MPRIS_MAX_PLAYERS];
  int             playerCount;
} mpris_info_t;

enum {
  MPRIS_BUS_DBUS,
  MPRIS_BUS_MAX,
};

static const char *bus [MPRIS_BUS_MAX] = {
  [MPRIS_BUS_DBUS] = "org.freedesktop.DBus",
};

enum {
  MPRIS_OBJP_DBUS,
  MPRIS_OBJP_MP2,
  MPRIS_OBJP_MAX,
};

static const char *objpath [MPRIS_OBJP_MAX] = {
  [MPRIS_OBJP_DBUS] = "/org/freedesktop/DBus",
  [MPRIS_OBJP_MP2] = "/org/mpris/MediaPlayer2",
};

enum {
  MPRIS_INTFC_DBUS,
  MPRIS_INTFC_DBUS_PROP,
  MPRIS_INTFC_MP2_PLAYER,
  MPRIS_INTFC_MAX,
};

static const char *interface [MPRIS_INTFC_MAX] = {
  [MPRIS_INTFC_DBUS] = "org.freedesktop.DBus",
  [MPRIS_INTFC_DBUS_PROP] = "org.freedesktop.DBus.Properties",
  [MPRIS_INTFC_MP2_PLAYER] = "org.mpris.MediaPlayer2.Player",
};

enum {
  MPRIS_METHOD_GET,
  MPRIS_METHOD_SET,
  MPRIS_METHOD_OPENURI,
  MPRIS_METHOD_STOP,
  MPRIS_METHOD_PLAY,
  MPRIS_METHOD_PAUSE,
  MPRIS_METHOD_NEXT,
  MPRIS_METHOD_SET_POS,
  MPRIS_METHOD_LIST_NAMES,
  MPRIS_METHOD_INTROSPECT,
  MPRIS_METHOD_MAX,
};

static const char *method [MPRIS_METHOD_MAX] = {
  [MPRIS_METHOD_GET] = "Get",
  [MPRIS_METHOD_SET] = "Set",
  [MPRIS_METHOD_OPENURI] = "OpenUri",
  [MPRIS_METHOD_STOP] = "Stop",
  [MPRIS_METHOD_PLAY] = "Play",
  [MPRIS_METHOD_PAUSE] = "Pause",
  [MPRIS_METHOD_NEXT] = "Next",
  [MPRIS_METHOD_SET_POS] = "SetPosition",
  [MPRIS_METHOD_LIST_NAMES] = "ListNames",
  [MPRIS_METHOD_INTROSPECT] = "Introspect",
};

enum {
  MPRIS_PROP_MP2,
  MPRIS_PROP_MP2_PLAYER,
  MPRIS_PROP_MAX,
};

static const char *property [MPRIS_PROP_MAX] = {
  [MPRIS_PROP_MP2] = "org.mpris.MediaPlayer2",
  [MPRIS_PROP_MP2_PLAYER] = "org.mpris.MediaPlayer2.Player",
};

enum {
  MPRIS_PROPNM_SUPP_URI,
  MPRIS_PROPNM_SUPP_MIME,
  MPRIS_PROPNM_CAN_CONTROL,
  MPRIS_PROPNM_CAN_SEEK,
  MPRIS_PROPNM_CAN_PLAY,
  MPRIS_PROPNM_CAN_PAUSE,
  MPRIS_PROPNM_IDENTITY,
  MPRIS_PROPNM_PB_STATUS,
  MPRIS_PROPNM_METADATA,
  MPRIS_PROPNM_POS,
  MPRIS_PROPNM_MIN_RATE,
  MPRIS_PROPNM_MAX_RATE,
  MPRIS_PROPNM_VOLUME,
  MPRIS_PROPNM_RATE,
  MPRIS_PROPNM_MAX,
};

static const char *propname [MPRIS_PROPNM_MAX] = {
  [MPRIS_PROPNM_SUPP_URI] = "SupportedUriSchemes",
  [MPRIS_PROPNM_SUPP_MIME] = "SupportedMimeTypes",
  [MPRIS_PROPNM_CAN_CONTROL] = "CanControl",
  [MPRIS_PROPNM_CAN_SEEK] = "CanSeek",
  [MPRIS_PROPNM_CAN_PLAY] = "CanPlay",
  [MPRIS_PROPNM_CAN_PAUSE] = "CanPause",
  [MPRIS_PROPNM_IDENTITY] = "Identity",
  [MPRIS_PROPNM_PB_STATUS] = "PlaybackStatus",
  [MPRIS_PROPNM_METADATA] = "Metadata",
  [MPRIS_PROPNM_POS] = "Position",
  [MPRIS_PROPNM_MIN_RATE] = "MinimumRate",
  [MPRIS_PROPNM_MAX_RATE] = "MaximumRate",
  [MPRIS_PROPNM_VOLUME] = "Volume",
  [MPRIS_PROPNM_RATE] = "Rate",
};

static bool mprisCheckForPlayer (mpris_t *mpris, const char *plinm);
static bool mprisGetProperty (mpris_t *mpris, const char *prop, const char *propnm);
static bool mprisGetPropBool (mpris_t *mpris, const char *prop, const char *propnm);
static char *mprisGetPropString (mpris_t *mpris, const char *prop, const char *propnm);
static int64_t mprisGetPropInt64 (mpris_t *mpris, const char *prop, const char *propnm);
static double mprisGetPropDouble (mpris_t *mpris, const char *prop, const char *propnm);
static bool mprisSetPropDouble (mpris_t *mpris, const char *prop, const char *propnm, double val);

static mpris_info_t mprisInfo;
static bool         initialized = false;

int
mprisGetPlayerList (mpris_t *origmpris, char **ret, int max)
{
  mpris_t     *mpris;
  int         c;
  const char  **out = NULL;
  const char  **tout;
  long        len;

  if (initialized) {
    mprisCleanup ();
  }

  c = 0;
  for (int i = 0; i < MPRIS_MAX_PLAYERS; ++i) {
    mprisInfo.playerInfo [i].bus = NULL;
    mprisInfo.playerInfo [i].name = NULL;
  }
  mprisInfo.playerCount = 0;
  initialized = true;

  mpris = origmpris;
  if (origmpris == NULL) {
    mpris = mprisInit (NULL);
  }

  dbusMessageInit (mpris->dbus);
  dbusMessageSetData (mpris->dbus, "()", NULL);
  dbusMessage (mpris->dbus, bus [MPRIS_BUS_DBUS], objpath [MPRIS_OBJP_DBUS],
      interface [MPRIS_INTFC_DBUS], method [MPRIS_METHOD_LIST_NAMES]);
  dbusResultGet (mpris->dbus, &out, &len, NULL);

  tout = out;
  while (*tout != NULL) {
    /* only interested in mediaplayer2 */
    if (strncmp (*tout, property [MPRIS_PROP_MP2], strlen (property [MPRIS_PROP_MP2])) == 0) {
      int         rval;
      char        *ident;
      char        tbuff [200];
      const char  **svout = NULL;
      const char  **tsvout;
      int         ok;

      mpris->mpbus = *tout;

      /* bypass players that cannot be controlled */
      rval = mprisGetProperty (mpris, property [MPRIS_PROP_MP2_PLAYER],
          propname [MPRIS_PROPNM_CAN_CONTROL]);
      if (! rval) {
        ++tout;
        continue;
      }

      /* bypass players that do not have basic play/pause */
      rval = mprisGetProperty (mpris, property [MPRIS_PROP_MP2_PLAYER],
          propname [MPRIS_PROPNM_CAN_PLAY]);
      if (! rval) {
        ++tout;
        continue;
      }

      rval = mprisGetProperty (mpris, property [MPRIS_PROP_MP2_PLAYER],
          propname [MPRIS_PROPNM_CAN_PAUSE]);
      if (! rval) {
        ++tout;
        continue;
      }

      dbusMessageInit (mpris->dbus);
      dbusMessageSetData (mpris->dbus, "(ss)",
          property [MPRIS_PROP_MP2], propname [MPRIS_PROPNM_SUPP_URI]);
      dbusMessage (mpris->dbus, mpris->mpbus, objpath [MPRIS_OBJP_MP2],
          interface [MPRIS_INTFC_DBUS_PROP], method [MPRIS_METHOD_GET]);
      dbusResultGet (mpris->dbus, &svout, &len, NULL);

      ok = 0;
      tsvout = svout;
      while (*tsvout != NULL) {
        if (strcmp (*tsvout, "file") == 0) {
          ++ok;
          break;
        }
        ++tsvout;
      }
      mdfree (svout);

      dbusMessageInit (mpris->dbus);
      dbusMessageSetData (mpris->dbus, "(ss)",
          property [MPRIS_PROP_MP2], propname [MPRIS_PROPNM_SUPP_MIME]);
      dbusMessage (mpris->dbus, mpris->mpbus, objpath [MPRIS_OBJP_MP2],
          interface [MPRIS_INTFC_DBUS_PROP], method [MPRIS_METHOD_GET]);
      dbusResultGet (mpris->dbus, &svout, &len, NULL);

      tsvout = svout;
      while (*tsvout != NULL) {
        /* an exhaustive check for all the different audio types */
        /* is not done.  */
        if (strcmp (*tsvout, "audio/mpeg") == 0) {
          ++ok;
          break;
        }
        ++tsvout;
      }
      mdfree (svout);

      /* must support at least 'file' uri and 'audio' mime types */
      if (ok < 2) {
        ++tout;
        continue;
      }

      ident = mprisGetPropString (mpris, property [MPRIS_PROP_MP2],
          propname [MPRIS_PROPNM_IDENTITY]);
      mdextalloc (ident);
      mprisInfo.playerInfo [c].bus = mdstrdup (*tout);
      snprintf (tbuff, sizeof (tbuff), "%s%s", MPRIS_PFX, ident);
      mdfree (ident);
      mprisInfo.playerInfo [c].name = mdstrdup (tbuff);
      if (ret != NULL) {
        ret [c] = mprisInfo.playerInfo [c].name;
      }
      ++c;
      if (c >= max - 1 || c >= MPRIS_MAX_PLAYERS - 1) {
        break;
      }
    }
    ++tout;
  }
  mdfree (out);

  mprisInfo.playerCount = c;
  if (ret != NULL) {
    ret [c] = NULL;
  }

  if (origmpris == NULL) {
    mprisFree (mpris);
  }

  return c;
}

void
mprisCleanup (void)
{
  if (! initialized) {
    return;
  }

  for (int i = 0; i < mprisInfo.playerCount; ++i) {
    dataFree (mprisInfo.playerInfo [i].bus);
    dataFree (mprisInfo.playerInfo [i].name);
    mprisInfo.playerInfo [i].bus = NULL;
    mprisInfo.playerInfo [i].name = NULL;
  }
  mprisInfo.playerCount = 0;
  initialized = false;
}


mpris_t *
mprisInit (const char *plinm)
{
  mpris_t   *mpris;
  double    minrate, maxrate;

  mpris = mdmalloc (sizeof (mpris_t));
  mpris->state = PLI_STATE_NONE;
  mpris->ident = MPRIS_IDENT;
  mpris->dbus = dbusConnInit ();
  mpris->canseek = false;
  mpris->hasspeed = false;

  if (! initialized) {
    mprisGetPlayerList (mpris, NULL, MPRIS_MAX_PLAYERS);
  }

  if (plinm == NULL) {
    return mpris;
  }

  if (! mprisCheckForPlayer (mpris, plinm)) {
    /* the player may have been started since the first initialization */
    /* clean up and reload */
    mprisCleanup ();
    mprisGetPlayerList (mpris, NULL, MPRIS_MAX_PLAYERS);
    mprisCheckForPlayer (mpris, plinm);
  }

  if (mpris->mpbus == NULL || ! *mpris->mpbus) {
    return mpris;
  }

  mpris->canseek = mprisGetPropBool (mpris, property [MPRIS_PROP_MP2_PLAYER],
      propname [MPRIS_PROPNM_CAN_SEEK]);
  minrate = mprisGetPropDouble (mpris, property [MPRIS_PROP_MP2_PLAYER],
      propname [MPRIS_PROPNM_MIN_RATE]);
  maxrate = mprisGetPropDouble (mpris, property [MPRIS_PROP_MP2_PLAYER],
      propname [MPRIS_PROPNM_MAX_RATE]);
  if (minrate <= 0.7 && maxrate >= 1.3) {
    mpris->hasspeed = true;
  }

  return mpris;
}

void
mprisFree (mpris_t *mpris)
{
  if (mpris == NULL || mpris->ident != MPRIS_IDENT) {
    return;
  }

  mpris->ident = BDJ4_IDENT_FREE;
  mpris->mpbus = NULL;
  dbusConnClose (mpris->dbus);
  mpris->dbus = NULL;
  mdfree (mpris);
}

bool
mprisCanSeek (mpris_t *mpris)
{
  if (mpris == NULL || mpris->ident != MPRIS_IDENT || mpris->mpbus == NULL) {
    return false;
  }

  return mpris->canseek;
}

bool
mprisHasSpeed (mpris_t *mpris)
{
  if (mpris == NULL || mpris->ident != MPRIS_IDENT || mpris->mpbus == NULL) {
    return false;
  }

  return mpris->hasspeed;
}

void
mprisMedia (mpris_t *mpris, const char *fulluri, int sourceType)
{
  char  tbuff [MAXPATHLEN];

  if (mpris == NULL || mpris->ident != MPRIS_IDENT || mpris->mpbus == NULL) {
    return;
  }

  if (sourceType == AUDIOSRC_TYPE_FILE) {
    snprintf (tbuff, sizeof (tbuff), "%s%s", AS_FILE_PFX, fulluri);
  } else {
    strlcpy (tbuff, fulluri, sizeof (tbuff));
  }

  dbusMessageInit (mpris->dbus);
  dbusMessageSetData (mpris->dbus, "(s)", tbuff, NULL);
  dbusMessage (mpris->dbus, mpris->mpbus, objpath [MPRIS_OBJP_MP2],
      interface [MPRIS_INTFC_MP2_PLAYER], method [MPRIS_METHOD_OPENURI]);

  /* want mpris:trackid */

  dbusMessageInit (mpris->dbus);
  dbusMessageSetData (mpris->dbus, "(ss)", property [MPRIS_PROP_MP2_PLAYER],
      propname [MPRIS_PROPNM_METADATA], NULL);
  dbusMessage (mpris->dbus, mpris->mpbus, objpath [MPRIS_OBJP_MP2],
      interface [MPRIS_INTFC_DBUS_PROP], method [MPRIS_METHOD_GET]);
  *mpris->trackid = '\0';
  dbusResultGet (mpris->dbus, mpris->trackid, NULL, NULL);

  mpris->state = PLI_STATE_PLAYING;
}

plistate_t
mprisState (mpris_t *mpris)
{
  char  *rval;

  if (mpris == NULL || mpris->ident != MPRIS_IDENT || mpris->mpbus == NULL) {
    return PLI_STATE_NONE;
  }

  rval = mprisGetPropString (mpris, property [MPRIS_PROP_MP2_PLAYER],
      propname [MPRIS_PROPNM_PB_STATUS]);
  if (strcmp (rval, "Stopped") == 0) {
    mpris->state = PLI_STATE_STOPPED;
  } else if (strcmp (rval, "Playing") == 0) {
    mpris->state = PLI_STATE_PLAYING;
  } else if (strcmp (rval, "Paused") == 0) {
    mpris->state = PLI_STATE_PAUSED;
  }
  mdfree (rval);

  return mpris->state;
}

int64_t
mprisGetPosition (mpris_t *mpris)
{
  int64_t   val;

  if (mpris == NULL || mpris->ident != MPRIS_IDENT || mpris->mpbus == NULL) {
    return 0;
  }

  val = mprisGetPropInt64 (mpris, property [MPRIS_PROP_MP2_PLAYER],
      propname [MPRIS_PROPNM_POS]);
  return val;
}

int64_t
mprisGetDuration (mpris_t *mpris)
{
  int64_t   dur;

  if (mpris == NULL || mpris->ident != MPRIS_IDENT || mpris->mpbus == NULL) {
    return 0;
  }

  dbusMessageInit (mpris->dbus);
  dbusMessageSetData (mpris->dbus, "(ss)", property [MPRIS_PROP_MP2_PLAYER],
      propname [MPRIS_PROPNM_METADATA], NULL);
  dbusMessage (mpris->dbus, mpris->mpbus, objpath [MPRIS_OBJP_MP2],
      interface [MPRIS_INTFC_DBUS_PROP], method [MPRIS_METHOD_GET]);
  dbusResultGet (mpris->dbus, mpris->trackid, &dur, NULL);
  /* duration is in nanoseconds */
  dur /= 1000;
  dur /= 1000;

  return dur;
}

void
mprisPause (mpris_t *mpris)
{
  if (mpris == NULL || mpris->ident != MPRIS_IDENT || mpris->mpbus == NULL) {
    return;
  }

  dbusMessageInit (mpris->dbus);
  dbusMessage (mpris->dbus, mpris->mpbus, objpath [MPRIS_OBJP_MP2],
      interface [MPRIS_INTFC_MP2_PLAYER], method [MPRIS_METHOD_PAUSE]);
  mpris->state = PLI_STATE_PAUSED;
}

void
mprisPlay (mpris_t *mpris)
{
  if (mpris == NULL || mpris->ident != MPRIS_IDENT || mpris->mpbus == NULL) {
    return;
  }

  dbusMessageInit (mpris->dbus);
  dbusMessage (mpris->dbus, mpris->mpbus, objpath [MPRIS_OBJP_MP2],
      interface [MPRIS_INTFC_MP2_PLAYER], method [MPRIS_METHOD_PLAY]);
  mpris->state = PLI_STATE_PLAYING;
}

void
mprisStop (mpris_t *mpris)
{
  if (mpris == NULL || mpris->ident != MPRIS_IDENT || mpris->mpbus == NULL) {
    return;
  }

  dbusMessageInit (mpris->dbus);
  dbusMessage (mpris->dbus, mpris->mpbus, objpath [MPRIS_OBJP_MP2],
      interface [MPRIS_INTFC_MP2_PLAYER], method [MPRIS_METHOD_STOP]);
  mpris->state = PLI_STATE_STOPPED;
}

bool
mprisSetVolume (mpris_t *mpris, double vol)
{
  bool    rc;

  if (mpris == NULL || mpris->ident != MPRIS_IDENT || mpris->mpbus == NULL) {
    return false;
  }

  rc = mprisSetPropDouble (mpris, property [MPRIS_PROP_MP2_PLAYER],
      propname [MPRIS_PROPNM_VOLUME], vol);
  return rc;
}

bool
mprisSetPosition (mpris_t *mpris, int64_t pos)
{
  bool    rc = false;
  void    *to;

  if (mpris == NULL || mpris->ident != MPRIS_IDENT || mpris->mpbus == NULL) {
    return false;
  }

  if (! mpris->canseek) {
    return rc;
  }

  // type (ox)
  // mpris:trackid nanoseconds
  dbusMessageInit (mpris->dbus);
  // ### this needs to be tested, unknown if correct.
  to = dbusMessageBuildObj (mpris->trackid);
  dbusMessageSetData (mpris->dbus, "(ox)", to, pos * 1000, NULL);
  rc = dbusMessage (mpris->dbus, mpris->mpbus, objpath [MPRIS_OBJP_MP2],
      interface [MPRIS_INTFC_DBUS_PROP], method [MPRIS_METHOD_SET_POS]);
  return rc;
}

bool
mprisSetRate (mpris_t *mpris, double rate)
{
  bool    rc;

  if (mpris == NULL || mpris->ident != MPRIS_IDENT || mpris->mpbus == NULL) {
    return false;
  }

  rc = mprisSetPropDouble (mpris, property [MPRIS_PROP_MP2_PLAYER],
      propname [MPRIS_PROPNM_RATE], rate);
  return rc;
}

/* internal routines */

bool
mprisCheckForPlayer (mpris_t *mpris, const char *plinm)
{
  bool  rc = false;

  for (int i = 0; i < mprisInfo.playerCount; ++i) {
    if (strcmp (mprisInfo.playerInfo [i].name, plinm) == 0) {
      mpris->mpbus = mprisInfo.playerInfo [i].bus;
      rc = true;
      break;
    }
  }

  return rc;
}

static bool
mprisGetProperty (mpris_t *mpris, const char *prop, const char *propnm)
{
  bool      rc;

  dbusMessageInit (mpris->dbus);
  dbusMessageSetData (mpris->dbus, "(ss)", prop, propnm, NULL);
  rc = dbusMessage (mpris->dbus, mpris->mpbus, objpath [MPRIS_OBJP_MP2],
      interface [MPRIS_INTFC_DBUS_PROP], method [MPRIS_METHOD_GET]);
  return rc;
}

static bool
mprisGetPropBool (mpris_t *mpris, const char *prop, const char *propnm)
{
  gboolean  rval = false;

  if (mprisGetProperty (mpris, prop, propnm)) {
    dbusResultGet (mpris->dbus, &rval, NULL);
  }
  return rval;
}

static char *
mprisGetPropString (mpris_t *mpris, const char *prop, const char *propnm)
{
  char  *rval = NULL;

  if (mprisGetProperty (mpris, prop, propnm)) {
    dbusResultGet (mpris->dbus, &rval, NULL);
  }
  return rval;
}

static int64_t
mprisGetPropInt64 (mpris_t *mpris, const char *prop, const char *propnm)
{
  int64_t   rval = -1;

  if (mprisGetProperty (mpris, prop, propnm)) {
    dbusResultGet (mpris->dbus, &rval, NULL);
  }
  return rval;
}

static double
mprisGetPropDouble (mpris_t *mpris, const char *prop, const char *propnm)
{
  double  rval = 0.0;

  if (mprisGetProperty (mpris, prop, propnm)) {
    dbusResultGet (mpris->dbus, &rval, NULL);
  }
  return rval;
}

static bool
mprisSetPropDouble (mpris_t *mpris,
    const char *prop, const char *propnm, double val)
{
  bool      rc;
  void      *tv;

  dbusMessageInit (mpris->dbus);
  /* for whatever reason, dbus wraps the double values as a variant */
  tv = dbusMessageBuild ("d", val, NULL);
  dbusMessageSetData (mpris->dbus, "(ssv)", prop, propnm, tv, NULL);
  rc = dbusMessage (mpris->dbus, mpris->mpbus, objpath [MPRIS_OBJP_MP2],
      interface [MPRIS_INTFC_DBUS_PROP], method [MPRIS_METHOD_SET]);
  return rc;
}

#endif /* __linux__ */
