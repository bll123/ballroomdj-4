/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */

#include "config.h"

#import <Foundation/NSObject.h>
#import <AVFoundation/AVFoundation.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <MacTypes.h>
#include <Cocoa/Cocoa.h>

/* macos's dynamic libraries work differently than linux */
#if defined (BDJ4_MEM_DEBUG)
# undef BDJ4_MEM_DEBUG
#endif

#include "audiosrc.h"
#include "log.h"
#include "macosavi.h"
#include "mdebug.h"

typedef struct macosav {
  AVPlayer        *player [PLI_MAX_SOURCE];
  AVPlayerItem    *plitem [PLI_MAX_SOURCE];
  double          drate;
  plistate_t      plistate;
  int             curr;
  bool            inCrossFade;
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
  for (int i = 0; i < PLI_MAX_SOURCE; ++i) {
    macosav->player [i] = NULL;
    macosav->plitem [i] = NULL;
  }

  macosav->drate = 1.0;
  macosav->plistate = PLI_STATE_NONE;
  macosav->curr = 0;
  macosav->inCrossFade = false;

  return macosav;
}

void
macosavClose (macosav_t *macosav)
{
  if (macosav == NULL) {
    return;
  }

  for (int i = 0; i < PLI_MAX_SOURCE; ++i) {
    if (macosav->player [i] != NULL) {
      [macosav->player [i] pause];
      [macosav->player [i] release];
    }
    if (macosav->plitem [i] != NULL) {
      [macosav->plitem [i] release];
    }
    macosav->player [i] = NULL;
    macosav->plitem [i] = NULL;
  }
  mdfree (macosav);
}

int
macosavMedia (macosav_t *macosav, const char *fullMediaPath, int sourceType)
{
  NSString        *ffn = NULL;
  NSURL           *url = NULL;
  AVPlayerItem    *plitem = NULL;
  int             count = 0;
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

  if (macosav->player [macosav->curr] == NULL) {
    macosav->player [macosav->curr] = [[AVPlayer alloc] init];
  } else {
    [macosav->player [macosav->curr] pause];
  }

  plitem = macosav->plitem [macosav->curr];
  macosav->plitem [macosav->curr] = [AVPlayerItem playerItemWithURL: url];
  if (macosavCheckPlayerItemError (macosav) != 0) {
    return -1;
  }
  if (sourceType != AUDIOSRC_TYPE_FILE) {
    macosav->player [macosav->curr].automaticallyWaitsToMinimizeStalling = true;
  }

  [macosav->player [macosav->curr]
      replaceCurrentItemWithPlayerItem: macosav->plitem [macosav->curr]];
  if (macosavCheckPlayerError (macosav) != 0) {
    return -1;
  }

  if (plitem != NULL) {
    [plitem release];
  }

  plitem = macosav->player [macosav->curr].currentItem;
  if (plitem != macosav->plitem [macosav->curr]) {
    fprintf (stderr, "AVPlayer: fatal: player item not configured\n");
  }

  /* maosavState() calls the runloop */
  plistate = macosavState (macosav);
  while (plistate == PLI_STATE_NONE) {
    plistate = macosavState (macosav);
    ++count;
  }
  if (plistate == PLI_STATE_ERROR) {
    return -1;
  }

  return 0;
}

int
macosavCrossFade (macosav_t *macosav, const char *ffn, int sourceType)
{
  if (macosav == NULL) {
    return -1;
  }

  macosav->curr = (PLI_MAX_SOURCE - 1) - macosav->curr;
  macosav->inCrossFade = true;
  macosavMedia (macosav, ffn, sourceType);

  return 0;
}

