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

#if 0

static void mprisiMethodRoot (GDBusConnection *connection, const char *sender, const char *object_path, const char *interface_name, const char *method_name, GVariant *parameters, GDBusMethodInvocation *invocation, gpointer udata);
static void mprisiMethodPlayer (GDBusConnection *connection, const char *sender, const char *_object_path, const char *interface_name, const char *method_name, GVariant *parameters, GDBusMethodInvocation *invocation, gpointer udata);
static GVariant * mprisiPropertyGetRoot (GDBusConnection *connection, const char *sender, const char *object_path, const char *interface_name, const char *property_name, GError **error, gpointer udata);
static gboolean mprisiPropertySetRoot (GDBusConnection *connection, const char *sender, const char *object_path, const char *interface_name, const char *property_name, GVariant *value, GError **error, gpointer udata);
static gboolean mprisiPropertySetPlayer (GDBusConnection *connection, const char *sender, const char *object_path, const char *interface_name, const char *property_name, GVariant *value, GError **error, gpointer udata);

/* not yet converted */
static gboolean emit_property_changes (gpointer data);
static void emit_seeked_signal (contdata_t *contdata);
static GVariant * set_playback_status (contdata_t *contdata);
static void set_stopped_status (contdata_t *contdata);
static void handle_property_change (const char *name, void *data, contdata_t *contdata);
static gboolean event_handler (int fd, GIOCondition condition, gpointer data);
static void wakeup_handler (void *fd);

#endif

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
fprintf (stderr, "cont-mpris chk-ready\n");
  ret = dbusCheckAcquireName (contdata->dbus);
fprintf (stderr, "cont-mpris chk: ret: %d\n", ret);
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

fprintf (stderr, "mprisi: set-play-state\n");
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

fprintf (stderr, "mprisi: set-repeat-state\n");
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

fprintf (stderr, "mprisi: set-position\n");
  contdata->pos = pos;
}

