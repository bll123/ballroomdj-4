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
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <string.h>

/* 4.15.0 the gcc compiler in msys2 is currently broken for cppwinrt */
/* 4.15.3.1 gcc compiler 15.1.0 has the fix */
#define SMTC_ENABLED    1

#if SMTC_ENABLED
# include <winrt/Windows.Foundation.h>
# include <winrt/Windows.Media.h>
# include <winrt/Windows.Media.Playback.h>
# include <winrt/Windows.Storage.h>
# include <winrt/Windows.Storage.Streams.h>
# include <winrt/Windows.Storage.FileProperties.h>
#endif

#include "audiosrc.h"   // for file:// prefix
#include "bdj4.h"
#include "bdj4ui.h"     // for speed constants
#include "bdjstring.h"
#include "callback.h"
#include "controller.h"
#include "log.h"
#include "mdebug.h"
#include "pathbld.h"
#include "pathdisp.h"
#include "pathutil.h"
#include "player.h"
#include "nlist.h"
#include "tmutil.h"

#if SMTC_ENABLED
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Storage;
using namespace winrt::Windows::Media;
using namespace winrt::Windows;
using namespace winrt;
#endif

typedef struct mpintfc mpintfc_t;

typedef struct contdata {
  char                *instname;
  callback_t          *cb;
  callback_t          *cburi;
  nlist_t             *metadata;
  int                 playstate;      // BDJ4 play state
#if SMTC_ENABLED
  MediaPlaybackStatus playstatus;     // Windows playback status
#endif
  int32_t             pos;
  int                 rate;
  int                 volume;
  mpintfc_t           *mpsmtc;
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

#if SMTC_ENABLED
struct mpintfc
{
  mpintfc (const mpintfc&) = delete;
  void operator= (const mpintfc&) = delete;

  explicit mpintfc (contdata_t *contdata) :
      mediaPlayer { nullptr },
      SMTC { nullptr },
      smtcUpdater { nullptr },
      defaultArt { nullptr },
      contdata { contdata }
  {
  }

  void
  smtcSetDefaultArt (void)
  {
    smtcUpdater.Thumbnail (defaultArt);
  }

  void
  smtcSetArt (const char *fn)
  {
logBasic ("set-art: %s\n", fn);
    auto hsfn = winrt::to_hstring (fn);
    auto sfile = StorageFile::GetFileFromPathAsync (hsfn).get ();
    if (sfile == nullptr) {
      smtcSetDefaultArt ();
      return;
    }
    /* if the audio file has no image, then the generic image associated */
    /* with audio files will be returned. */
    /* this is often the default player icon (e.g. vlc) */
    /* or default file type icon. */
    /* test and see if the thumbnail image is an icon, */
    /* and replace with our default image */
    auto thumb = sfile.GetThumbnailAsync (
        FileProperties::ThumbnailMode::MusicView, (uint32_t) 256).get ();
    if (thumb == nullptr ||
        thumb.Size () == 0 ||
        thumb.Type () == FileProperties::ThumbnailType::Icon) {
logBasic ("  use default-a\n");
      smtcSetDefaultArt ();
      return;
    }

logBasic ("   fn-aaa\n");
    auto istream = thumb.CloneStream ();
logBasic ("   fn-bbb\n");
    auto art = Streams::RandomAccessStreamReference::CreateFromStream (istream);
logBasic ("   fn-ccc\n");
    if (art == nullptr) {
logBasic ("  use default-b\n");
      smtcSetDefaultArt ();
      return;
    }
logBasic ("   fn-eee\n");
    smtcUpdater.Thumbnail (art);
logBasic ("   fn-fin\n");
  }

  void
  smtcSetArtUri (const char *fn)
  {
logBasic ("set-art-uri: %s\n", fn);
    auto hsfn = winrt::to_hstring (fn);
logBasic ("   uri-aaa\n");
    auto uri = Uri (hsfn);
logBasic ("   uri-bbb\n");
    auto art = Streams::RandomAccessStreamReference::CreateFromUri (uri);
logBasic ("   uri-ccc\n");
    if (art == nullptr) {
logBasic (" use default-c\n");
      smtcSetDefaultArt ();
      return;
    }
logBasic ("   uri-ddd\n");
    smtcUpdater.Thumbnail (art);
logBasic ("   uri-fin\n");
  }

  void
  smtcMediaPlayerInit (void)
  {
    char            tbuff [MAXPATHLEN];
    winrt::hstring  tstr;

    /* initializing the 'apartment' causes a crash */

    mediaPlayer = Playback::MediaPlayer ();
    mediaPlayer.CommandManager ().IsEnabled (false);
    SMTC = mediaPlayer.SystemMediaTransportControls ();
    smtcUpdater = SMTC.DisplayUpdater ();

    SMTC.ButtonPressed (
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
            /* not supported -- do not implement */
            break;
          }
          case SystemMediaTransportControlsButton::Rewind: {
            break;
          }
          case SystemMediaTransportControlsButton::ChannelDown:
          case SystemMediaTransportControlsButton::ChannelUp:
          case SystemMediaTransportControlsButton::FastForward:
          case SystemMediaTransportControlsButton::Record: {
            /* not supported */
            break;
          }
        } /* switch on button type */
      } /* button-pressed */
    );  /* button-pressed def */

    SMTC.IsPlayEnabled (false);
    SMTC.IsPauseEnabled (false);
    SMTC.IsStopEnabled (false);
    SMTC.IsPreviousEnabled (false);
    SMTC.IsNextEnabled (false);
    SMTC.IsRewindEnabled (false);

    SMTC.PlaybackStatus (MediaPlaybackStatus::Closed);
    SMTC.IsEnabled (true);

    smtcUpdater.Type (MediaPlaybackType::Music);

    pathbldMakePath (tbuff, sizeof (tbuff),
        "bdj4_icon", BDJ4_IMG_PNG_EXT, PATHBLD_MP_DIR_IMG);
    pathDisplayPath (tbuff, sizeof (tbuff));
    auto hsfn = winrt::to_hstring (tbuff);
    auto sfile = StorageFile::GetFileFromPathAsync (hsfn).get ();
    defaultArt = Streams::RandomAccessStreamReference::CreateFromFile (sfile);
    smtcSetDefaultArt ();

    smtcUpdater.Update ();
  }

  void
  smtcMediaPlayerStop (void)
  {
    mediaPlayer = Playback::MediaPlayer (nullptr);
  }

  void
  smtcSendPlaybackStatus (MediaPlaybackStatus nstate)
  {
    SMTC.PlaybackStatus (nstate);
//    smtcUpdater.Update ();
  }

  void
  smtcSetPlay (bool val)
  {
    SMTC.IsPlayEnabled (val);
//    smtcUpdater.Update ();
  }

  void
  smtcSetPause (bool val)
  {
    SMTC.IsPauseEnabled (val);
//    smtcUpdater.Update ();
  }

  void
  smtcSetRewind (bool val)
  {
    SMTC.IsRewindEnabled (val);
//    smtcUpdater.Update ();
  }

  void
  smtcSetNextEnabled (void)
  {
    SMTC.IsNextEnabled (true);
//    smtcUpdater.Update ();
  }

  void
  smtcSetRepeat (bool state)
  {
    SMTC.AutoRepeatMode (MediaPlaybackAutoRepeatMode::None);
    if (state) {
      SMTC.AutoRepeatMode (MediaPlaybackAutoRepeatMode::Track);
    }
//    smtcUpdater.Update ();
  }

  void
  smtcSendMetadata (int astype)
  {
    const char      *tmp;
    winrt::hstring  tstr;
    uint64_t    tval;
    TimeSpan    ts;

    auto timeline = SystemMediaTransportControlsTimelineProperties ();

    tval = nlistGetNum (contdata->metadata, CONT_METADATA_SONGSTART);
    ts = std::chrono::milliseconds (tval);
    timeline.StartTime (ts);
    timeline.MinSeekTime (ts);

    tval = nlistGetNum (contdata->metadata, CONT_METADATA_DURATION);
    ts = std::chrono::milliseconds (tval);
    timeline.MaxSeekTime (ts);
    timeline.EndTime (ts);

    ts = std::chrono::milliseconds (0);
    timeline.Position (ts);

    // Update the System Media transport Controls
    SMTC.UpdateTimelineProperties (timeline);

    tstr = winrt::to_hstring (nlistGetStr (contdata->metadata, CONT_METADATA_TITLE));
    smtcUpdater.MusicProperties ().Title (tstr);
    tstr = winrt::to_hstring (nlistGetStr (contdata->metadata, CONT_METADATA_ARTIST));
    smtcUpdater.MusicProperties ().Artist (tstr);
    tstr = winrt::to_hstring (nlistGetStr (contdata->metadata, CONT_METADATA_ALBUM));
    smtcUpdater.MusicProperties ().AlbumTitle (tstr);
    tstr = winrt::to_hstring (nlistGetStr (contdata->metadata, CONT_METADATA_ALBUMARTIST));
    smtcUpdater.MusicProperties ().AlbumArtist (tstr);

    tmp = nlistGetStr (contdata->metadata, CONT_METADATA_ART_URI);
    if (tmp == NULL) {
      tmp = nlistGetStr (contdata->metadata, CONT_METADATA_URI);
    }
logBasic ("smtc: type: %d uri: %s\n", astype, tmp);
    if (tmp != NULL &&
        astype != AUDIOSRC_TYPE_FILE) {
      char    tbuff [MAXPATHLEN];

      stpecpy (tbuff, tbuff + sizeof (tbuff), tmp);
      smtcSetArtUri (tbuff);
    }
    if (tmp != NULL &&
        astype == AUDIOSRC_TYPE_FILE &&
        strlen (tmp) > AS_FILE_PFX_LEN) {
      char    tbuff [MAXPATHLEN];

      stpecpy (tbuff, tbuff + sizeof (tbuff), tmp + AS_FILE_PFX_LEN);
      pathDisplayPath (tbuff, sizeof (tbuff));
      smtcSetArt (tbuff);
    }

    smtcUpdater.Update ();
  }