void
macosavCrossFadeVolume (macosav_t *macosav, int vol)
{
  int     previdx;
  double  dvol;

  if (macosav == NULL || macosav->player [macosav->curr] == NULL) {
    return;
  }
  if (macosav->inCrossFade == false) {
    return;
  }

  previdx = (PLI_MAX_SOURCE - 1) - macosav->curr;
  dvol = (double) vol / 100.0;
  if (dvol <= 0.0) {
    dvol = 0.0;
  }
  macosav->player [previdx].volume = dvol;
  if (dvol <= 0.0) {
    macosavRunLoop (macosav);
    [macosav->player [previdx] pause];
    macosav->player [previdx].muted = true;
    macosav->inCrossFade = false;
  }

  dvol = 1.0 - dvol;
  if (dvol > 0.0) {
    /* make sure the active player is no longer muted */
    macosav->player [macosav->curr].muted = false;
  }
  macosav->player [macosav->curr].volume = dvol;

  return;
}

int
macosavPlay (macosav_t *macosav)
{
  if (macosav == NULL || macosav->player [macosav->curr] == NULL) {
    return -1;
  }

  macosavRunLoop (macosav);
  macosav->player [macosav->curr].defaultRate = macosav->drate;
  [macosav->player [macosav->curr] play];
  return 0;
}

int
macosavPause (macosav_t *macosav)
{
  if (macosav == NULL || macosav->player [macosav->curr] == NULL) {
    return -1;
  }

  macosavRunLoop (macosav);
  [macosav->player [macosav->curr] pause];
  return 0;
}

int
macosavStop (macosav_t *macosav)
{
  if (macosav == NULL || macosav->player [macosav->curr] == NULL) {
    return -1;
  }

  macosavRunLoop (macosav);
  [macosav->player [macosav->curr] pause];
  return 0;
}

ssize_t
macosavGetDuration (macosav_t *macosav)
{
  CMTime    tm;
  ssize_t   duration;

  if (macosav == NULL || macosav->player [macosav->curr] == NULL) {
    return 0;
  }

  macosavRunLoop (macosav);
  tm = [macosav->plitem [macosav->curr] duration];
  duration = (ssize_t) (CMTimeGetSeconds (tm) * 1000.0);
  return duration;
}

ssize_t
macosavGetTime (macosav_t *macosav)
{
  CMTime    tm;
  ssize_t   currtm;

  if (macosav == NULL || macosav->player [macosav->curr] == NULL) {
    return 0;
  }

  macosavRunLoop (macosav);
  tm = [macosav->player [macosav->curr] currentTime];
  currtm = (ssize_t) (CMTimeGetSeconds (tm) * 1000.0);
  return 0;
}

plistate_t
macosavState (macosav_t *macosav)
{
  AVPlayerStatus      status;

  if (macosav == NULL || macosav->player [macosav->curr] == NULL) {
    return PLI_STATE_NONE;
  }

  macosavRunLoop (macosav);

  status = [macosav->player [macosav->curr] status];

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

  if (macosav == NULL || macosav->player [macosav->curr] == NULL) {
    return 0;
  }

  macosavRunLoop (macosav);
  dpos = (double) pos / 1000.0;
  /* audio frames per second */
  tm = CMTimeMakeWithSeconds (dpos, 192000);
  tolerance = CMTimeMakeWithSeconds (0.01, 192000);
  [macosav->player [macosav->curr] seekToTime: tm
      toleranceBefore: tolerance toleranceAfter: tolerance];
  return pos;
}

double
macosavRate (macosav_t *macosav, double drate)
{
  if (macosav == NULL || macosav->player [macosav->curr] == NULL) {
    return 100.0;
  }

  macosavRunLoop (macosav);
  macosav->drate = drate;
  macosav->player [macosav->curr].rate = drate;
  return drate;
}

/* internal routines */

static void
macosavCheckTCStatus (macosav_t *macosav)
{
  AVPlayerTimeControlStatus tcstatus;

  tcstatus = macosav->player [macosav->curr].timeControlStatus;

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
}

static long
macosavCheckPlayerError (macosav_t *macosav)
{
  NSError   *err;

  err = [macosav->player [macosav->curr] error];
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

  err = [macosav->plitem [macosav->curr] error];
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

  runloop = [NSRunLoop currentRunLoop];
  /* only run the loop once */
  /* using beforeDate blocks */
  [runloop runUntilDate: [NSDate dateWithTimeIntervalSinceNow: 0.01]];
}
