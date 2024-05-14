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

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>

#if __linux__

#include "bdj4.h"
#include "controller.h"
#include "dbusi.h"
#include "mdebug.h"
#include "tmutil.h"

// #include <libavformat/avformat.h>

static const char *introspection_xml =
    "<node>\n"
    "  <interface name=\"org.mpris.MediaPlayer2\">\n"
    "    <method name=\"Raise\">\n"
    "    </method>\n"
    "    <method name=\"Quit\">\n"
    "    </method>\n"
    "    <property name=\"CanQuit\" type=\"b\" access=\"read\"/>\n"
    "    <property name=\"Fullscreen\" type=\"b\" access=\"readwrite\"/>\n"
    "    <property name=\"CanSetFullscreen\" type=\"b\" access=\"read\"/>\n"
    "    <property name=\"CanRaise\" type=\"b\" access=\"read\"/>\n"
    "    <property name=\"HasTrackList\" type=\"b\" access=\"read\"/>\n"
    "    <property name=\"Identity\" type=\"s\" access=\"read\"/>\n"
    "    <property name=\"DesktopEntry\" type=\"s\" access=\"read\"/>\n"
    "    <property name=\"SupportedUriSchemes\" type=\"as\" access=\"read\"/>\n"
    "    <property name=\"SupportedMimeTypes\" type=\"as\" access=\"read\"/>\n"
    "  </interface>\n"
    "  <interface name=\"org.mpris.MediaPlayer2.Player\">\n"
    "    <method name=\"Next\">\n"
    "    </method>\n"
    "    <method name=\"Previous\">\n"
    "    </method>\n"
    "    <method name=\"Pause\">\n"
    "    </method>\n"
    "    <method name=\"PlayPause\">\n"
    "    </method>\n"
    "    <method name=\"Stop\">\n"
    "    </method>\n"
    "    <method name=\"Play\">\n"
    "    </method>\n"
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
    "    <property name=\"Shuffle\" type=\"b\" access=\"readwrite\"/>\n"
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
  int                 root_interface_id;
  int                 player_interface_id;
  int                 playstatus;
  int                 repeatstatus;
//  GHashTable          *changed_properties;
//  GVariant            *metadata;
  bool                seek_expected;
  bool                idle;
  bool                paused;
} contdata_t;

static bool mprisiMethodCallback (const char *intfc, const char *method, void *udata);
static bool mprisiPropertyGetCallback (const char *intfc, const char *method, void *udata);




#if 0
static void mprisiBusAcquired (GDBusConnection *connection, const char *name, gpointer udata);
static void mprisiBusNameLost (GDBusConnection *connection, const char *_name, gpointer udata);
static void mprisiMethodRoot (GDBusConnection *connection, const char *sender, const char *object_path, const char *interface_name, const char *method_name, GVariant *parameters, GDBusMethodInvocation *invocation, gpointer udata);
static void mprisiMethodPlayer (GDBusConnection *connection, const char *sender, const char *_object_path, const char *interface_name, const char *method_name, GVariant *parameters, GDBusMethodInvocation *invocation, gpointer udata);
static GVariant * mprisiPropertyGetRoot (GDBusConnection *connection, const char *sender, const char *object_path, const char *interface_name, const char *property_name, GError **error, gpointer udata);
static GVariant * mprisiPropertyGetPlayer (GDBusConnection *connection, const char *sender, const char *object_path, const char *interface_name, const char *property_name, GError **error, gpointer udata);
static gboolean mprisiPropertySetRoot (GDBusConnection *connection, const char *sender, const char *object_path, const char *interface_name, const char *property_name, GVariant *value, GError **error, gpointer udata);
static gboolean mprisiPropertySetPlayer (GDBusConnection *connection, const char *sender, const char *object_path, const char *interface_name, const char *property_name, GVariant *value, GError **error, gpointer udata);

/* not yet converted */
static GVariant *create_metadata (contdata_t *contdata);
static gboolean emit_property_changes (gpointer data);
static void emit_seeked_signal (contdata_t *contdata);
static GVariant * set_playback_status (contdata_t *contdata);
static void set_stopped_status (contdata_t *contdata);
static void handle_property_change (const char *name, void *data, contdata_t *contdata);
static gboolean event_handler (int fd, GIOCondition condition, gpointer data);
static void wakeup_handler (void *fd);

