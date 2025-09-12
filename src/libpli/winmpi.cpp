/*
 * Copyright 2025 Brad Lanam Pleasant Hill CA
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
#include "winmpi.h"

using namespace winrt::Windows::Media;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Storage;
using namespace winrt::Windows;
using namespace winrt;

enum {
  MP_MAX_SOURCE = 2,
};

typedef struct winmpintfc winmpintfc_t;

typedef struct windata {
  winmpintfc_t  *winmp [MP_MAX_SOURCE];
  double        vol [MP_MAX_SOURCE];
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
  mpMedia (const hstring &hsfn)
  {
    StorageFile   sfile = nullptr;

    /* create a new media player for each playback item */
    /* in order to allow cross-fading */
    mediaPlayer = Playback::MediaPlayer ();
    mediaPlayer.CommandManager ().IsEnabled (false);

    try {
      sfile = StorageFile::GetFileFromPathAsync (hsfn).get ();
    } catch (std::exception &exc) {
      logMsg (LOG_DBG, LOG_IMPORTANT, "win-media open-fail %s", exc.what ());
      return;
    }
    if (sfile == NULL) {
      logMsg (LOG_DBG, LOG_IMPORTANT, "win-media fail-no-file");
      return;
    }
    auto source = Core::MediaSource::CreateFromStorageFile (sfile);
    if (source == NULL) {
      logMsg (LOG_DBG, LOG_IMPORTANT, "win-media source-fail");
      return;
    }
    try {
      mediaPlayer.Source (source);
    } catch (std::exception &exc) {
      logMsg (LOG_DBG, LOG_IMPORTANT, "win-source source-fail");
      return;
    }
    auto ac = mediaPlayer.AudioCategory ();
    ac = Playback::MediaPlayerAudioCategory::Media;
  }

  void
  mpURI (const string &hsfn)
  {
    auto uri = Uri (hsfn);
    if (uri == NULL) {
      return;
    }
    auto source = Core::MediaSource::CreateFromUri (uri);
    if (source == NULL) {
      return;
    }
    /* the item wrapper did not help with the crash */
    auto pbitem = Playback::MediaPlaybackItem (source);
logBasic ("winmp: winmp-uri-a\n");
    /* this is crashing when a uri is used */
    try {
      mediaPlayer.Source (pbitem);
    } catch (std::exception &exc) {
      logMsg (LOG_DBG, LOG_IMPORTANT, "win-uri source-fail %s", exc.what ());
      return;
    }
logBasic ("winmp: winmp-uri-b\n");
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
  char    tbuff [MAXPATHLEN];
  wchar_t *wfn = NULL;

  if (windata == NULL) {
    return 1;
  }

logBasic ("winmp: %d media: %s\n", sourceType, fn);
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

  windata->curr = (MP_MAX_SOURCE - 1) - windata->curr;
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
  for (int i = 0; i < MP_MAX_SOURCE; ++i) {
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

  for (int i = 0; i < MP_MAX_SOURCE; ++i) {
    if (windata->winmp [i] != NULL) {
      windata->winmp [i]->mpClose ();
    }
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

  previdx = (MP_MAX_SOURCE - 1) - windata->curr;
  dvol = (double) vol / 100.0;
  windata->winmp [previdx]->mpVolume (dvol);
  if (dvol <= 0.0) {
    windata->winmp [previdx]->mpStop ();
    windata->inCrossFade = false;
  }

  dvol = 1.0 - dvol;
  windata->winmp [windata->curr]->mpVolume (dvol);

  return;
}
