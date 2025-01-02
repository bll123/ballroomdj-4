/*
 * Copyright 2024-2025 Brad Lanam Pleasant Hill CA
 *
 * Framework from
 *  https://github.com/spmn/vlc-win10smtc/tree/master
 *    GPLv3 License
 * References:
 *  https://github.com/AlienCowEatCake/audacious-win-smtc
 *    Uses msys2 to build, but the link is different than what i needed.
 *  2024-10-29 found more references to general smtc code (not winrt)
 *  https://github.com/bolucat/Firefox/blob/dc6f9cd8a7a1ce2e3abf7ba52b53b6b7fa009a9a/widget/windows/WindowsSMTCProvider.cpp
 *  https://github.com/chromium/chromium/blob/f32b9239d52625aba333cc23a03f98d0d532e9df/components/system_media_controls/win/system_media_controls_win.cc
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>
#include <string.h>

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Media.h>
#include <winrt/Windows.Media.Playback.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Storage.Streams.h>

#include "bdj4.h"
#include "bdj4ui.h"     // for speed constants
#include "callback.h"
#include "controller.h"
#include "log.h"
#include "mdebug.h"
#include "pathutil.h"
#include "player.h"
#include "nlist.h"
#include "tmutil.h"

#define DEFAULT_THUMBNAIL_URI L"https://ballroomdj4.sourceforge.io/img/bdj4_icon.png"

using namespace winrt::Windows::Media;
using namespace winrt::Windows;

typedef struct mpintfc mpintfc_t;

typedef struct contdata {
  char                *instname;
  callback_t          *cb;
  callback_t          *cburi;
  nlist_t             *metadata;
  int                 playstate;      // BDJ4 play state
  MediaPlaybackStatus playstatus;     // Windows playback status
  int32_t             pos;
  int                 rate;
  int                 volume;
  mpintfc_t          *sys;
} contdata_t;

void smtcNext (contdata_t *contdata);
void smtcPlay (contdata_t *contdata);
void smtcPause (contdata_t *contdata);
void smtcStop (contdata_t *contdata);

void
contiDesc (const char **ret, int max)
{
  int         c = 0;

  if (max < 2) {
    return;
  }

  ret [c++] = "Windows SMTC";
  ret [c++] = NULL;
}

struct mpintfc
{
  mpintfc (const mpintfc&) = delete;
  void operator= (const mpintfc&) = delete;

  explicit mpintfc (contdata_t *contdata) :
      mediaPlayer { nullptr },
      defaultArt { nullptr },
      contdata { contdata }
  {
  }

  void
  smtcMediaPlayerInit (void)
  {
    winrt::hstring  tstr;

    /* initializing the 'apartment' causes a crash */

    mediaPlayer = Playback::MediaPlayer ();
    mediaPlayer.CommandManager ().IsEnabled (false);

    SMTC().ButtonPressed (
        [this] (SystemMediaTransportControls sender,
        SystemMediaTransportControlsButtonPressedEventArgs args) {

        switch (args.Button()) {
          case SystemMediaTransportControlsButton::Play: {
            smtcPlay (contdata);
            break;
          }
          case SystemMediaTransportControlsButton::Pause: {
            smtcPause (contdata);
            break;
          }
          case SystemMediaTransportControlsButton::Stop: {
            smtcStop (contdata);
            break;
          }
          case SystemMediaTransportControlsButton::Next: {
            smtcNext (contdata);
            break;
          }
          case SystemMediaTransportControlsButton::Previous: {
            /* not supported */
            break;
          }
          case SystemMediaTransportControlsButton::ChannelDown:
          case SystemMediaTransportControlsButton::ChannelUp:
          case SystemMediaTransportControlsButton::FastForward:
          case SystemMediaTransportControlsButton::Rewind:
          case SystemMediaTransportControlsButton::Record: {
            break;
          }
        } /* switch on button type */
      } /* button-pressed */
    );  /* button-pressed def */

    SMTC ().IsPlayEnabled (false);
    SMTC ().IsPauseEnabled (false);
    SMTC ().IsStopEnabled (false);
    SMTC ().IsPreviousEnabled (false);
    SMTC ().IsNextEnabled (false);

    SMTC ().PlaybackStatus (MediaPlaybackStatus::Closed);
    SMTC ().IsEnabled (true);

    /* why is it so difficult to open a file? */
    /* create-from-uri also cannot handle the file:// protocol */
    winrt::Windows::Foundation::Uri uri{ DEFAULT_THUMBNAIL_URI };
    defaultArt = Storage::Streams::RandomAccessStreamReference::CreateFromUri (uri);

    smtcUpdater ().Thumbnail (defaultArt);
    smtcUpdater ().Type (MediaPlaybackType::Music);
    smtcUpdater ().Update ();
  }

  void
  smtcMediaPlayerStop (void)
  {
    mediaPlayer = Playback::MediaPlayer (nullptr);
  }

  void
  smtcSendPlaybackStatus (MediaPlaybackStatus nstate)
  {
    SMTC ().PlaybackStatus (nstate);
    smtcUpdater ().Update ();
  }

  void
  smtcSetPlay (bool val)
  {
    SMTC ().IsPlayEnabled (val);
    smtcUpdater ().Update ();
  }

  void
  smtcSetPause (bool val)
  {
    SMTC ().IsPauseEnabled (val);
    smtcUpdater ().Update ();
  }

  void
  smtcSetNextEnabled (void)
  {
    SMTC ().IsNextEnabled (true);
    smtcUpdater ().Update ();
  }

  void
  smtcSendMetadata (void)
  {
    winrt::hstring tstr;


    tstr = winrt::to_hstring (nlistGetStr (contdata->metadata, CONT_METADATA_TITLE));
    smtcUpdater ().MusicProperties ().Title (tstr);
    tstr = winrt::to_hstring (nlistGetStr (contdata->metadata, CONT_METADATA_ARTIST));
    smtcUpdater ().MusicProperties ().Artist (tstr);
    tstr = winrt::to_hstring (nlistGetStr (contdata->metadata, CONT_METADATA_ALBUM));
    smtcUpdater ().MusicProperties ().AlbumTitle (tstr);
    tstr = winrt::to_hstring (nlistGetStr (contdata->metadata, CONT_METADATA_ALBUMARTIST));
    smtcUpdater ().MusicProperties ().AlbumArtist (tstr);
//    tstr = winrt::to_hstring (nlistGetStr (contdata->metadata, CONT_METADATA_GENRE));
//    smtcUpdater ().MusicProperties ().Genres (tstr);

    // TODO: artwork
    smtcUpdater ().Thumbnail (defaultArt);

    smtcUpdater ().Update ();
  }

  SystemMediaTransportControls
  SMTC () {
    return mediaPlayer.SystemMediaTransportControls ();
  }

  SystemMediaTransportControlsDisplayUpdater
  smtcUpdater () {
    /* this is actually the older method */
    return SMTC().DisplayUpdater ();
  }

  Playback::MediaPlayer mediaPlayer;
  Storage::Streams::RandomAccessStreamReference defaultArt;
  contdata_t *contdata;
};