static GDBusInterfaceVTable vtable_root = {
    mprisiMethodRoot, mprisiPropertyGetRoot, mprisiPropertySetRoot,
};

static GDBusInterfaceVTable vtable_player = {
    mprisiMethodPlayer, mprisiPropertyGetPlayer, mprisiPropertySetPlayer,
};
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

fprintf (stderr, "cont-mpris init\n");
  contdata = mdmalloc (sizeof (contdata_t));
  contdata->playstatus = MPRIS_STATUS_STOP;
  contdata->repeatstatus = MPRIS_REPEAT_NONE;
//  contdata->changed_properties = g_hash_table_new (g_str_hash, g_str_equal);
  contdata->seek_expected = false;
  contdata->idle = false;
  contdata->paused = false;
  contdata->root_interface_id = -1;
  contdata->player_interface_id = -1;

  contdata->dbus = dbusConnInit ();
  dbusConnectAcquireName (contdata->dbus, instname, interface [MPRIS_INTFC_MP2]);

  return contdata;
}

void
contiFree (contdata_t *contdata)
{
fprintf (stderr, "cont-mpris free\n");
  if (contdata->dbus != NULL && contdata->root_interface_id >= 0) {
    dbusUnregisterObject (contdata->dbus, contdata->root_interface_id);
    dbusUnregisterObject (contdata->dbus, contdata->player_interface_id);
  }
  if (contdata->dbus != NULL) {
    dbusConnClose (contdata->dbus);
  }
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
fprintf (stderr, "cont-mpris chk: ret: %d\n", ret);
  if (ret == DBUS_NAME_OPEN) {
    rc = true;
  }

  return rc;
}

/* internal routines */

static bool
mprisiMethodCallback (const char *intfc, const char *method, void *udata)
{
  contdata_t    *contdata = udata;
fprintf (stderr, "mprisi-method: %s %s\n", intfc, method);

  return false;
}

