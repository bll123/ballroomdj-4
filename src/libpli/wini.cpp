/*
 * Copyright 2025 Brad Lanam Pleasant Hill CA
 *
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>
#include <string.h>

#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>

#include <winrt/Windows.Media.h>
#include <winrt/Windows.Media.Core.h>
#include <winrt/Windows.Media.Playback.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.System.h>
#include <winrt/Windows.Foundation.h>

#include "bdj4.h"
#include "audiosrc.h"
#include "bdjstring.h"
#include "log.h"
#include "mdebug.h"
#include "osutils.h"
#include "pathdisp.h"
#include "pli.h"
#include "wini.h"

using namespace winrt::Windows::Media;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Storage;
using namespace winrt::Windows;
using namespace winrt;

typedef struct windata {
  Playback::MediaPlayer           mediaPlayer;
  plistate_t                      state;
} windata_t;

ssize_t
winGetDuration (windata_t *windata)
{
logBasic ("winGetDuration\n");
  if (windata == NULL) {
    return 0;
  }

  return 0;
}

ssize_t
winGetTime (windata_t *windata)
{
  ssize_t   pos;
logBasic ("winGetTime\n");
  if (windata == NULL) {
    return 0;
  }

  auto pbSession = windata->mediaPlayer.PlaybackSession ();
// ### conversion issue
//  pos = pbSession.Position ();

  return pos;
}

int
winStop (windata_t *windata)
{
  if (windata == NULL) {
    return 0;
  }
logBasic ("\n");
  return 0;
}

int
winPause (windata_t *windata)
{
  if (windata == NULL) {
    return 0;
  }
logBasic ("winPause\n");
  auto pbSession = windata->mediaPlayer.PlaybackSession ();
  if (pbSession.CanPause ()) {
    windata->mediaPlayer.Pause ();
  }
  return 0;
}

int
winPlay (windata_t *windata)
{
logBasic ("winPlay\n");
  windata->mediaPlayer.Play ();
  return 0;
}

ssize_t
winSeek (windata_t *windata, ssize_t pos)
{
logBasic ("winSeek\n");
  if (windata == NULL) {
    return 0;
  }

  return pos;
}

double
winRate (windata_t *windata, double drate)
{
logBasic ("winRate\n");
  if (windata == NULL) {
    return 0;
  }

  return 100.0;
}

int
winMedia (windata_t *windata, const char *fn, int sourceType)
{
  char    tbuff [MAXPATHLEN];

  if (windata == NULL) {
    return 0;
  }

  stpecpy (tbuff, tbuff + sizeof (tbuff), fn);
  pathDisplayPath (tbuff, sizeof (tbuff));
logBasic ("winMedia %s\n", tbuff);
  auto wfn = osToWideChar (tbuff);
  auto hs = hstring (wfn);
  auto sfile = StorageFile::GetFileFromPathAsync (hs).get ();
  auto source = Core::MediaSource::CreateFromStorageFile (sfile);
//  windata->mediaPlayer.Source = Core::MediaSource::CreateFromUri (source);
  auto ac = windata->mediaPlayer.AudioCategory ();
  ac = Playback::MediaPlayerAudioCategory::Media;
  mdfree (wfn);
  return 0;
}

windata_t *
winInit (void)
{
  windata_t   *windata;

logBasic ("winInit\n");
  windata = (windata_t *) mdmalloc (sizeof (windata_t));
  try {
    windata->mediaPlayer = Playback::MediaPlayer ();
logBasic ("  aa\n");
  } catch (const std::exception &exc) {
logBasic ("  ng-a %ld\n", (long) GetLastError());
logBasic ("  ng-a %s\n", exc.what ());
  } catch (...) {
logBasic ("  ng-b %ld\n", (long) GetLastError());
  }
logBasic ("  bb\n");

  windata->mediaPlayer.CommandManager ().IsEnabled (false);
logBasic ("  cc\n");
  windata->state = PLI_STATE_IDLE;

  return windata;
}

void
winClose (windata_t *windata)
{
logBasic ("winClose\n");
  if (windata == NULL) {
    return;
  }

  if (windata->mediaPlayer != NULL) {
    // windata->mediaPlayer.Dispose ();
    windata->mediaPlayer = NULL;
  }

  mdfree (windata);
  return;
}

plistate_t
winState (windata_t *windata)
{
  auto  pbSession = windata->mediaPlayer.PlaybackSession ();
  auto  wstate = pbSession.PlaybackState ();

  switch (wstate) {
    case Playback::MediaPlaybackState::None: {
      windata->state = PLI_STATE_IDLE;
      break;
    }
    case Playback::MediaPlaybackState::Opening: {
      windata->state = PLI_STATE_OPENING;
      break;
    }
    case Playback::MediaPlaybackState::Buffering: {
      windata->state = PLI_STATE_OPENING;
      break;
    }
    case Playback::MediaPlaybackState::Playing: {
      windata->state = PLI_STATE_PLAYING;
      break;
    }
    case Playback::MediaPlaybackState::Paused: {
      windata->state = PLI_STATE_PAUSED;
      break;
    }
  }

  return windata->state;
}

/* internal routines */

