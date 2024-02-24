/*
 *  Copyright 2024 Brad Lanam Pleasant Hill CA
 *
 * Using the framework from:
 *    https://github.com/hoyon/mpv-mpris/blob/master/mpris.c
 *    Copyright (c) 2017 Ho-Yon Mak (MIT License)
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>

#if __linux__

#include <gio/gio.h>
#include <glib.h>

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

typedef struct controller {
  gint                bus_id;
  GDBusNodeInfo       *introspection_data = NULL;
  GDBusInterfaceInfo  *root_interface_info;
  GDBusInterfaceInfo  *player_interface_info;
  GDBusConnection     *connection;
  guint               root_interface_id;
  guint               player_interface_id;

  const char          *status;
  const char          *loop_status;
  GHashTable          *changed_properties;
  GVariant            *metadata;
  gboolean            seek_expected;
  gboolean            idle;
  gboolean            paused;
} controller_t;

static const char *STATUS_PLAYING = "Playing";
static const char *STATUS_PAUSED = "Paused";
static const char *STATUS_STOPPED = "Stopped";
static const char *LOOP_NONE = "None";
static const char *LOOP_TRACK = "Track";
static const char *LOOP_PLAYLIST = "Playlist";

static GDBusInterfaceVTable dbusMP2InterfaceList = {
  mprisiMP2,
  mprisiMP2GetProperty,
  mprisiMP2SetProperty,
  NULL,
};

static GDBusInterfaceVTable dbusPlayerInterfaceList = {
  mprisiPlayer,
  mprisiPlayerGetProperty,
  mprisiPlayerSetProperty,
  NULL,
};

static void mprisiBusAcquired (GDBusConnection *connection, const char *busname, void *udata);
static void mprisiBusLost (GDBusConnection *connection, const char *busname, void *udata);
static void mprisiMP2 (GDBusConnection *connection, const char *sender, const char *object_path, const char *interface_name, const char *method_name, GVariant *parameters, GDBusMethodInvocation *invocation, void *udata);
static GVariant *mprisiMP2GetProperty (GDBusConnection *connection, const char *sender, const char *object_path, const char *interface_name, const char *property_name, GError **error, void *udata);
static gboolean mprisiMP2SetProperty (GDBusConnection *connection, const char *sender, const char *object_path, const char *interface_name, const char *property_name, GVariant *value, GError **error, void *udata);
static void mprisiPlayer (GDBusConnection *connection, const char *sender, const char *_object_path, const char *interface_name, const char *method_name, GVariant *parameters, GDBusMethodInvocation *invocation, void *udata);
static GVariant * mprisiPlayerGetProperty (GDBusConnection *connection, const char *sender, const char *object_path, const char *interface_name, const char *property_name, GError **error, void *udata);
static gboolean mprisiPlayerSetProperty (GDBusConnection *connection, const char *sender, const char *object_path, const char *interface_name, const char *property_name, GVariant *value, GError **error, void *udata);

int
controllerInit (const char *instname)
{
  controller_t    *cont;
  GError          *error = NULL;
  GSource         *timeout_source;
  char            fullinstname [200];

  cont = mdmalloc (sizeof (controller_t));

  snprintf (fullinstname, sizeof (fullinstname), "org.mpris.MediaPlayer2.%s",
      instname);

  cont->introspection_data = g_dbus_node_info_new_for_xml (
      introspection_xml, &error);
  if (error != NULL) {
    g_printerr ("%s", error->message);
  }

  cont->root_interface_info = g_dbus_node_info_lookup_interface (
      cont->introspection_data, "org.mpris.MediaPlayer2");
  cont->player_interface_info = g_dbus_node_info_lookup_interface (
      cont->introspection_data, "org.mpris.MediaPlayer2.Player");

  cont->status = STATUS_STOPPED;
  cont->loop_status = LOOP_NONE;
  cont->changed_properties = g_hash_table_new (g_str_hash, g_str_equal);
  cont->seek_expected = FALSE;
  cont->idle = FALSE;
  cont->paused = FALSE;

  cont->bus_id = g_bus_own_name (G_BUS_TYPE_SESSION,
      fullinstname,
      G_BUS_NAME_OWNER_FLAGS_DO_NOT_QUEUE,
      mprisiBusAcquired,
      NULL,
      mprisiBusLost,
      &cont, NULL);

  return cont;
}

int
controllerCleanup (controller_t *cont)
{
  g_dbus_connection_unregister_object (cont->connection, cont->root_interface_id);
  g_dbus_connection_unregister_object (cont->connection, cont->player_interface_id);

  g_bus_unown_name (cont->bus_id);
  g_dbus_node_info_unref (cont->introspection_data);
}

// Register D-Bus object and interfaces
static void
mprisiBusAcquired (GDBusConnection *connection,
    const char *busname,
    void *udata)
{
  GError            *error = NULL;
  controller_t      *cont = udata;

  cont->connection = connection;
  cont->root_interface_id = g_dbus_connection_register_object (
      connection, "/org/mpris/MediaPlayer2",
      cont->root_interface_info,
      &dbusMP2InterfaceList,
      udata, NULL, &error);
  if (error != NULL) {
    g_printerr ("%s", error->message);
  }

  cont->player_interface_id = g_dbus_connection_register_object (
      connection, "/org/mpris/MediaPlayer2",
      cont->player_interface_info,
      &dbusPlayerInterfaceList,
      udata, NULL, &error);
  if (error != NULL) {
    g_printerr ("%s", error->message);
  }
}

static void
mprisiBusLost (GDBusConnection *connection,
    const char *busname,
    void *udata)
{
  if (connection) {
    controller_t  *cont = udata;
    pid_t         pid = getpid ();
    char          *tname;

    tname = g_strdup_printf ("org.mpris.MediaPlayer2.mpv.instance%d", pid);
    cont->bus_id = g_bus_own_name (G_BUS_TYPE_SESSION,
        name,
        G_BUS_NAME_OWNER_FLAGS_NONE,
        NULL, NULL, NULL,
        &cont, NULL);
    g_free (name);
  }
}

static void
mprisiMP2 (GDBusConnection *connection,
    const char *sender,
    const char *object_path,
    const char *interface_name,
    const char *method_name,
    GVariant *parameters,
    GDBusMethodInvocation *invocation,
    void *udata)
{
    controller_t *ud = (controller_t*)udata;

    if (strcmp (method_name, "Quit") == 0) {
        const char *cmd[] = {"quit", NULL};
        mpv_command_async (ud->mpv, 0, cmd);
        g_dbus_method_invocation_return_value (invocation, NULL);

    } else if (strcmp (method_name, "Raise") == 0) {
        // Can't raise
        g_dbus_method_invocation_return_value (invocation, NULL);

    } else {
        g_dbus_method_invocation_return_error (invocation, G_DBUS_ERROR,
                                              G_DBUS_ERROR_UNKNOWN_METHOD,
                                              "Unknown method");
    }
}

static GVariant *
mprisiMP2GetProperty (GDBusConnection *connection,
    const char *sender,
    const char *object_path,
    const char *interface_name,
    const char *property_name,
    GError **error,
    void *udata)
{
    controller_t *ud = (controller_t*)udata;
    GVariant *ret;

    if (strcmp (property_name, "CanQuit") == 0) {
      ret = g_variant_new_boolean (TRUE);
    } else if (strcmp (property_name, "Fullscreen") == 0) {
      ret = g_variant_new_boolean (FALSE);
    } else if (strcmp (property_name, "CanSetFullscreen") == 0) {
      ret = g_variant_new_boolean (FALSE);
    } else if (strcmp (property_name, "CanRaise") == 0) {
      ret = g_variant_new_boolean (FALSE);
    } else if (strcmp (property_name, "HasTrackList") == 0) {
      ret = g_variant_new_boolean (FALSE);
    } else if (strcmp (property_name, "Identity") == 0) {
      ret = g_variant_new_string ("BDJ4");
    } else if (strcmp (property_name, "DesktopEntry") == 0) {
      ret = g_variant_new_string ("BDJ4");
    } else if (strcmp (property_name, "SupportedUriSchemes") == 0) {
        GVariantBuilder builder;

        g_variant_builder_init (&builder, G_VARIANT_TYPE ("as"));
        g_variant_builder_add (&builder, "s", "https");
        g_variant_builder_add (&builder, "s", "file");
        ret = g_variant_builder_end (&builder);
    } else if (strcmp (property_name, "SupportedMimeTypes") == 0) {
        GVariantBuilder builder;

        g_variant_builder_init (&builder, G_VARIANT_TYPE ("as"));
        /* need to get this list from the player interface, probably */
        g_variant_builder_add (&builder, "s", "application/ogg");
        g_variant_builder_add (&builder, "s", "audio/mpeg");
        // TODO add the rest
        ret = g_variant_builder_end (&builder);

    } else {
        ret = NULL;
        g_set_error (error, G_DBUS_ERROR,
                    G_DBUS_ERROR_UNKNOWN_PROPERTY,
                    "Unknown property %s", property_name);
    }

    return ret;
}

