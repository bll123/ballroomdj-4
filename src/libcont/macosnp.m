/*
 * Copyright 2026 Brad Lanam Pleasant Hill CA
 *
 * https://stackoverflow.com/questions/70225858/where-to-add-current-time-and-duration-time-for-avplayer-and-avaudioplayer-using
 * https://stackoverflow.com/questions/69865710/setting-media-playback-info-in-macos-not-working
 * https://github.com/MarshallOfSound/electron-media-service/blob/master/src/darwin/service.mm
 */
#include "config.h"

#import <Foundation/NSObject.h>
#import <AudioToolbox/AudioServices.h>
#import "MediaPlayer/MPNowPlayingInfoCenter.h"
#import "MediaPlayer/MPRemoteCommandCenter.h"
#import "MediaPlayer/MPRemoteCommand.h"
#import "MediaPlayer/MPMediaItem.h"
#import "MediaPlayer/MPRemoteCommandEvent.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>
#include <string.h>
#include <math.h>

#include <MacTypes.h>
#include <Cocoa/Cocoa.h>

#include "audiosrc.h"
#include "bdj4.h"
#include "bdj4intl.h"
#include "callback.h"
#include "controller.h"
#include "log.h"
#include "mdebug.h"
#include "pathbld.h"
#include "player.h"

@interface RemoteController : NSObject {
  contdata_t  *contdata;
}
- (instancetype) initWithContData: (contdata_t *) tcontdata;
- (instancetype) init NS_UNAVAILABLE;
@end

typedef struct contdata {
  char                    *instname;
  MPNowPlayingInfoCenter  *infocenter;
  MPRemoteCommandCenter   *remotecmd;
  NSMutableDictionary     *songInfo;
  RemoteController        *remotecontrol;
  callback_t              *cb;
  callback_t              *cburi;
  playerstate_t           plstate;
} contdata_t;

@implementation RemoteController
- (instancetype) initWithContData: (contdata_t *) tcontdata {
  contdata = tcontdata;
  return self;
}
- (MPRemoteCommandHandlerStatus) remotePlay {
  if (contdata->cb != NULL) {
    callbackHandlerII (contdata->cb, CONTROLLER_PLAY, 0);
  }
  return MPRemoteCommandHandlerStatusSuccess;
}
- (MPRemoteCommandHandlerStatus) remotePause {
  if (contdata->cb != NULL) {
    callbackHandlerII (contdata->cb, CONTROLLER_PAUSE, 0);
  }
  return MPRemoteCommandHandlerStatusSuccess;
}
- (MPRemoteCommandHandlerStatus) remoteStop {
  if (contdata->cb != NULL) {
    callbackHandlerII (contdata->cb, CONTROLLER_STOP, 0);
  }
  return MPRemoteCommandHandlerStatusSuccess;
}
- (MPRemoteCommandHandlerStatus) remoteTogglePlayPause {
  if (contdata->cb != NULL) {
    callbackHandlerII (contdata->cb, CONTROLLER_PLAYPAUSE, 0);
  }
  return MPRemoteCommandHandlerStatusSuccess;
}
- (MPRemoteCommandHandlerStatus) remoteNext {
  if (contdata->cb != NULL) {
    callbackHandlerII (contdata->cb, CONTROLLER_NEXT, 0);
  }
  return MPRemoteCommandHandlerStatusSuccess;
}
- (MPRemoteCommandHandlerStatus)
    remoteChangePlaybackPosition: (MPChangePlaybackPositionCommandEvent *) event {
  int32_t   position;

  position = (int32_t) (event.positionTime * 1000.0);
  if (contdata->cb != NULL) {
    callbackHandlerII (contdata->cb, CONTROLLER_SEEK, position);
  }
  return MPRemoteCommandHandlerStatusSuccess;
}
/* macos: now playing does not show a rate control */
- (MPRemoteCommandHandlerStatus)
    remoteChangePlaybackRate: (MPChangePlaybackRateCommandEvent *) event {
  return MPRemoteCommandHandlerStatusSuccess;
}
/* macos: now playing does not show a repeat mode control */
- (MPRemoteCommandHandlerStatus)
remoteChangeRepeatMode: (MPChangeRepeatModeCommandEvent *) event {
  int   repeat = false;

  /* MPRepeatTypeNone */
  /* MPRepeatTypeAll - not supported */
  if (event.repeatType == MPRepeatTypeOne) {
    repeat = true;
  }
  if (contdata->cb != NULL) {
    callbackHandlerII (contdata->cb, CONTROLLER_REPEAT, repeat);
  }
  return MPRemoteCommandHandlerStatusSuccess;
}
@end

void
contiDesc (const char **ret, int max)
{
  int         c = 0;

  if (max < 2) {
    return;
  }

  ret [c++] = _("MacOS Now Playing");
  ret [c++] = NULL;
}

