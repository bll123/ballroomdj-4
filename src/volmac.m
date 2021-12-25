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
audioDisconnect (void) {
  return;
}

int
process (volaction_t action, char *sinkname, int *vol, char **sinklist)
{
  AudioDeviceID outputDeviceID;
  UInt32 outputDeviceIDSize = sizeof (outputDeviceID);
  OSStatus status;
  AudioObjectPropertyAddress propertyAOPA;
  Float32 volume;
  UInt32 volumeSize = sizeof (volume);
  int ivol;

  propertyAOPA.mSelector = kAudioHardwarePropertyDefaultOutputDevice;
  propertyAOPA.mScope = kAudioObjectPropertyScopeGlobal;
  propertyAOPA.mElement = kAudioObjectPropertyElementMaster;

  status = AudioObjectGetPropertyData(
      kAudioObjectSystemObject,
      &propertyAOPA,
      0,
      NULL,
      &outputDeviceIDSize,
      &outputDeviceID);

  if (outputDeviceID == kAudioObjectUnknown) {
    return 0;
  }

  propertyAOPA.mSelector = kAudioHardwareServiceDeviceProperty_VirtualMasterVolume;
  propertyAOPA.mScope = kAudioDevicePropertyScopeOutput;
  propertyAOPA.mElement = kAudioObjectPropertyElementMaster;

  if (set == 1) {
    volume = (Float32) vol / 100.0;
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
  ivol = (int) round(volume*100.0);
  return ivol;
}

#endif /* hdr_mactypes */