static gboolean
mprisiMP2SetProperty (GDBusConnection *connection,
    const char *sender,
    const char *object_path,
    const char *interface_name,
    const char *property_name,
    GVariant *value,
    GError **error,
    void *udata)
{
    controller_t *ud = (controller_t*)udata;

    g_set_error (error, G_DBUS_ERROR,
        G_DBUS_ERROR_UNKNOWN_PROPERTY,
        "Cannot set property %s", property_name);
    return FALSE;
}

static void
mprisiPlayer (GDBusConnection *connection,
    const char *sender,
    const char *_object_path,
    const char *interface_name,
    const char *method_name,
    GVariant *parameters,
    GDBusMethodInvocation *invocation,
    void *udata)
{
    controller_t *ud = (controller_t*) udata;

    if (strcmp (method_name, "Pause") == 0) {
        int paused = TRUE;
        /* pause the player */
        g_dbus_method_invocation_return_value (invocation, NULL);

    } else if (strcmp (method_name, "PlayPause") == 0) {
        int paused;

        if (ud->status == STATUS_PAUSED) {
          paused = FALSE;
        } else {
          paused = TRUE;
        }
        /* play/pause the player */
        g_dbus_method_invocation_return_value (invocation, NULL);

    } else if (strcmp (method_name, "Play") == 0) {
        int paused = FALSE;
        /* play/pause the player */
        g_dbus_method_invocation_return_value (invocation, NULL);

    } else if (strcmp (method_name, "Stop") == 0) {
        const char *cmd[] = {"stop", NULL};
        /* stop the player */
        g_dbus_method_invocation_return_value (invocation, NULL);

    } else if (strcmp (method_name, "Next") == 0) {
        const char *cmd[] = {"playlist_next", NULL};
        /* next-song */
        g_dbus_method_invocation_return_value (invocation, NULL);

    } else if (strcmp (method_name, "Previous") == 0) {
        /* not supported */
        g_dbus_method_invocation_return_value (invocation, NULL);
    } else if (strcmp (method_name, "Seek") == 0) {
        int64_t offset_us; // in microseconds
        char *offset_str;
        g_variant_get (parameters, " (x)", &offset_us);
        double offset_s = offset_us / 1000000.0;
        offset_str = g_strdup_printf ("%f", offset_s);

        const char *cmd[] = {"seek", offset_str, NULL};
        /* seek to position */
        g_dbus_method_invocation_return_value (invocation, NULL);
        g_free (offset_str);

    } else if (strcmp (method_name, "SetPosition") == 0) {
        int64_t current_id;
        char *object_path;
        double new_position_s;
        int64_t new_position_us;

        /* seek to position */
        g_variant_get (parameters, " (&ox)", &object_path, &new_position_us);
        new_position_s = ( (float)new_position_us) / 1000000.0; // us -> s

        if (current_id == g_ascii_strtoll (object_path + 1, NULL, 10)) {
            mpv_set_property (ud->mpv, "time-pos", MPV_FORMAT_DOUBLE, &new_position_s);
        }

        g_dbus_method_invocation_return_value (invocation, NULL);

    } else if (strcmp (method_name, "OpenUri") == 0) {
        char *uri;
        g_variant_get (parameters, " (&s)", &uri);
        const char *cmd[] = {"loadfile", uri, NULL};
        /* insert an external request */
        g_dbus_method_invocation_return_value (invocation, NULL);

    } else {
        g_dbus_method_invocation_return_error (invocation, G_DBUS_ERROR,
            G_DBUS_ERROR_UNKNOWN_METHOD,
            "Unknown method");
    }
}

