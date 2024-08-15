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
#include "mdebug.h"
#include "player.h"
#include "nlist.h"
#include "tmutil.h"

// ### remove
#define DEFAULT_THUMBNAIL_URI L"https://upload.wikimedia.org/wikipedia/commons/3/38/VLC_icon.png"

using namespace winrt::Windows::Media;

typedef struct intf_sys intf_sys_t;

typedef struct contdata {
  char                *instname;
  callback_t          *cb;
  callback_t          *cburi;
  nlist_t             *metadata;
  void                *metav;
  int                 playstate;      // BDJ4 play state
  int32_t             pos;
  int                 rate;
  int                 volume;
  intf_sys_t          *sys;
} contdata_t;

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

struct intf_sys
{
  intf_sys (const intf_sys&) = delete;
  void operator= (const intf_sys&) = delete;

  explicit intf_sys (contdata_t *intf) :
      mediaPlayer { nullptr },
      defaultArt { nullptr },
//        contdata { contdata },
//        playlist { nullptr, }, // pl_Get (intf) },
//        input { nullptr },
      advertise { false },
      metadata_advertised { false }
  {
  }

  void InitializeMediaPlayer()
  {
    winrt::init_apartment();

    mediaPlayer = Playback::MediaPlayer ();
    mediaPlayer.CommandManager ().IsEnabled (false);

    SMTC().ButtonPressed (
        [this](SystemMediaTransportControls sender, SystemMediaTransportControlsButtonPressedEventArgs args) {
//                playlist_Lock(playlist);

    switch (args.Button()) {
            case SystemMediaTransportControlsButton::Play: {
//                    playlist_Play(playlist);
                break;
            }

            case SystemMediaTransportControlsButton::Pause: {
//                    playlist_Pause(playlist);
                break;
            }

            case SystemMediaTransportControlsButton::Stop: {
//                    playlist_Stop(playlist);
                break;
            }

            case SystemMediaTransportControlsButton::Next: {
//                    playlist_Next(playlist);
                break;
            }

            case SystemMediaTransportControlsButton::Previous: {
//                    playlist_Prev(playlist);
                break;
            }

          case SystemMediaTransportControlsButton::ChannelDown:
          case SystemMediaTransportControlsButton::ChannelUp:
          case SystemMediaTransportControlsButton::FastForward:
          case SystemMediaTransportControlsButton::Rewind:
          case SystemMediaTransportControlsButton::Record: {
            break;
          }
      }

//                playlist_Unlock(playlist);
        }
    );

    SMTC ().IsPlayEnabled (true);
    SMTC ().IsPauseEnabled (true);
    SMTC ().IsStopEnabled (true);
    SMTC ().IsPreviousEnabled (false);
    SMTC ().IsNextEnabled (true);

    SMTC ().PlaybackStatus (MediaPlaybackStatus::Closed);
    SMTC ().IsEnabled (true);

    winrt::Windows::Foundation::Uri uri{ DEFAULT_THUMBNAIL_URI };
    defaultArt = winrt::Windows::Storage::Streams::RandomAccessStreamReference::CreateFromUri (uri);

    Disp ().Thumbnail (defaultArt);
    Disp ().Type (MediaPlaybackType::Music);
    Disp ().Update ();
  }

  void UninitializeMediaPlayer()
  {
    mediaPlayer = Playback::MediaPlayer (nullptr);
    winrt::uninit_apartment ();
  }

  void AdvertiseState ()
  {
    static_assert ((int)MediaPlaybackStatus::Closed == 0, "Treat default case explicitely");

//        static std::unordered_map<input_state_e, MediaPlaybackStatus> map = {
//            {OPENING_S, MediaPlaybackStatus::Changing},
//            {PLAYING_S, MediaPlaybackStatus::Playing},
//            {PAUSE_S, MediaPlaybackStatus::Paused},
//            {END_S, MediaPlaybackStatus::Stopped}
//        };
    // Default/implicit case: set playback status to `Closed`

//        SMTC ().PlaybackStatus (map[input_state]);
    Disp ().Update ();
  }

