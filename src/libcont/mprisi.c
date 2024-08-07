/*
 * Copyright 2024 Brad Lanam Pleasant Hill CA
 *
 * Using the framework from:
 *    https://github.com/hoyon/mpv-mpris/blob/master/mpris.c
 *    Copyright (c) 2017 Ho-Yon Mak (MIT License)
 *
 * Note that the glib main loop must be called by something.
 * When using the GTK UI, this is done by GTK, and no special
 * handling is needed.  c.f. utility/dbustest.c
 *
 */
#include "config.h"

#if __linux__

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>
#include <string.h>

#include <glib.h>

#include "bdj4.h"
#include "callback.h"
#include "controller.h"
#include "dbusi.h"
#include "mdebug.h"
#include "player.h"
#include "nlist.h"
#include "tmutil.h"

static const char *introspection_xml =
    "<node>\n"
    "  <interface name=\"org.mpris.MediaPlayer2\">\n"
    "    <method name=\"Raise\"></method>\n"
    "    <method name=\"Quit\"></method>\n"
    "    <property name=\"CanQuit\" type=\"b\" access=\"read\"/>\n"
    "    <property name=\"Fullscreen\" type=\"b\" access=\"read\"/>\n"
    "    <property name=\"CanSetFullscreen\" type=\"b\" access=\"read\"/>\n"
    "    <property name=\"CanRaise\" type=\"b\" access=\"read\"/>\n"
    "    <property name=\"HasTrackList\" type=\"b\" access=\"read\"/>\n"
    "    <property name=\"Identity\" type=\"s\" access=\"read\"/>\n"
    "    <property name=\"DesktopEntry\" type=\"s\" access=\"read\"/>\n"
    "    <property name=\"SupportedUriSchemes\" type=\"as\" access=\"read\"/>\n"
    "    <property name=\"SupportedMimeTypes\" type=\"as\" access=\"read\"/>\n"
    "  </interface>\n"
    "  <interface name=\"org.mpris.MediaPlayer2.Player\">\n"
    "    <method name=\"Next\"></method>\n"
    "    <method name=\"Previous\"></method>\n"
    "    <method name=\"Pause\"></method>\n"
    "    <method name=\"PlayPause\"></method>\n"
    "    <method name=\"Stop\"></method>\n"
    "    <method name=\"Play\"></method>\n"
    "    <method name=\"Seek\">\n"
    "      <arg type=\"x\" name=\"Offset\" direction=\"in\"/>\n"
    "    </method>\n"
    "    <method name=\"SetPosition\">\n"
    "      <arg type=\"o\" name=\"TrackId\" direction=\"in\"/>\n"
    "      <arg type=\"x\" name=\"Offset\" direction=\"in\"/>\n"
    "    </method>\n"
    "    <method name=\"OpenUri\">\n"
    "      <arg type=\"s\" name=\"Uri\" direction=\"in\"/>\n"
    "    </method>\n"
    "    <signal name=\"Seeked\">\n"
    "      <arg type=\"x\" name=\"Position\" direction=\"out\"/>\n"
    "    </signal>\n"
    "    <property name=\"PlaybackStatus\" type=\"s\" access=\"read\"/>\n"
    "    <property name=\"LoopStatus\" type=\"s\" access=\"readwrite\"/>\n"
    "    <property name=\"Rate\" type=\"d\" access=\"readwrite\"/>\n"
    "    <property name=\"Shuffle\" type=\"b\" access=\"read\"/>\n"
    "    <property name=\"Metadata\" type=\"a{sv}\" access=\"read\"/>\n"
    "    <property name=\"Volume\" type=\"d\" access=\"readwrite\"/>\n"
    "    <property name=\"Position\" type=\"x\" access=\"read\"/>\n"
    "    <property name=\"MinimumRate\" type=\"d\" access=\"read\"/>\n"
    "    <property name=\"MaximumRate\" type=\"d\" access=\"read\"/>\n"
    "    <property name=\"CanGoNext\" type=\"b\" access=\"read\"/>\n"
    "    <property name=\"CanGoPrevious\" type=\"b\" access=\"read\"/>\n"
    "    <property name=\"CanPlay\" type=\"b\" access=\"read\"/>\n"
    "    <property name=\"CanPause\" type=\"b\" access=\"read\"/>\n"
    "    <property name=\"CanSeek\" type=\"b\" access=\"read\"/>\n"
    "    <property name=\"CanControl\" type=\"b\" access=\"read\"/>\n"
    "  </interface>\n"
    "</node>\n";

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
  MPRIS_INTFC_MP2,
  MPRIS_INTFC_MP2_PLAYER,
  MPRIS_INTFC_MAX,
};