static GVariant *
mprisiPlayerGetProperty (GDBusConnection *connection,
    const char *sender,
    const char *object_path,
    const char *interface_name,
    const char *property_name,
    GError **error,
    void *udata)
{
    controller_t *ud = (controller_t*)udata;
    GVariant *ret;

    if (strcmp (property_name, "PlaybackStatus") == 0) {
        ret = g_variant_new_string (ud->status);

    } else if (strcmp (property_name, "LoopStatus") == 0) {
        ret = g_variant_new_string (ud->loop_status);

    } else if (strcmp (property_name, "Rate") == 0) {
        double rate;
        /* get current speed */
        ret = g_variant_new_double (rate);

    } else if (strcmp (property_name, "Shuffle") == 0) {
        ret = g_variant_new_boolean (FALSE);
    } else if (strcmp (property_name, "Metadata") == 0) {
        if (!ud->metadata) {
            ud->metadata = create_metadata (ud);
        }
        // Increase reference count to prevent it from being freed after returning
        g_variant_ref (ud->metadata);
        ret = ud->metadata;

    } else if (strcmp (property_name, "Volume") == 0) {
        double volume;
        /* get volume */
        volume /= 100;
        ret = g_variant_new_double (volume);

    } else if (strcmp (property_name, "Position") == 0) {
        double position_s;
        int64_t position_us;
        /* get current position */
        position_us = position_s * 1000000.0; // s -> us
        ret = g_variant_new_int64 (position_us);

    } else if (strcmp (property_name, "MinimumRate") == 0) {
        ret = g_variant_new_double (70.0 / 100.0);

    } else if (strcmp (property_name, "MaximumRate") == 0) {
        ret = g_variant_new_double (130.0);

    } else if (strcmp (property_name, "CanGoNext") == 0) {
        ret = g_variant_new_boolean (TRUE);

    } else if (strcmp (property_name, "CanGoPrevious") == 0) {
        ret = g_variant_new_boolean (FALSE);

    } else if (strcmp (property_name, "CanPlay") == 0) {
        ret = g_variant_new_boolean (TRUE);

    } else if (strcmp (property_name, "CanPause") == 0) {
        ret = g_variant_new_boolean (TRUE);

    } else if (strcmp (property_name, "CanSeek") == 0) {
        ret = g_variant_new_boolean (TRUE);

    } else if (strcmp (property_name, "CanControl") == 0) {
        ret = g_variant_new_boolean (TRUE);

    } else {
        ret = NULL;
        g_set_error (error, G_DBUS_ERROR,
                    G_DBUS_ERROR_UNKNOWN_PROPERTY,
                    "Unknown property %s", property_name);
    }

    return ret;
}

