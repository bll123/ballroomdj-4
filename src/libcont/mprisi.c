/*
 * Copyright 2024-2025 Brad Lanam Pleasant Hill CA
 *
 * References:
 *    https://github.com/w0rp/xmms2-mpris/tree/master/src
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

#include "mpris-root.h"
#include "mpris-player.h"

#include "bdj4.h"
#include "bdj4ui.h"     // for speed constants
#include "callback.h"
#include "controller.h"
#include "dbusi.h"
#include "mdebug.h"
#include "player.h"
#include "nlist.h"
#include "tmutil.h"

static const char *urischemes [] = {
  "file",
  NULL,
};

static const char *mimetypes [] = {
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
  NULL,
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

static const char *metadatastr [CONT_METADATA_MAX] = {
  [CONT_METADATA_ALBUM] = "xesam:album",
  [CONT_METADATA_ALBUMARTIST] = "xesam:albumArtist",
  [CONT_METADATA_ARTIST] = "xesam:artist",
  [CONT_METADATA_TITLE] = "xesam:title",
  [CONT_METADATA_TRACKID] = "mpris:trackid",
  [CONT_METADATA_DURATION] = "mpris:length",
  [CONT_METADATA_URI] = "xesam:url",
  [CONT_METADATA_ART_URI] = "xesam:url",
  [CONT_METADATA_GENRE] = "xesam:genre",
};

enum {
  MPRIS_PB_STATUS_PLAY,
  MPRIS_PB_STATUS_PAUSE,
  MPRIS_PB_STATUS_STOP,
  MPRIS_PB_STATUS_MAX,
};

static const char *playstatusstr [MPRIS_PB_STATUS_MAX] = {
  [MPRIS_PB_STATUS_PLAY] = "Playing",
  [MPRIS_PB_STATUS_PAUSE] = "Paused",
  [MPRIS_PB_STATUS_STOP] = "Stopped",
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
  mprisMediaPlayer2   *mprisroot;
  mprisMediaPlayer2Player *mprisplayer;
  callback_t          *cb;
  callback_t          *cburi;
  nlist_t             *metadata;
  void                *metav;
  int                 playstate;      // BDJ4 play state
  int                 playstatus;
  int                 repeatstatus;
  int32_t             pos;
  int                 rate;
  int                 volume;
} contdata_t;

static void mprisInitializeRoot (contdata_t *contdata);
static void mprisInitializePlayer (contdata_t *contdata);
static gboolean mprisQuit (mprisMediaPlayer2 *root, GDBusMethodInvocation *invocation, void *udata);
static gboolean mprisNext (mprisMediaPlayer2Player *player, GDBusMethodInvocation *invocation, void *udata);
static gboolean mprisPlay (mprisMediaPlayer2Player *player, GDBusMethodInvocation *invocation, void *udata);
static gboolean mprisPause (mprisMediaPlayer2Player *player, GDBusMethodInvocation *invocation, void *udata);
static gboolean mprisPlayPause (mprisMediaPlayer2Player *player, GDBusMethodInvocation *invocation, void *udata);
static gboolean mprisStop (mprisMediaPlayer2Player *player, GDBusMethodInvocation *invocation, void *udata);
static gboolean mprisSetPosition (mprisMediaPlayer2Player* player, GDBusMethodInvocation* invocation, const gchar *track_id, gint64 position, void *udata);
static gboolean mprisSeek (mprisMediaPlayer2Player* player, GDBusMethodInvocation* invocation, gint64 offset, void *udata);
static gboolean mprisOpenURI (mprisMediaPlayer2Player *player, GDBusMethodInvocation *invocation, const gchar *uri, void *udata);
static gboolean mprisRepeat (mprisMediaPlayer2Player *player, GDBusMethodInvocation *invocation, void *udata);
static gboolean mprisRate (mprisMediaPlayer2Player *player, GDBusMethodInvocation *invocation, void *udata);

void
contiDesc (const char **ret, int max)
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
  contdata->mprisroot = NULL;
  contdata->mprisplayer = NULL;
  contdata->metadata = NULL;
  contdata->metav = NULL;
  contdata->playstate = PL_STATE_STOPPED;
  contdata->playstatus = MPRIS_PB_STATUS_STOP;
  contdata->repeatstatus = MPRIS_REPEAT_NONE;
  contdata->pos = 0;
  contdata->rate = 100;
  contdata->volume = 0;

  contdata->dbus = dbusConnInit ();
  dbusConnectAcquireName (contdata->dbus, contdata->instname,
      interface [MPRIS_INTFC_MP2]);

  return contdata;
}

void
contiFree (contdata_t *contdata)
{
  if (contdata == NULL) {
    return;
  }

  nlistFree (contdata->metadata);
  if (contdata->dbus != NULL) {
    dbusConnClose (contdata->dbus);
  }
  dataFree (contdata->instname);
  mdfree (contdata);
}

void
contiSetup (contdata_t *contdata)
{
  mprisInitializeRoot (contdata);
  mprisInitializePlayer (contdata);
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
contiSetCallbacks (contdata_t *contdata, callback_t *cb, callback_t *cburi)
{
  if (contdata == NULL || cb == NULL) {
    return;
  }

  contdata->cb = cb;
  contdata->cburi = cburi;
}

void
contiSetPlayState (contdata_t *contdata, int state)
{
  int   nstate = MPRIS_PB_STATUS_STOP;
  bool  canplay = false;
  bool  canpause = false;
  bool  canseek = false;

  if (contdata == NULL) {
    return;
  }

  contdata->playstate = state;

  switch (state) {
    case PL_STATE_LOADING:
    case PL_STATE_PLAYING: {
      nstate = MPRIS_PB_STATUS_PLAY;
      canplay = true;
      canpause = true;
      canseek = true;
      break;
    }
    case PL_STATE_IN_FADEOUT: {
      nstate = MPRIS_PB_STATUS_PLAY;
      canplay = false;
      canpause = false;
      canseek = false;
      break;
    }
    case PL_STATE_PAUSED: {
      nstate = MPRIS_PB_STATUS_PAUSE;
      canplay = true;
      canpause = true;    // so that play-pause will work
      canseek = true;
      break;
    }
    case PL_STATE_IN_CROSSFADE:
    case PL_STATE_IN_GAP: {
      nstate = MPRIS_PB_STATUS_PLAY;
      canplay = false;
      canpause = false;
      canseek = false;
      break;
    }
    case PL_STATE_UNKNOWN:
    case PL_STATE_STOPPED: {
      /* the mpris-stopped state is completely stopped, not running */
      /* want to be able to update the display when the player is stopped */
      nstate = MPRIS_PB_STATUS_PAUSE;
      canplay = true;
      canpause = true;      // so that play-pause will work
      canseek = false;
      break;
    }
    default: {
      break;
    }
  }

  mpris_media_player2_player_set_can_play (contdata->mprisplayer, canplay);
  mpris_media_player2_player_set_can_pause (contdata->mprisplayer, canpause);
  mpris_media_player2_player_set_can_seek (contdata->mprisplayer, canseek);

  if (contdata->playstatus != nstate) {
    mpris_media_player2_player_set_playback_status (contdata->mprisplayer,
        playstatusstr [nstate]);
    contdata->playstatus = nstate;
  }
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
    mpris_media_player2_player_set_loop_status (contdata->mprisplayer,
        repeatstr [nstate]);
    contdata->repeatstatus = nstate;
  }
}

