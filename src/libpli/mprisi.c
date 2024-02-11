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

#include "bdj4.h"
#include "dbusi.h"
#include "mdebug.h"
#include "mprisi.h"
#include "osdirutil.h"

enum {
  MPRIS_MAX_PLAYERS = 10,
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
  char            cwd [MAXPATHLEN];
  char            trackid [DBUS_MAX_TRACKID];
  int64_t         dur;
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
static const char *mprisGetPropString (mpris_t *mpris, const char *prop, const char *propnm);
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

  mpris = origmpris;
  if (origmpris == NULL) {
    mpris = mprisInit (NULL);
  }

  dbusMessageInit (mpris->dbus);
  dbusMessageSetData (mpris->dbus, "()", NULL);
  dbusMessage (mpris->dbus, bus [MPRIS_BUS_DBUS], objpath [MPRIS_OBJP_DBUS],
      interface [MPRIS_INTFC_DBUS], method [MPRIS_METHOD_LIST_NAMES]);
  dbusResultGet (mpris->dbus, &out, &len, NULL);

  while (*out != NULL) {
    /* only interested in mediaplayer2 */
    if (strncmp (*out, property [MPRIS_PROP_MP2], strlen (property [MPRIS_PROP_MP2])) == 0) {
      int         rval;
      const char  *ident;
      char        tbuff [200];

      mpris->mpbus = *out;

      /* bypass players that cannot be controlled */
      rval = mprisGetProperty (mpris, property [MPRIS_PROP_MP2_PLAYER],
          propname [MPRIS_PROPNM_CAN_CONTROL]);
      if (! rval) {
        continue;
      }

      /* bypass players that do not have basic play/pause */
      rval = mprisGetProperty (mpris, property [MPRIS_PROP_MP2_PLAYER],
          propname [MPRIS_PROPNM_CAN_PLAY]);
      if (! rval) {
        continue;
      }

      rval = mprisGetProperty (mpris, property [MPRIS_PROP_MP2_PLAYER],
          propname [MPRIS_PROPNM_CAN_PAUSE]);
      if (! rval) {
        continue;
      }

      ident = mprisGetPropString (mpris, property [MPRIS_PROP_MP2],
          propname [MPRIS_PROPNM_IDENTITY]);
      mprisInfo.playerInfo [c].bus = mdstrdup (*out);
      snprintf (tbuff, sizeof (tbuff), "MPRIS %s", ident);
      mprisInfo.playerInfo [c].name = mdstrdup (tbuff);
      if (ret != NULL) {
        ret [c] = mprisInfo.playerInfo [c].name;
      }
      ++c;
      if (c >= max - 1 || c >= MPRIS_MAX_PLAYERS - 1) {
        break;
      }
    }
    ++out;
  }

  mprisInfo.playerCount = c;
  if (ret != NULL) {
    ret [c] = NULL;
  }

  if (origmpris == NULL) {
    mprisFree (mpris);
  }
  initialized = true;

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
  }
  initialized = true;
}


mpris_t *
mprisInit (const char *plinm)
{
  mpris_t   *mpris;
  double    minrate, maxrate;

  mpris = mdmalloc (sizeof (mpris_t));
  mpris->ident = MPRIS_IDENT;
  mpris->dbus = dbusConnInit ();
  mpris->canseek = false;
  mpris->hasspeed = false;
  osGetCurrentDir (mpris->cwd, sizeof (mpris->cwd));

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

  // ### need to check and see if mpbus is set */

  mpris->canseek = mprisGetPropBool (mpris, property [MPRIS_PROP_MP2_PLAYER],
      propname [MPRIS_PROPNM_CAN_SEEK]);
  minrate = mprisGetPropDouble (mpris, property [MPRIS_PROP_MP2_PLAYER],
      propname [MPRIS_PROPNM_MIN_RATE]);
  maxrate = mprisGetPropDouble (mpris, property [MPRIS_PROP_MP2_PLAYER],
      propname [MPRIS_PROPNM_MAX_RATE]);
  if (minrate < 1.0 && maxrate > 1.0) {
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

  mpris->ident = 0;
  mpris->mpbus = NULL;
  dbusConnClose (mpris->dbus);
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
mprisMedia (mpris_t *mpris, const char *uri)
{
  char  tbuff [MAXPATHLEN];

  if (mpris == NULL || mpris->ident != MPRIS_IDENT || mpris->mpbus == NULL) {
    return;
  }

// ### need to check if this is an absolute path
// ### should this processing be moved up to a higher level?
// ### there may be other players that need a full path
  snprintf (tbuff, sizeof (tbuff), "file://%s/%s", mpris->cwd, uri);

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
}

const char *
mprisPlaybackStatus (mpris_t *mpris)
{
  const char  *rval;

  if (mpris == NULL || mpris->ident != MPRIS_IDENT || mpris->mpbus == NULL) {
    return NULL;
  }

  rval = mprisGetPropString (mpris, property [MPRIS_PROP_MP2_PLAYER],
      propname [MPRIS_PROPNM_PB_STATUS]);
  return rval;
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
}

void
mprisNext (mpris_t *mpris)
{
  if (mpris == NULL || mpris->ident != MPRIS_IDENT || mpris->mpbus == NULL) {
    return;
  }

  dbusMessageInit (mpris->dbus);
  dbusMessage (mpris->dbus, mpris->mpbus, objpath [MPRIS_OBJP_MP2],
      interface [MPRIS_INTFC_MP2_PLAYER], method [MPRIS_METHOD_NEXT]);
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

  if (mpris == NULL || mpris->ident != MPRIS_IDENT || mpris->mpbus == NULL) {
    return false;
  }

  // type (ox)
  // mpris:trackid nanoseconds
  dbusMessageInit (mpris->dbus);
  dbusMessageSetData (mpris->dbus, "(ox)", mpris->trackid, pos * 1000, NULL);
  rc = dbusMessage (mpris->dbus, mpris->mpbus, objpath [MPRIS_OBJP_MP2],
      interface [MPRIS_INTFC_DBUS_PROP], method [MPRIS_METHOD_SET]);
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

static const char *
mprisGetPropString (mpris_t *mpris, const char *prop, const char *propnm)
{
  const char  *rval = NULL;

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

  dbusMessageInit (mpris->dbus);
  dbusMessageSetData (mpris->dbus, "(ssd)", prop, propnm, &val, NULL);
  rc = dbusMessage (mpris->dbus, mpris->mpbus, objpath [MPRIS_OBJP_MP2],
      interface [MPRIS_INTFC_DBUS_PROP], method [MPRIS_METHOD_GET]);
  return rc;
}

#endif /* __linux__ */
