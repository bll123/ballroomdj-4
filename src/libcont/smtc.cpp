/*
 * Copyright 2024 Brad Lanam Pleasant Hill CA
 *
 * Framework from
 *  https://github.com/spmn/vlc-win10smtc/tree/master
 *    GPLv3 License
 * References:
 *  https://github.com/AlienCowEatCake/audacious-win-smtc
 *    Uses msys2 to build, but the link is different than what i needed.
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
#include "player.h"
#include "nlist.h"
#include "tmutil.h"

// ### remove
#define DEFAULT_THUMBNAIL_URI L"https://upload.wikimedia.org/wikipedia/commons/3/38/VLC_icon.png"

using namespace winrt::Windows::Media;

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
    logBasic ("constructor\n");
  }

  void
  smtcMediaPlayerInit()
  {
    logBasic ("start-init\n");
//    winrt::init_apartment();

    logBasic ("  init-b\n");
    mediaPlayer = Playback::MediaPlayer ();
    mediaPlayer.CommandManager ().IsEnabled (false);
    logBasic ("  init-c\n");

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

    logBasic ("  init-d\n");

    SMTC ().IsPlayEnabled (false);
    SMTC ().IsPauseEnabled (false);
    SMTC ().IsStopEnabled (false);
    SMTC ().IsPreviousEnabled (false);
    SMTC ().IsNextEnabled (false);

    SMTC ().PlaybackStatus (MediaPlaybackStatus::Closed);
    SMTC ().IsEnabled (true);

    logBasic ("  init-e\n");

    winrt::Windows::Foundation::Uri uri{ DEFAULT_THUMBNAIL_URI };
    defaultArt = winrt::Windows::Storage::Streams::RandomAccessStreamReference::CreateFromUri (uri);

    logBasic ("  init-f\n");
    smtcUpdater ().Thumbnail (defaultArt);
    smtcUpdater ().Type (MediaPlaybackType::Music);
    smtcUpdater ().Update ();
    logBasic ("  init-g\n");
  }

  void
  smtcMediaPlayerStop (void)
  {
    logBasic ("mp-stop\n");
    mediaPlayer = Playback::MediaPlayer (nullptr);
//    winrt::uninit_apartment ();
  }

  void
  smtcSendPlaybackStatus (MediaPlaybackStatus nstate)
  {
    logBasic ("mp-send-pb-status\n");
    SMTC ().PlaybackStatus (nstate);
    smtcUpdater ().Update ();
  }

  void
  smtcSetPlay (bool val)
  {
    logBasic ("mp-set-play\n");
    SMTC ().IsPlayEnabled (val);
    smtcUpdater ().Update ();
  }

  void
  smtcSetPause (bool val)
  {
    logBasic ("mp-set-pause\n");
    SMTC ().IsPauseEnabled (val);
    smtcUpdater ().Update ();
  }

  void
  smtcSetNextEnabled (void)
  {
    logBasic ("mp-set-next\n");
    SMTC ().IsNextEnabled (true);
    smtcUpdater ().Update ();
  }

  void
  smtcSendMetadata (void)
  {
    winrt::hstring tstr;

    logBasic ("mp-send-metadata\n");
    auto to_hstring = [] (const char * buf, winrt::hstring def) {
      winrt::hstring ret;

      if (buf) {
        ret = winrt::to_hstring (buf);
      } else {
        ret = def;
      }

      return ret;
    };

    tstr = to_hstring (nlistGetStr (contdata->metadata, CONT_METADATA_TITLE), L"Unknown Title");
    smtcUpdater ().MusicProperties ().Title (tstr);
    tstr = to_hstring (nlistGetStr (contdata->metadata, CONT_METADATA_ARTIST), L"Unknown Artist");
    smtcUpdater ().MusicProperties ().Artist (tstr);
    tstr = to_hstring (nlistGetStr (contdata->metadata, CONT_METADATA_ALBUM), L"Unknown Album");
    smtcUpdater ().MusicProperties ().AlbumTitle (tstr);
    tstr = to_hstring (nlistGetStr (contdata->metadata, CONT_METADATA_ALBUMARTIST), L"Unknown Album Artist");
    smtcUpdater ().MusicProperties ().AlbumArtist (tstr);
//    tstr = to_hstring (nlistGetStr (contdata->metadata, CONT_METADATA_GENRE), L"Unknown Genre");
//    smtcUpdater ().MusicProperties ().Genres (tstr);

    // TODO: use artwork provided by ID3tag (if exists)
    smtcUpdater ().Thumbnail (defaultArt);

    smtcUpdater ().Update ();
  }

  SystemMediaTransportControls
  SMTC () {
    return mediaPlayer.SystemMediaTransportControls ();
  }

  SystemMediaTransportControlsDisplayUpdater
  smtcUpdater () {
    return SMTC().DisplayUpdater ();
  }

  Playback::MediaPlayer mediaPlayer;
  winrt::Windows::Storage::Streams::RandomAccessStreamReference defaultArt;
  contdata_t *contdata;
};

contdata_t *
contiInit (const char *instname)
{
  contdata_t  *contdata;

  logBasic ("c-init\n");
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
  logBasic ("c-init-fin\n");

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
  logBasic ("c-setup\n");
  contdata->sys->smtcMediaPlayerInit ();
  logBasic ("c-setup-fin\n");
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

  logBasic ("c-set-play-state\n");
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

  logBasic ("c-set-repeat-state\n");
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

  logBasic ("c-set-rate\n");
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

  logBasic ("c-set-current\n");
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
  logBasic ("  curr-b\n");

  contdata->sys->smtcSendMetadata ();
  contdata->sys->smtcSetNextEnabled ();
  logBasic ("  curr-c\n");
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

