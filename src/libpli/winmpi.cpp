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
#include "winmpi.h"

using namespace winrt::Windows::Media;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Storage;
using namespace winrt::Windows;
using namespace winrt;

typedef struct winmpintfc winmpintfc_t;

typedef struct windata {
  winmpintfc_t  *winmp;
  plistate_t    state;
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
  mpInit (void)
  {
    /* initializing the 'apartment' causes a crash */

    mediaPlayer = Playback::MediaPlayer ();
    mediaPlayer.CommandManager ().IsEnabled (false);
  }

  void
  mpPause (void)
  {
    auto pbSession = mediaPlayer.PlaybackSession ();
    if (pbSession.CanPause ()) {
      mediaPlayer.Pause ();
    }
  }

  void
  mpPlay (void)
  {
    try {
      mediaPlayer.Play ();
    } catch (std::exception &exc) {
      logMsg (LOG_DBG, LOG_IMPORTANT, "winmp-play fail %s\n", exc.what ());
    }
  }

  void
  mpStop (void)
  {
    mpPause ();
    mediaPlayer.Source (nullptr);
  }

  ssize_t
  mpSeek (ssize_t pos)
  {
    TimeSpan    ts;

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

    try {
      sfile = StorageFile::GetFileFromPathAsync (hsfn).get ();
    } catch (std::exception &exc) {
      logMsg (LOG_DBG, LOG_IMPORTANT, "win-media open-fail %s\n", exc.what ());
      return;
    }
    if (sfile == NULL) {
      logMsg (LOG_DBG, LOG_IMPORTANT, "win-media fail-no-file\n");
      return;
    }
    auto source = Core::MediaSource::CreateFromStorageFile (sfile);
    if (source == NULL) {
      logMsg (LOG_DBG, LOG_IMPORTANT, "win-media source-fail\n");
      return;
    }
    try {
      mediaPlayer.Source (source);
    } catch (std::exception &exc) {
      logMsg (LOG_DBG, LOG_IMPORTANT, "win-source source-fail\n");
      return;
    }
    auto ac = mediaPlayer.AudioCategory ();
    ac = Playback::MediaPlayerAudioCategory::Media;
  }

  void
  mpURI (const hstring &hsfn)
  {
logBasic ("winmp-uri-a\n");
    auto uri = Uri (hsfn);
    if (uri == NULL) {
logBasic ("win-uri fail-c\n");
      return;
    }
    auto source = Core::MediaSource::CreateFromUri (uri);
    if (source == NULL) {
logBasic ("win-uri fail-d\n");
      return;
    }
logBasic ("winmp-uri-b\n");
    mediaPlayer.Source (source);
logBasic ("winmp-uri-c\n");
    auto ac = mediaPlayer.AudioCategory ();
logBasic ("winmp-uri-d\n");
    ac = Playback::MediaPlayerAudioCategory::Media;
logBasic ("winmp-uri fin\n");
  }

  void
  mpClose (void)
  {
    mediaPlayer = Playback::MediaPlayer (nullptr);
  }

  ssize_t
  mpDuration (void)
  {
    ssize_t   dur;

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
    auto pbSession = mediaPlayer.PlaybackSession ();
    pbSession.PlaybackRate (rate);
    rate = pbSession.PlaybackRate ();
    return rate;
  }

  Playback::MediaPlaybackState
  mpState (void)
  {
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

  dur = windata->winmp->mpDuration ();
  return dur;
}

ssize_t
winmpGetTime (windata_t *windata)
{
  ssize_t   pos;

  if (windata == NULL) {
    return 0;
  }

  pos = windata->winmp->mpTime ();
  return pos;
}

int
winmpStop (windata_t *windata)
{
  if (windata == NULL) {
    return 0;
  }

  windata->winmp->mpStop ();
  return 0;
}

int
winmpPause (windata_t *windata)
{
  if (windata == NULL) {
    return 0;
  }
  if (windata->winmp == NULL) {
    return 0;
  }

  windata->winmp->mpPause ();
  return 0;
}

int
winmpPlay (windata_t *windata)
{
  if (windata == NULL) {
    return 0;
  }
  if (windata->winmp == NULL) {
    return 0;
  }

  windata->winmp->mpPlay ();
  return 0;
}

ssize_t
winmpSeek (windata_t *windata, ssize_t pos)
{
  if (windata == NULL) {
    return 0;
  }
  if (windata->winmp == NULL) {
    return 0;
  }

  pos = windata->winmp->mpSeek (pos);
  return pos;
}

double
winmpRate (windata_t *windata, double drate)
{
  if (windata == NULL) {
    return 100.0;
  }
  if (windata->winmp == NULL) {
    return 100.0;
  }

  drate = windata->winmp->mpRate (drate);
  return drate;
}

int
winmpMedia (windata_t *windata, const char *fn, int sourceType)
{
  char    tbuff [MAXPATHLEN];

  if (windata == NULL) {
    return 1;
  }
  if (windata->winmp == NULL) {
    return 2;
  }

  auto wfn = osToWideChar (fn);
  if (sourceType == AUDIOSRC_TYPE_FILE) {
    stpecpy (tbuff, tbuff + sizeof (tbuff), fn);
    pathDisplayPath (tbuff, sizeof (tbuff));
    auto wfn = osToWideChar (tbuff);
    auto hsfn = hstring (wfn);
    windata->winmp->mpMedia (hsfn);
  } else {
    auto hsfn = hstring (wfn);
    windata->winmp->mpURI (hsfn);
  }
  mdfree (wfn);

  return 0;
}

windata_t *
winmpInit (void)
{
  windata_t   *windata;

  windata = (windata_t *) mdmalloc (sizeof (windata_t));
  windata->state = PLI_STATE_IDLE;
  windata->winmp = new winmpintfc (windata);
  windata->winmp->mpInit ();

  return windata;
}

void
winmpClose (windata_t *windata)
{
  if (windata == NULL) {
    return;
  }

  if (windata->winmp != NULL) {
    windata->winmp->mpClose ();
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
  if (windata->winmp == NULL) {
    return PLI_STATE_IDLE;
  }

  auto  wstate = windata->winmp->mpState ();

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