static gboolean
mprisiPlayerSetProperty (GDBusConnection *connection,
    const char *sender,
    const char *object_path,
    const char *interface_name,
    const char *property_name,
    GVariant *value,
    GError **error,
    void *udata)
{
  int     rc = FALSE;
    controller_t *ud = (controller_t*)udata;

    if (strcmp (property_name, "LoopStatus") == 0) {
        const char *status;
        int t = TRUE;
        int f = FALSE;
        status = g_variant_get_string (value, NULL);
        if (strcmp (status, "Track") == 0) {
          /* turn repeat toggle on */
          rc = TRUE;
        } else if (strcmp (status, "Playlist") == 0) {
          /* not supported */
        } else {
          /* turn repeat toggle off */
          rc = TRUE;
        }

    } else if (strcmp (property_name, "Rate") == 0) {
        double rate = g_variant_get_double (value);
        /* set the speed */
        rc = TRUE;

    } else if (strcmp (property_name, "Volume") == 0) {
        double volume = g_variant_get_double (value);
        /* set the volume */
        volume *= 100;
        rc = TRUE;
    } else {
        g_set_error (error, G_DBUS_ERROR,
                    G_DBUS_ERROR_UNKNOWN_PROPERTY,
                    "Cannot set property %s", property_name);
    }

    return rc;
}