  void ReadAndAdvertiseMetadata ()
  {
//        if (!input) {
//            return;
//        }

//        input_item_t* item = input_GetItem (input);
    winrt::hstring title, artist;

    auto to_hstring = [](char* buf, winrt::hstring def) {
        winrt::hstring ret;

        if (buf) {
            ret = winrt::to_hstring(buf);
//                libvlc_free (buf);
        }
        else {
            ret = def;
        }

        return ret;
    };

//        title = to_hstring (input_item_GetTitleFbName(item), L"Unknown Title");
//        artist = to_hstring (input_item_GetArtist(item), L"Unknown Artist");

    Disp().MusicProperties().Title(title);
    Disp().MusicProperties().Artist(artist);

    // TODO: use artwork provided by ID3tag (if exists)
    Disp().Thumbnail(defaultArt);

    Disp().Update();
  }

  SystemMediaTransportControls SMTC() {
    return mediaPlayer.SystemMediaTransportControls();
  }

  SystemMediaTransportControlsDisplayUpdater Disp() {
    return SMTC().DisplayUpdater();
  }

  Playback::MediaPlayer mediaPlayer;
  winrt::Windows::Storage::Streams::RandomAccessStreamReference defaultArt;

//    intf_thread_t* intf;
//    playlist_t* playlist;
//    input_thread_t* input;
//    input_state_e input_state;
//    vlc_thread_t thread;
//    vlc_mutex_t lock;
//    vlc_cond_t wait;

  bool advertise;
  bool metadata_advertised; // was the last song advertised to Windows?
};

contdata_t *
contiInit (const char *instname)
{
  contdata_t  *contdata;

  contdata = (contdata_t *) mdmalloc (sizeof (contdata_t));
  contdata->instname = mdstrdup (instname);
  contdata->cb = NULL;
  contdata->metadata = NULL;
  contdata->metav = NULL;
  contdata->playstate = PL_STATE_STOPPED;
  contdata->pos = 0;
  contdata->rate = 100;
  contdata->volume = 0;

  contdata->sys = new intf_sys (contdata);

  return contdata;
}

void
contiFree (contdata_t *contdata)
{
  if (contdata == NULL) {
    return;
  }

  delete contdata->sys;

  nlistFree (contdata->metadata);
  dataFree (contdata->instname);
  mdfree (contdata);
}

void
contiSetup (contdata_t *contdata)
{
  contdata->sys->InitializeMediaPlayer ();
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
//      nstate = MPRIS_PB_STATUS_PLAY;
      canplay = false;
      canpause = true;
      canseek = true;
      break;
    }
    case PL_STATE_IN_FADEOUT: {
//      nstate = MPRIS_PB_STATUS_PLAY;
      canplay = false;
      canpause = false;
      canseek = false;
      break;
    }
    case PL_STATE_PAUSED: {
//      nstate = MPRIS_PB_STATUS_PAUSE;
      canplay = true;
      canpause = false;
      canseek = true;
      break;
    }
    case PL_STATE_IN_GAP: {
//      nstate = MPRIS_PB_STATUS_PLAY;
      canplay = false;
      canpause = false;
      canseek = false;
      break;
    }
    case PL_STATE_UNKNOWN:
    case PL_STATE_STOPPED: {
//      nstate = MPRIS_PB_STATUS_PAUSE;
      canplay = true;
      canpause = false;
      canseek = false;
      break;
    }
  }

//  mpris_media_player2_player_set_can_play (contdata->mprisplayer, canplay);
//  mpris_media_player2_player_set_can_pause (contdata->mprisplayer, canpause);
//  mpris_media_player2_player_set_can_seek (contdata->mprisplayer, canseek);

//  if (contdata->playstatus != nstate) {
//    mpris_media_player2_player_set_playback_status (contdata->mprisplayer,
//        playstatusstr [nstate]);
//    contdata->playstatus = nstate;
//  }
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
//    mpris_media_player2_player_set_volume (contdata->mprisplayer, dval);
  }
  contdata->volume = volume;
}

