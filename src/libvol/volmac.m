/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#import <AudioToolbox/AudioServices.h>
#import <Foundation/NSObject.h>

#include <stdio.h>
#include <stdlib.h>

#include <MacTypes.h>
#include <Cocoa/Cocoa.h>

/* macos's dynamic libraries work differently than linux */
#if defined (BDJ4_MEM_DEBUG)
# undef BDJ4_MEM_DEBUG
#endif

#include "mdebug.h"
#include "volsink.h"
#include "volume.h"

enum {
  CHAN_STEREO = 2,
};

typedef struct {
  AudioDeviceID   defdev;
  bool            changed;
} volmac_t;

void
voliDesc (const char **ret, int max)
{
  int   c = 0;

  if (max < 2) {
    return;
  }

  ret [c++] = "MacOS";
  ret [c++] = NULL;
}

void
voliDisconnect (void)
{
  return;
}

void
voliCleanup (void **udata)
{
  volmac_t  *volmac;

  if (udata == NULL || *udata == NULL) {
    return;
  }

  volmac = *udata;
  mdfree (volmac);

  return;
}

int
voliProcess (volaction_t action, const char *sinkname,
    int *vol, volsinklist_t *sinklist, void **udata)
{
  AudioDeviceID   outputDeviceID = 0;
  AudioDeviceID   defaultDeviceID = 0;
  AudioDeviceID   systemDeviceID = 0;
  UInt32          propSize = 0;
  AudioObjectPropertyAddress propertyAOPA;
  Float32         volume;
  int             ivol;
  int             status;
  UInt32          channels [CHAN_STEREO];
  volmac_t        *volmac = NULL;

  if (action == VOL_HAVE_SINK_LIST) {
    return true;
  }

  /* get the system output device */
  propertyAOPA.mSelector = kAudioHardwarePropertyDefaultSystemOutputDevice;
  propertyAOPA.mScope = kAudioObjectPropertyScopeGlobal;
  propertyAOPA.mElement = kAudioObjectPropertyElementMain;

  propSize = sizeof (systemDeviceID);
  AudioObjectGetPropertyData (
      kAudioObjectSystemObject,
      &propertyAOPA,
      0,
      NULL,
      &propSize,
      &systemDeviceID);

  if (systemDeviceID == kAudioObjectUnknown) {
    return -1;
  }

  /* get the default output device */
  propertyAOPA.mSelector = kAudioHardwarePropertyDefaultOutputDevice;
  propertyAOPA.mScope = kAudioObjectPropertyScopeGlobal;
  propertyAOPA.mElement = kAudioObjectPropertyElementMain;

  propSize = sizeof (defaultDeviceID);
  AudioObjectGetPropertyData (
      kAudioObjectSystemObject,
      &propertyAOPA,
      0,
      NULL,
      &propSize,
      &defaultDeviceID);

  if (defaultDeviceID == kAudioObjectUnknown) {
    return -1;
  }

  if (*udata == NULL) {
    volmac = mdmalloc (sizeof (*volmac));
    volmac->defdev = defaultDeviceID;
    volmac->changed = false;
    *udata = volmac;
  } else {
    volmac = *udata;
    /* leave the changed flag set until the user fetches it */
    if (volmac->defdev != defaultDeviceID) {
      volmac->defdev = defaultDeviceID;
      volmac->changed = true;
    }
  }

  if (action == VOL_CHK_SINK) {
    bool    orig;

    /* leave the changed flag set until the user fetches it */
    orig = volmac->changed;
    volmac->changed = false;
    return orig;
  }

  if (action == VOL_GETSINKLIST) {
    OSStatus      error;
    AudioDeviceID *audioDeviceList;
    int           deviceCount;
    NSString      *result;


    dataFree (sinklist->defname);
    /* MacOS uses '0' as the default device ID */
    sinklist->defname = mdstrdup ("0");
    sinklist->count = 0;
    sinklist->sinklist = NULL;

    propertyAOPA.mSelector = kAudioHardwarePropertyDevices;
    propertyAOPA.mScope = kAudioObjectPropertyScopeGlobal;
    propertyAOPA.mElement = kAudioObjectPropertyElementMain;

    error = AudioObjectGetPropertyDataSize (kAudioObjectSystemObject,
        &propertyAOPA, 0, NULL, &propSize);
    if (error != noErr) {
      return -1;
    }

    deviceCount = propSize / sizeof (AudioDeviceID);
    if (deviceCount == 0) {
      return -1;
    }

    audioDeviceList = (AudioDeviceID *) mdmalloc (propSize);
    error = AudioObjectGetPropertyData (kAudioObjectSystemObject,
        &propertyAOPA, 0, NULL, &propSize, audioDeviceList);

    for (int i = 0; i < deviceCount; i++) {
      AudioBufferList streamConfiguration;
      int             ccount;
      int             sinkidx;
      char            tmp [40];

      propSize = sizeof (streamConfiguration);

      propertyAOPA.mSelector = kAudioDevicePropertyStreamConfiguration;
      propertyAOPA.mScope = kAudioObjectPropertyScopeOutput;
      propertyAOPA.mElement = kAudioObjectPropertyElementMain;
      error = AudioObjectGetPropertyData (audioDeviceList [i],
          &propertyAOPA, 0, NULL, &propSize, &streamConfiguration);
      ccount = 0;
      for (NSUInteger i = 0; i < streamConfiguration.mNumberBuffers; i++) {
        ccount += streamConfiguration.mBuffers[i].mNumberChannels;
      }
      if (ccount < 1) {
        /* no output */
        continue;
      }

      ++sinklist->count;
      sinkidx = sinklist->count - 1;
      sinklist->sinklist = (volsinkitem_t *) mdrealloc (sinklist->sinklist,
          sinklist->count * sizeof (volsinkitem_t));

      propSize = sizeof (CFStringRef);

      sinklist->sinklist [sinkidx].idxNumber = i;
      sinklist->sinklist [sinkidx].defaultFlag = false;

      propertyAOPA.mSelector = kAudioDevicePropertyDeviceNameCFString;
      propertyAOPA.mScope = kAudioObjectPropertyScopeGlobal;
      propertyAOPA.mElement = kAudioObjectPropertyElementMain;
      error = AudioObjectGetPropertyData (audioDeviceList [i],
          &propertyAOPA, 0, NULL, &propSize, &result);
      sinklist->sinklist [sinkidx].description = mdstrdup ([result UTF8String]);

      snprintf (tmp, sizeof (tmp), "%d", audioDeviceList [i]);
      sinklist->sinklist [sinkidx].name = mdstrdup (tmp);

      if (audioDeviceList [i] == defaultDeviceID) {
        sinklist->sinklist [sinkidx].defaultFlag = true;
      }
    }

    mdfree (audioDeviceList);

    return 0;
  }

  if (*sinkname) {
    outputDeviceID = atoi (sinkname);
  } else {
    outputDeviceID = defaultDeviceID;
  }

  propertyAOPA.mSelector = kAudioDevicePropertyPreferredChannelsForStereo;
  propertyAOPA.mScope = kAudioDevicePropertyScopeOutput;
  propertyAOPA.mElement = kAudioObjectPropertyElementWildcard;
  propSize = sizeof (channels);
  status = AudioObjectGetPropertyData (
      outputDeviceID,
      &propertyAOPA,
      0,
      NULL,
      &propSize,
      &channels );

  /* Neither HasProperty nor IsPropertySettable work to determine */
  /* if the volume can be set. */
  /* Instead of guessing how to do this, just try one way, then try */
  /* the other. */
  /* When using channels, set the volume on both channels. */

  propertyAOPA.mSelector = kAudioDevicePropertyVolumeScalar;
  propertyAOPA.mScope = kAudioDevicePropertyScopeOutput;
  propertyAOPA.mElement = kAudioObjectPropertyElementMain;

  if (action == VOL_SET) {
    propSize = sizeof (volume);
    volume = (Float32) ((double) (*vol) / 100.0);
    status = AudioObjectSetPropertyData (
        outputDeviceID,
        &propertyAOPA,
        0,
        NULL,
        propSize,
        &volume);
    if (status != 0) {
      /* set all channels */
      for (int i = 0; i < CHAN_STEREO; ++i) {
        propertyAOPA.mElement = channels [i];
        propSize = sizeof (volume);
        status = AudioObjectSetPropertyData (
            outputDeviceID,
            &propertyAOPA,
            0,
            NULL,
            propSize,
            &volume);
       }
    }
  }

  if (vol != NULL && (action == VOL_SET || action == VOL_GET)) {
    propertyAOPA.mElement = kAudioObjectPropertyElementMain;
    propSize = sizeof (volume);
    status = AudioObjectGetPropertyData (
        outputDeviceID,
        &propertyAOPA,
        0,
        NULL,
        &propSize,
        &volume);
    if (status != 0) {
      propertyAOPA.mElement = channels [0];
      propSize = sizeof (volume);
      status = AudioObjectGetPropertyData (
          outputDeviceID,
          &propertyAOPA,
          0,
          NULL,
          &propSize,
          &volume);
    }
    if (status != 0) {
      volume = 0.0;
    }
    ivol = (int) (round((double) volume * 100.0));
    *vol = ivol;
    return *vol;
  }

  return -1;
}
