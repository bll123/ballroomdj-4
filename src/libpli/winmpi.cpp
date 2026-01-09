/*
 * Copyright 2025-2026 Brad Lanam Pleasant Hill CA
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
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Devices.Enumeration.h>

#include "bdj4.h"
#include "audiosrc.h"
#include "bdjstring.h"
#include "log.h"
#include "mdebug.h"
#include "osutils.h"
#include "pathdisp.h"
#include "pli.h"
#include "winmpi.h"

using namespace winrt::Windows::Media;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Storage;
using namespace winrt::Windows::Devices::Enumeration;
using namespace winrt::Windows;
using namespace winrt;

typedef struct winmpintfc winmpintfc_t;

typedef struct windata {
  winmpintfc_t  *winmp [PLI_MAX_SOURCE];
  double        vol [PLI_MAX_SOURCE];
  char          *audiodev;
  size_t        audiodevlen;
  int           curr;
  plistate_t    state;
  bool          inCrossFade;
} windata_t;

struct winmpintfc
{
  winmpintfc (const winmpintfc&) = delete;
  void operator= (const winmpintfc&) = delete;

  explicit winmpintfc (windata_t *windata) :
      mediaPlayer { nullptr },
      windata { windata }
  {
  }

  void
  mpPause (void)
  {
    if (mediaPlayer == nullptr) {
      return;
    }
    auto pbSession = mediaPlayer.PlaybackSession ();
    if (pbSession.CanPause ()) {
      mediaPlayer.Pause ();
    }
  }

  void
  mpPlay (void)
  {
    if (mediaPlayer == nullptr) {
      return;
    }
    try {
      mediaPlayer.Play ();
    } catch (std::exception &exc) {
      logMsg (LOG_DBG, LOG_IMPORTANT, "winmp-play fail %s", exc.what ());
    }
  }

  void
  mpStop (void)
  {
    if (mediaPlayer == nullptr) {
      return;
    }
    mpPause ();
    mediaPlayer.Source (nullptr);
    mediaPlayer = Playback::MediaPlayer (nullptr);
  }

  ssize_t
  mpSeek (ssize_t pos)
  {
    TimeSpan    ts;

    if (mediaPlayer == nullptr) {
      return 0;
    }

    auto pbSession = mediaPlayer.PlaybackSession ();

    if (mediaPlayer.CanSeek ()) {
      ts = std::chrono::milliseconds (pos);
      pbSession.Position (ts);
    }
    ts = pbSession.Position ();
    auto ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(ts).count ();
    return ms;
  }

  void
  mpInitPlayer (void)
  {
    /* create a new media player for each playback item */
    /* in order to allow cross-fading */
    mediaPlayer = Playback::MediaPlayer ();
    mediaPlayer.CommandManager ().IsEnabled (false);

    if (windata->audiodev != NULL) {
      mpSetAudioDevice (windata->audiodev);
    }
  }

  void
  mpMedia (const hstring &hsfn)
  {
    StorageFile   sfile = nullptr;

    mpInitPlayer ();

    try {
      sfile = StorageFile::GetFileFromPathAsync (hsfn).get ();
    } catch (std::exception &exc) {
      logMsg (LOG_DBG, LOG_IMPORTANT, "winmp-media open-fail %s", exc.what ());
      return;
    }
    if (sfile == nullptr) {
      logMsg (LOG_DBG, LOG_IMPORTANT, "winmp-media file-null");
      return;
    }
    auto source = Core::MediaSource::CreateFromStorageFile (sfile);
    if (source == nullptr) {
      logMsg (LOG_DBG, LOG_IMPORTANT, "winmp-media source-null");
      return;
    }
    try {
      mediaPlayer.Source (source);
    } catch (std::exception &exc) {
      logMsg (LOG_DBG, LOG_IMPORTANT, "winmp-media source fail");
      return;
    }
    auto ac = mediaPlayer.AudioCategory ();
    ac = Playback::MediaPlayerAudioCategory::Media;
  }

  void
  mpURI (const hstring &hsfn)
  {
    mpInitPlayer ();

    auto uri = Uri (hsfn);
    if (uri == nullptr) {
      return;
    }
    auto source = Core::MediaSource::CreateFromUri (uri);
    if (source == nullptr) {
      logMsg (LOG_DBG, LOG_IMPORTANT, "winmp-uri create-fail");
      return;
    }
    try {
      mediaPlayer.Source (source);
    } catch (std::exception &exc) {
      logMsg (LOG_DBG, LOG_IMPORTANT, "winmp-uri source-fail %s", exc.what ());
      return;
    }
    auto ac = mediaPlayer.AudioCategory ();
    ac = Playback::MediaPlayerAudioCategory::Media;
  }

  void
  mpClose (void)
  {
    if (mediaPlayer != nullptr) {
      mpStop ();
    }
    return;
  }

  ssize_t
  mpDuration (void)
  {
    ssize_t   dur;

    if (mediaPlayer == nullptr) {
      return 0;
    }

    auto pbSession = mediaPlayer.PlaybackSession ();
    auto sessdur = pbSession.NaturalDuration ();
    /* timespan in winrt is a std::chrono::duration */
    /* the count is in 100ns intervals */
    /* I think there may be an easier way to do this, but */
    /* my c++ knowledge is minimal */
    auto ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(sessdur).count ();
    dur = ms;
    return dur;
  }

  ssize_t
  mpTime (void)
  {
    ssize_t   pos;

    if (mediaPlayer == nullptr) {
      return 0;
    }

    auto pbSession = mediaPlayer.PlaybackSession ();
    auto sesspos = pbSession.Position ();
    auto ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(sesspos).count ();
    pos = ms;
    return pos;
  }

  double
  mpRate (double rate)
  {
    if (mediaPlayer == nullptr) {
      return 1.0;
    }

    auto pbSession = mediaPlayer.PlaybackSession ();
    pbSession.PlaybackRate (rate);
    rate = pbSession.PlaybackRate ();
    return rate;
  }

  double
  mpVolume (double dvol)
  {
    if (mediaPlayer == nullptr) {
      return dvol;
    }

    mediaPlayer.Volume (dvol);
    return dvol;
  }

  int
  mpGetVolume (void)
  {
    double    dvol;
    int       vol;

    if (mediaPlayer == nullptr) {
      return 0;
    }

    dvol = mediaPlayer.Volume ();
    vol = dvol * 100.0;
    return vol;
  }

  DeviceInformation
  mpLocateAudioDevice (const char *audiodev)
  {
    /* .createfromidasync() crashes */
    /* instead, loop through the devices */
    /* and locate a matching device */
    /* the device names are prefixed with other text */
    auto devices = DeviceInformation::FindAllAsync (
        DeviceClass::AudioRender).get ();
    for (auto&& tdev : devices) {
      char          *tmp;
      const wchar_t *val;
      size_t        len;

      /* returns a wide string as char * */
      val = tdev.Properties().TryLookup (L"System.Devices.DeviceInstanceId")
            .try_as<winrt::hstring>().value_or (L"None").c_str();
      tmp = osFromWideChar (val);
      len = strlen (tmp);
      if (len >= windata->audiodevlen &&
          strcmp (tmp + (len - windata->audiodevlen), audiodev) == 0) {
        mdfree (tmp);
        /* valid in C++, not in C */
        return tdev;
      }
      mdfree (tmp);
    }

    return nullptr;
  }

  int
  mpSetAudioDevice (const char *audiodev)
  {
    int                 rc = -1;

    if (mediaPlayer == nullptr) {
      return rc;
    }

    auto dev = mpLocateAudioDevice (audiodev);

    if (dev == nullptr) {
      logMsg (LOG_DBG, LOG_IMPORTANT, "winmp-sad: Unable to locate audio device: %s", audiodev);
      return rc;
    }

    try {
      mediaPlayer.AudioDevice (dev);
      rc = 0;
    } catch (std::exception &exc) {
      logMsg (LOG_DBG, LOG_IMPORTANT, "winmp-sad fail %s", exc.what ());
    }
    return rc;
  }

  Playback::MediaPlaybackState
  mpState (void)
  {
    if (mediaPlayer == nullptr) {
      return Playback::MediaPlaybackState::None;
    }

    auto  pbSession = mediaPlayer.PlaybackSession ();
    auto  wstate = pbSession.PlaybackState ();
    return wstate;
  }

  Playback::MediaPlayer mediaPlayer;
  windata_t *windata;
};