void
contiSetPosition (contdata_t *contdata, int32_t pos)
{
  if (contdata == NULL) {
    return;
  }

  if (contdata->pos != pos) {
    int64_t   tpos;
    int32_t   pdiff;

    tpos = pos * 1000;    // microseconds
    pdiff = pos - contdata->pos;
    mpris_media_player2_player_set_position (contdata->mprisplayer, tpos);
    if (pdiff < 0 || pdiff > 300) {
      /* the seek signal is only supposed to be sent when there is */
      /* a large change */
      mpris_media_player2_player_emit_seeked (contdata->mprisplayer, tpos);
    }
    contdata->pos = pos;
  }
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
    mpris_media_player2_player_set_rate (contdata->mprisplayer, dval);
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
    mpris_media_player2_player_set_volume (contdata->mprisplayer, dval);
  }
  contdata->volume = volume;
}

void
contiSetCurrent (contdata_t *contdata, contmetadata_t *cmetadata)
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

  if (cmetadata->trackid >= 0) {
    snprintf (tbuff, sizeof (tbuff), "/org/bdj4/%" PRId32,
        cmetadata->trackid);
  } else {
    snprintf (tbuff, sizeof (tbuff), "/NoTrack");
  }
  nlistSetStr (contdata->metadata, CONT_METADATA_TRACKID, tbuff);

  nlistSetNum (contdata->metadata, CONT_METADATA_DURATION, cmetadata->duration);

  if (cmetadata->title != NULL) {
    nlistSetStr (contdata->metadata, CONT_METADATA_TITLE, cmetadata->title);
  }
  if (cmetadata->artist != NULL) {
    nlistSetStr (contdata->metadata, CONT_METADATA_ARTIST, cmetadata->artist);
  }
  if (cmetadata->album != NULL) {
    nlistSetStr (contdata->metadata, CONT_METADATA_ALBUM, cmetadata->album);
  }
  if (cmetadata->albumartist != NULL) {
    nlistSetStr (contdata->metadata, CONT_METADATA_ALBUMARTIST,
        cmetadata->albumartist);
  }
  if (cmetadata->genre != NULL) {
    nlistSetStr (contdata->metadata, CONT_METADATA_GENRE, cmetadata->genre);
  }
  if (cmetadata->uri != NULL) {
    nlistSetStr (contdata->metadata, CONT_METADATA_URI, cmetadata->uri);
  }
  if (cmetadata->arturi != NULL) {
    nlistSetStr (contdata->metadata, CONT_METADATA_ART_URI, cmetadata->arturi);
  }

  nlistStartIterator (contdata->metadata, &miter);
  dbusMessageInitArray (contdata->dbus, "a{sv}");
  while ((mkey = nlistIterateKey (contdata->metadata, &miter)) >= 0) {
    if (mkey == CONT_METADATA_DURATION) {
      tv = dbusMessageBuild ("x", nlistGetNum (contdata->metadata, mkey));
    } else if (mkey == CONT_METADATA_TRACKID) {
      tv = dbusMessageBuild ("o", nlistGetStr (contdata->metadata, mkey));
    } else {
      tv = dbusMessageBuild ("s", nlistGetStr (contdata->metadata, mkey));
    }
    dbusMessageAppendArray (contdata->dbus, "{sv}",
        metadatastr [mkey], tv, NULL);
  }
  tv = dbusMessageFinalizeArray (contdata->dbus);
  contdata->metav = tv;

  mpris_media_player2_player_set_metadata (contdata->mprisplayer, contdata->metav);
  mpris_media_player2_player_set_can_go_next (contdata->mprisplayer, true);
}

