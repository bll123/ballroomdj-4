/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#import <Foundation/NSObject.h>
#import <AVFoundation/AVFoundation.h>

#include <stdio.h>
#include <stdlib.h>

#include <MacTypes.h>
#include <Cocoa/Cocoa.h>

/* macos's dynamic libraries work differently than linux */
#if defined (BDJ4_MEM_DEBUG)
# undef BDJ4_MEM_DEBUG
#endif

#include "audiosrc.h"
#include "log.h"
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
static long macosavCheckPlayerItemError (macosav_t *macosav);
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

logStderr ("close-a\n");
  if (macosav->player != NULL) {
    [macosav->player pause];
    [macosav->player release];
//    macosavRunLoop (macosav);
  }
logStderr ("close-b\n");
  if (macosav->plitem != NULL) {
    [macosav->plitem release];
//    macosavRunLoop (macosav);
  }
logStderr ("close-c\n");
  macosav->player = NULL;
  macosav->plitem = NULL;
  mdfree (macosav);
}

int
macosavMedia (macosav_t *macosav, const char *fullMediaPath, int sourceType)
{
  NSString        *ffn = NULL;
  NSURL           *url = NULL;
  AVPlayerItem    *plitem = NULL;
  int             count = 0;
  int             first = false;
  plistate_t      plistate;

  if (macosav == NULL || fullMediaPath == NULL) {
    return -1;
  }

  ffn = [NSString stringWithUTF8String: fullMediaPath];
  if (sourceType == AUDIOSRC_TYPE_FILE) {
    url = [NSURL fileURLWithPath: ffn isDirectory: false];
  } else {
    url = [NSURL URLWithString: ffn];
  }

  if (url == NULL) {
    return -1;
  }

logStderr ("m: a\n");
  if (macosav->player == NULL) {
    macosav->player = [[AVPlayer alloc] init];
    first = true;
  } else {
    [macosav->player pause];
  }
  plitem = macosav->plitem;
  macosav->plitem = [AVPlayerItem playerItemWithURL: url];
  if (macosavCheckPlayerItemError (macosav) != 0) {
    return -1;
  }
logStderr ("m: b\n");
  [macosav->player replaceCurrentItemWithPlayerItem: macosav->plitem];
logStderr ("m: c\n");
  if (macosavCheckPlayerError (macosav) != 0) {
    return -1;
  }
  if (plitem != NULL) {
    [plitem release];
  }
  plitem = macosav->player.currentItem;
  if (plitem != macosav->plitem) {
    fprintf (stderr, "AVPlayer: fatal: player item not configured\n");
  }

// this seems to hang the player when using a file
//  macosav->player.automaticallyWaitsToMinimizeStalling = true;
logStderr ("m: d\n");

//  if (first) {
    macosavRunLoop (macosav);
    plistate = macosavState (macosav);
    while (plistate == PLI_STATE_NONE || plistate == PLI_STATE_ERROR) {
      macosavRunLoop (macosav);
      plistate = macosavState (macosav);
      ++count;
    }
//  }
logStderr ("m: e: %d\n", count);

  return 0;
}

int
macosavPlay (macosav_t *macosav)
{
  if (macosav == NULL || macosav->player == NULL) {
    return -1;
  }

logStderr ("play\n");
//  macosavRunLoop (macosav);
  macosav->player.defaultRate = macosav->drate;
  [macosav->player play];
//  macosavRunLoop (macosav);
  return 0;
}

int
macosavPause (macosav_t *macosav)
{
  if (macosav == NULL || macosav->player == NULL) {
    return -1;
  }

logStderr ("pause\n");
//  macosavRunLoop (macosav);
  [macosav->player pause];
//  macosavRunLoop (macosav);
  return 0;
}

int
macosavStop (macosav_t *macosav)
{
  if (macosav == NULL || macosav->player == NULL) {
    return -1;
  }

logStderr ("stop\n");
//  macosavRunLoop (macosav);
  [macosav->player pause];
//  macosavRunLoop (macosav);
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
logStderr ("get-dur: %ld\n", (long) duration);
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
logStderr ("get-tm: %ld\n", (long) currtm);
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
  CMTime    tm;
  CMTime    tolerance;
  double    dpos;

  if (macosav == NULL || macosav->player == NULL) {
    return 0;
  }

logStderr ("seek %ld\n", (long) pos);
//  macosavRunLoop (macosav);
  dpos = (double) pos / 1000.0;
  /* audio frames per second */
  tm = CMTimeMakeWithSeconds (dpos, 192000);
  tolerance = CMTimeMakeWithSeconds (0.01, 192000);
  [macosav->player seekToTime: tm
      toleranceBefore: tolerance toleranceAfter: tolerance];
//  macosavRunLoop (macosav);
  return pos;
}

double
macosavRate (macosav_t *macosav, double drate)
{
  if (macosav == NULL || macosav->player == NULL) {
    return 100.0;
  }

logStderr ("rate: %.2f\n", drate);
//  macosavRunLoop (macosav);
  macosav->drate = drate;
  macosav->player.rate = drate;
//  macosavRunLoop (macosav);
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
  if ([err code] != 0) {
    fprintf (stderr, "AVPlayer error: %ld %s\n", [err code],
        [[err localizedDescription] UTF8String]);
  }
  return [err code];
}

static long
macosavCheckPlayerItemError (macosav_t *macosav)
{
  NSError   *err;

  err = [macosav->plitem error];
  if ([err code] != 0) {
    fprintf (stderr, "AVPlayerItem error: %ld %s\n", [err code],
        [[err localizedDescription] UTF8String]);
  }
  return [err code];
}

static void
macosavRunLoop (macosav_t *macosav)
{
  NSRunLoop   *runloop;

  /* only run the loop once */
  runloop = [NSRunLoop currentRunLoop];
  [runloop runMode:NSDefaultRunLoopMode beforeDate:[NSDate distantFuture]];
}