static const char *interface [MPRIS_INTFC_MAX] = {
  [MPRIS_INTFC_DBUS] = "org.freedesktop.DBus",
  [MPRIS_INTFC_DBUS_PROP] = "org.freedesktop.DBus.Properties",
  [MPRIS_INTFC_MP2] = "org.mpris.MediaPlayer2",
  [MPRIS_INTFC_MP2_PLAYER] = "org.mpris.MediaPlayer2.Player",
};

enum {
  MPRIS_PROP_PB_STATUS,
  MPRIS_PROP_REPEAT_STATUS,
  MPRIS_PROP_RATE,
  MPRIS_PROP_METADATA,
  MPRIS_PROP_VOLUME,
  MPRIS_PROP_POSITION,
  MPRIS_PROP_MAX,
};

static const char *propstr [MPRIS_PROP_MAX] = {
  [MPRIS_PROP_PB_STATUS] = "PlaybackStatus",
  [MPRIS_PROP_REPEAT_STATUS] = "LoopStatus",
  [MPRIS_PROP_RATE] = "Rate",
  [MPRIS_PROP_METADATA] = "Metadata",
  [MPRIS_PROP_VOLUME] = "Volume",
  [MPRIS_PROP_POSITION] = "Position",
};

enum {
  MPRIS_METADATA_ALBUM,
  MPRIS_METADATA_ALBUMARTIST,
  MPRIS_METADATA_ARTIST,
  MPRIS_METADATA_TITLE,
  MPRIS_METADATA_TRACKID,
  MPRIS_METADATA_DURATION,
  MPRIS_METADATA_MAX,
};

static const char *metadatastr [MPRIS_METADATA_MAX] = {
  [MPRIS_METADATA_ALBUM] = "xesam:album",
  [MPRIS_METADATA_ALBUMARTIST] = "xesam:albumArtist",
  [MPRIS_METADATA_ARTIST] = "xesam:artist",
  [MPRIS_METADATA_TITLE] = "xesam:title",
  [MPRIS_METADATA_TRACKID] = "mpris:trackid",
  [MPRIS_METADATA_DURATION] = "mpris:length",
};

enum {
  MPRIS_STATUS_PLAY,
  MPRIS_STATUS_PAUSE,
  MPRIS_STATUS_STOP,
  MPRIS_STATUS_MAX,
};

static const char *statusstr [MPRIS_STATUS_MAX] = {
  [MPRIS_STATUS_PLAY] = "Playing",
  [MPRIS_STATUS_PAUSE] = "Paused",
  [MPRIS_STATUS_STOP] = "Stopped",
};

enum {
  MPRIS_REPEAT_NONE,
  MPRIS_REPEAT_TRACK,
  MPRIS_REPEAT_PLAYLIST,
  MPRIS_REPEAT_MAX,
};

static const char *repeatstr [MPRIS_REPEAT_MAX] = {
  [MPRIS_REPEAT_NONE] = "None",
  [MPRIS_REPEAT_TRACK] = "Track",
  [MPRIS_REPEAT_PLAYLIST] = "Playlist",
};

typedef struct contdata {
  char                *instname;
  dbus_t              *dbus;
  callback_t          *cb;
  nlist_t             *chgprop;
  nlist_t             *metadata;
  void                *metav;
  int                 root_interface_id;
  int                 player_interface_id;
  int                 playstatus;
  int                 repeatstatus;
  double              pos;
  int                 rate;
  int                 volume;
  bool                seek_expected : 1;
  bool                idle : 1;
  bool                paused : 1;
} contdata_t;

static bool mprisiMethodCallback (const char *intfc, const char *method, void *udata);
static bool mprisiPropertyGetCallback (const char *intfc, const char *method, void *udata);
static void mprisSendPropertyChange (contdata_t *contdata);

void
contiDesc (char **ret, int max)
{
  int         c = 0;

  if (max < 2) {
    return;
  }

  ret [c++] = "Linux MPRIS";
  ret [c++] = NULL;
}