#if 0

static gchar *
string_to_utf8 (gchar *maybe_utf8)
{
    gchar *attempted_validation;
    attempted_validation = g_utf8_make_valid (maybe_utf8, -1);

    if (g_utf8_validate (attempted_validation, -1, NULL)) {
        return attempted_validation;
    } else {
        g_free (attempted_validation);
        return g_strdup ("<invalid utf8>");
    }
}


static gchar *
path_to_uri (mpv_handle *mpv, char *path)
{
#if GLIB_CHECK_VERSION (2, 58, 0)
    // version which uses g_canonicalize_filename which expands .. and .
    // and makes the uris neater
    char* working_dir;
    gchar* canonical;
    gchar *uri;

    working_dir = mpv_get_property_string (mpv, "working-directory");
    canonical = g_canonicalize_filename (path, working_dir);
    uri = g_filename_to_uri (canonical, NULL, NULL);

    mpv_free (working_dir);
    g_free (canonical);

    return uri;
#else
    // for compatibility with older versions of glib
    gchar *converted;
    if (g_path_is_absolute (path)) {
        converted = g_filename_to_uri (path, NULL, NULL);
    } else {
        char* working_dir;
        gchar* absolute;

        working_dir = mpv_get_property_string (mpv, "working-directory");
        absolute = g_build_filename (working_dir, path, NULL);
        converted = g_filename_to_uri (absolute, NULL, NULL);

        mpv_free (working_dir);
        g_free (absolute);
    }

    return converted;
#endif
}

static void
add_metadata_uri (mpv_handle *mpv, GVariantDict *dict)
{
    char *path;
    char *uri;

    path = mpv_get_property_string (mpv, "path");
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

    mpv_free (path);
}

static const char *youtube_url_pattern =
    "^https?:\\/\\/ (?:youtu.be\\/| (?:www\\.)?youtube\\.com\\/watch\\?v=) (?<id>[a-zA-Z0-9_-]*)\\??.*";

static GRegex *youtube_url_regex;

// cached last file path, owned by mpv
static char *cached_path = NULL;

