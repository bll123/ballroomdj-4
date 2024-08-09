/*
 * Copyright 2024 Brad Lanam Pleasant Hill CA
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

static const char *playstatusstr [MPRIS_STATUS_MAX] = {
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
  mprisMediaPlayer2   *mprisroot;
  mprisMediaPlayer2Player *mprisplayer;
  callback_t          *cb;
  nlist_t             *chgprop;
  nlist_t             *metadata;
  void                *metav;
  int                 playstate;      // BDJ4 play state
  int                 playstatus;
  int                 repeatstatus;
  int32_t             pos;
  int                 rate;
  int                 volume;
  bool                seek_expected : 1;
  bool                idle : 1;
  bool                paused : 1;
} contdata_t;

static void mprisInitializeRoot (contdata_t *contdata);
static void mprisInitializePlayer (contdata_t *contdata);
static gboolean mprisNext (mprisMediaPlayer2Player *player, GDBusMethodInvocation *invocation, void *udata);
static gboolean mprisPlay (mprisMediaPlayer2Player *player, GDBusMethodInvocation *invocation, void *udata);
static gboolean mprisPause (mprisMediaPlayer2Player *player, GDBusMethodInvocation *invocation, void *udata);
static gboolean mprisPlayPause (mprisMediaPlayer2Player *player, GDBusMethodInvocation *invocation, void *udata);
static gboolean mprisStop (mprisMediaPlayer2Player *player, GDBusMethodInvocation *invocation, void *udata);

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
  contdata->mprisroot = NULL;
  contdata->mprisplayer = NULL;
  contdata->chgprop = nlistAlloc ("cont-mprisi-prop", LIST_ORDERED, NULL);
  contdata->metadata = NULL;
  contdata->metav = NULL;
  contdata->playstate = PL_STATE_STOPPED;
  contdata->playstatus = MPRIS_STATUS_STOP;
  contdata->repeatstatus = MPRIS_REPEAT_NONE;
  contdata->pos = 0.0;
  contdata->rate = 100;
  contdata->volume = 0;
  contdata->seek_expected = false;
  contdata->idle = false;
  contdata->paused = false;

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

  nlistFree (contdata->chgprop);
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
      nstate = MPRIS_STATUS_PLAY;
      canplay = false;
      canpause = false;
      canseek = true;
      break;
    }
    case PL_STATE_IN_FADEOUT: {
      nstate = MPRIS_STATUS_PLAY;
      canplay = false;
      canpause = false;
      canseek = false;
      break;
    }
    case PL_STATE_PAUSED: {
      nstate = MPRIS_STATUS_PAUSE;
      canplay = true;
      canpause = false;
      canseek = true;
      break;
    }
    case PL_STATE_UNKNOWN:
    case PL_STATE_IN_GAP:
    case PL_STATE_STOPPED: {
      nstate = MPRIS_STATUS_STOP;
      canplay = true;
      canpause = false;
      canseek = false;
      break;
    }
  }

  mpris_media_player2_player_set_can_play (contdata->mprisplayer, canplay);
  mpris_media_player2_player_set_can_pause (contdata->mprisplayer, canpause);
  mpris_media_player2_player_set_can_seek (contdata->mprisplayer, canseek);

  if (contdata->playstatus != nstate) {
    mpris_media_player2_player_set_playback_status (contdata->mprisplayer,
        playstatusstr [nstate]);
//    nlistSetNum (contdata->chgprop, MPRIS_PROP_PB_STATUS, nstate);
//    mprisSendPropertyChange (contdata);
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
//    nlistSetNum (contdata->chgprop, MPRIS_PROP_REPEAT_STATUS, nstate);
//    mprisSendPropertyChange (contdata);
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
    mpris_media_player2_player_set_position (contdata->mprisplayer, pos);
    mpris_media_player2_player_emit_seeked (contdata->mprisplayer, pos);
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
    mpris_media_player2_player_set_rate (contdata->mprisplayer, dval);
//    nlistSetDouble (contdata->chgprop, MPRIS_PROP_RATE, dval);
//    mprisSendPropertyChange (contdata);
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
//    nlistSetDouble (contdata->chgprop, MPRIS_PROP_VOLUME, dval);
//    mprisSendPropertyChange (contdata);
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
    dbusMessageAppendArray (contdata->dbus, "{sv}",
        metadatastr [mkey], tv, NULL);
  }
  tv = dbusMessageFinalizeArray (contdata->dbus);
  contdata->metav = tv;

//  nlistSetNum (contdata->chgprop, MPRIS_PROP_METADATA, 1);
  mpris_media_player2_player_set_metadata (contdata->mprisplayer, contdata->metav);
  mpris_media_player2_player_set_can_go_next (contdata->mprisplayer, true);
//  mprisSendPropertyChange (contdata);
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

  dbusSetInterfaceSkeleton (contdata->dbus, contdata->mprisroot,
      objpath [MPRIS_OBJP_MP2]);
}

static void
mprisInitializePlayer (contdata_t *contdata)
{

  contdata->mprisplayer = mpris_media_player2_player_skeleton_new ();

  mpris_media_player2_player_set_playback_status (contdata->mprisplayer,
      playstatusstr [MPRIS_STATUS_STOP]);
  mpris_media_player2_player_set_loop_status (contdata->mprisplayer,
      repeatstr [MPRIS_REPEAT_NONE]);
  mpris_media_player2_player_set_rate (contdata->mprisplayer, 0.0);
  mpris_media_player2_player_set_volume (contdata->mprisplayer, true);
  mpris_media_player2_player_set_position (contdata->mprisplayer, 0);
  mpris_media_player2_player_set_minimum_rate (contdata->mprisplayer, 0.7);
  mpris_media_player2_player_set_maximum_rate (contdata->mprisplayer, 1.3);
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
  g_signal_connect (contdata->mprisplayer, "handle-playpause",
      G_CALLBACK (mprisPlayPause), contdata);
  g_signal_connect (contdata->mprisplayer, "handle-stop",
      G_CALLBACK (mprisStop), contdata);
//  g_signal_connect (contdata->mprisplayer, "handle-set-position", G_CALLBACK (mpris_set_position), NULL);
//  g_signal_connect (contdata->mprisplayer, "handle-seek", (GCallback) mpris_seek_position, NULL);
//  g_signal_connect (contdata->mprisplayer, "notify::volume", (GCallback) mpris_volume_changed, NULL);

  dbusSetInterfaceSkeleton (contdata->dbus, contdata->mprisplayer,
      objpath [MPRIS_OBJP_MP2]);
}

static gboolean
mprisNext (mprisMediaPlayer2Player *player,
    GDBusMethodInvocation *invocation,
    void *udata)
{
  mpris_media_player2_player_complete_next (player, invocation);
  return true;
}

static gboolean
mprisPlay (mprisMediaPlayer2Player *player,
    GDBusMethodInvocation *invocation,
    void *udata)
{
  mpris_media_player2_player_complete_next (player, invocation);
  return true;
}

static gboolean
mprisPause (mprisMediaPlayer2Player *player,
    GDBusMethodInvocation *invocation,
    void *udata)
{
  mpris_media_player2_player_complete_next (player, invocation);
  return true;
}

static gboolean
mprisPlayPause (mprisMediaPlayer2Player *player,
    GDBusMethodInvocation *invocation,
    void *udata)
{
  mpris_media_player2_player_complete_next (player, invocation);
  return true;
}

static gboolean
mprisStop (mprisMediaPlayer2Player *player,
    GDBusMethodInvocation *invocation,
    void *udata)
{
  mpris_media_player2_player_complete_next (player, invocation);
  return true;
}

#endif /* __linux__ */