contdata_t *
contiInit (const char *instname)
{
  contdata_t  *contdata;

  contdata = mdmalloc (sizeof (contdata_t));
  contdata->instname = mdstrdup (instname);
  contdata->cb = NULL;
  contdata->chgprop = nlistAlloc ("cont-mprisi-prop", LIST_ORDERED, NULL);
  contdata->metadata = NULL;
  contdata->metav = NULL;
  contdata->root_interface_id = -1;
  contdata->player_interface_id = -1;
  contdata->playstatus = MPRIS_STATUS_STOP;
  contdata->repeatstatus = MPRIS_REPEAT_NONE;
  contdata->pos = 0.0;
  contdata->rate = 100;
  contdata->volume = 0;
  contdata->seek_expected = false;
  contdata->idle = false;
  contdata->paused = false;

  contdata->dbus = dbusConnInit ();
  dbusConnectAcquireName (contdata->dbus, instname, interface [MPRIS_INTFC_MP2]);

  return contdata;
}

void
contiFree (contdata_t *contdata)
{
  if (contdata == NULL) {
    return;
  }

  dataFree (contdata->instname);
  nlistFree (contdata->chgprop);
  nlistFree (contdata->metadata);
  if (contdata->dbus != NULL && contdata->root_interface_id >= 0) {
    dbusUnregisterObject (contdata->dbus, contdata->root_interface_id);
    dbusUnregisterObject (contdata->dbus, contdata->player_interface_id);
  }
  if (contdata->dbus != NULL) {
    dbusConnClose (contdata->dbus);
  }
  mdfree (contdata);
}

void
contiSetup (contdata_t *contdata)
{
  dbusSetIntrospectionData (contdata->dbus, introspection_xml);

  contdata->root_interface_id = dbusRegisterObject (contdata->dbus,
      objpath [MPRIS_OBJP_MP2], interface [MPRIS_INTFC_MP2]);
  contdata->player_interface_id = dbusRegisterObject (contdata->dbus,
      objpath [MPRIS_OBJP_MP2], interface [MPRIS_INTFC_MP2_PLAYER]);

  dbusSetCallbacks (contdata->dbus, contdata,
      mprisiMethodCallback, mprisiPropertyGetCallback);
}

bool
contiCheckReady (contdata_t *contdata)
{
  bool    rc = false;
  int     ret;

  if (contdata == NULL || contdata->dbus == NULL) {
    return rc;
  }
  ret = dbusCheckAcquireName (contdata->dbus);
  if (ret == DBUS_NAME_OPEN) {
    rc = true;
  }

  return rc;
}

void
contiSetCallback (contdata_t *contdata, callback_t *cb)
{
  if (contdata == NULL || cb == NULL) {
    return;
  }

  contdata->cb = cb;
}

void
contiSetPlayState (contdata_t *contdata, int state)
{
  int   nstate = MPRIS_STATUS_STOP;

  if (contdata == NULL) {
    return;
  }

  switch (state) {
    case PL_STATE_LOADING:
    case PL_STATE_PLAYING:
    case PL_STATE_IN_FADEOUT:
    case PL_STATE_IN_GAP: {
      nstate = MPRIS_STATUS_PLAY;
      break;
    }
    case PL_STATE_PAUSED: {
      nstate = MPRIS_STATUS_PAUSE;
      break;
    }
    default: {
      nstate = MPRIS_STATUS_STOP;
      break;
    }
  }
  if (contdata->playstatus != nstate) {
    nlistSetNum (contdata->chgprop, MPRIS_PROP_PB_STATUS, nstate);
    mprisSendPropertyChange (contdata);
  }
  contdata->playstatus = nstate;
}

void
contiSetRepeatState (contdata_t *contdata, bool state)
{
  int   nstate;

  if (contdata == NULL) {
    return;
  }

  nstate = MPRIS_REPEAT_NONE;
  if (state) {
    nstate = MPRIS_REPEAT_TRACK;
  }
  if (contdata->repeatstatus != nstate) {
    nlistSetNum (contdata->chgprop, MPRIS_PROP_REPEAT_STATUS, nstate);
    mprisSendPropertyChange (contdata);
  }
  contdata->repeatstatus = nstate;
}

void
contiSetPosition (contdata_t *contdata, double pos)
{
  if (contdata == NULL) {
    return;
  }

  contdata->pos = pos;
}

