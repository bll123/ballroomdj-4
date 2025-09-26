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
 *
 *
 * Rewind and Position appear to do nothing, so don't bother with them.
 *
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
      defaultImage { nullptr },
      contdata { contdata }
  {
  }

  void
  smtcSetDefaultImage (void)
  {
    smtcUpdater.Thumbnail (defaultImage);
  }

  void
  smtcSetImage (const char *fn)
  {
    auto hsfn = winrt::to_hstring (fn);
    auto sfile = StorageFile::GetFileFromPathAsync (hsfn).get ();
    if (sfile == nullptr) {
      smtcSetDefaultImage ();
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
      smtcSetDefaultImage ();
      return;
    }

    auto istream = thumb.CloneStream ();
    auto art = Streams::RandomAccessStreamReference::CreateFromStream (istream);
    if (art == nullptr) {
      smtcSetDefaultImage ();
      return;
    }
    smtcUpdater.Thumbnail (art);
  }

  void
  smtcSetImageUri (const char *fn)
  {
    auto hsfn = winrt::to_hstring (fn);
    auto uri = Uri (hsfn);
    auto image = Streams::RandomAccessStreamReference::CreateFromUri (uri);
    if (image == nullptr) {
      smtcSetDefaultImage ();
      return;
    }
    smtcUpdater.Thumbnail (image);
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
          case SystemMediaTransportControlsButton::Rewind:
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
    SMTC.IsStopEnabled (true);
    SMTC.IsPreviousEnabled (false);
    SMTC.IsNextEnabled (true);

    SMTC.PlaybackStatus (MediaPlaybackStatus::Closed);
    SMTC.IsEnabled (true);

    smtcUpdater.Type (MediaPlaybackType::Music);

    pathbldMakePath (tbuff, sizeof (tbuff),
        "bdj4_icon", BDJ4_IMG_PNG_EXT, PATHBLD_MP_DIR_IMG);
    pathDisplayPath (tbuff, sizeof (tbuff));
    auto hsfn = winrt::to_hstring (tbuff);
    auto sfile = StorageFile::GetFileFromPathAsync (hsfn).get ();
    defaultImage = Streams::RandomAccessStreamReference::CreateFromFile (sfile);
    smtcSetDefaultImage ();

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
  }

  void
  smtcSetPlay (bool val)
  {
    SMTC.IsPlayEnabled (val);
  }

  void
  smtcSetPause (bool val)
  {
    SMTC.IsPauseEnabled (val);
  }

  void
  smtcSetNextEnabled (void)
  {
    SMTC.IsNextEnabled (true);
  }

  void
  smtcSendMetadata (int astype)
  {
    const char      *tmp;
    winrt::hstring  tstr;

    tmp = nlistGetStr (contdata->metadata, CONT_METADATA_TITLE);
    if (tmp != NULL) {
      tstr = winrt::to_hstring (tmp);
      smtcUpdater.MusicProperties ().Title (tstr);
    }
    tmp = nlistGetStr (contdata->metadata, CONT_METADATA_ARTIST);
    if (tmp != NULL) {
      tstr = winrt::to_hstring (tmp);
      smtcUpdater.MusicProperties ().Artist (tstr);
    }
    tmp = nlistGetStr (contdata->metadata, CONT_METADATA_ALBUM);
    if (tmp != NULL) {
      tstr = winrt::to_hstring (tmp);
      smtcUpdater.MusicProperties ().AlbumTitle (tstr);
    }
    tmp = nlistGetStr (contdata->metadata, CONT_METADATA_ALBUMARTIST);
    if (tmp != NULL) {
      tstr = winrt::to_hstring (tmp);
      smtcUpdater.MusicProperties ().AlbumArtist (tstr);
    }

    /* by preference, use any image-uri set for the song */
    tmp = nlistGetStr (contdata->metadata, CONT_METADATA_IMAGE_URI);
    if (tmp != NULL && *tmp) {
      smtcSetImageUri (tmp);
    }
    if (tmp == NULL) {
      tmp = nlistGetStr (contdata->metadata, CONT_METADATA_URI);
      if (tmp != NULL &&
          astype == AUDIOSRC_TYPE_FILE &&
          strlen (tmp) > AS_FILE_PFX_LEN) {
        char    tbuff [MAXPATHLEN];

        stpecpy (tbuff, tbuff + sizeof (tbuff), tmp + AS_FILE_PFX_LEN);
        pathDisplayPath (tbuff, sizeof (tbuff));
        smtcSetImage (tbuff);
      }
    }

    smtcUpdater.Update ();
  }

  Playback::MediaPlayer                       mediaPlayer;
  SystemMediaTransportControls                SMTC;
  SystemMediaTransportControlsDisplayUpdater  smtcUpdater;
  Streams::RandomAccessStreamReference        defaultImage;
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
    case PL_STATE_IN_CROSSFADE:
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

  contdata->mpsmtc->smtcSetPlay (canplay);
  contdata->mpsmtc->smtcSetPause (canpause);

  if (contdata->playstatus != nstate) {
    contdata->mpsmtc->smtcSendPlaybackStatus (nstate);
    contdata->playstatus = nstate;
  }
#endif
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
  if (cmetadata->imageuri != NULL && *cmetadata->imageuri) {
    nlistSetStr (contdata->metadata, CONT_METADATA_IMAGE_URI, cmetadata->imageuri);
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

