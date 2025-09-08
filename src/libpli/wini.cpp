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

typedef struct winiintfc winiintfc_t;

typedef struct windata {
  winiintfc_t   *wini;
  plistate_t    state;
} windata_t;

struct winiintfc
{
  winiintfc (const winiintfc&) = delete;
  void operator= (const winiintfc&) = delete;

  explicit winiintfc (windata_t *windata) :
      mediaPlayer { nullptr },
//      defaultArt { nullptr },
      windata { windata }
  {
  }

  void
  winiMediaPlayerInit (void)
  {
    winrt::hstring  tstr;

    /* initializing the 'apartment' causes a crash */

logBasic ("init-a\n");
    mediaPlayer = Playback::MediaPlayer ();
logBasic ("init-b\n");
    mediaPlayer.CommandManager ().IsEnabled (false);
logBasic ("init-c\n");
  }

  void
  winiPlayerPause (void)
  {
logBasic ("wini-pause\n");
    auto pbSession = mediaPlayer.PlaybackSession ();
    if (pbSession.CanPause ()) {
      mediaPlayer.Pause ();
    }
logBasic ("wini-pause fin\n");
  }

  void
  winiPlayerPlay (void)
  {
logBasic ("wini-play\n");
    try {
      mediaPlayer.Play ();
    } catch (std::exception &exc) {
logBasic ("wini-play fail-a %s\n", exc.what ());
    } catch (...) {
logBasic ("wini-play fail-b\n");
    }
logBasic ("wini-play fin\n");
  }

  void
  winiPlayerMedia (const hstring &hsfn)
  {
    StorageFile   sfile = nullptr;

logBasic ("wini-media-a\n");
    try {
      sfile = StorageFile::GetFileFromPathAsync (hsfn).get ();
    } catch (std::exception &exc) {
logBasic ("win-media fail-a %s\n", exc.what ());
      return;
    } catch (...) {
logBasic ("win-media fail-b\n");
      return;
    }
    if (sfile == NULL) {
logBasic ("win-media fail-c\n");
      return;
    }
    auto source = Core::MediaSource::CreateFromStorageFile (sfile);
    if (source == NULL) {
logBasic ("win-media fail-d\n");
      return;
    }
logBasic ("wini-media-b\n");
    try {
// ### crashes
// ### it's supposed to be a property, mediaPlayer.Source = source
// ### but that doesn't compile.
      mediaPlayer.Source (source);
    } catch (std::exception &exc) {
logBasic ("win-media fail-src-a %s\n", exc.what ());
      return;
    } catch (...) {
logBasic ("win-media fail-src-b\n");
      return;
    }
logBasic ("wini-media-c\n");
// ## crashing here
//    auto ac = mediaPlayer.AudioCategory ();
//logBasic ("wini-media-d\n");
//    ac = Playback::MediaPlayerAudioCategory::Media;
logBasic ("wini-media fin\n");
  }

  void
  winiPlayerURI (const hstring &hsfn)
  {
logBasic ("wini-uri-a\n");
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
logBasic ("wini-uri-b\n");
    mediaPlayer.Source (source);
logBasic ("wini-uri-c\n");
// ## crashing here
//    auto ac = mediaPlayer.AudioCategory ();
//logBasic ("wini-uri-d\n");
//    ac = Playback::MediaPlayerAudioCategory::Media;
logBasic ("wini-uri fin\n");
  }

  void
  winiPlayerClose (void)
  {
logBasic ("wini-close\n");
    mediaPlayer = Playback::MediaPlayer (nullptr);
logBasic ("wini-close fin\n");
  }

  Playback::MediaPlaybackState
  winiPlayerState (void)
  {
logBasic ("wini-state\n");
    auto  pbSession = mediaPlayer.PlaybackSession ();
    auto  wstate = pbSession.PlaybackState ();
logBasic ("wini-state fin\n");
    return wstate;
  }

  Playback::MediaPlayer mediaPlayer;
  windata_t *windata;
};