void
contiSetRate (contdata_t *contdata, int rate)
{
  if (contdata == NULL) {
    return;
  }

  if (contdata->rate != rate) {
    double    dval;

    dval = (double) rate / 100.0;
    nlistSetDouble (contdata->chgprop, MPRIS_PROP_RATE, dval);
    mprisSendPropertyChange (contdata);
  }
  contdata->rate = rate;
}

void
contiSetVolume (contdata_t *contdata, int volume)
{
  if (contdata == NULL) {
    return;
  }

  if (contdata->volume != volume) {
    double    dval;

    dval = (double) volume / 100.0;
    nlistSetDouble (contdata->chgprop, MPRIS_PROP_VOLUME, dval);
    mprisSendPropertyChange (contdata);
  }
  contdata->volume = volume;
}

void
contiSetCurrent (contdata_t *contdata, const char *album,
    const char *albumartist, const char *artist, const char *title,
    int32_t trackid, int32_t duration)
{
  char        tbuff [200];
  nlistidx_t  miter;
  nlistidx_t  mkey;
  void        *tv;

  if (contdata == NULL) {
    return;
  }

  nlistFree (contdata->metadata);
  contdata->metadata = nlistAlloc ("cont-mprisi-meta", LIST_ORDERED, NULL);

  if (trackid >= 0) {
    snprintf (tbuff, sizeof (tbuff), "/org/bdj4/playlist/%" PRId32, trackid);
  } else {
    snprintf (tbuff, sizeof (tbuff), "/noplaylist");
  }
  nlistSetStr (contdata->metadata, MPRIS_METADATA_TRACKID, tbuff);
  nlistSetNum (contdata->metadata, MPRIS_METADATA_DURATION, duration);

  if (title != NULL && *title) {
    nlistSetStr (contdata->metadata, MPRIS_METADATA_TITLE, title);
  }
  if (artist != NULL && *artist) {
    nlistSetStr (contdata->metadata, MPRIS_METADATA_ARTIST, artist);
  }
  if (album != NULL && *album) {
    nlistSetStr (contdata->metadata, MPRIS_METADATA_ALBUM, album);
  }
  if (albumartist != NULL && *albumartist) {
    nlistSetStr (contdata->metadata, MPRIS_METADATA_ALBUMARTIST, albumartist);
  }

  nlistStartIterator (contdata->metadata, &miter);
  dbusMessageInitArray (contdata->dbus, "a{sv}");
  while ((mkey = nlistIterateKey (contdata->metadata, &miter)) >= 0) {
    if (mkey == MPRIS_METADATA_DURATION) {
      tv = dbusMessageBuild ("x", nlistGetNum (contdata->metadata, mkey));
    } else if (mkey == MPRIS_METADATA_TRACKID) {
      tv = dbusMessageBuild ("o", nlistGetStr (contdata->metadata, mkey));
    } else {
      tv = dbusMessageBuild ("s", nlistGetStr (contdata->metadata, mkey));
    }
    dbusMessageAppendArray (contdata->dbus, "a{sv}",
        metadatastr [mkey], tv, NULL);
  }
  tv = dbusMessageFinalizeArray (contdata->dbus);
  contdata->metav = tv;

  nlistSetNum (contdata->chgprop, MPRIS_PROP_METADATA, 1);
  mprisSendPropertyChange (contdata);
}

/* internal routines */

static bool
mprisiMethodCallback (const char *intfc, const char *method, void *udata)
{
  contdata_t    *contdata = udata;
  int           cmd = CONTROLLER_NONE;
  long          val = 0;

fprintf (stderr, "-- mprisi-method: %s %s\n", intfc, method);

  if (contdata == NULL || contdata->cb == NULL) {
    return true;
  }

  if (strcmp (method, "Play") == 0) {
    cmd = CONTROLLER_PLAY;
  } else if (strcmp (method, "PlayPause") == 0) {
    cmd = CONTROLLER_PLAYPAUSE;
  } else if (strcmp (method, "Pause") == 0) {
    cmd = CONTROLLER_PAUSE;
  } else if (strcmp (method, "Stop") == 0) {
    cmd = CONTROLLER_STOP;
  } else if (strcmp (method, "Next") == 0) {
    cmd = CONTROLLER_NEXT;
  } else if (strcmp (method, "Previous") == 0) {
    cmd = CONTROLLER_PREVIOUS;
  } else if (strcmp (method, "Seek") == 0) {
    cmd = CONTROLLER_SEEK;
  } else if (strcmp (method, "SetPosition") == 0) {
    cmd = CONTROLLER_SET_POS;
  } else if (strcmp (method, "OpenUri") == 0) {
    cmd = CONTROLLER_URI;
  } else if (strcmp (method, "Raise") == 0) {
    cmd = CONTROLLER_RAISE;
  } else if (strcmp (method, "Quit") == 0) {
    cmd = CONTROLLER_QUIT;
  }

  callbackHandlerII (contdata->cb, val, cmd);

  return true;
}

