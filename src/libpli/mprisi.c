/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
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

#include "dbusi.h"
#include "mdebug.h"
#include "mprisi.h"

typedef struct mpris {
  dbus_t      *dbus;
  char        *mpbus;
} mpris_t;

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
  MPRIS_INTFC_DBUS_INTROSPECT,
  MPRIS_INTFC_DBUS_PROP,
  MPRIS_INTFC_MP2_PLAYER,
  MPRIS_INTFC_MAX,
};

static const char *interface [MPRIS_INTFC_MAX] = {
  [MPRIS_INTFC_DBUS] = "org.freedesktop.DBus",
  [MPRIS_INTFC_DBUS_INTROSPECT] = "org.freedesktop.DBus.Introspectable",
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
  [MPRIS_PROPNM_PB_STATUS] = "PlaybackStatus",
  [MPRIS_PROPNM_METADATA] = "Metadata",
  [MPRIS_PROPNM_POS] = "Position",
  [MPRIS_PROPNM_MIN_RATE] = "MinimumRate",
  [MPRIS_PROPNM_MAX_RATE] = "MaximumRate",
  [MPRIS_PROPNM_VOLUME] = "Volume",
  [MPRIS_PROPNM_RATE] = "Rate",
};

static bool mprisGetProperty (mpris_t *mpris, const char *prop, const char *propnm);
static bool mprisGetPropBool (mpris_t *mpris, const char *prop, const char *propnm);
static const char *mprisGetPropString (mpris_t *mpris, const char *prop, const char *propnm);
static int64_t mprisGetPropInt64 (mpris_t *mpris, const char *prop, const char *propnm);
static double mprisGetPropDouble (mpris_t *mpris, const char *prop, const char *propnm);
static bool mprisSetPropDouble (mpris_t *mpris, const char *prop, const char *propnm, double val);

mpris_t *
mprisInit (const char *plinm)
{
  mpris_t   *mpris;
  int       cc;
  double    minrate, maxrate;

  mpris = mdmalloc (sizeof (mpris_t));
  mpris->dbus = dbusConnInit ();

  // temporary
  mpris->mpbus = "org.mpris.MediaPlayer2.vlc";

  if (plinm != NULL) {
    ;
  }

  cc = mprisGetPropBool (mpris, property [MPRIS_PROP_MP2_PLAYER],
      propname [MPRIS_PROPNM_CAN_CONTROL]);
  minrate = mprisGetPropDouble (mpris, property [MPRIS_PROP_MP2_PLAYER],
      propname [MPRIS_PROPNM_MIN_RATE]);
  maxrate = mprisGetPropDouble (mpris, property [MPRIS_PROP_MP2_PLAYER],
      propname [MPRIS_PROPNM_MAX_RATE]);

  return mpris;
}

void
mprisFree (mpris_t *mpris)
{
  if (mpris == NULL) {
    return;
  }

  dbusConnClose (mpris->dbus);
  mdfree (mpris);
}

void
mprisMedia (mpris_t *mpris, const char *uri)
{
  dbusMessageInit (mpris->dbus);
  dbusMessageSetData (mpris->dbus, "(s)", uri, NULL);
  dbusMessage (mpris->dbus, mpris->mpbus, objpath [MPRIS_OBJP_MP2],
      interface [MPRIS_INTFC_MP2_PLAYER], method [MPRIS_METHOD_OPENURI]);
}

const char *
mprisPlaybackStatus (mpris_t *mpris)
{
  const char  *rval;

  rval = mprisGetPropString (mpris, property [MPRIS_PROP_MP2_PLAYER],
      propname [MPRIS_PROPNM_PB_STATUS]);
  return rval;
}

int64_t
mprisGetPosition (mpris_t *mpris)
{
  int64_t   val;

  val = mprisGetPropInt64 (mpris, property [MPRIS_PROP_MP2_PLAYER],
      propname [MPRIS_PROPNM_POS]);
  return val;
}

void
mprisPause (mpris_t *mpris)
{
  dbusMessageInit (mpris->dbus);
  dbusMessage (mpris->dbus, mpris->mpbus, objpath [MPRIS_OBJP_MP2],
      interface [MPRIS_INTFC_MP2_PLAYER], method [MPRIS_METHOD_PAUSE]);
}

void
mprisPlay (mpris_t *mpris)
{
  dbusMessageInit (mpris->dbus);
  dbusMessage (mpris->dbus, mpris->mpbus, objpath [MPRIS_OBJP_MP2],
      interface [MPRIS_INTFC_MP2_PLAYER], method [MPRIS_METHOD_PLAY]);
}

void
mprisStop (mpris_t *mpris)
{
  dbusMessageInit (mpris->dbus);
  dbusMessage (mpris->dbus, mpris->mpbus, objpath [MPRIS_OBJP_MP2],
      interface [MPRIS_INTFC_MP2_PLAYER], method [MPRIS_METHOD_STOP]);
}

void
mprisNext (mpris_t *mpris)
{
  dbusMessageInit (mpris->dbus);
  dbusMessage (mpris->dbus, mpris->mpbus, objpath [MPRIS_OBJP_MP2],
      interface [MPRIS_INTFC_MP2_PLAYER], method [MPRIS_METHOD_NEXT]);
}

bool
mprisSetVolume (mpris_t *mpris, double vol)
{
  bool    rc;

  rc = mprisSetPropDouble (mpris, property [MPRIS_PROP_MP2_PLAYER],
      propname [MPRIS_PROPNM_VOLUME], vol);
  return rc;
}

bool
mprisSetPosition (mpris_t *mpris, int64_t pos)
{
  bool    rc = false;

  // type (ox)
  // mpris:trackid nanoseconds (check this)
//  rc = mprisSetPropDouble (mpris, property [MPRIS_PROP_MP2_PLAYER],
//      propname [MPRIS_PROPNM_SET_POS], vol);
  return rc;
}

bool
mprisSetRate (mpris_t *mpris, double rate)
{
  bool    rc;

  rc = mprisSetPropDouble (mpris, property [MPRIS_PROP_MP2_PLAYER],
      propname [MPRIS_PROPNM_RATE], rate);
  return rc;
}

/* internal routines */

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