/* internal routines */

static void
mprisInitializeRoot (contdata_t *contdata)
{
  contdata->mprisroot = mpris_media_player2_skeleton_new ();

  mpris_media_player2_set_can_quit (contdata->mprisroot, true);
  mpris_media_player2_set_can_raise (contdata->mprisroot, false);
  mpris_media_player2_set_has_track_list (contdata->mprisroot, false);
  mpris_media_player2_set_identity (contdata->mprisroot, BDJ4_NAME);
  mpris_media_player2_set_desktop_entry (contdata->mprisroot, BDJ4_NAME);
  mpris_media_player2_set_supported_mime_types (contdata->mprisroot, mimetypes);
  mpris_media_player2_set_supported_uri_schemes (contdata->mprisroot, urischemes);

  g_signal_connect (contdata->mprisroot, "handle-quit",
      G_CALLBACK (mprisQuit), contdata);

  dbusSetInterfaceSkeleton (contdata->dbus, contdata->mprisroot,
      objpath [MPRIS_OBJP_MP2]);
}

static void
mprisInitializePlayer (contdata_t *contdata)
{

  contdata->mprisplayer = mpris_media_player2_player_skeleton_new ();

  mpris_media_player2_player_set_playback_status (contdata->mprisplayer,
      playstatusstr [MPRIS_PB_STATUS_STOP]);
  mpris_media_player2_player_set_loop_status (contdata->mprisplayer,
      repeatstr [MPRIS_REPEAT_NONE]);
  mpris_media_player2_player_set_rate (contdata->mprisplayer, 0.0);
  mpris_media_player2_player_set_volume (contdata->mprisplayer, true);
  mpris_media_player2_player_set_position (contdata->mprisplayer, 0);
  mpris_media_player2_player_set_minimum_rate (contdata->mprisplayer,
      SPD_LOWER / 100.0);
  mpris_media_player2_player_set_maximum_rate (contdata->mprisplayer,
      SPD_UPPER / 100.0);
  mpris_media_player2_player_set_shuffle (contdata->mprisplayer, false);
  mpris_media_player2_player_set_can_go_next (contdata->mprisplayer, false);
  mpris_media_player2_player_set_can_go_previous (contdata->mprisplayer, false);
  mpris_media_player2_player_set_can_play (contdata->mprisplayer, false);
  mpris_media_player2_player_set_can_pause (contdata->mprisplayer, false);
  mpris_media_player2_player_set_can_seek (contdata->mprisplayer, false);
  mpris_media_player2_player_set_can_control (contdata->mprisplayer, true);

  g_signal_connect (contdata->mprisplayer, "handle-next",
      G_CALLBACK (mprisNext), contdata);
  g_signal_connect (contdata->mprisplayer, "handle-play",
      G_CALLBACK (mprisPlay), contdata);
  g_signal_connect (contdata->mprisplayer, "handle-pause",
      G_CALLBACK (mprisPause), contdata);
  g_signal_connect (contdata->mprisplayer, "handle-play-pause",
      G_CALLBACK (mprisPlayPause), contdata);
  g_signal_connect (contdata->mprisplayer, "handle-stop",
      G_CALLBACK (mprisStop), contdata);
  g_signal_connect (contdata->mprisplayer, "handle-set-position",
      G_CALLBACK (mprisSetPosition), contdata);
  g_signal_connect (contdata->mprisplayer, "handle-seek",
      G_CALLBACK (mprisSeek), contdata);
  g_signal_connect (contdata->mprisplayer, "handle-open-uri",
      G_CALLBACK (mprisOpenURI), contdata);
  g_signal_connect (contdata->mprisplayer, "notify::loop-status",
      G_CALLBACK (mprisRepeat), contdata);
  g_signal_connect (contdata->mprisplayer, "notify::rate",
      G_CALLBACK (mprisRate), contdata);

  dbusSetInterfaceSkeleton (contdata->dbus, contdata->mprisplayer,
      objpath [MPRIS_OBJP_MP2]);
}