contdata_t *
contiInit (const char *instname)
{
  contdata_t  *contdata;

  contdata = mdmalloc (sizeof (contdata_t));
  contdata->instname = mdstrdup (instname);
  contdata->cb = NULL;
  contdata->cburi = NULL;
  contdata->plstate = PL_STATE_STOPPED;

  contdata->infocenter = [MPNowPlayingInfoCenter defaultCenter];
  contdata->remotecmd = [MPRemoteCommandCenter sharedCommandCenter];
  contdata->songInfo = [[NSMutableDictionary alloc] init];
  contdata->remotecontrol = [[RemoteController alloc]
      initWithContData: contdata];

  [contdata->remotecmd playCommand].enabled = false;
  [contdata->remotecmd pauseCommand].enabled = false;
  [contdata->remotecmd stopCommand].enabled = true;
  [contdata->remotecmd togglePlayPauseCommand].enabled = false;
  [contdata->remotecmd changePlaybackPositionCommand].enabled = false;
  [contdata->remotecmd changePlaybackRateCommand].enabled = false;
  [contdata->remotecmd changeRepeatModeCommand].enabled = false;
  [contdata->remotecmd nextTrackCommand].enabled = true;
  [contdata->remotecmd previousTrackCommand].enabled = false;

  [[contdata->remotecmd playCommand]
      addTarget: contdata->remotecontrol
      action: @selector (remotePlay)];
  [[contdata->remotecmd pauseCommand]
      addTarget: contdata->remotecontrol
      action: @selector (remotePause)];
  [[contdata->remotecmd stopCommand]
      addTarget: contdata->remotecontrol
      action: @selector (remoteStop)];
  [[contdata->remotecmd togglePlayPauseCommand]
      addTarget: contdata->remotecontrol
      action: @selector (remoteTogglePlayPause)];
  [[contdata->remotecmd changePlaybackPositionCommand]
      addTarget: contdata->remotecontrol
      action: @selector (remoteChangePlaybackPosition:)];
  [[contdata->remotecmd changePlaybackRateCommand]
      addTarget: contdata->remotecontrol
      action: @selector (remoteChangePlaybackRate:)];
  [[contdata->remotecmd changeRepeatModeCommand]
      addTarget: contdata->remotecontrol
      action: @selector (remoteChangeRepeatMode:)];
  [[contdata->remotecmd nextTrackCommand]
      addTarget: contdata->remotecontrol
      action: @selector (remoteNext)];

  return contdata;
}

void
contiFree (contdata_t *contdata)
{
  if (contdata == NULL) {
    return;
  }

  dataFree (contdata->instname);
  mdfree (contdata);
}

bool
contiSetup (void *tcontdata)
{
  return true;
}