ssize_t
winGetDuration (windata_t *windata)
{
  if (windata == NULL) {
    return 0;
  }

logBasic ("winGetDuration\n");
  return 30000;
}

ssize_t
winGetTime (windata_t *windata)
{
  ssize_t   pos;

  if (windata == NULL) {
    return 0;
  }

logBasic ("winGetTime\n");
//  auto pbSession = windata->mediaPlayer.PlaybackSession ();
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

logBasic ("winStop\n");
  return 0;
}

int
winPause (windata_t *windata)
{
  if (windata == NULL) {
    return 0;
  }
  if (windata->wini == NULL) {
    return 0;
  }

logBasic ("winPause\n");
  windata->wini->winiPlayerPause ();
  return 0;
}

int
winPlay (windata_t *windata)
{
  if (windata == NULL) {
    return 0;
  }
  if (windata->wini == NULL) {
    return 0;
  }

logBasic ("winPlay\n");
  windata->wini->winiPlayerPlay ();
  return 0;
}

ssize_t
winSeek (windata_t *windata, ssize_t pos)
{
  if (windata == NULL) {
    return 0;
  }
  if (windata->wini == NULL) {
    return 0;
  }

logBasic ("winSeek\n");
  return pos;
}

double
winRate (windata_t *windata, double drate)
{
  if (windata == NULL) {
    return 100.0;
  }
  if (windata->wini == NULL) {
    return 100.0;
  }

logBasic ("winRate\n");
  return 100.0;
}

int
winMedia (windata_t *windata, const char *fn, int sourceType)
{
  char    tbuff [MAXPATHLEN];

  if (windata == NULL) {
    return 1;
  }
  if (windata->wini == NULL) {
    return 2;
  }

  auto wfn = osToWideChar (fn);
  if (sourceType == AUDIOSRC_TYPE_FILE) {
    stpecpy (tbuff, tbuff + sizeof (tbuff), fn);
    pathDisplayPath (tbuff, sizeof (tbuff));
logBasic ("winMedia %s\n", tbuff);
    auto wfn = osToWideChar (tbuff);
    auto hsfn = hstring (wfn);
    windata->wini->winiPlayerMedia (hsfn);
  } else {
    auto hsfn = hstring (wfn);
    windata->wini->winiPlayerURI (hsfn);
  }
  mdfree (wfn);

  return 0;
}

windata_t *
winInit (void)
{
  windata_t   *windata;

logBasic ("winInit\n");
  windata = (windata_t *) mdmalloc (sizeof (windata_t));
  windata->state = PLI_STATE_IDLE;
  windata->wini = new winiintfc (windata);
logBasic ("winInit done\n");

  return windata;
}

void
winClose (windata_t *windata)
{
  if (windata == NULL) {
    return;
  }

  if (windata->wini != NULL) {
    windata->wini->winiPlayerClose ();
  }

logBasic ("winClose\n");
  mdfree (windata);
  return;
}

plistate_t
winState (windata_t *windata)
{
  if (windata == NULL) {
    return PLI_STATE_IDLE;
  }
  if (windata->wini == NULL) {
    return PLI_STATE_IDLE;
  }

  auto  wstate = windata->wini->winiPlayerState ();

  switch (wstate) {
    case Playback::MediaPlaybackState::None: {
logBasic ("  state idle\n");
      windata->state = PLI_STATE_IDLE;
      break;
    }
    case Playback::MediaPlaybackState::Opening: {
logBasic ("  state opening\n");
      windata->state = PLI_STATE_OPENING;
      break;
    }
    case Playback::MediaPlaybackState::Buffering: {
logBasic ("  state buffering\n");
      windata->state = PLI_STATE_OPENING;
      break;
    }
    case Playback::MediaPlaybackState::Playing: {
logBasic ("  state playing\n");
      windata->state = PLI_STATE_PLAYING;
      break;
    }
    case Playback::MediaPlaybackState::Paused: {
logBasic ("  state paused\n");
      windata->state = PLI_STATE_PAUSED;
      break;
    }
  }

  return windata->state;
}