contdata_t *
contiInit (const char *instname)
{
  contdata_t  *contdata;

  contdata = (contdata_t *) mdmalloc (sizeof (contdata_t));
  contdata->instname = mdstrdup (instname);
  contdata->cb = NULL;
  contdata->metadata = NULL;
  contdata->playstate = PL_STATE_STOPPED;
  contdata->playstatus = MediaPlaybackStatus::Closed;
  contdata->pos = 0;
  contdata->rate = 100;
  contdata->volume = 0;

  contdata->sys = new mpintfc (contdata);

  return contdata;
}

void
contiFree (contdata_t *contdata)
{
  if (contdata == NULL) {
    return;
  }

  contdata->sys->smtcSendPlaybackStatus (MediaPlaybackStatus::Closed);
  contdata->sys->smtcMediaPlayerStop ();
  delete contdata->sys;

  nlistFree (contdata->metadata);
  dataFree (contdata->instname);
  mdfree (contdata);
}

void
contiSetup (contdata_t *contdata)
{
  contdata->sys->smtcMediaPlayerInit ();
  return;
}

bool
contiCheckReady (contdata_t *contdata)
{
  bool    rc = false;

  rc = true;
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
  MediaPlaybackStatus nstate = contdata->playstatus;
  bool                canplay = false;
  bool                canpause = false;
  bool                canseek = false;

  if (contdata == NULL) {
    return;
  }

  contdata->playstate = state;

  switch (state) {
    case PL_STATE_LOADING:
    case PL_STATE_PLAYING: {
      nstate = MediaPlaybackStatus::Playing;
      canplay = false;
      canpause = true;
      canseek = true;
      break;
    }
    case PL_STATE_IN_FADEOUT: {
      nstate = MediaPlaybackStatus::Playing;
      canplay = false;
      canpause = false;
      canseek = false;
      break;
    }
    case PL_STATE_PAUSED: {
      nstate = MediaPlaybackStatus::Paused;
      canplay = true;
      canpause = false;
      canseek = true;
      break;
    }
    case PL_STATE_IN_GAP: {
      nstate = MediaPlaybackStatus::Changing;
      canplay = false;
      canpause = false;
      canseek = false;
      break;
    }
    case PL_STATE_UNKNOWN:
    case PL_STATE_STOPPED: {
      nstate = MediaPlaybackStatus::Stopped;
      canplay = true;
      canpause = false;
      canseek = false;
      break;
    }
  }

  contdata->sys->smtcSetPlay (canplay);
  contdata->sys->smtcSetPause (canpause);

  if (contdata->playstatus != nstate) {
    contdata->sys->smtcSendPlaybackStatus (nstate);
    contdata->playstatus = nstate;
  }
}