bool
contiCheckReady (contdata_t *contdata)
{
  return true;
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
contiSetPlayState (contdata_t *contdata, playerstate_t state)
{
  bool  canplay = false;
  bool  canpause = false;
  bool  canseek = false;

  if (contdata == NULL) {
    return;
  }

  contdata->plstate = state;

  switch (state) {
    case PL_STATE_LOADING:
    case PL_STATE_PLAYING: {
      contdata->infocenter.playbackState = MPNowPlayingPlaybackStatePlaying;
      canplay = true;
      canpause = true;
      canseek = true;
      break;
    }
    case PL_STATE_IN_FADEOUT: {
      contdata->infocenter.playbackState = MPNowPlayingPlaybackStatePlaying;
      canplay = false;
      canpause = false;
      canseek = false;
      break;
    }
    case PL_STATE_PAUSED: {
      contdata->infocenter.playbackState = MPNowPlayingPlaybackStatePaused;
      canplay = true;
      canpause = true;    // so that play-pause will work
      canseek = true;
      break;
    }
    case PL_STATE_IN_CROSSFADE:
    case PL_STATE_IN_GAP: {
      contdata->infocenter.playbackState = MPNowPlayingPlaybackStatePlaying;
      canplay = false;
      canpause = false;
      canseek = false;
      break;
    }
    case PL_STATE_MAX:
    case PL_STATE_UNKNOWN: {
      contdata->infocenter.playbackState = MPNowPlayingPlaybackStateUnknown;
      canplay = false;
      canpause = false;
      canseek = false;
      break;
    }
    case PL_STATE_STOPPED: {
      contdata->infocenter.playbackState = MPNowPlayingPlaybackStateStopped;
      canplay = true;
      canpause = false;
      canseek = false;
      break;
    }
  }

  [contdata->remotecmd playCommand].enabled = canplay;
  [contdata->remotecmd pauseCommand].enabled = canpause;
  [contdata->remotecmd togglePlayPauseCommand].enabled = canplay | canpause;
  [contdata->remotecmd changePlaybackPositionCommand].enabled = canseek;
  [contdata->remotecmd changePlaybackRateCommand].enabled = canseek;
  [contdata->remotecmd changeRepeatModeCommand].enabled = canseek;

  return;
}

void
contiSetRepeatState (contdata_t *contdata, bool state)
{
  return;
}

void
contiSetPosition (contdata_t *contdata, int32_t pos)
{
  if (contdata == NULL) {
    return;
  }

  [contdata->songInfo setObject:
      [NSNumber numberWithFloat: (Float32) pos / 1000.0]
      forKey: MPNowPlayingInfoPropertyElapsedPlaybackTime];

  return;
}

void
contiSetRate (contdata_t *contdata, int rate)
{
  if (contdata == NULL) {
    return;
  }

  return;
}

void
contiSetVolume (contdata_t *contdata, int volume)
{
  if (contdata == NULL) {
    return;
  }

  return;
}

void
contiSetCurrent (contdata_t *contdata, contmetadata_t *cmetadata)
{
  MPMediaType         mt;
  int64_t             dur;
  MPMediaItemArtwork  *artwork = NULL;

  if (contdata == NULL) {
    return;
  }

  if (cmetadata->album != NULL) {
    [contdata->songInfo setObject:
        [NSString stringWithUTF8String: cmetadata->album]
        forKey: MPMediaItemPropertyAlbumTitle];
  }
  if (cmetadata->albumartist != NULL) {
    [contdata->songInfo setObject:
        [NSString stringWithUTF8String: cmetadata->albumartist]
        forKey: MPMediaItemPropertyAlbumArtist];
  }
  if (cmetadata->artist != NULL) {
    [contdata->songInfo setObject:
        [NSString stringWithUTF8String: cmetadata->artist]
        forKey: MPMediaItemPropertyArtist];
  }
  if (cmetadata->title != NULL) {
    [contdata->songInfo setObject:
        [NSString stringWithUTF8String: cmetadata->title]
        forKey: MPMediaItemPropertyTitle];
  }
  if (cmetadata->genre != NULL) {
    [contdata->songInfo setObject:
        [NSString stringWithUTF8String: cmetadata->genre]
        forKey: MPMediaItemPropertyGenre];
  }

  dur = cmetadata->duration;
  [contdata->songInfo setObject:
      [NSNumber numberWithFloat: (Float32) dur / 1000.0]
      forKey: MPMediaItemPropertyPlaybackDuration];

  [contdata->songInfo setObject:
      [NSNumber numberWithFloat: 0.0]
      forKey: MPNowPlayingInfoPropertyElapsedPlaybackTime];

  mt = MPMediaTypeMusic;
  if (cmetadata->astype == AUDIOSRC_TYPE_PODCAST) {
    mt = MPMediaTypePodcast;
  }
  [contdata->songInfo setObject:
      [NSNumber numberWithInt: mt]
      forKey: MPNowPlayingInfoPropertyMediaType];

  {
    char      tbuff [MAXPATHLEN];
    NSURL     *url;
    NSString  *str;
    NSImage   *image = NULL;
    CGSize    cgsize;

    cgsize.width = 256.0;
    cgsize.height = 256.0;

    if (cmetadata->imageuri != NULL && *cmetadata->imageuri) {
      str = [NSString stringWithUTF8String: cmetadata->imageuri];
      url = [NSURL URLWithString: str];
      image = [[NSImage alloc] initWithContentsOfURL: url];
    } else {
      if (cmetadata->uri != NULL && *cmetadata->uri) {
        OSStatus      error;
        AudioFileID   afile;
        UInt32        propSize = 0;
        UInt32        writable = 0;
        CFDataRef     imgdata;

        str = [NSString stringWithUTF8String: cmetadata->uri];
        url = [NSURL URLWithString: str];

        error = AudioFileOpenURL ((CFURLRef) url,
            kAudioFileReadPermission, 0, &afile);
        if (error == 0) {
          error = AudioFileGetPropertyInfo (afile, kAudioFilePropertyAlbumArtwork,
              &propSize, &writable);
        }
        if (error == 0) {
          error = AudioFileGetProperty (afile, kAudioFilePropertyAlbumArtwork,
              &propSize, &imgdata);
          if (error == 0) {
            image = [[NSImage alloc] initWithData: (NSData *) imgdata];
          }
        }
        if (error != 0) {
          image = NULL;
        }
      }

      if (image == NULL) {
        pathbldMakePath (tbuff, sizeof (tbuff),
            "bdj4_icon_sq", BDJ4_IMG_SVG_EXT, PATHBLD_MP_DIR_IMG);
        str = [NSString stringWithUTF8String: tbuff];
        image = [[NSImage alloc] initWithContentsOfFile: str];
      }
    }

    /* no idea if this is correct, though it seems to work */
    /* image may need resizing */
    artwork = [[MPMediaItemArtwork alloc]
        initWithBoundsSize: image.size
        requestHandler: ^NSImage* _Nonnull(CGSize cgsize) { return image; }];
  }

  if (artwork != NULL) {
    [contdata->songInfo setObject: artwork
        forKey: MPMediaItemPropertyArtwork];
  }

  [contdata->infocenter setNowPlayingInfo: contdata->songInfo];

  return;
}