void
contiSetCurrent (contdata_t *contdata, contmetadata_t *cmetadata)
{
  char        tbuff [200];
  nlistidx_t  miter;
  nlistidx_t  mkey;
//  void        *tv;

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

  nlistStartIterator (contdata->metadata, &miter);
//  dbusMessageInitArray (contdata->dbus, "a{sv}");
  while ((mkey = nlistIterateKey (contdata->metadata, &miter)) >= 0) {
//    if (mkey == CONT_METADATA_DURATION) {
//      tv = dbusMessageBuild ("x", nlistGetNum (contdata->metadata, mkey));
//    } else if (mkey == CONT_METADATA_TRACKID) {
//      tv = dbusMessageBuild ("o", nlistGetStr (contdata->metadata, mkey));
//    } else {
//      tv = dbusMessageBuild ("s", nlistGetStr (contdata->metadata, mkey));
//    }
//    dbusMessageAppendArray (contdata->dbus, "{sv}",
//        metadatastr [mkey], tv, NULL);
  }
//  tv = dbusMessageFinalizeArray (contdata->dbus);
//  contdata->metav = tv;

//  mpris_media_player2_player_set_metadata (contdata->mprisplayer, contdata->metav);
//  mpris_media_player2_player_set_can_go_next (contdata->mprisplayer, true);
}

#if 0

int InputEvent (vlc_object_t* object, char const* cmd,
    vlc_value_t oldval, vlc_value_t newval, void* data)
{
    VLC_UNUSED(cmd);
    VLC_UNUSED(oldval);

    intf_thread_t* intf = (intf_thread_t*)data;
    intf_sys* sys = intf->p_sys;
    input_thread_t* input = (input_thread_t*)object;

    if (newval.i_int == INPUT_EVENT_STATE) {
        input_state_e state = (input_state_e)var_GetInteger(input, "state");

        // send update to winrt thread
        vlc_mutex_lock(&sys->lock);
        sys->advertise = true;
        sys->input_state = state;
        vlc_cond_signal(&sys->wait);
        vlc_mutex_unlock(&sys->lock);
    }
    else if (newval.i_int == INPUT_EVENT_DEAD) {
        assert(sys->input);
        vlc_object_release(sys->input);
        sys->input = nullptr;
    }

    return VLC_SUCCESS;
}

int PlaylistEvent (vlc_object_t* object, char const* cmd,
    vlc_value_t oldval, vlc_value_t newval, void* data)
{
    VLC_UNUSED(object); VLC_UNUSED(cmd); VLC_UNUSED(oldval);

    intf_thread_t* intf = (intf_thread_t*)data;
    intf_sys* sys = intf->p_sys;
    input_thread_t* input = (input_thread_t*)newval.p_address;

    if (input == nullptr) {
      return VLC_SUCCESS;
    }

    sys->metadata_advertised = false; // new song, mark it as unadvertised
    sys->input = (input_thread_t*)vlc_object_hold(input);
    var_AddCallback(input, "intf-event", InputEvent, intf);

    return VLC_SUCCESS;
}

void* Thread (void* handle)
{
    intf_thread_t* intf = (intf_thread_t*) handle;
    intf_sys* sys = intf->p_sys;
    int canc;

    sys->InitializeMediaPlayer ();
    vlc_cleanup_push(
        [](void* sys) {
            ((intf_sys*)sys)->UninitializeMediaPlayer();
        },
        sys
    );

    while (1) {
        vlc_mutex_lock(&sys->lock);
        mutex_cleanup_push(&sys->lock);

        while (!sys->advertise) {
            vlc_cond_wait(&sys->wait, &sys->lock);
        }

        canc = vlc_savecancel();

        sys->AdvertiseState();
        if (sys->input_state >= PLAYING_S && !sys->metadata_advertised) {
            sys->ReadAndAdvertiseMetadata();
            sys->metadata_advertised = true;
        }
        sys->advertise = false;

        vlc_restorecancel(canc);

        vlc_cleanup_pop();
        vlc_mutex_unlock(&sys->lock);
    }

    vlc_cleanup_pop();
    sys->UninitializeMediaPlayer(); // irrelevant; control flow shouldn't get here unless some UB occurs
    return nullptr;
}

int Open(vlc_object_t* object)
{
    intf_thread_t* intf = (intf_thread_t*)object;
    intf_sys* sys = new intf_sys (intf);

    intf->p_sys = sys;

    if (!sys) {
      return 1;
        return VLC_EGENERIC;
    }

    vlc_mutex_init(&sys->lock);
    vlc_cond_init(&sys->wait);

    if (vlc_clone(&sys->thread, Thread, intf, VLC_THREAD_PRIORITY_LOW)) {
        vlc_mutex_destroy(&sys->lock);
        vlc_cond_destroy(&sys->wait);
        delete sys;
        return VLC_EGENERIC;
    }

    var_AddCallback(sys->playlist, "input-current", PlaylistEvent, intf);
    return VLC_SUCCESS;
}

#endif