#if 0

#if SMTC_ENABLED
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
    Uri uri{ DEFAULT_THUMBNAIL_URI };
    defaultArt = Streams::RandomAccessStreamReference::CreateFromUri (uri);

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
  Streams::RandomAccessStreamReference defaultArt;
  contdata_t *contdata;
};
#endif

contdata_t *
contiInit (const char *instname)
{
  contdata_t  *contdata;

  contdata = (contdata_t *) mdmalloc (sizeof (contdata_t));
  contdata->instname = mdstrdup (instname);
  contdata->cb = NULL;
  contdata->metadata = NULL;
  contdata->playstate = PL_STATE_STOPPED;
#if SMTC_ENABLED
  contdata->playstatus = MediaPlaybackStatus::Closed;
#endif
  contdata->pos = 0;
  contdata->rate = 100;
  contdata->volume = 0;

#if SMTC_ENABLED
  contdata->sys = new mpintfc (contdata);
#endif

  return contdata;
}

void
contiFree (contdata_t *contdata)
{
  if (contdata == NULL) {
    return;
  }

#if SMTC_ENABLED
  contdata->sys->smtcSendPlaybackStatus (MediaPlaybackStatus::Closed);
  contdata->sys->smtcMediaPlayerStop ();
  delete contdata->sys;
#endif

  nlistFree (contdata->metadata);
  dataFree (contdata->instname);
  mdfree (contdata);
}

void
contiSetup (contdata_t *contdata)
{
#if SMTC_ENABLED
  contdata->sys->smtcMediaPlayerInit ();
#endif
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
#if SMTC_ENABLED
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
#endif
}

void
contiSetRepeatState (contdata_t *contdata, bool state)
{
  if (contdata == NULL) {
    return;
  }

  /* not implemented */
}

void
contiSetPosition (contdata_t *contdata, int32_t pos)
{
  if (contdata == NULL) {
    return;
  }

  /* not implemented */

  if (contdata->pos != pos) {
    int64_t   tpos;
    int32_t   pdiff;

    tpos = pos * 1000;    // microseconds
    pdiff = pos - contdata->pos;
    if (pdiff < 0 || pdiff > 300) {
      /* the seek signal is only supposed to be sent when there is */
      /* a large change */
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

  /* not implemented */

  if (contdata->rate != rate) {
    double    dval;

    dval = (double) rate / 100.0;
  }
  contdata->rate = rate;
}

void
contiSetVolume (contdata_t *contdata, int volume)
{
  if (contdata == NULL) {
    return;
  }

  /* not implemented */

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

#if SMTC_ENABLED
  contdata->sys->smtcSendMetadata ();
  contdata->sys->smtcSetNextEnabled ();
#endif
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

#endif