//  SystemMediaTransportControls
//  SMTC {
//    return mediaPlayer.SystemMediaTransportControls ();
//  }

//  SystemMediaTransportControlsDisplayUpdater
//  smtcUpdater () {
//    /* this is actually the older method */
//    return SMTC().DisplayUpdater ();
//  }

  Playback::MediaPlayer                       mediaPlayer;
  SystemMediaTransportControls                SMTC;
  SystemMediaTransportControlsDisplayUpdater  smtcUpdater;
  Streams::RandomAccessStreamReference        defaultArt;
  contdata_t                                  *contdata;
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
  contdata->mpsmtc = new mpintfc (contdata);
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
  contdata->mpsmtc->smtcSendPlaybackStatus (MediaPlaybackStatus::Closed);
  contdata->mpsmtc->smtcMediaPlayerStop ();
  delete contdata->mpsmtc;
#endif

  nlistFree (contdata->metadata);
  dataFree (contdata->instname);
  mdfree (contdata);
}

void
contiSetup (contdata_t *contdata)
{
#if SMTC_ENABLED
  contdata->mpsmtc->smtcMediaPlayerInit ();
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
  bool                canrewind = false;

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
      canrewind = true;
      break;
    }
    case PL_STATE_IN_FADEOUT: {
      nstate = MediaPlaybackStatus::Playing;
      canplay = false;
      canpause = false;
      canseek = false;
      canrewind = false;
      break;
    }
    case PL_STATE_PAUSED: {
      nstate = MediaPlaybackStatus::Paused;
      canplay = true;
      canpause = false;
      canseek = true;
      canrewind = true;
      break;
    }
    case PL_STATE_IN_CROSSFADE:
    case PL_STATE_IN_GAP: {
      nstate = MediaPlaybackStatus::Changing;
      canplay = false;
      canpause = false;
      canseek = false;
      canrewind = false;
      break;
    }
    case PL_STATE_UNKNOWN:
    case PL_STATE_STOPPED: {
      nstate = MediaPlaybackStatus::Stopped;
      canplay = true;
      canpause = false;
      canseek = false;
      canrewind = false;
      break;
    }
  }

  contdata->mpsmtc->smtcSetPlay (canplay);
  contdata->mpsmtc->smtcSetPause (canpause);
  contdata->mpsmtc->smtcSetRewind (canrewind);

  if (contdata->playstatus != nstate) {
    contdata->mpsmtc->smtcSendPlaybackStatus (nstate);
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

  contdata->mpsmtc->smtcSetRepeat (state);
}

void
contiSetPosition (contdata_t *contdata, int32_t pos)
{
  if (contdata == NULL) {
    return;
  }

  if (contdata->pos != pos) {
    int32_t   pdiff;

    pdiff = pos - contdata->pos;
    if (pdiff < 0 || pdiff > 300) {
      /* the seek signal is only supposed to be sent when there is */
      /* a large change */
// ### to-do set the position
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
  contdata->mpsmtc->smtcSendMetadata (cmetadata->astype);
  contdata->mpsmtc->smtcSetNextEnabled ();
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