static bool
mprisiPropertyGetCallback (const char *intfc, const char *prop, void *udata)
{
  contdata_t    *contdata = udata;
  bool          rc = false;
  void          *tv;
fprintf (stderr, "mprisi-prop-get: %s %s\n", intfc, prop);

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
#if 0
      GVariantBuilder builder;

      g_variant_builder_init (&builder, G_VARIANT_TYPE ("as") );
      g_variant_builder_add (&builder, "s", "file");
      g_variant_builder_add (&builder, "s", "https");
  //      g_variant_builder_add (&builder, "s", "mms");  // deprecated: rtsp
  //      g_variant_builder_add (&builder, "s", "rtsp");
      ret = g_variant_builder_end (&builder);
#endif
    } else if (strcmp (prop, "SupportedMimeTypes") == 0) {
      rc = true;
#if 0
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
#endif
    }
  }

  if (strcmp (intfc, interface [MPRIS_INTFC_MP2_PLAYER]) == 0) {
    if (strcmp (prop, "PlaybackStatus") == 0) {
      tv = dbusMessageBuild ("s", statusstr [contdata->playstatus], NULL);
      dbusMessageSetData (contdata->dbus, "(v)", tv, NULL);
      rc = true;
    } else if (strcmp (prop, "LoopStatus") == 0) {
      tv = dbusMessageBuild ("s", repeatstr [contdata->repeatstatus], NULL);
      dbusMessageSetData (contdata->dbus, "(v)", tv, NULL);
      rc = true;
    } else if (strcmp (prop, "Rate") == 0) {
      rc = true;
//      double rate;
  //      mpv_get_property (contdata->mpv, "speed", MPV_FORMAT_DOUBLE, &rate);
//      ret = g_variant_new_double (rate);
    } else if (strcmp (prop, "Shuffle") == 0) {
      tv = dbusMessageBuild ("b", false, NULL);
      dbusMessageSetData (contdata->dbus, "(v)", tv, NULL);
      rc = true;
    } else if (strcmp (prop, "Metadata") == 0) {
      rc = true;
#if 0
      if (!contdata->metadata) {
        contdata->metadata = create_metadata (contdata);
      }
      // Increase reference count to prevent it from being freed after returning
      g_variant_ref (contdata->metadata);
      ret = contdata->metadata;
#endif
    } else if (strcmp (prop, "Volume") == 0) {
      rc = true;
//      double volume;

  //      mpv_get_property (contdata->mpv, "volume", MPV_FORMAT_DOUBLE, &volume);
//      volume /= 100;
//      ret = g_variant_new_double (volume);
    } else if (strcmp (prop, "Position") == 0) {
      rc = true;
//      double  position_s;
//      int64_t position_us;
  //    mpv_get_property (contdata->mpv, "time-pos", MPV_FORMAT_DOUBLE, &position_s);
//      position_us = position_s * 1000000.0; // s -> us
//      ret = g_variant_new_int64 (position_us);
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

static GVariant *
mprisiPropertyGetPlayer (GDBusConnection *connection,
    const char *sender,
    const char *object_path,
    const char *interface_name,
    const char *prop,
    GError **error,
    gpointer udata)
{
  contdata_t  *contdata = (contdata_t*) udata;
  GVariant    *ret;

  if (strcmp (prop, "PlaybackStatus") == 0) {
    ret = g_variant_new_string (contdata->playstatus);
  } else if (strcmp (prop, "LoopStatus") == 0) {
    ret = g_variant_new_string (contdata->repeatstatus);
  } else if (strcmp (prop, "Rate") == 0) {
    double rate;

//      mpv_get_property (contdata->mpv, "speed", MPV_FORMAT_DOUBLE, &rate);
    ret = g_variant_new_double (rate);
  } else if (strcmp (prop, "Shuffle") == 0) {
    int shuffle;

//      mpv_get_property (contdata->mpv, "playlist-shuffle", MPV_FORMAT_FLAG, &shuffle);
    ret = g_variant_new_boolean (shuffle);
  } else if (strcmp (prop, "Metadata") == 0) {
    if (!contdata->metadata) {
      contdata->metadata = create_metadata (contdata);
    }
    // Increase reference count to prevent it from being freed after returning
    g_variant_ref (contdata->metadata);
    ret = contdata->metadata;
  } else if (strcmp (prop, "Volume") == 0) {
    double volume;

//      mpv_get_property (contdata->mpv, "volume", MPV_FORMAT_DOUBLE, &volume);
    volume /= 100;
    ret = g_variant_new_double (volume);
  } else if (strcmp (prop, "Position") == 0) {
    double  position_s;
    int64_t position_us;
//    mpv_get_property (contdata->mpv, "time-pos", MPV_FORMAT_DOUBLE, &position_s);
    position_us = position_s * 1000000.0; // s -> us
    ret = g_variant_new_int64 (position_us);
  } else if (strcmp (prop, "MinimumRate") == 0) {
    ret = g_variant_new_double (0.01);
  } else if (strcmp (prop, "MaximumRate") == 0) {
    ret = g_variant_new_double (100);
  } else if (strcmp (prop, "CanGoNext") == 0) {
    ret = g_variant_new_boolean (TRUE);
  } else if (strcmp (prop, "CanGoPrevious") == 0) {
    ret = g_variant_new_boolean (TRUE);
  } else if (strcmp (prop, "CanPlay") == 0) {
    ret = g_variant_new_boolean (TRUE);
  } else if (strcmp (prop, "CanPause") == 0) {
    ret = g_variant_new_boolean (TRUE);
  } else if (strcmp (prop, "CanSeek") == 0) {
    ret = g_variant_new_boolean (TRUE);
  } else if (strcmp (prop, "CanControl") == 0) {
    ret = g_variant_new_boolean (TRUE);
  } else {
    ret = NULL;
    g_set_error (error, G_DBUS_ERROR,
        G_DBUS_ERROR_UNKNOWN_PROPERTY,
        "Unknown property %s", prop);
  }

  return ret;
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

static gchar *
string_to_utf8 (gchar *maybe_utf8)
{
    gchar *attempted_validation;
    attempted_validation = g_utf8_make_valid (maybe_utf8, -1);

    if (g_utf8_validate (attempted_validation, -1, NULL) ) {
        return attempted_validation;
    } else {
        g_free (attempted_validation);
        return g_strdup ("<invalid utf8>");
    }
}

static void add_metadata_item_string (void *mpv, GVariantDict *dict,
                                     const char *property, const char *tag)
{
  char *temp = NULL;
//    char *temp = mpv_mprisiPropertyGetstring (mpv, property);
    if (temp) {
        char *utf8 = string_to_utf8 (temp);
        g_variant_dict_insert (dict, tag, "s", utf8);
        g_free (utf8);
//        mpv_free (temp);
    }
}

static void add_metadata_item_int (void *mpv, GVariantDict *dict,
                                  const char *property, const char *tag)
{
    int64_t value;
    int     res = -1;
//    int res = mpv_get_property (mpv, property, MPV_FORMAT_INT64, &value);
    if (res >= 0) {
        g_variant_dict_insert (dict, tag, "x", value);
    }
}

static void add_metadata_item_string_list (void *mpv, GVariantDict *dict,
                                          const char *property, const char *tag)
{
    char  *temp = NULL;
//    char *temp = mpv_mprisiPropertyGetstring (mpv, property);

    if (temp) {
        GVariantBuilder builder;
        char **list = g_strsplit (temp, ", ", 0);
        char **iter = list;
        g_variant_builder_init (&builder, G_VARIANT_TYPE ("as") );

        for (; *iter; iter++) {
            char *item = *iter;
            char *utf8 = string_to_utf8 (item);
            g_variant_builder_add (&builder, "s", utf8);
            g_free (utf8);
        }

        g_variant_dict_insert (dict, tag, "as", &builder);

        g_strfreev (list);
//        mpv_free (temp);
    }
}

static gchar *
path_to_uri (void *mpv, char *path)
{
#if GLIB_CHECK_VERSION (2, 58, 0)
    // version which uses g_canonicalize_filename which expands .. and .
    // and makes the uris neater
    char* working_dir;
    gchar* canonical;
    gchar *uri;

//    working_dir = mpv_mprisiPropertyGetstring (mpv, "working-directory");
    canonical = g_canonicalize_filename (path, working_dir);
    uri = g_filename_to_uri (canonical, NULL, NULL);

//    mpv_free (working_dir);
    g_free (canonical);

    return uri;
#else
    // for compatibility with older versions of glib
    gchar *converted;
    if (g_path_is_absolute (path) ) {
        converted = g_filename_to_uri (path, NULL, NULL);
    } else {
        char* working_dir;
        gchar* absolute;

//        working_dir = mpv_mprisiPropertyGetstring (mpv, "working-directory");
        absolute = g_build_filename (working_dir, path, NULL);
        converted = g_filename_to_uri (absolute, NULL, NULL);

//        mpv_free (working_dir);
        g_free (absolute);
    }

    return converted;
#endif
}

static void add_metadata_uri (void *mpv, GVariantDict *dict)
{
    char *path;
    char *uri;

//    path = mpv_mprisiPropertyGetstring (mpv, "path");
    if (!path) {
        return;
    }

    uri = g_uri_parse_scheme (path);
    if (uri) {
        g_variant_dict_insert (dict, "xesam:url", "s", path);
        g_free (uri);
    } else {
        gchar *converted = path_to_uri (mpv, path);
        g_variant_dict_insert (dict, "xesam:url", "s", converted);
        g_free (converted);
    }

//    mpv_free (path);
}

// cached last file path, owned by mpv
static char *cached_path = NULL;

static void add_metadata_content_created (void *mpv, GVariantDict *dict)
{
    char *date_str = NULL;
    GDate* date = g_date_new ();
//    char *date_str = mpv_mprisiPropertyGetstring (mpv, "metadata/by-key/Date");

    if (!date_str) {
        return;
    }

    if (strlen (date_str) == 4) {
        gint64 year = g_ascii_strtoll (date_str, NULL, 10);
        if (year != 0) {
            g_date_set_dmy (date, 1, 1, year);
        }
    } else {
        g_date_set_parse (date, date_str);
    }

    if (g_date_valid (date) ) {
        gchar iso8601[20];
        g_date_strftime (iso8601, 20, "%Y-%m-%dT00:00:00Z", date);
        g_variant_dict_insert (dict, "xesam:contentCreated", "s", iso8601);
    }

    g_date_free (date);
//    mpv_free (date_str);
}

static GVariant *create_metadata (contdata_t *contdata)
{
    GVariantDict dict;
    int64_t track;
    double duration;
    char *temp_str;
    int res;

    g_variant_dict_init (&dict, NULL);

    // mpris:trackid
//    mpv_get_property (contdata->mpv, "playlist-pos", MPV_FORMAT_INT64, &track);
    // playlist-pos < 0 if there is no playlist or current track
    if (track < 0) {
        temp_str = g_strdup ("/noplaylist");
    } else {
        temp_str = g_strdup_printf ("/%" PRId64, track);
    }
    g_variant_dict_insert (&dict, "mpris:trackid", "o", temp_str);
    g_free (temp_str);

    // mpris:length
//    res = mpv_get_property (contdata->mpv, "duration", MPV_FORMAT_DOUBLE, &duration);
//    if (res == MPV_ERROR_SUCCESS) {
        g_variant_dict_insert (&dict, "mpris:length", "x", (int64_t) (duration * 1000000.0) );
//    }

    // initial value. Replaced with metadata value if available
    add_metadata_item_string (contdata, &dict, "media-title", "xesam:title");

    add_metadata_item_string (contdata, &dict, "metadata/by-key/Title", "xesam:title");
    add_metadata_item_string (contdata, &dict, "metadata/by-key/Album", "xesam:album");
    add_metadata_item_string (contdata, &dict, "metadata/by-key/Genre", "xesam:genre");

    /* Musicbrainz metadata mappings
       (https://picard-docs.musicbrainz.org/en/appendices/tag_mapping.html) */

    // IDv3 metadata format
    add_metadata_item_string (contdata, &dict, "metadata/by-key/MusicBrainz Artist Id", "mb:artistId");
    add_metadata_item_string (contdata, &dict, "metadata/by-key/MusicBrainz Track Id", "mb:trackId");
    add_metadata_item_string (contdata, &dict, "metadata/by-key/MusicBrainz Album Artist Id", "mb:albumArtistId");
    add_metadata_item_string (contdata, &dict, "metadata/by-key/MusicBrainz Album Id", "mb:albumId");
    add_metadata_item_string (contdata, &dict, "metadata/by-key/MusicBrainz Release Track Id", "mb:releaseTrackId");
    add_metadata_item_string (contdata, &dict, "metadata/by-key/MusicBrainz Work Id", "mb:workId");

    // Vorbis & APEv2 metadata format
    add_metadata_item_string (contdata, &dict, "metadata/by-key/MUSICBRAINZ_ARTISTID", "mb:artistId");
    add_metadata_item_string (contdata, &dict, "metadata/by-key/MUSICBRAINZ_TRACKID", "mb:trackId");
    add_metadata_item_string (contdata, &dict, "metadata/by-key/MUSICBRAINZ_ALBUMARTISTID", "mb:albumArtistId");
    add_metadata_item_string (contdata, &dict, "metadata/by-key/MUSICBRAINZ_ALBUMID", "mb:albumId");
    add_metadata_item_string (contdata, &dict, "metadata/by-key/MUSICBRAINZ_RELEASETRACKID", "mb:releaseTrackId");
    add_metadata_item_string (contdata, &dict, "metadata/by-key/MUSICBRAINZ_WORKID", "mb:workId");

    add_metadata_item_string_list (contdata, &dict, "metadata/by-key/uploader", "xesam:artist");
    add_metadata_item_string_list (contdata, &dict, "metadata/by-key/Artist", "xesam:artist");
    add_metadata_item_string_list (contdata, &dict, "metadata/by-key/Album_Artist", "xesam:albumArtist");
    add_metadata_item_string_list (contdata, &dict, "metadata/by-key/Composer", "xesam:composer");

    add_metadata_item_int (contdata, &dict, "metadata/by-key/Track", "xesam:trackNumber");
    add_metadata_item_int (contdata, &dict, "metadata/by-key/Disc", "xesam:discNumber");

    add_metadata_uri (contdata, &dict);
//    add_metadata_art (contdata, &dict);
    add_metadata_content_created (contdata, &dict);

    return g_variant_dict_end (&dict);
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