static gboolean
mprisQuit (mprisMediaPlayer2 *root,
    GDBusMethodInvocation *invocation,
    void *udata)
{
  contdata_t  *contdata = udata;

  if (contdata->cb != NULL) {
    callbackHandlerII (contdata->cb, CONTROLLER_QUIT, 0);
  }
  mpris_media_player2_complete_quit (root, invocation);
  return true;
}

static gboolean
mprisNext (mprisMediaPlayer2Player *player,
    GDBusMethodInvocation *invocation,
    void *udata)
{
  contdata_t  *contdata = udata;

  if (contdata->cb != NULL) {
    callbackHandlerII (contdata->cb, CONTROLLER_NEXT, 0);
  }
  mpris_media_player2_player_complete_next (player, invocation);
  return true;
}

static gboolean
mprisPlay (mprisMediaPlayer2Player *player,
    GDBusMethodInvocation *invocation,
    void *udata)
{
  contdata_t  *contdata = udata;

  if (contdata->cb != NULL) {
    callbackHandlerII (contdata->cb, CONTROLLER_PLAY, 0);
  }
  mpris_media_player2_player_complete_play (player, invocation);
  return true;
}

static gboolean
mprisPause (mprisMediaPlayer2Player *player,
    GDBusMethodInvocation *invocation,
    void *udata)
{
  contdata_t  *contdata = udata;

  if (contdata->cb != NULL) {
    callbackHandlerII (contdata->cb, CONTROLLER_PAUSE, 0);
  }
  mpris_media_player2_player_complete_pause (player, invocation);
  return true;
}

