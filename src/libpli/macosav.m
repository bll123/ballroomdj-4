/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#import <Foundation/NSObject.h>
#import <AVFoundation/AVFoundation.h>
// #import <AudioToolbox/AudioServices.h>

#include <stdio.h>
#include <stdlib.h>

#include <MacTypes.h>
#include <Cocoa/Cocoa.h>

/* macos's dynamic libraries work differently than linux */
#if defined (BDJ4_MEM_DEBUG)
# undef BDJ4_MEM_DEBUG
#endif

#include "audiosrc.h"
#include "macosav.h"
#include "mdebug.h"

typedef struct macosav {
  AVPlayer                  *player;
  AVPlayerItem              *plitem;
  AVPlayerTimeControlStatus tcstatus;
  double                    drate;
  plistate_t                plistate;
} macosav_t;

static void macosavCheckTCStatus (macosav_t *macosav);
static long macosavCheckPlayerError (macosav_t *macosav);
static void macosavRunLoop (macosav_t *macosav);

macosav_t *
macosavInit (void)
{
  macosav_t   *macosav;

  macosav = mdmalloc (sizeof (macosav_t));
  macosav->player = NULL;
  macosav->plitem = NULL;
  macosav->tcstatus = AVPlayerTimeControlStatusWaitingToPlayAtSpecifiedRate;
  macosav->drate = 1.0;
  macosav->plistate = PLI_STATE_NONE;

  return macosav;
}

void
macosavClose (macosav_t *macosav)
{
  if (macosav == NULL) {
    return;
  }

  [macosav->player release];
  macosav->player = NULL;
  macosav->plitem = NULL;
  mdfree (macosav);
}

int
macosavMedia (macosav_t *macosav, const char *fullMediaPath, int sourceType)
{
  NSString        *ffn = NULL;
  NSURL           *url = NULL;
  int             count = 0;
  plistate_t      plistate;

  if (macosav == NULL || fullMediaPath == NULL) {
    return -1;
  }

  ffn = [NSString stringWithUTF8String: fullMediaPath];
  if (sourceType == AUDIOSRC_TYPE_FILE) {
    url = [NSURL fileURLWithPath: ffn isDirectory: false];
  } else {
  }

  if (url != NULL) {
    macosav->player = [AVPlayer playerWithURL: url];
    if (macosavCheckPlayerError (macosav) != 0) {
      return -1;
    }
    macosav->player.automaticallyWaitsToMinimizeStalling = false;
    macosav->plitem = [macosav->player currentItem];
  }

  plistate = macosavState (macosav);
  while (plistate == PLI_STATE_NONE || plistate == PLI_STATE_ERROR) {
    plistate = macosavState (macosav);
    ++count;
  }
fprintf (stderr, "media-count: %d\n", count);

  return 0;
}

int
macosavPlay (macosav_t *macosav)
{
  if (macosav == NULL || macosav->player == NULL) {
    return -1;
  }

  macosav->player.defaultRate = macosav->drate;
  [macosav->player play];
  macosavRunLoop (macosav);
  return 0;
}

int
macosavPause (macosav_t *macosav)
{
  if (macosav == NULL || macosav->player == NULL) {
    return -1;
  }

  [macosav->player pause];
  macosavRunLoop (macosav);
  return 0;
}

int
macosavStop (macosav_t *macosav)
{
  if (macosav == NULL || macosav->player == NULL) {
    return -1;
  }

  [macosav->player pause];
  macosavRunLoop (macosav);
  return 0;
}

ssize_t
macosavGetDuration (macosav_t *macosav)
{
  CMTime    tm;
  ssize_t   duration;

  if (macosav == NULL || macosav->player == NULL) {
    return 0;
  }

  macosavRunLoop (macosav);
  tm = [macosav->plitem duration];
  duration = (ssize_t) (CMTimeGetSeconds (tm) * 1000.0);
  return duration;
}

ssize_t
macosavGetTime (macosav_t *macosav)
{
  CMTime    tm;
  ssize_t   currtm;

  if (macosav == NULL || macosav->player == NULL) {
    return 0;
  }

  macosavRunLoop (macosav);
  tm = [macosav->player currentTime];
  currtm = (ssize_t) (CMTimeGetSeconds (tm) * 1000.0);
  return 0;
}

plistate_t
macosavState (macosav_t *macosav)
{
  AVPlayerStatus      status;

  if (macosav == NULL || macosav->player == NULL) {
    return PLI_STATE_NONE;
  }

  macosavRunLoop (macosav);

  status = [macosav->player status];

  switch (status) {
    case AVPlayerStatusUnknown: {
      macosav->plistate = PLI_STATE_NONE;
      break;
    }
    case AVPlayerStatusReadyToPlay: {
      macosav->plistate = PLI_STATE_OPENING;
      macosavCheckTCStatus (macosav);
      break;
    }
    case AVPlayerStatusFailed: {
      macosav->plistate = PLI_STATE_ERROR;
      break;
    }
  }

  return macosav->plistate;
}

ssize_t
macosavSeek (macosav_t *macosav, ssize_t pos)
{
  if (macosav == NULL || macosav->player == NULL) {
    return 0;
  }

  macosavRunLoop (macosav);
  return pos;
}

double
macosavRate (macosav_t *macosav, double drate)
{
  if (macosav == NULL || macosav->player == NULL) {
    return 100.0;
  }

  macosav->drate = drate;

  macosavRunLoop (macosav);
  return drate;
}

/* internal routines */

static void
macosavCheckTCStatus (macosav_t *macosav)
{
  AVPlayerTimeControlStatus tcstatus;

  tcstatus = macosav->player.timeControlStatus;

  switch (tcstatus) {
    case AVPlayerTimeControlStatusPaused: {
      macosav->plistate = PLI_STATE_PAUSED;
      break;
    }
    case AVPlayerTimeControlStatusWaitingToPlayAtSpecifiedRate: {
      macosav->plistate = PLI_STATE_BUFFERING;
      break;
    }
    case AVPlayerTimeControlStatusPlaying: {
      macosav->plistate = PLI_STATE_PLAYING;
      break;
    }
  }

  macosav->tcstatus = tcstatus;
}

static long
macosavCheckPlayerError (macosav_t *macosav)
{
  NSError   *err;

  err = [macosav->player error];
  return [err code];
}

static void
macosavRunLoop (macosav_t *macosav)
{
  NSRunLoop           *runloop;

  runloop = [NSRunLoop currentRunLoop];
  [runloop runMode:NSDefaultRunLoopMode beforeDate:[NSDate distantFuture]];
}
