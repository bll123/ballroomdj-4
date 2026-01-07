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
  NSString    *ffn = NULL;
  NSURL       *url = NULL;

  if (macosav == NULL || fullMediaPath == NULL) {
    return -1;
  }

  ffn = [NSString stringWithUTF8String: fullMediaPath];
  if (sourceType == AUDIOSRC_TYPE_FILE) {
    url = [NSURL fileURLWithPath: ffn isDirectory: false];
fprintf (stderr, "url: %s\n", [url fileSystemRepresentation]);
  } else {
  }

  if (url != NULL) {
    NSError   *err;

    macosav->player = [AVPlayer playerWithURL: url];
    err = [macosav->player error];
    if (err != 0) {
fprintf (stderr, "pl: err: %ld %s\n", [err code], [[err domain] UTF8String]);
    }
    macosav->player.automaticallyWaitsToMinimizeStalling = false;
    macosav->plitem = [macosav->player currentItem];
    err = [macosav->plitem error];
    if (err != 0) {
fprintf (stderr, "plitem: err: %ld %s\n", [err code], [[err domain] UTF8String]);
    }
macosavGetDuration (macosav);
  }

  return 0;
}

int
macosavPlay (macosav_t *macosav)
{
  if (macosav == NULL || macosav->player == NULL) {
    return -1;
  }

fprintf (stderr, "play drate:%.2f\n", macosav->drate);
  macosav->player.defaultRate = macosav->drate;
  [macosav->player play];
  return 0;
}

int
macosavPause (macosav_t *macosav)
{
  if (macosav == NULL || macosav->player == NULL) {
    return -1;
  }

fprintf (stderr, "pause\n");
  [macosav->player pause];
  return 0;
}

int
macosavStop (macosav_t *macosav)
{
  if (macosav == NULL || macosav->player == NULL) {
    return -1;
  }

fprintf (stderr, "stop\n");
  [macosav->player pause];
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

  tm = [macosav->plitem duration];
  duration = (ssize_t) (CMTimeGetSeconds (tm) * 1000.0);
fprintf (stderr, "dur: %ld\n", (long) duration);
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

  tm = [macosav->player currentTime];
  currtm = (ssize_t) (CMTimeGetSeconds (tm) * 1000.0);
fprintf (stderr, "currtm: %ld\n", (long) currtm);
  return 0;
}

plistate_t
macosavState (macosav_t *macosav)
{
  AVPlayerStatus            status;

  if (macosav == NULL || macosav->player == NULL) {
    return PLI_STATE_NONE;
  }

  status = [macosav->player status];

  switch (status) {
    case AVPlayerStatusUnknown: {
      macosav->plistate = PLI_STATE_IDLE;
fprintf (stderr, "status: unk\n");

      macosavCheckTCStatus (macosav);
      break;
    }
    case AVPlayerStatusReadyToPlay: {
      float   rate;

      rate = [macosav->player rate];
fprintf (stderr, "rate: %.2f\n", (double) rate);
fprintf (stderr, "status: ready\n");

      macosavCheckTCStatus (macosav);
      break;
    }
    case AVPlayerStatusFailed: {
      macosav->plistate = PLI_STATE_STOPPED;
fprintf (stderr, "status: failed\n");
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

  return pos;
}

double
macosavRate (macosav_t *macosav, double drate)
{
  if (macosav == NULL || macosav->player == NULL) {
    return 100.0;
  }

  macosav->drate = drate;

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
      NSError     *err;

      macosav->plistate = PLI_STATE_PAUSED;
fprintf (stderr, "tcstatus: paused\n");
      err = [macosav->player error];
      if (err != 0) {
fprintf (stderr, "pl: err: %ld %s\n", [err code], [[err domain] UTF8String]);
      }
      err = [macosav->plitem error];
      if (err != 0) {
fprintf (stderr, "plitem: err: %ld %s\n", [err code], [[err domain] UTF8String]);
      }
      break;
    }
    case AVPlayerTimeControlStatusWaitingToPlayAtSpecifiedRate: {
      NSString    *reason;
      NSError   *err;

      macosav->plistate = PLI_STATE_BUFFERING;
      reason = [macosav->player reasonForWaitingToPlay];
fprintf (stderr, "tcstatus: wait/buffering\n");
fprintf (stderr, "reason: %s\n", [reason UTF8String]);
      [macosav->player play];
      err = [macosav->player error];
      if (err != 0) {
fprintf (stderr, "pl: err: %ld %s\n", [err code], [[err domain] UTF8String]);
      }
      err = [macosav->plitem error];
      if (err != 0) {
fprintf (stderr, "plitem: err: %ld %s\n", [err code], [[err domain] UTF8String]);
      }
      break;
    }
    case AVPlayerTimeControlStatusPlaying: {
      NSError   *err;

      macosav->plistate = PLI_STATE_PLAYING;
fprintf (stderr, "tcstatus: playing\n");
fprintf (stderr, "is-muted: %d\n", macosav->player.isMuted);
macosavGetDuration (macosav);
      err = [macosav->player error];
      if (err != 0) {
fprintf (stderr, "pl: err: %ld %s\n", [err code], [[err domain] UTF8String]);
      }
      err = [macosav->plitem error];
      if (err != 0) {
fprintf (stderr, "plitem: err: %ld %s\n", [err code], [[err domain] UTF8String]);
      }
      break;
    }
  }

  macosav->tcstatus = tcstatus;
}