void
contiSetRepeatState (contdata_t *contdata, bool state)
{
//  int   nstate = 0;

  if (contdata == NULL) {
    return;
  }

//  if (contdata->repeatstatus != nstate) {
//    mpris_media_player2_player_set_loop_status (contdata->mprisplayer,
//        repeatstr [nstate]);
//    contdata->repeatstatus = nstate;
//  }
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
//    mpris_media_player2_player_set_position (contdata->mprisplayer, tpos);
    if (pdiff < 0 || pdiff > 300) {
      /* the seek signal is only supposed to be sent when there is */
      /* a large change */
//      mpris_media_player2_player_emit_seeked (contdata->mprisplayer, tpos);
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
//    mpris_media_player2_player_set_rate (contdata->mprisplayer, dval);
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
  }
  contdata->volume = volume;
}

void
contiSetCurrent (contdata_t *contdata, contmetadata_t *cmetadata)
{
  char        tbuff [200];

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

  nlistSetNum (contdata->metadata, CONT_METADATA_SONGSTART, cmetadata->songstart);
  nlistSetNum (contdata->metadata, CONT_METADATA_SONGEND, cmetadata->songend);
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

  contdata->sys->smtcSendMetadata ();
  contdata->sys->smtcSetNextEnabled ();
}

void
smtcNext (contdata_t *contdata)
{
  if (contdata->cb != NULL) {
    callbackHandlerII (contdata->cb, CONTROLLER_NEXT, 0);
  }
}

void
smtcPlay (contdata_t *contdata)
{
  if (contdata->cb != NULL) {
    callbackHandlerII (contdata->cb, CONTROLLER_PLAY, 0);
  }
}

void
smtcPause (contdata_t *contdata)
{
  if (contdata->cb != NULL) {
    callbackHandlerII (contdata->cb, CONTROLLER_PAUSE, 0);
  }
}

void
smtcStop (contdata_t *contdata)
{
  if (contdata->cb != NULL) {
    callbackHandlerII (contdata->cb, CONTROLLER_STOP, 0);
  }
}