static GVariant *create_metadata (controller_t *ud)
{
    GVariantDict dict;
    int64_t track;
    double duration;
    char *temp_str;
    int res;

    g_variant_dict_init (&dict, NULL);

    // mpris:trackid
    mpv_get_property (ud->mpv, "playlist-pos", MPV_FORMAT_INT64, &track);
    // playlist-pos < 0 if there is no playlist or current track
    if (track < 0) {
        temp_str = g_strdup ("/noplaylist");
    } else {
        temp_str = g_strdup_printf ("/%" PRId64, track);
    }
    g_variant_dict_insert (&dict, "mpris:trackid", "o", temp_str);
    g_free (temp_str);

    // mpris:length
    res = mpv_get_property (ud->mpv, "duration", MPV_FORMAT_DOUBLE, &duration);
    if (res == MPV_ERROR_SUCCESS) {
        g_variant_dict_insert (&dict, "mpris:length", "x", (int64_t) (duration * 1000000.0));
    }

    // initial value. Replaced with metadata value if available
    add_metadata_item_string (ud->mpv, &dict, "media-title", "xesam:title");

    add_metadata_item_string (ud->mpv, &dict, "metadata/by-key/Title", "xesam:title");
    add_metadata_item_string (ud->mpv, &dict, "metadata/by-key/Album", "xesam:album");
    add_metadata_item_string (ud->mpv, &dict, "metadata/by-key/Genre", "xesam:genre");

    /* Musicbrainz metadata mappings
       (https://picard-docs.musicbrainz.org/en/appendices/tag_mapping.html) */

    // IDv3 metadata format
    add_metadata_item_string (ud->mpv, &dict, "metadata/by-key/MusicBrainz Artist Id", "mb:artistId");
    add_metadata_item_string (ud->mpv, &dict, "metadata/by-key/MusicBrainz Track Id", "mb:trackId");
    add_metadata_item_string (ud->mpv, &dict, "metadata/by-key/MusicBrainz Album Artist Id", "mb:albumArtistId");
    add_metadata_item_string (ud->mpv, &dict, "metadata/by-key/MusicBrainz Album Id", "mb:albumId");
    add_metadata_item_string (ud->mpv, &dict, "metadata/by-key/MusicBrainz Release Track Id", "mb:releaseTrackId");
    add_metadata_item_string (ud->mpv, &dict, "metadata/by-key/MusicBrainz Work Id", "mb:workId");

    // Vorbis & APEv2 metadata format
    add_metadata_item_string (ud->mpv, &dict, "metadata/by-key/MUSICBRAINZ_ARTISTID", "mb:artistId");
    add_metadata_item_string (ud->mpv, &dict, "metadata/by-key/MUSICBRAINZ_TRACKID", "mb:trackId");
    add_metadata_item_string (ud->mpv, &dict, "metadata/by-key/MUSICBRAINZ_ALBUMARTISTID", "mb:albumArtistId");
    add_metadata_item_string (ud->mpv, &dict, "metadata/by-key/MUSICBRAINZ_ALBUMID", "mb:albumId");
    add_metadata_item_string (ud->mpv, &dict, "metadata/by-key/MUSICBRAINZ_RELEASETRACKID", "mb:releaseTrackId");
    add_metadata_item_string (ud->mpv, &dict, "metadata/by-key/MUSICBRAINZ_WORKID", "mb:workId");

    add_metadata_item_string_list (ud->mpv, &dict, "metadata/by-key/uploader", "xesam:artist");
    add_metadata_item_string_list (ud->mpv, &dict, "metadata/by-key/Artist", "xesam:artist");
    add_metadata_item_string_list (ud->mpv, &dict, "metadata/by-key/Album_Artist", "xesam:albumArtist");
    add_metadata_item_string_list (ud->mpv, &dict, "metadata/by-key/Composer", "xesam:composer");

    add_metadata_item_int (ud->mpv, &dict, "metadata/by-key/Track", "xesam:trackNumber");
    add_metadata_item_int (ud->mpv, &dict, "metadata/by-key/Disc", "xesam:discNumber");

    add_metadata_uri (ud->mpv, &dict);
    add_metadata_content_created (ud->mpv, &dict);

    return g_variant_dict_end (&dict);
}

static gboolean
emit_property_changes (gpointer data)
{
    controller_t *ud = (controller_t*)data;
    GError *error = NULL;
    gpointer prop_name, prop_value;
    GHashTableIter iter;

    if (g_hash_table_size (ud->changed_properties) > 0) {
        GVariant *params;
        GVariantBuilder *properties = g_variant_builder_new (G_VARIANT_TYPE ("a{sv}"));
        GVariantBuilder *invalidated = g_variant_builder_new (G_VARIANT_TYPE ("as"));
        g_hash_table_iter_init (&iter, ud->changed_properties);
        while (g_hash_table_iter_next (&iter, &prop_name, &prop_value)) {
            if (prop_value) {
                g_variant_builder_add (properties, "{sv}", prop_name, prop_value);
            } else {
                g_variant_builder_add (invalidated, "s", prop_name);
            }
        }
        params = g_variant_new (" (sa{sv}as)",
                               "org.mpris.MediaPlayer2.Player", properties, invalidated);
        g_variant_builder_unref (properties);
        g_variant_builder_unref (invalidated);

        g_dbus_connection_emit_signal (ud->connection, NULL,
                                      "/org/mpris/MediaPlayer2",
                                      "org.freedesktop.DBus.Properties",
                                      "PropertiesChanged",
                                      params, &error);
        if (error != NULL) {
            g_printerr ("%s", error->message);
        }

        g_hash_table_remove_all (ud->changed_properties);
    }
    return TRUE;
}

