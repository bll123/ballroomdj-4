#include "config.h"

#if _hdr_MacTypes

#import "AudioToolbox/AudioServices.h"
#import "Foundation/NSObject.h"

#include <stdio.h>
#include <stdlib.h>
#include <MacTypes.h>
#include <Cocoa/Cocoa.h>

#include "volsink.h"
#include "volume.h"

void
volumeDisconnect (void) {
  return;
}

void
volumeCleanup (void **udata) {
  return;
}

int
volumeProcess (volaction_t action, const char *sinkname,
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
  UInt32          channels [2];
  UInt32          transporttype;

  if (action == VOL_HAVE_SINK_LIST) {
    return true;
  }

  /* get the system output device */
  propertyAOPA.mSelector = kAudioHardwarePropertyDefaultSystemOutputDevice;
  propertyAOPA.mScope = kAudioObjectPropertyScopeGlobal;
  propertyAOPA.mElement = kAudioObjectPropertyElementMaster;

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
  propertyAOPA.mElement = kAudioObjectPropertyElementMaster;

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

  if (action == VOL_GETSINKLIST) {
    OSStatus      error;
    AudioDeviceID *audioDeviceList;
    int           deviceCount;
    NSString      *result;


    sinklist->defname = NULL;
    sinklist->count = 0;
    sinklist->sinklist = NULL;

    propertyAOPA.mSelector = kAudioHardwarePropertyDevices;
    propertyAOPA.mScope = kAudioObjectPropertyScopeGlobal;
    propertyAOPA.mElement = kAudioObjectPropertyElementMaster;

    error = AudioObjectGetPropertyDataSize (kAudioObjectSystemObject,
        &propertyAOPA, 0, NULL, &propSize);
    if (error != noErr) {
      return -1;
    }

    deviceCount = propSize / sizeof (AudioDeviceID);
    if (deviceCount == 0) {
      return -1;
    }

    audioDeviceList = (AudioDeviceID *) malloc (propSize);
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
      propertyAOPA.mElement = kAudioObjectPropertyElementMaster;
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
      sinklist->sinklist = (volsinkitem_t *) realloc (sinklist->sinklist,
          sinklist->count * sizeof (volsinkitem_t));

      propSize = sizeof (CFStringRef);

      sinklist->sinklist [sinkidx].idxNumber = i;
      sinklist->sinklist [sinkidx].defaultFlag = false;

      propertyAOPA.mSelector = kAudioDevicePropertyDeviceNameCFString;
      propertyAOPA.mScope = kAudioObjectPropertyScopeGlobal;
      propertyAOPA.mElement = kAudioObjectPropertyElementMaster;
      error = AudioObjectGetPropertyData (audioDeviceList [i],
          &propertyAOPA, 0, NULL, &propSize, &result);
      sinklist->sinklist [sinkidx].description = strdup ([result UTF8String]);

      snprintf (tmp, sizeof (tmp), "%d", audioDeviceList [i]);
      sinklist->sinklist [sinkidx].name = strdup (tmp);

      if (audioDeviceList [i] == defaultDeviceID) {
        sinklist->sinklist [sinkidx].defaultFlag = true;
        sinklist->defname = strdup (sinklist->sinklist [sinkidx].name);
      }
    }

    free (audioDeviceList);

    return 0;
  }

  if (sinkname != NULL && *sinkname && action == VOL_SET_SYSTEM_DFLT) {
    outputDeviceID = atoi (sinkname);

    /* set the default output device */
    propertyAOPA.mSelector = kAudioHardwarePropertyDefaultOutputDevice;
    propertyAOPA.mScope = kAudioObjectPropertyScopeGlobal;
    propertyAOPA.mElement = channels [0]; // kAudioObjectPropertyElementMaster;

    propSize = sizeof (outputDeviceID);

    status = AudioObjectSetPropertyData (
        kAudioObjectSystemObject,
        &propertyAOPA,
        0,
        NULL,
        propSize,
        &outputDeviceID);
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

  propertyAOPA.mSelector = kAudioDevicePropertyTransportType;
  propertyAOPA.mScope = kAudioObjectPropertyScopeGlobal;
  propertyAOPA.mElement = kAudioObjectPropertyElementMaster;
  propSize = sizeof (transporttype);
  status = AudioObjectGetPropertyData (
      outputDeviceID,
      &propertyAOPA,
      0,
      NULL,
      &propSize,
      &transporttype );

  propertyAOPA.mSelector = kAudioDevicePropertyVolumeScalar;
  propertyAOPA.mScope = kAudioDevicePropertyScopeOutput;
  if (transporttype == kAudioDeviceTransportTypeBuiltIn) {
    propertyAOPA.mElement = kAudioObjectPropertyElementMaster;
  } else {
    propertyAOPA.mElement = channels [0]; // channel number
  }

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
  }

  if (vol != NULL && (action == VOL_SET || action == VOL_GET)) {
    propSize = sizeof (volume);
    status = AudioObjectGetPropertyData (
        outputDeviceID,
        &propertyAOPA,
        0,
        NULL,
        &propSize,
        &volume);
    if (status != 0) {
      volume = 0.0;
    }
    ivol = (int) (round((double) volume * 100.0));
    *vol = ivol;
    return *vol;
  }

  return -1;
}

#endif /* hdr_mactypes */
