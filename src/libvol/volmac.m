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

int
volumeProcess (volaction_t action, const char *sinkname,
    int *vol, volsinklist_t *sinklist)
{
  AudioDeviceID   outputDeviceID = 0;
  UInt32          propSize = 0;
  AudioObjectPropertyAddress propertyAOPA;
  Float32         volume;
  int             ivol;

  if (action == VOL_HAVE_SINK_LIST) {
    return true;
  }

  /* get the default output device */
  propertyAOPA.mSelector = kAudioHardwarePropertyDefaultOutputDevice;
  propertyAOPA.mScope = kAudioObjectPropertyScopeGlobal;
  propertyAOPA.mElement = kAudioObjectPropertyElementMaster;

  propSize = sizeof (outputDeviceID);
  AudioObjectGetPropertyData (
      kAudioObjectSystemObject,
      &propertyAOPA,
      0,
      NULL,
      &propSize,
      &outputDeviceID);

  if (outputDeviceID == kAudioObjectUnknown) {
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

      if (audioDeviceList [i] == outputDeviceID) {
        sinklist->sinklist [sinkidx].defaultFlag = true;
        sinklist->defname = strdup (sinklist->sinklist [sinkidx].name);
      }
    }

    free (audioDeviceList);

    return 0;
  }

  propertyAOPA.mSelector = kAudioDevicePropertyVolumeScalar;
  propertyAOPA.mScope = kAudioDevicePropertyScopeOutput;
  propertyAOPA.mElement = kAudioObjectPropertyElementMaster;

  if (action == VOL_SET) {
    propSize = sizeof (volume);
    volume = (Float32) ((double) (*vol) / 100.0);
    AudioObjectSetPropertyData (
        outputDeviceID,
        &propertyAOPA,
        0,
        NULL,
        propSize,
        &volume);
  }

  propSize = sizeof (volume);
  AudioObjectGetPropertyData (
      outputDeviceID,
      &propertyAOPA,
      0,
      NULL,
      &propSize,
      &volume);
  ivol = (int) (round((double) volume * 100.0));
  *vol = ivol;

  return 0;
}

#endif /* hdr_mactypes */