static void
emit_seeked_signal (controller_t *ud)
{
    GVariant *params;
    double position_s;
    int64_t position_us;
    GError *error = NULL;
    mpv_get_property (ud->mpv, "time-pos", MPV_FORMAT_DOUBLE, &position_s);
    position_us = position_s * 1000000.0; // s -> us
    params = g_variant_new (" (x)", position_us);

    g_dbus_connection_emit_signal (ud->connection, NULL,
                                  "/org/mpris/MediaPlayer2",
                                  "org.mpris.MediaPlayer2.Player",
                                  "Seeked",
                                  params, &error);

    if (error != NULL) {
        g_printerr ("%s", error->message);
    }
}

static GVariant *
set_playback_status (controller_t *ud)
{
    if (ud->idle) {
        ud->status = STATUS_STOPPED;
    } else if (ud->paused) {
        ud->status = STATUS_PAUSED;
    } else {
        ud->status = STATUS_PLAYING;
    }
    return g_variant_new_string (ud->status);
}

static void
set_stopped_status (controller_t *ud)
{
  const char *prop_name = "PlaybackStatus";
  GVariant *prop_value = g_variant_new_string (STATUS_STOPPED);

  ud->status = STATUS_STOPPED;

  g_hash_table_insert (ud->changed_properties,
                      (gpointer)prop_name, prop_value);

  emit_property_changes (ud);
}

static void
handle_property_change (const char *name, void *data, controller_t *ud)
{
    const char *prop_name = NULL;
    GVariant *prop_value = NULL;
    if (strcmp (name, "pause") == 0) {
        ud->paused = * (int*)data;
        prop_name = "PlaybackStatus";
        prop_value = set_playback_status (ud);

    } else if (strcmp (name, "idle-active") == 0) {
        ud->idle = * (int*)data;
        prop_name = "PlaybackStatus";
        prop_value = set_playback_status (ud);

    } else if (strcmp (name, "media-title") == 0 ||
               strcmp (name, "duration") == 0) {
        // Free existing metadata object
        if (ud->metadata) {
            g_variant_unref (ud->metadata);
        }
        ud->metadata = create_metadata (ud);
        prop_name = "Metadata";
        prop_value = ud->metadata;

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
        char *status = * (char **)data;
        if (strcmp (status, "no") != 0) {
            ud->loop_status = LOOP_TRACK;
        } else {
            char *playlist_status;
            mpv_get_property (ud->mpv, "loop-playlist", MPV_FORMAT_STRING, &playlist_status);
            if (strcmp (playlist_status, "no") != 0) {
                ud->loop_status = LOOP_PLAYLIST;
            } else {
                ud->loop_status = LOOP_NONE;
            }
            mpv_free (playlist_status);
        }
        prop_name = "LoopStatus";
        prop_value = g_variant_new_string (ud->loop_status);

    } else if (strcmp (name, "loop-playlist") == 0) {
        char *status = * (char **)data;
        if (strcmp (status, "no") != 0) {
            ud->loop_status = LOOP_PLAYLIST;
        } else {
            char *file_status;
            mpv_get_property (ud->mpv, "loop-file", MPV_FORMAT_STRING, &file_status);
            if (strcmp (file_status, "no") != 0) {
                ud->loop_status = LOOP_TRACK;
            } else {
                ud->loop_status = LOOP_NONE;
            }
            mpv_free (file_status);
        }
        prop_name = "LoopStatus";
        prop_value = g_variant_new_string (ud->loop_status);

    } else if (strcmp (name, "fullscreen") == 0) {
        gboolean *status = data;
        prop_name = "Fullscreen";
        prop_value = g_variant_new_boolean (*status);

    }

    if (prop_name) {
        if (prop_value) {
            g_variant_ref (prop_value);
        }
        g_hash_table_insert (ud->changed_properties,
                            (gpointer)prop_name, prop_value);
    }
}

#endif


#endif /* linux */