ssize_t
winmpGetDuration (windata_t *windata)
{
  ssize_t   dur;

  if (windata == NULL) {
    return 0;
  }
  if (windata->winmp [windata->curr] == NULL) {
    return 0;
  }

  dur = windata->winmp [windata->curr]->mpDuration ();
  return dur;
}

ssize_t
winmpGetTime (windata_t *windata)
{
  ssize_t   pos;

  if (windata == NULL) {
    return 0;
  }
  if (windata->winmp [windata->curr] == NULL) {
    return 0;
  }

  pos = windata->winmp [windata->curr]->mpTime ();
  return pos;
}

int
winmpStop (windata_t *windata)
{
  if (windata == NULL) {
    return 0;
  }
  if (windata->winmp [windata->curr] == NULL) {
    return 0;
  }

  windata->winmp [windata->curr]->mpStop ();
  return 0;
}

int
winmpPause (windata_t *windata)
{
  if (windata == NULL) {
    return 0;
  }
  if (windata->winmp [windata->curr] == NULL) {
    return 0;
  }

  windata->winmp [windata->curr]->mpPause ();
  return 0;
}

int
winmpPlay (windata_t *windata)
{
  if (windata == NULL) {
    return 0;
  }
  if (windata->winmp [windata->curr] == NULL) {
    return 0;
  }

  windata->winmp [windata->curr]->mpPlay ();
  return 0;
}