static gboolean
mprisPlayPause (mprisMediaPlayer2Player *player,
    GDBusMethodInvocation *invocation,
    void *udata)
{
  contdata_t  *contdata = udata;

  if (contdata->cb != NULL) {
    callbackHandlerII (contdata->cb, CONTROLLER_PLAYPAUSE, 0);
  }
  mpris_media_player2_player_complete_play_pause (player, invocation);
  return true;
}

static gboolean
mprisStop (mprisMediaPlayer2Player *player,
    GDBusMethodInvocation *invocation,
    void *udata)
{
  contdata_t  *contdata = udata;

  if (contdata->cb != NULL) {
    callbackHandlerII (contdata->cb, CONTROLLER_STOP, 0);
  }
  mpris_media_player2_player_complete_stop (player, invocation);
  return true;
}

static gboolean
mprisSetPosition (mprisMediaPlayer2Player* player,
    GDBusMethodInvocation* invocation,
    const gchar *track_id,
    gint64 position,
    void *udata)
{
  contdata_t  *contdata = udata;

  if (contdata->cb != NULL) {
    position /= 1000;     // microseconds
    callbackHandlerII (contdata->cb, CONTROLLER_SEEK, position);
  }
  mpris_media_player2_player_complete_set_position (player, invocation);
  return true;
}

static gboolean
mprisSeek (mprisMediaPlayer2Player* player,
    GDBusMethodInvocation* invocation,
    gint64 offset,
    void *udata)
{
  contdata_t  *contdata = udata;

  if (contdata->cb != NULL) {
    offset /= 1000;   // microseconds
    offset += contdata->pos;
    callbackHandlerII (contdata->cb, CONTROLLER_SEEK, offset);
  }
  mpris_media_player2_player_complete_seek (player, invocation);
  return true;
}

static gboolean
mprisOpenURI (mprisMediaPlayer2Player *player,
    GDBusMethodInvocation *invocation,
    const gchar *uri,
    void *udata)
{
  contdata_t  *contdata = udata;

  if (contdata->cburi != NULL) {
    callbackHandlerSI (contdata->cburi, uri, CONTROLLER_OPEN_URI);
  }
  mpris_media_player2_player_complete_open_uri (player, invocation);
  return true;
}

static gboolean
mprisRepeat (mprisMediaPlayer2Player *player,
    GDBusMethodInvocation *invocation,
    void *udata)
{
  contdata_t  *contdata = udata;
  const char  *repstatus;
  int         repid = MPRIS_REPEAT_NONE;
  bool        repflag = false;

  repstatus = mpris_media_player2_player_get_loop_status (player);
  for (int i = 0; i < MPRIS_REPEAT_MAX; ++i) {
    if (strcmp (repstatus, repeatstr [i]) == 0) {
      repid = i;
      break;
    }
  }

  if (repid == contdata->repeatstatus) {
    return true;
  }
  /* MPRIS_REPEAT_PLAYLIST is not valid for BDJ4 */
  if (repid == MPRIS_REPEAT_PLAYLIST) {
    return false;
  }

  if (repid == MPRIS_REPEAT_TRACK) {
    repflag = true;
  }

  if (contdata->cb != NULL) {
    callbackHandlerII (contdata->cb, CONTROLLER_REPEAT, repflag);
  }
  contdata->repeatstatus = repid;

  return true;
}

static gboolean
mprisRate (mprisMediaPlayer2Player *player,
    GDBusMethodInvocation *invocation,
    void *udata)
{
  contdata_t  *contdata = udata;
  double      nrate;
  int         rate;

  nrate = mpris_media_player2_player_get_rate (player);
  nrate *= 100.0;
  rate = nrate;

  if (contdata->rate == rate) {
    return true;
  }
  if (rate < SPD_LOWER || rate > SPD_UPPER) {
    return false;
  }

  if (contdata->cb != NULL) {
    callbackHandlerII (contdata->cb, CONTROLLER_RATE, rate);
  }
  contdata->rate = rate;

  return true;
}

#endif /* __linux__ */