static bool
mprisiPropertyGetCallback (const char *intfc, const char *prop, void *udata)
{
  contdata_t    *contdata = udata;
  bool          rc = false;
fprintf (stderr, "-- mprisi-prop-get: %s %s\n", intfc, prop);

  dbusMessageInit (contdata->dbus);

  if (strcmp (intfc, interface [MPRIS_INTFC_MP2]) == 0) {
    if (strcmp (prop, "CanQuit") == 0) {
      dbusMessageSetData (contdata->dbus, "b", true, NULL);
      rc = true;
    } else if (strcmp (prop, "Fullscreen") == 0) {
      dbusMessageSetData (contdata->dbus, "b", false, NULL);
      rc = true;
    } else if (strcmp (prop, "CanSetFullscreen") == 0) {
      dbusMessageSetData (contdata->dbus, "b", false, NULL);
      rc = true;
    } else if (strcmp (prop, "CanRaise") == 0) {
      dbusMessageSetData (contdata->dbus, "b", false, NULL);
      rc = true;
    } else if (strcmp (prop, "HasTrackList") == 0) {
      dbusMessageSetData (contdata->dbus, "b", false, NULL);
      rc = true;
    } else if (strcmp (prop, "Identity") == 0) {
      dbusMessageSetData (contdata->dbus, "s", contdata->instname, NULL);
      rc = true;
    } else if (strcmp (prop, "DesktopEntry") == 0) {
      dbusMessageSetData (contdata->dbus, "s", BDJ4_NAME, NULL);
      rc = true;
    } else if (strcmp (prop, "SupportedUriSchemes") == 0) {
      rc = true;
      dbusMessageSetDataArray (contdata->dbus, "as",
          "file",
          NULL);
    } else if (strcmp (prop, "SupportedMimeTypes") == 0) {
      rc = true;
      /* this is the list from vlc w/o video */
      dbusMessageSetDataArray (contdata->dbus, "as",
          "audio/mpeg",
          "audio/x-mpeg",
          "audio/mp4",
          "application/ogg",
          "application/x-ogg",
          "audio/wav",
          "audio/x-wav",
          "audio/3gpp",
          "audio/3gpp2",
          "audio/x-matroska",
          "application/xspf+xml",
          NULL);
    }
  }

  if (strcmp (intfc, interface [MPRIS_INTFC_MP2_PLAYER]) == 0) {
    if (strcmp (prop, propstr [MPRIS_PROP_PB_STATUS]) == 0) {
      dbusMessageSetData (contdata->dbus, "s", statusstr [contdata->playstatus], NULL);
      rc = true;
    } else if (strcmp (prop, propstr [MPRIS_PROP_REPEAT_STATUS]) == 0) {
      dbusMessageSetData (contdata->dbus, "s", repeatstr [contdata->repeatstatus], NULL);
      rc = true;
    } else if (strcmp (prop, propstr [MPRIS_PROP_RATE]) == 0) {
      dbusMessageSetData (contdata->dbus, "d", (double) contdata->rate, NULL);
      rc = true;
    } else if (strcmp (prop, "Shuffle") == 0) {
      dbusMessageSetData (contdata->dbus, "b", false, NULL);
      rc = true;
    } else if (strcmp (prop, propstr [MPRIS_PROP_METADATA]) == 0) {
      if (contdata->metav != NULL) {
        dbusMessageSetData (contdata->dbus, "a{sv}", contdata->metav, NULL);
        rc = true;
      }
    } else if (strcmp (prop, propstr [MPRIS_PROP_VOLUME]) == 0) {
      dbusMessageSetData (contdata->dbus, "d", (double) contdata->volume, NULL);
      rc = true;
    } else if (strcmp (prop, propstr [MPRIS_PROP_POSITION]) == 0) {
      dbusMessageSetData (contdata->dbus, "x", (double) contdata->pos, NULL);
      rc = true;
    } else if (strcmp (prop, "MinimumRate") == 0) {
      dbusMessageSetData (contdata->dbus, "d", 0.7, NULL);
      rc = true;
    } else if (strcmp (prop, "MaximumRate") == 0) {
      dbusMessageSetData (contdata->dbus, "d", 1.3, NULL);
      rc = true;
    } else if (strcmp (prop, "CanGoNext") == 0) {
      bool    tstate = false;

      if (contdata->playstatus != MPRIS_STATUS_STOP) {
        tstate = true;
      }
      dbusMessageSetData (contdata->dbus, "b", tstate, NULL);
      rc = true;
    } else if (strcmp (prop, "CanGoPrevious") == 0) {
      dbusMessageSetData (contdata->dbus, "b", false, NULL);
      rc = true;
    } else if (strcmp (prop, "CanPlay") == 0) {
      bool  pstate = false;

      if (contdata->playstatus != MPRIS_STATUS_PLAY) {
        pstate = true;
      }
      dbusMessageSetData (contdata->dbus, "b", pstate, NULL);
      rc = true;
    } else if (strcmp (prop, "CanPause") == 0) {
      bool  pstate = false;

      if (contdata->playstatus == MPRIS_STATUS_PLAY) {
        pstate = true;
      }
      dbusMessageSetData (contdata->dbus, "b", pstate, NULL);
      rc = true;
    } else if (strcmp (prop, "CanSeek") == 0) {
      bool  pstate = false;

      if (contdata->playstatus != MPRIS_STATUS_STOP) {
        pstate = true;
      }
      dbusMessageSetData (contdata->dbus, "b", pstate, NULL);
      rc = true;
    } else if (strcmp (prop, "CanControl") == 0) {
      dbusMessageSetData (contdata->dbus, "b", true, NULL);
      rc = true;
    }
  }

  return rc;
}