ssize_t
winmpSeek (windata_t *windata, ssize_t pos)
{
  if (windata == NULL) {
    return 0;
  }
  if (windata->winmp [windata->curr] == NULL) {
    return 0;
  }

  pos = windata->winmp [windata->curr]->mpSeek (pos);
  return pos;
}

double
winmpRate (windata_t *windata, double drate)
{
  if (windata == NULL) {
    return 100.0;
  }
  if (windata->winmp [windata->curr] == NULL) {
    return 100.0;
  }

  drate = windata->winmp [windata->curr]->mpRate (drate);
  return drate;
}

int
winmpMedia (windata_t *windata, const char *fn, int sourceType)
{
  char    tbuff [BDJ4_PATH_MAX];
  wchar_t *wfn = NULL;

  if (windata == NULL) {
    return 1;
  }

  if (sourceType == AUDIOSRC_TYPE_FILE) {
    stpecpy (tbuff, tbuff + sizeof (tbuff), fn);
    pathDisplayPath (tbuff, sizeof (tbuff));
    wfn = osToWideChar (tbuff);
    auto hsfn = hstring (wfn);
    windata->winmp [windata->curr]->mpMedia (hsfn);
  } else {
    wfn = osToWideChar (fn);
    auto hsfn = hstring (wfn);
    windata->winmp [windata->curr]->mpURI (hsfn);
  }
  mdfree (wfn);

  return 0;
}

int
winmpCrossFade (windata_t *windata, const char *fn, int sourceType)
{
  if (windata == NULL) {
    return 1;
  }

  windata->curr = (PLI_MAX_SOURCE - 1) - windata->curr;
  windata->inCrossFade = true;
  winmpMedia (windata, fn, sourceType);

  return 0;
}

windata_t *
winmpInit (void)
{
  windata_t   *windata;

  windata = (windata_t *) mdmalloc (sizeof (windata_t));
  windata->state = PLI_STATE_IDLE;
  windata->inCrossFade = false;
  windata->curr = 0;
  windata->audiodev = NULL;
  windata->audiodevlen = 0;
  for (int i = 0; i < PLI_MAX_SOURCE; ++i) {
    windata->winmp [i] = new winmpintfc (windata);
  }

  return windata;
}

void
winmpClose (windata_t *windata)
{
  if (windata == NULL) {
    return;
  }

  for (int i = 0; i < PLI_MAX_SOURCE; ++i) {
    if (windata->winmp [i] != NULL) {
      windata->winmp [i]->mpClose ();
    }
  }

  if (windata->audiodev != NULL) {
    mdfree (windata->audiodev);
    windata->audiodev = NULL;
    windata->audiodevlen = 0;
  }

  mdfree (windata);
  return;
}

plistate_t
winmpState (windata_t *windata)
{
  if (windata == NULL) {
    return PLI_STATE_IDLE;
  }
  if (windata->winmp [windata->curr] == NULL) {
    return PLI_STATE_STOPPED;
  }

  auto  wstate = windata->winmp [windata->curr]->mpState ();

  switch (wstate) {
    case Playback::MediaPlaybackState::None: {
      windata->state = PLI_STATE_STOPPED;
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

int
winmpGetVolume (windata_t *windata)
{
  int   vol;

  vol = windata->winmp [windata->curr]->mpGetVolume ();
  return vol;
}

void
winmpCrossFadeVolume (windata_t *windata, int vol)
{
  int     previdx;
  double  dvol;

  if (windata == NULL) {
    return;
  }
  if (windata->winmp [windata->curr] == NULL) {
    return;
  }
  if (windata->inCrossFade == false) {
    return;
  }

  previdx = (PLI_MAX_SOURCE - 1) - windata->curr;
  dvol = (double) vol / 100.0;
  if (dvol <= 0.0) {
    dvol = 0.0;
  }
  windata->winmp [previdx]->mpVolume (dvol);
  if (dvol <= 0.0) {
    windata->winmp [previdx]->mpStop ();
    windata->inCrossFade = false;
  }

  dvol = 1.0 - dvol;
  windata->winmp [windata->curr]->mpVolume (dvol);

  return;
}

int
winmpSetAudioDevice (windata_t *windata, const char *dev, plidev_t plidevtype)
{
  if (windata->audiodev != NULL) {
    mdfree (windata->audiodev);
    windata->audiodev = NULL;
    windata->audiodevlen = 0;
  }

  if (dev != NULL && *dev) {
    windata->audiodev = strdup (dev);
    windata->audiodevlen = strlen (dev);
  }

  return 0;
}
