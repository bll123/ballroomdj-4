#include "config.h"

#if _hdr_MacTypes

#import "AudioToolbox/AudioServices.h"
#import "Foundation/NSObject.h"

#include <stdio.h>
#include <stdlib.h>
#include <MacTypes.h>

#include "volume.h"

/*
 * has objective-c code to get all output devices...
 * http://stackoverflow.com/questions/11347989/get-built-in-output-from-core-audio
 */

void
volumeDisconnect (void) {
  return;
}

int
volumeProcess (volaction_t action, char *sinkname,
    int *vol, volsinklist_t *sinklist)
{
  AudioDeviceID   outputDeviceID;
  UInt32          outputDeviceIDSize = sizeof (outputDeviceID);
  AudioObjectPropertyAddress propertyAOPA;
  Float32         volume;
  UInt32          volumeSize = sizeof (volume);
  int             ivol;

  if (action == VOL_HAVE_SINK_LIST) {
    return false;
  }

  propertyAOPA.mSelector = kAudioHardwarePropertyDefaultOutputDevice;
  propertyAOPA.mScope = kAudioObjectPropertyScopeGlobal;
  propertyAOPA.mElement = kAudioObjectPropertyElementMaster;

  AudioObjectGetPropertyData(
      kAudioObjectSystemObject,
      &propertyAOPA,
      0,
      NULL,
      &outputDeviceIDSize,
      &outputDeviceID);

  if (outputDeviceID == kAudioObjectUnknown) {
    return -1;
  }

// ### this is marked as deprecated.
// what is the replacement?
  propertyAOPA.mSelector = kAudioHardwareServiceDeviceProperty_VirtualMasterVolume;
  propertyAOPA.mScope = kAudioDevicePropertyScopeOutput;
  propertyAOPA.mElement = kAudioObjectPropertyElementMaster;

  if (action == VOL_SET) {
    volume = (Float32) ((double) (*vol) / 100.0);
    AudioObjectSetPropertyData(
        outputDeviceID,
        &propertyAOPA,
        0,
        NULL,
        volumeSize,
        &volume);
  }

  AudioObjectGetPropertyData(
      outputDeviceID,
      &propertyAOPA,
      0,
      NULL,
      &volumeSize,
      &volume);
  ivol = (int) (round((double) volume * 100.0));
  *vol = ivol;
  return 0;
}

#endif /* hdr_mactypes */