static void
mprisSendPropertyChange (contdata_t *contdata)
{
  nlistidx_t  iter;
  nlistidx_t  key;

  if (nlistGetCount (contdata->chgprop) == 0) {
    return;
  }

fprintf (stderr, "mprisi: send-prop-chg\n");
  dbusMessageInit (contdata->dbus);
  nlistStartIterator (contdata->chgprop, &iter);
  while ((key = nlistIterateKey (contdata->chgprop, &iter)) >= 0) {
    int         val = 0;
    double      dval = 0.0;
    const char  *str = NULL;
    void        *tvv = NULL;
    void        *tv = NULL;
    void        *sv = NULL;
    void        *emptyv = NULL;

    switch (key) {
      case MPRIS_PROP_PB_STATUS: {
        val = nlistGetNum (contdata->chgprop, key);
        str = statusstr [val];
        break;
      }
      case MPRIS_PROP_REPEAT_STATUS: {
        val = nlistGetNum (contdata->chgprop, key);
        str = repeatstr [val];
        break;
      }
      case MPRIS_PROP_METADATA: {
        tvv = contdata->metav;
        break;
      }
      default: {
        dval = nlistGetDouble (contdata->chgprop, key);
        break;
      }
    }

    if (str != NULL) {
      tvv = dbusMessageBuild ("s", str, NULL);
    } else if (tvv == NULL) {
      tvv = dbusMessageBuild ("d", dval, NULL);
    }
    dbusMessageInitArray (contdata->dbus, "a{sv}");
    dbusMessageAppendArray (contdata->dbus, "a{sv}", propstr [key], tvv, NULL);
    tv = dbusMessageFinalizeArray (contdata->dbus);
    emptyv = dbusMessageEmptyArray ("as");
    sv = dbusMessageBuild ("s", interface [MPRIS_INTFC_MP2_PLAYER], NULL);
    /* need to set the type from the children's types */
    dbusMessageSetDataTuple (contdata->dbus, "(sa{sv}as)",
        sv, tv, emptyv, NULL);
  }

  dbusEmitSignal (contdata->dbus,
      objpath [MPRIS_OBJP_MP2], interface [MPRIS_INTFC_DBUS_PROP],
      "PropertiesChanged");

  nlistFree (contdata->chgprop);
  contdata->chgprop = nlistAlloc ("mprisi-chgprop", LIST_ORDERED, NULL);
}

#endif /* __linux__ */