void
contiSetRate (contdata_t *contdata, int rate)
{
  if (contdata == NULL) {
    return;
  }

fprintf (stderr, "mprisi: set-rate\n");
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

fprintf (stderr, "mprisi: set-volume\n");
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

fprintf (stderr, "mprisi: set-current\n");

  nlistFree (contdata->metadata);
  contdata->metadata = nlistAlloc ("cont-mprisi-meta", LIST_ORDERED, NULL);

  if (trackid >= 0) {
    snprintf (tbuff, sizeof (tbuff), "/%" PRId32, trackid);
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
fprintf (stderr, "    key %d %ld\n", mkey, (long) nlistGetNum (contdata->metadata, mkey));
      tv = dbusMessageBuild ("x", nlistGetNum (contdata->metadata, mkey));
    } else if (mkey == MPRIS_METADATA_TRACKID) {
fprintf (stderr, "    key %d %s\n", mkey, nlistGetStr (contdata->metadata, mkey));
      tv = dbusMessageBuild ("o", nlistGetStr (contdata->metadata, mkey));
    } else {
fprintf (stderr, "    key %d %s\n", mkey, nlistGetStr (contdata->metadata, mkey));
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
  void          *tv;
fprintf (stderr, "-- mprisi-prop-get: %s %s\n", intfc, prop);

  dbusMessageInit (contdata->dbus);

  if (strcmp (intfc, interface [MPRIS_INTFC_MP2]) == 0) {
    if (strcmp (prop, "CanQuit") == 0) {
      tv = dbusMessageBuild ("b", true, NULL);
      dbusMessageSetData (contdata->dbus, "(v)", tv, NULL);
      rc = true;
    } else if (strcmp (prop, "Fullscreen") == 0) {
      tv = dbusMessageBuild ("b", false, NULL);
      dbusMessageSetData (contdata->dbus, "(v)", tv, NULL);
      rc = true;
    } else if (strcmp (prop, "CanSetFullscreen") == 0) {
      tv = dbusMessageBuild ("b", false, NULL);
      dbusMessageSetData (contdata->dbus, "(v)", tv, NULL);
      rc = true;
    } else if (strcmp (prop, "CanRaise") == 0) {
      tv = dbusMessageBuild ("b", false, NULL);
      dbusMessageSetData (contdata->dbus, "(v)", tv, NULL);
      rc = true;
    } else if (strcmp (prop, "HasTrackList") == 0) {
      tv = dbusMessageBuild ("b", false, NULL);
      dbusMessageSetData (contdata->dbus, "(v)", tv, NULL);
      rc = true;
    } else if (strcmp (prop, "Identity") == 0) {
      tv = dbusMessageBuild ("s", BDJ4_NAME, NULL);
      dbusMessageSetData (contdata->dbus, "(v)", tv, NULL);
      rc = true;
    } else if (strcmp (prop, "DesktopEntry") == 0) {
      tv = dbusMessageBuild ("s", BDJ4_NAME, NULL);
      dbusMessageSetData (contdata->dbus, "(v)", tv, NULL);
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
          /* do not know what this is */
//          "application/x-mplayer2",
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
      tv = dbusMessageBuild ("s", statusstr [contdata->playstatus], NULL);
      dbusMessageSetData (contdata->dbus, "(v)", tv, NULL);
      rc = true;
    } else if (strcmp (prop, propstr [MPRIS_PROP_REPEAT_STATUS]) == 0) {
      tv = dbusMessageBuild ("s", repeatstr [contdata->repeatstatus], NULL);
      dbusMessageSetData (contdata->dbus, "(v)", tv, NULL);
      rc = true;
    } else if (strcmp (prop, propstr [MPRIS_PROP_RATE]) == 0) {
      tv = dbusMessageBuild ("d", (double) contdata->rate, NULL);
      dbusMessageSetData (contdata->dbus, "(v)", tv, NULL);
      rc = true;
    } else if (strcmp (prop, "Shuffle") == 0) {
      tv = dbusMessageBuild ("b", false, NULL);
      dbusMessageSetData (contdata->dbus, "(v)", tv, NULL);
      rc = true;
    } else if (strcmp (prop, propstr [MPRIS_PROP_METADATA]) == 0) {
      if (contdata->metav != NULL) {
        dbusMessageSetData (contdata->dbus, "(v)", contdata->metav, NULL);
        rc = true;
      }
    } else if (strcmp (prop, propstr [MPRIS_PROP_VOLUME]) == 0) {
      tv = dbusMessageBuild ("d", (double) contdata->volume, NULL);
      dbusMessageSetData (contdata->dbus, "(v)", tv, NULL);
      rc = true;
    } else if (strcmp (prop, propstr [MPRIS_PROP_POSITION]) == 0) {
      tv = dbusMessageBuild ("x", (double) contdata->pos, NULL);
      dbusMessageSetData (contdata->dbus, "(v)", tv, NULL);
      rc = true;
    } else if (strcmp (prop, "MinimumRate") == 0) {
      tv = dbusMessageBuild ("d", 0.7, NULL);
      dbusMessageSetData (contdata->dbus, "(v)", tv, NULL);
      rc = true;
    } else if (strcmp (prop, "MaximumRate") == 0) {
      tv = dbusMessageBuild ("d", 1.3, NULL);
      dbusMessageSetData (contdata->dbus, "(v)", tv, NULL);
      rc = true;
    } else if (strcmp (prop, "CanGoNext") == 0) {
      tv = dbusMessageBuild ("b", true, NULL);
      dbusMessageSetData (contdata->dbus, "(v)", tv, NULL);
      rc = true;
    } else if (strcmp (prop, "CanGoPrevious") == 0) {
      tv = dbusMessageBuild ("b", false, NULL);
      dbusMessageSetData (contdata->dbus, "(v)", tv, NULL);
      rc = true;
    } else if (strcmp (prop, "CanPlay") == 0) {
      tv = dbusMessageBuild ("b", true, NULL);
      dbusMessageSetData (contdata->dbus, "(v)", tv, NULL);
      rc = true;
    } else if (strcmp (prop, "CanPause") == 0) {
      tv = dbusMessageBuild ("b", true, NULL);
      dbusMessageSetData (contdata->dbus, "(v)", tv, NULL);
      rc = true;
    } else if (strcmp (prop, "CanSeek") == 0) {
      tv = dbusMessageBuild ("b", true, NULL);
      dbusMessageSetData (contdata->dbus, "(v)", tv, NULL);
      rc = true;
    } else if (strcmp (prop, "CanControl") == 0) {
      tv = dbusMessageBuild ("b", true, NULL);
      dbusMessageSetData (contdata->dbus, "(v)", tv, NULL);
      rc = true;
    }
  }

  return rc;
}

static void
mprisSendPropertyChange (contdata_t *contdata)
{
  nlistidx_t    iter;
  nlistidx_t    key;

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
//    void        *tv = NULL;
    void        *tvv = NULL;

    switch (key) {
      case MPRIS_PROP_PB_STATUS: {
fprintf (stderr, "  pb-status\n");
        val = nlistGetNum (contdata->chgprop, key);
        str = statusstr [val];
        break;
      }
      case MPRIS_PROP_REPEAT_STATUS: {
fprintf (stderr, "  rep-status\n");
        val = nlistGetNum (contdata->chgprop, key);
        str = repeatstr [val];
        break;
      }
      case MPRIS_PROP_METADATA: {
fprintf (stderr, "  metadata\n");
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
//    tv = dbusMessageBuild ("{sv}", propstr [key], tvv, NULL);
fprintf (stderr, "  %s\n", propstr [key]);
    dbusMessageSetDataArray (contdata->dbus, "a{sv}", propstr [key], tvv, NULL);
  }

  dbusEmitSignal (contdata->dbus, objpath [MPRIS_OBJP_MP2],
      interface [MPRIS_INTFC_DBUS_PROP], "PropertiesChanged");

  nlistFree (contdata->chgprop);
  contdata->chgprop = nlistAlloc ("mprisi-chgprop", LIST_ORDERED, NULL);
}


#if 0

static void
mprisiMethodRoot (GDBusConnection *connection,
    const char *sender,
    const char *object_path,
    const char *interface_name,
    const char *method_name,
    GVariant *parameters,
    GDBusMethodInvocation *invocation,
    gpointer udata)
{
    contdata_t *contdata = (contdata_t*) udata;

    if (strcmp (method_name, "Quit") == 0) {
//      mpv_command_async (contdata->mpv, 0, cmd);
      g_dbus_method_invocation_return_value (invocation, NULL);
    } else if (strcmp (method_name, "Raise") == 0) {
      g_dbus_method_invocation_return_value (invocation, NULL);
    } else {
      g_dbus_method_invocation_return_error (invocation, G_DBUS_ERROR,
          G_DBUS_ERROR_UNKNOWN_METHOD,
          "Unknown method");
    }
}

static GVariant *
mprisiPropertyGetRoot (GDBusConnection *connection,
    const char *sender,
    const char *object_path,
    const char *interface_name,
    const char *prop,
    GError **error,
    gpointer udata)
{
  contdata_t *contdata = (contdata_t*) udata;
  GVariant *ret;

  if (strcmp (prop, "CanQuit") == 0) {
    ret = g_variant_new_boolean (TRUE);
  } else if (strcmp (prop, "Fullscreen") == 0) {
    int fullscreen;

//      mpv_get_property (contdata->mpv, "fullscreen", MPV_FORMAT_FLAG, &fullscreen);
    ret = g_variant_new_boolean (fullscreen);
  } else if (strcmp (prop, "CanSetFullscreen") == 0) {
    int can_fullscreen;

//      mpv_get_property (contdata->mpv, "vo-configured", MPV_FORMAT_FLAG, &can_fullscreen);
    ret = g_variant_new_boolean (can_fullscreen);
  } else if (strcmp (prop, "CanRaise") == 0) {
    ret = g_variant_new_boolean (FALSE);
  } else if (strcmp (prop, "HasTrackList") == 0) {
    ret = g_variant_new_boolean (FALSE);
  } else if (strcmp (prop, "Identity") == 0) {
    ret = g_variant_new_string ("BDJ4");
  } else if (strcmp (prop, "DesktopEntry") == 0) {
    ret = g_variant_new_string ("BDJ4");
  } else if (strcmp (prop, "SupportedUriSchemes") == 0) {
    GVariantBuilder builder;

    g_variant_builder_init (&builder, G_VARIANT_TYPE ("as") );
    g_variant_builder_add (&builder, "s", "file");
    g_variant_builder_add (&builder, "s", "https");
//      g_variant_builder_add (&builder, "s", "mms");  // deprecated: rtsp
//      g_variant_builder_add (&builder, "s", "rtsp");
    ret = g_variant_builder_end (&builder);
  } else if (strcmp (prop, "SupportedMimeTypes") == 0) {
    GVariantBuilder builder;

    g_variant_builder_init (&builder, G_VARIANT_TYPE ("as") );
    /* this is what VLC outputs, without any video types */
    g_variant_builder_add (&builder, "s", "audio/mpeg");
    g_variant_builder_add (&builder, "s", "audio/x-mpeg");
    g_variant_builder_add (&builder, "s", "audio/mp4");
    g_variant_builder_add (&builder, "s", "application/ogg");
    g_variant_builder_add (&builder, "s", "application/x-ogg");
    /* do not know what this is */
    g_variant_builder_add (&builder, "s", "application/x-mplayer2");
    g_variant_builder_add (&builder, "s", "audio/wav");
    g_variant_builder_add (&builder, "s", "audio/x-wav");
    g_variant_builder_add (&builder, "s", "audio/3gpp");
    g_variant_builder_add (&builder, "s", "audio/3gpp2");
    g_variant_builder_add (&builder, "s", "audio/x-matroska");
    g_variant_builder_add (&builder, "s", "application/xspf+xml");
    ret = g_variant_builder_end (&builder);
  } else {
    ret = NULL;
    g_set_error (error, G_DBUS_ERROR,
      G_DBUS_ERROR_UNKNOWN_PROPERTY,
      "Unknown property %s", prop);
  }

  return ret;
}

static void
mprisiMethodPlayer (GDBusConnection *connection,
    const char *sender,
    const char *_object_path,
    const char *interface_name,
    const char *method_name,
    GVariant *parameters,
    GDBusMethodInvocation *invocation,
    gpointer udata)
{
  contdata_t *contdata = (contdata_t*) udata;
  if (strcmp (method_name, "Pause") == 0) {
    int paused = TRUE;

//    mpv_set_property (contdata->mpv, "pause", MPV_FORMAT_FLAG, &paused);
    g_dbus_method_invocation_return_value (invocation, NULL);
  } else if (strcmp (method_name, "PlayPause") == 0) {
    int paused;

    if (contdata->status == STATUS_PAUSED) {
      paused = FALSE;
    } else {
      paused = TRUE;
    }

//    mpv_set_property (contdata->mpv, "pause", MPV_FORMAT_FLAG, &paused);
    g_dbus_method_invocation_return_value (invocation, NULL);
  } else if (strcmp (method_name, "Play") == 0) {
    int paused = FALSE;

//      mpv_set_property (contdata->mpv, "pause", MPV_FORMAT_FLAG, &paused);
    g_dbus_method_invocation_return_value (invocation, NULL);
  } else if (strcmp (method_name, "Stop") == 0) {
//    mpv_command_async (contdata->mpv, 0, cmd);
    g_dbus_method_invocation_return_value (invocation, NULL);
  } else if (strcmp (method_name, "Next") == 0) {
//      mpv_command_async (contdata->mpv, 0, cmd);
    g_dbus_method_invocation_return_value (invocation, NULL);
  } else if (strcmp (method_name, "Previous") == 0) {
//    mpv_command_async (contdata->mpv, 0, cmd);
    g_dbus_method_invocation_return_value (invocation, NULL);
  } else if (strcmp (method_name, "Seek") == 0) {
    int64_t offset_us;        // in microseconds
    double  offset_s = offset_us / 1000000.0;

    g_variant_get (parameters, "(x)", &offset_us);
//      mpv_command_async (contdata->mpv, 0, cmd);
    g_dbus_method_invocation_return_value (invocation, NULL);
  } else if (strcmp (method_name, "SetPosition") == 0) {
    int64_t current_id;
    char *object_path;
    double new_position_s;
    int64_t new_position_us;

//      mpv_get_property (contdata->mpv, "playlist-pos", MPV_FORMAT_INT64, &current_id);
    g_variant_get (parameters, "(&ox)", &object_path, &new_position_us);
    new_position_s = ( (float) new_position_us) / 1000000.0; // us -> s

    if (current_id == g_ascii_strtoll (object_path + 1, NULL, 10) ) {
//       mpv_set_property (contdata->mpv, "time-pos", MPV_FORMAT_DOUBLE, &new_position_s);
    }

    g_dbus_method_invocation_return_value (invocation, NULL);
  } else if (strcmp (method_name, "OpenUri") == 0) {
    char *uri;

    g_variant_get (parameters, "(&s)", &uri);

//      mpv_command_async (contdata->mpv, 0, cmd);
    g_dbus_method_invocation_return_value (invocation, NULL);
  } else {
    g_dbus_method_invocation_return_error (invocation, G_DBUS_ERROR,
        G_DBUS_ERROR_UNKNOWN_METHOD,
        "Unknown method");
  }
}

static gboolean
mprisiPropertySetRoot (GDBusConnection *connection,
    const char *sender,
    const char *object_path,
    const char *interface_name,
    const char *prop,
    GVariant *value,
    GError **error,
    gpointer udata)
{
  contdata_t *contdata = (contdata_t*) udata;

  if (strcmp (prop, "Fullscreen") == 0) {
    int fullscreen;

    g_variant_get (value, "b", &fullscreen);
//    mpv_set_property (contdata->mpv, "fullscreen", MPV_FORMAT_FLAG, &fullscreen);
  } else {
    g_set_error (error, G_DBUS_ERROR,
        G_DBUS_ERROR_UNKNOWN_PROPERTY,
        "Cannot set property %s", prop);
    return FALSE;
  }
  return TRUE;
}

static gboolean
mprisiPropertySetPlayer (GDBusConnection *connection,
    const char *sender,
    const char *object_path,
    const char *interface_name,
    const char *prop,
    GVariant *value,
    GError **error,
    gpointer udata)
{
  contdata_t *contdata = (contdata_t*) udata;

  if (strcmp (prop, "LoopStatus") == 0) {
    const char *status;
    int t = TRUE;
    int f = FALSE;

    status = g_variant_get_string (value, NULL);
    if (strcmp (status, "Track") == 0) {
//      mpv_set_property (contdata->mpv, "loop-file", MPV_FORMAT_FLAG, &t);
//      mpv_set_property (contdata->mpv, "loop-playlist", MPV_FORMAT_FLAG, &f);
    } else if (strcmp (status, "Playlist") == 0) {
//          mpv_set_property (contdata->mpv, "loop-file", MPV_FORMAT_FLAG, &f);
//          mpv_set_property (contdata->mpv, "loop-playlist", MPV_FORMAT_FLAG, &t);
    } else {
//        mpv_set_property (contdata->mpv, "loop-file", MPV_FORMAT_FLAG, &f);
//          mpv_set_property (contdata->mpv, "loop-playlist", MPV_FORMAT_FLAG, &f);
    }
  } else if (strcmp (prop, "Rate") == 0) {
    double rate = g_variant_get_double (value);

//      mpv_set_property (contdata->mpv, "speed", MPV_FORMAT_DOUBLE, &rate);
  } else if (strcmp (prop, "Shuffle") == 0) {
    int shuffle = g_variant_get_boolean (value);

//    mpv_set_property (contdata->mpv, "playlist-shuffle", MPV_FORMAT_FLAG, &shuffle);
  } else if (strcmp (prop, "Volume") == 0) {
    double volume = g_variant_get_double (value);

    volume *= 100;
//    mpv_set_property (contdata->mpv, "volume", MPV_FORMAT_DOUBLE, &volume);
  } else {
    g_set_error (error, G_DBUS_ERROR,
        G_DBUS_ERROR_UNKNOWN_PROPERTY,
        "Cannot set property %s", prop);
    return FALSE;
  }

  return TRUE;
}

static gboolean
emit_property_changes (gpointer data)
{
    contdata_t *contdata = (contdata_t*) data;
    GError *error = NULL;
    gpointer prop_name, prop_value;
    GHashTableIter iter;

    if (g_hash_table_size (contdata->changed_properties) > 0) {
        GVariant *params;
        GVariantBuilder *properties = g_variant_builder_new (G_VARIANT_TYPE ("a{sv}") );
        GVariantBuilder *invalidated = g_variant_builder_new (G_VARIANT_TYPE ("as") );
        g_hash_table_iter_init (&iter, contdata->changed_properties);
        while (g_hash_table_iter_next (&iter, &prop_name, &prop_value) ) {
            if (prop_value) {
                g_variant_builder_add (properties, "{sv}", prop_name, prop_value);
            } else {
                g_variant_builder_add (invalidated, "s", prop_name);
            }
        }
        params = g_variant_new ("(sa{sv}as)",
            "org.mpris.MediaPlayer2.Player", properties, invalidated);
        g_variant_builder_unref (properties);
        g_variant_builder_unref (invalidated);

        g_dbus_connection_emit_signal (contdata->connection, NULL,
                                      "/org/mpris/MediaPlayer2",
                                      "org.freedesktop.DBus.Properties",
                                      "PropertiesChanged",
                                      params, &error);
        if (error != NULL) {
            g_printerr ("%s", error->message);
        }

        g_hash_table_remove_all (contdata->changed_properties);
    }
    return TRUE;
}

static void
emit_seeked_signal (contdata_t *contdata)
{
    GVariant *params;
    double position_s;
    int64_t position_us;
    GError *error = NULL;
//    mpv_get_property (contdata, "time-pos", MPV_FORMAT_DOUBLE, &position_s);
    position_us = position_s * 1000000.0; // s -> us
    params = g_variant_new ("(x)", position_us);

    g_dbus_connection_emit_signal (contdata->connection, NULL,
                                  "/org/mpris/MediaPlayer2",
                                  "org.mpris.MediaPlayer2.Player",
                                  "Seeked",
                                  params, &error);

    if (error != NULL) {
        g_printerr ("%s", error->message);
    }
}

static GVariant *
set_playback_status (contdata_t *contdata)
{
  if (contdata->idle) {
    contdata->status = STATUS_STOPPED;
  } else if (contdata->paused) {
    contdata->status = STATUS_PAUSED;
  } else {
    contdata->status = STATUS_PLAYING;
  }
  return g_variant_new_string (contdata->status);
}

static void
set_stopped_status (contdata_t *contdata)
{
  const char *prop_name = "PlaybackStatus";
  GVariant *prop_value = g_variant_new_string (STATUS_STOPPED);

  contdata->status = STATUS_STOPPED;

  g_hash_table_insert (contdata->changed_properties,
                      (gpointer) prop_name, prop_value);

  emit_property_changes (contdata);
}

static void
handle_property_change (const char *name, void *data, contdata_t *contdata)
{
    const char *prop_name = NULL;
    GVariant *prop_value = NULL;
    if (strcmp (name, "pause") == 0) {
        contdata->paused = * (int*) data;
        prop_name = "PlaybackStatus";
        prop_value = set_playback_status (contdata);

    } else if (strcmp (name, "idle-active") == 0) {
        contdata->idle = * (int*) data;
        prop_name = "PlaybackStatus";
        prop_value = set_playback_status (contdata);

    } else if (strcmp (name, "media-title") == 0 ||
               strcmp (name, "duration") == 0) {
        // Free existing metadata object
        if (contdata->metadata) {
            g_variant_unref (contdata->metadata);
        }
        contdata->metadata = create_metadata (contdata);
        prop_name = "Metadata";
        prop_value = contdata->metadata;

    } else if (strcmp (name, "speed") == 0) {
        double *rate = data;
        prop_name = "Rate";
        prop_value = g_variant_new_double (*rate);

    } else if (strcmp (name, "volume") == 0) {
        double *volume = data;
        *volume /= 100;
        prop_name = "Volume";
        prop_value = g_variant_new_double (*volume);

    } else if (strcmp (name, "loop-file") == 0) {
        char *status = * (char **) data;
        if (strcmp (status, "no") != 0) {
            contdata->repeat = LOOP_TRACK;
        } else {
            char *playlist_status;
//            mpv_get_property (contdata, "loop-playlist", MPV_FORMAT_STRING, &playlist_status);
            if (strcmp (playlist_status, "no") != 0) {
                contdata->repeat = LOOP_PLAYLIST;
            } else {
                contdata->repeat = LOOP_NONE;
            }
//            mpv_free (playlist_status);
        }
        prop_name = "LoopStatus";
        prop_value = g_variant_new_string (contdata->repeat);
    } else if (strcmp (name, "loop-playlist") == 0) {
        char *status = * (char **) data;
        if (strcmp (status, "no") != 0) {
            contdata->repeat = LOOP_PLAYLIST;
        } else {
            char *file_status;
//            mpv_get_property (contdata, "loop-file", MPV_FORMAT_STRING, &file_status);
            if (strcmp (file_status, "no") != 0) {
                contdata->repeat = LOOP_TRACK;
            } else {
                contdata->repeat = LOOP_NONE;
            }
//            mpv_free (file_status);
        }
        prop_name = "LoopStatus";
        prop_value = g_variant_new_string (contdata->repeat);

    } else if (strcmp (name, "fullscreen") == 0) {
        gboolean *status = data;
        prop_name = "Fullscreen";
        prop_value = g_variant_new_boolean (*status);
    }

    if (prop_name) {
        if (prop_value) {
            g_variant_ref (prop_value);
        }
        g_hash_table_insert (contdata->changed_properties,
                            (gpointer) prop_name, prop_value);
    }
}

static gboolean
event_handler (int fd, GIOCondition condition, gpointer data)
{
    contdata_t *contdata = data;
    gboolean has_event = TRUE;

    // Discard data in pipe
    char unused[16];
    while (read (fd, unused, sizeof (unused) ) > 0);

    while (has_event) {
#if 0
//        mpv_event *event = mpv_wait_event (contdata, 0);
        switch (event->event_id) {
        case MPV_EVENT_NONE:
            has_event = FALSE;
            break;
        case MPV_EVENT_SHUTDOWN:
            set_stopped_status (contdata);
            g_main_loop_quit (contdata->loop);
            break;
        case MPV_EVENT_PROPERTY_CHANGE: {
//            mpv_event_property *prop_event = (mpv_event_property*) event->data;
            handle_property_change (prop_event->name, prop_event->data, contdata);
        } break;
        case MPV_EVENT_SEEK:
            contdata->seek_expected = TRUE;
            break;
        case MPV_EVENT_PLAYBACK_RESTART: {
            if (contdata->seek_expected) {
                emit_seeked_signal (contdata);
                contdata->seek_expected = FALSE;
            }
         } break;
        default:
            break;
        }
#endif
      break;
    }

    return TRUE;
}

static void wakeup_handler (void *fd)
{
  (void) !write (* ( (int*) fd) , "0", 1);
}

#endif

#endif /* __linux__ */
